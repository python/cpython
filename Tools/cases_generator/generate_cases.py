"""Generate the main interpreter switch.

Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
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

    def write(
        self, f: typing.TextIO, indent: str, dedent: int = 0
    ) -> None:
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
            if ceffect.name != "unused":
                # TODO: if name is 'descr' use PyObject *descr = read_obj(...)
                bits = ceffect.size * 16
                f.write(f"{indent}    uint{bits}_t {ceffect.name} = ")
                if ceffect.size == 1:
                    f.write(f"*(next_instr + {cache_offset});\n")
                else:
                    f.write(f"read_u{bits}(next_instr + {cache_offset});\n")
            cache_offset += ceffect.size
        assert cache_offset == self.cache_offset

        # Write input stack effect variable declarations and initializations
        for i, seffect in enumerate(reversed(self.input_effects), 1):
            if seffect.name != "unused":
                f.write(f"{indent}    PyObject *{seffect.name} = PEEK({i});\n")

        # Write output stack effect variable declarations
        for seffect in self.output_effects:
            if seffect.name != "unused":
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
        input_names = [seffect.name for seffect in self.input_effects]
        for i, output in enumerate(reversed(self.output_effects), 1):
            if output.name not in input_names and output.name != "unused":
                f.write(f"{indent}    POKE({i}, {output.name});\n")

        # Write cache effect
        if self.cache_offset:
            f.write(f"{indent}    next_instr += {self.cache_offset};\n")

    def write_body(
        self, f: typing.TextIO, ndent: str, dedent: int
    ) -> None:
        """Write the instruction body."""

        # Get lines of text with proper dedelt
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
                if ninputs:
                    f.write(f"{space}if ({cond}) goto pop_{ninputs}_{label};\n")
                else:
                    f.write(f"{space}if ({cond}) goto {label};\n")
            else:
                f.write(line)


class Analyzer:
    """Parse input, analyze it, and write to output."""

    filename: str
    src: str
    errors: int = 0

    def __init__(self, filename: str):
        """Read the input file."""
        self.filename = filename
        with open(filename) as f:
            self.src = f.read()

    instrs: dict[str, Instruction]
    supers: dict[str, parser.Super]
    families: dict[str, parser.Family]

    def parse(self) -> None:
        """Parse the source text."""
        psr = parser.Parser(self.src, filename=self.filename)

        # Skip until begin marker
        while tkn := psr.next(raw=True):
            if tkn.text == BEGIN_MARKER:
                break
        else:
            raise psr.make_syntax_error(f"Couldn't find {BEGIN_MARKER!r} in {psr.filename}")

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
            f"{len(self.supers)} supers, "
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

    def find_predictions(self) -> None:
        """Find the instructions that need PREDICTED() labels."""
        for instr in self.instrs.values():
            for target in re.findall(RE_PREDICTED, instr.block.text):
                if target_instr := self.instrs.get(target):
                    target_instr.predicted = True
                else:
                    print(
                        f"Unknown instruction {target!r} predicted in {instr.name!r}",
                        file=sys.stderr,
                    )
                    self.errors += 1

    def map_families(self) -> None:
        """Make instruction names back to their family, if they have one."""
        for family in self.families.values():
            for member in family.members:
                if member_instr := self.instrs.get(member):
                    member_instr.family = family
                else:
                    print(
                        f"Unknown instruction {member!r} referenced in family {family.name!r}",
                        file=sys.stderr,
                    )
                    self.errors += 1

    def check_families(self) -> None:
        """Check each family:

        - Must have at least 2 members
        - All members must be known instructions
        - All members must have the same cache, input and output effects
        """
        for family in self.families.values():
            if len(family.members) < 2:
                print(f"Family {family.name!r} has insufficient members")
                self.errors += 1
            members = [member for member in family.members if member in self.instrs]
            if members != family.members:
                unknown = set(family.members) - set(members)
                print(f"Family {family.name!r} has unknown members: {unknown}")
                self.errors += 1
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
                    self.errors += 1
                    print(
                        f"Family {family.name!r} has inconsistent "
                        f"(cache, inputs, outputs) effects:",
                        file=sys.stderr,
                    )
                    print(
                        f"  {family.members[0]} = {(cache, input, output)}; "
                        f"{member} = {(c, i, o)}",
                        file=sys.stderr,
                    )
                    self.errors += 1

    def write_instructions(self, filename: str) -> None:
        """Write instructions to output file."""
        indent = " " * 8
        with open(filename, "w") as f:
            # Write provenance header
            f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
            f.write(f"// from {os.path.relpath(self.filename)}\n")
            f.write(f"// Do not edit!\n")

            # Write regular instructions
            for name, instr in self.instrs.items():
                f.write(f"\n{indent}TARGET({name}) {{\n")
                if instr.predicted:
                    f.write(f"{indent}    PREDICTED({name});\n")
                instr.write(f, indent)
                if not always_exits(instr.block):
                    f.write(f"{indent}    DISPATCH();\n")
                f.write(f"{indent}}}\n")

            # Write super-instructions
            for name, sup in self.supers.items():
                components = [self.instrs[name] for name in sup.ops]
                f.write(f"\n{indent}TARGET({sup.name}) {{\n")
                for i, instr in enumerate(components):
                    if i > 0:
                        f.write(f"{indent}    NEXTOPARG();\n")
                        f.write(f"{indent}    next_instr++;\n")
                    f.write(f"{indent}    {{\n")
                    instr.write(f, indent, dedent=-4)
                    f.write(f"    {indent}}}\n")
                f.write(f"{indent}    DISPATCH();\n")
                f.write(f"{indent}}}\n")

            print(
                f"Wrote {len(self.instrs)} instructions and "
                f"{len(self.supers)} super-instructions to {filename}",
                file=sys.stderr,
            )


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
