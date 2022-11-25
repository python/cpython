"""Generate the main interpreter switch.

Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import contextlib
import dataclasses
import os
import re
import sys
import typing

import parser

DEFAULT_INPUT = "Python/bytecodes.c"
DEFAULT_OUTPUT = "Python/generated_cases.c.h"
BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"
RE_PREDICTED = r"(?s)(?:PREDICT\(|GO_TO_INSTRUCTION\(|DEOPT_IF\(.*?,\s*)(\w+)\);"
UNUSED = "unused"
BITS_PER_CODE_UNIT = 16

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("-i", "--input", type=str, default=DEFAULT_INPUT)
arg_parser.add_argument("-o", "--output", type=str, default=DEFAULT_OUTPUT)


# This is not a data class
class Instruction(parser.InstDef):
    """An instruction with additional data and code."""

    # Computed by constructor
    always_exits: bool
    cache_offset: int
    cache_effects: list[parser.CacheEffect]
    input_effects: list[parser.StackEffect]
    output_effects: list[parser.StackEffect]

    # Set later
    family: parser.Family | None = None
    predicted: bool = False

    def __init__(self, inst: parser.InstDef):
        super().__init__(inst.header, inst.block)
        self.context = inst.context
        self.always_exits = always_exits(self.block)
        self.cache_effects = [
            effect for effect in self.inputs if isinstance(effect, parser.CacheEffect)
        ]
        self.cache_offset = sum(c.size for c in self.cache_effects)
        self.input_effects = [
            effect for effect in self.inputs if isinstance(effect, parser.StackEffect)
        ]
        self.output_effects = self.outputs  # For consistency/completeness

    def write(self, f: typing.TextIO, indent: str, dedent: int = 0) -> None:
        """Write one instruction, sans prologue and epilogue."""
        if dedent < 0:
            indent += " " * -dedent  # DO WE NEED THIS?

        # Get cache offset and maybe assert that it is correct
        if family := self.family:
            if self.name == family.members[0]:
                if cache_size := family.size:
                    f.write(
                        f"{indent}    static_assert({cache_size} == "
                        f'{self.cache_offset}, "incorrect cache size");\n'
                    )

        # Write cache effect variable declarations
        cache_offset = 0
        for ceffect in self.cache_effects:
            if ceffect.name != UNUSED:
                bits = ceffect.size * BITS_PER_CODE_UNIT
                if bits == 64:
                    # NOTE: We assume that 64-bit data in the cache
                    # is always an object pointer.
                    # If this becomes false, we need a way to specify
                    # syntactically what type the cache data is.
                    f.write(
                        f"{indent}    PyObject *{ceffect.name} = "
                        f"read_obj(next_instr + {cache_offset});\n"
                    )
                else:
                    f.write(f"{indent}    uint{bits}_t {ceffect.name} = "
                        f"read_u{bits}(next_instr + {cache_offset});\n")
            cache_offset += ceffect.size
        assert cache_offset == self.cache_offset

        # Write input stack effect variable declarations and initializations
        for i, seffect in enumerate(reversed(self.input_effects), 1):
            if seffect.name != UNUSED:
                f.write(f"{indent}    PyObject *{seffect.name} = PEEK({i});\n")

        # Write output stack effect variable declarations
        input_names = {seffect.name for seffect in self.input_effects}
        input_names.add(UNUSED)
        for seffect in self.output_effects:
            if seffect.name not in input_names:
                f.write(f"{indent}    PyObject *{seffect.name};\n")

        self.write_body(f, indent, dedent)

        # Skip the rest if the block always exits
        if always_exits(self.block):
            return

        # Write net stack growth/shrinkage
        diff = len(self.output_effects) - len(self.input_effects)
        if diff > 0:
            f.write(f"{indent}    STACK_GROW({diff});\n")
        elif diff < 0:
            f.write(f"{indent}    STACK_SHRINK({-diff});\n")

        # Write output stack effect assignments
        unmoved_names = {UNUSED}
        for ieffect, oeffect in zip(self.input_effects, self.output_effects):
            if ieffect.name == oeffect.name:
                unmoved_names.add(ieffect.name)
        for i, seffect in enumerate(reversed(self.output_effects)):
            if seffect.name not in unmoved_names:
                f.write(f"{indent}    POKE({i+1}, {seffect.name});\n")

        # Write cache effect
        if self.cache_offset:
            f.write(f"{indent}    next_instr += {self.cache_offset};\n")

    def write_body(self, f: typing.TextIO, ndent: str, dedent: int) -> None:
        """Write the instruction body."""

        # Get lines of text with proper dedent
        blocklines = self.block.to_text(dedent=dedent).splitlines(True)

        # Remove blank lines from both ends
        while blocklines and not blocklines[0].strip():
            blocklines.pop(0)
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()

        # Remove leading and trailing braces
        assert blocklines and blocklines[0].strip() == "{"
        assert blocklines and blocklines[-1].strip() == "}"
        blocklines.pop()
        blocklines.pop(0)

        # Remove trailing blank lines
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()

        # Write the body, substituting a goto for ERROR_IF()
        for line in blocklines:
            if m := re.match(r"(\s*)ERROR_IF\((.+), (\w+)\);\s*$", line):
                space, cond, label = m.groups()
                # ERROR_IF() must pop the inputs from the stack.
                # The code block is responsible for DECREF()ing them.
                # NOTE: If the label doesn't exist, just add it to ceval.c.
                ninputs = len(self.input_effects)
                # Don't pop common input/output effects at the bottom!
                # These aren't DECREF'ed so they can stay.
                for ieff, oeff in zip(self.input_effects, self.output_effects):
                    if ieff.name == oeff.name:
                        ninputs -= 1
                    else:
                        break
                if ninputs:
                    f.write(f"{space}if ({cond}) goto pop_{ninputs}_{label};\n")
                else:
                    f.write(f"{space}if ({cond}) goto {label};\n")
            else:
                f.write(line)


@dataclasses.dataclass
class SuperComponent:
    instr: Instruction
    input_mapping: dict[str, parser.StackEffect]
    output_mapping: dict[str, parser.StackEffect]


class SuperInstruction(parser.Super):

    stack: list[str]
    initial_sp: int
    final_sp: int
    parts: list[SuperComponent]

    def __init__(self, sup: parser.Super):
        super().__init__(sup.kind, sup.name, sup.ops)
        self.context = sup.context

    def analyze(self, a: "Analyzer") -> None:
        components = self.check_components(a)
        self.stack, self.initial_sp = self.super_macro_analysis(a, components)
        sp = self.initial_sp
        self.parts = []
        for instr in components:
            input_mapping = {}
            for ieffect in reversed(instr.input_effects):
                sp -= 1
                if ieffect.name != UNUSED:
                    input_mapping[self.stack[sp]] = ieffect
            output_mapping = {}
            for oeffect in instr.output_effects:
                if oeffect.name != UNUSED:
                    output_mapping[self.stack[sp]] = oeffect
                sp += 1
            self.parts.append(SuperComponent(instr, input_mapping, output_mapping))
        self.final_sp = sp

    def check_components(self, a: "Analyzer") -> list[Instruction]:
        components: list[Instruction] = []
        if not self.ops:
            a.error(f"{self.kind.capitalize()}-instruction has no operands", self)
        for name in self.ops:
            if name not in a.instrs:
                a.error(f"Unknown instruction {name!r}", self)
            else:
                instr = a.instrs[name]
                if self.kind == "super" and instr.kind != "inst":
                    a.error(f"Super-instruction operand {instr.name} must be inst, not op", instr)
                components.append(instr)
        return components

    def super_macro_analysis(
        self, a: "Analyzer", components: list[Instruction]
    ) -> tuple[list[str], int]:
        """Analyze a super-instruction or macro.

        Print an error if there's a cache effect (which we don't support yet).

        Return the list of variable names and the initial stack pointer.
        """
        lowest = current = highest = 0
        for instr in components:
            if instr.cache_effects:
                a.error(
                    f"Super-instruction {self.name!r} has cache effects in {instr.name!r}",
                    instr,
                )
            current -= len(instr.input_effects)
            lowest = min(lowest, current)
            current += len(instr.output_effects)
            highest = max(highest, current)
        # At this point, 'current' is the net stack effect,
        # and 'lowest' and 'highest' are the extremes.
        # Note that 'lowest' may be negative.
        stack = [f"_tmp_{i+1}" for i in range(highest - lowest)]
        return stack, -lowest


class Analyzer:
    """Parse input, analyze it, and write to output."""

    filename: str
    src: str
    errors: int = 0

    def error(self, msg: str, node: parser.Node) -> None:
        lineno = 0
        if context := node.context:
            # Use line number of first non-comment in the node
            for token in context.owner.tokens[context.begin :  context.end]:
                lineno = token.line
                if token.kind != "COMMENT":
                    break
        print(f"{self.filename}:{lineno}: {msg}", file=sys.stderr)
        self.errors += 1

    def __init__(self, filename: str):
        """Read the input file."""
        self.filename = filename
        with open(filename) as f:
            self.src = f.read()

    instrs: dict[str, Instruction]  # Includes ops
    supers: dict[str, parser.Super]  # Includes macros
    super_instrs: dict[str, SuperInstruction]
    families: dict[str, parser.Family]

    def parse(self) -> None:
        """Parse the source text."""
        psr = parser.Parser(self.src, filename=self.filename)

        # Skip until begin marker
        while tkn := psr.next(raw=True):
            if tkn.text == BEGIN_MARKER:
                break
        else:
            raise psr.make_syntax_error(
                f"Couldn't find {BEGIN_MARKER!r} in {psr.filename}"
            )

        # Parse until end marker
        self.instrs = {}
        self.supers = {}
        self.families = {}
        while (tkn := psr.peek(raw=True)) and tkn.text != END_MARKER:
            if inst := psr.inst_def():
                self.instrs[inst.name] = instr = Instruction(inst)
            elif super := psr.super_def():
                self.supers[super.name] = super
            elif family := psr.family_def():
                self.families[family.name] = family
            else:
                raise psr.make_syntax_error(f"Unexpected token")

        print(
            f"Read {len(self.instrs)} instructions, "
            f"{len(self.supers)} supers/macros, "
            f"and {len(self.families)} families from {self.filename}",
            file=sys.stderr,
        )

    def analyze(self) -> None:
        """Analyze the inputs.

        Raises SystemExit if there is an error.
        """
        self.find_predictions()
        self.map_families()
        self.check_families()
        self.analyze_supers()

    def find_predictions(self) -> None:
        """Find the instructions that need PREDICTED() labels."""
        for instr in self.instrs.values():
            for target in re.findall(RE_PREDICTED, instr.block.text):
                if target_instr := self.instrs.get(target):
                    target_instr.predicted = True
                else:
                    self.error(
                        f"Unknown instruction {target!r} predicted in {instr.name!r}",
                        instr,  # TODO: Use better location
                    )

    def map_families(self) -> None:
        """Make instruction names back to their family, if they have one."""
        for family in self.families.values():
            for member in family.members:
                if member_instr := self.instrs.get(member):
                    member_instr.family = family
                else:
                    self.error(
                        f"Unknown instruction {member!r} referenced in family {family.name!r}",
                        family,
                    )

    def check_families(self) -> None:
        """Check each family:

        - Must have at least 2 members
        - All members must be known instructions
        - All members must have the same cache, input and output effects
        """
        for family in self.families.values():
            if len(family.members) < 2:
                self.error(f"Family {family.name!r} has insufficient members", family)
            members = [member for member in family.members if member in self.instrs]
            if members != family.members:
                unknown = set(family.members) - set(members)
                self.error(f"Family {family.name!r} has unknown members: {unknown}", family)
            if len(members) < 2:
                continue
            head = self.instrs[members[0]]
            cache = head.cache_offset
            input = len(head.input_effects)
            output = len(head.output_effects)
            for member in members[1:]:
                instr = self.instrs[member]
                c = instr.cache_offset
                i = len(instr.input_effects)
                o = len(instr.output_effects)
                if (c, i, o) != (cache, input, output):
                    self.error(
                        f"Family {family.name!r} has inconsistent "
                        f"(cache, inputs, outputs) effects:\n"
                        f"  {family.members[0]} = {(cache, input, output)}; "
                        f"{member} = {(c, i, o)}",
                        family,
                    )

    def analyze_supers(self) -> None:
        """Analyze each super instruction."""
        self.super_instrs = {}
        for name, sup in self.supers.items():
            dup = SuperInstruction(sup)
            dup.analyze(self)
            self.super_instrs[name] = dup

    def write_instructions(self, filename: str) -> None:
        """Write instructions to output file."""
        indent = " " * 8
        with open(filename, "w") as f:
            # Write provenance header
            f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
            f.write(f"// from {os.path.relpath(self.filename)}\n")
            f.write(f"// Do not edit!\n")

            # Write regular instructions
            n_instrs = 0
            for name, instr in self.instrs.items():
                if instr.kind != "inst":
                    continue  # ops are not real instructions
                n_instrs += 1
                f.write(f"\n{indent}TARGET({name}) {{\n")
                if instr.predicted:
                    f.write(f"{indent}    PREDICTED({name});\n")
                instr.write(f, indent)
                if not always_exits(instr.block):
                    f.write(f"{indent}    DISPATCH();\n")
                f.write(f"{indent}}}\n")

            # Write super-instructions and macros
            n_supers = 0
            n_macros = 0
            for sup in self.super_instrs.values():
                if sup.kind == "super":
                    n_supers += 1
                elif sup.kind == "macro":
                    n_macros += 1
                self.write_super_macro(f, sup, indent)

            print(
                f"Wrote {n_instrs} instructions, {n_supers} supers, "
                f"and {n_macros} macros to {filename}",
                file=sys.stderr,
            )

    def write_super_macro(
        self, f: typing.TextIO, sup: SuperInstruction, indent: str = ""
    ) -> None:

        # TODO: Make write() and block() methods of some Formatter class
        def write(arg: str) -> None:
            if arg:
                f.write(f"{indent}{arg}\n")
            else:
                f.write("\n")

        @contextlib.contextmanager
        def block(head: str):
            if head:
                write(head + " {")
            else:
                write("{")
            nonlocal indent
            indent += "    "
            yield
            indent = indent[:-4]
            write("}")

        write("")
        with block(f"TARGET({sup.name})"):
            for i, var in enumerate(sup.stack):
                if i < sup.initial_sp:
                    write(f"PyObject *{var} = PEEK({sup.initial_sp - i});")
                else:
                    write(f"PyObject *{var};")

            for i, comp in enumerate(sup.parts):
                if i > 0 and sup.kind == "super":
                    write("NEXTOPARG();")
                    write("next_instr++;")

                with block(""):
                    for var, ieffect in comp.input_mapping.items():
                        write(f"PyObject *{ieffect.name} = {var};")
                    for oeffect in comp.output_mapping.values():
                        write(f"PyObject *{oeffect.name};")
                    comp.instr.write_body(f, indent, dedent=-4)
                    for var, oeffect in comp.output_mapping.items():
                        write(f"{var} = {oeffect.name};")

            if sup.final_sp > sup.initial_sp:
                write(f"STACK_GROW({sup.final_sp - sup.initial_sp});")
            elif sup.final_sp < sup.initial_sp:
                write(f"STACK_SHRINK({sup.initial_sp - sup.final_sp});")
            for i, var in enumerate(reversed(sup.stack[:sup.final_sp]), 1):
                write(f"POKE({i}, {var});")
            write("DISPATCH();")


def always_exits(block: parser.Block) -> bool:
    """Determine whether a block always ends in a return/goto/etc."""
    text = block.text
    lines = text.splitlines()
    while lines and not lines[-1].strip():
        lines.pop()
    if not lines or lines[-1].strip() != "}":
        return False
    lines.pop()
    if not lines:
        return False
    line = lines.pop().rstrip()
    # Indent must match exactly (TODO: Do something better)
    if line[:12] != " " * 12:
        return False
    line = line[12:]
    return line.startswith(
        ("goto ", "return ", "DISPATCH", "GO_TO_", "Py_UNREACHABLE()")
    )


def main():
    """Parse command line, parse input, analyze, write output."""
    args = arg_parser.parse_args()  # Prints message and sys.exit(2) on error
    a = Analyzer(args.input)  # Raises OSError if file not found
    a.parse()  # Raises SyntaxError on failure
    a.analyze()  # Prints messages and raises SystemExit on failure
    if a.errors:
        sys.exit(f"Found {a.errors} errors")

    a.write_instructions(args.output)  # Raises OSError if file can't be written


if __name__ == "__main__":
    main()
