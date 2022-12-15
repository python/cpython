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
from parser import StackEffect

DEFAULT_INPUT = os.path.relpath(
    os.path.join(os.path.dirname(__file__), "../../Python/bytecodes.c")
)
DEFAULT_OUTPUT = os.path.relpath(
    os.path.join(os.path.dirname(__file__), "../../Python/generated_cases.c.h")
)
BEGIN_MARKER = "// BEGIN BYTECODES //"
END_MARKER = "// END BYTECODES //"
RE_PREDICTED = r"^\s*(?:PREDICT\(|GO_TO_INSTRUCTION\(|DEOPT_IF\(.*?,\s*)(\w+)\);\s*$"
UNUSED = "unused"
BITS_PER_CODE_UNIT = 16

arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)
arg_parser.add_argument(
    "-i", "--input", type=str, help="Instruction definitions", default=DEFAULT_INPUT
)
arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)


class Formatter:
    """Wraps an output stream with the ability to indent etc."""

    stream: typing.TextIO
    prefix: str

    def __init__(self, stream: typing.TextIO, indent: int) -> None:
        self.stream = stream
        self.prefix = " " * indent

    def write_raw(self, s: str) -> None:
        self.stream.write(s)

    def emit(self, arg: str) -> None:
        if arg:
            self.write_raw(f"{self.prefix}{arg}\n")
        else:
            self.write_raw("\n")

    @contextlib.contextmanager
    def indent(self):
        self.prefix += "    "
        yield
        self.prefix = self.prefix[:-4]

    @contextlib.contextmanager
    def block(self, head: str):
        if head:
            self.emit(head + " {")
        else:
            self.emit("{")
        with self.indent():
            yield
        self.emit("}")

    def stack_adjust(self, diff: int):
        if diff > 0:
            self.emit(f"STACK_GROW({diff});")
        elif diff < 0:
            self.emit(f"STACK_SHRINK({-diff});")

    def declare(self, dst: StackEffect, src: StackEffect | None):
        if dst.name == UNUSED:
            return
        typ = f"{dst.type} " if dst.type else "PyObject *"
        init = ""
        if src:
            cast = self.cast(dst, src)
            init = f" = {cast}{src.name}"
        self.emit(f"{typ}{dst.name}{init};")

    def assign(self, dst: StackEffect, src: StackEffect):
        if src.name == UNUSED:
            return
        cast = self.cast(dst, src)
        if m := re.match(r"^PEEK\((\d+)\)$", dst.name):
            self.emit(f"POKE({m.group(1)}, {cast}{src.name});")
        else:
            self.emit(f"{dst.name} = {cast}{src.name};")

    def cast(self, dst: StackEffect, src: StackEffect) -> str:
        return f"({dst.type or 'PyObject *'})" if src.type != dst.type else ""


@dataclasses.dataclass
class Instruction:
    """An instruction with additional data and code."""

    # Parts of the underlying instruction definition
    inst: parser.InstDef
    kind: typing.Literal["inst", "op"]
    name: str
    block: parser.Block
    block_text: list[str]  # Block.text, less curlies, less PREDICT() calls
    predictions: list[str]  # Prediction targets (instruction names)

    # Computed by constructor
    always_exits: bool
    cache_offset: int
    cache_effects: list[parser.CacheEffect]
    input_effects: list[StackEffect]
    output_effects: list[StackEffect]

    # Set later
    family: parser.Family | None = None
    predicted: bool = False

    def __init__(self, inst: parser.InstDef):
        self.inst = inst
        self.kind = inst.kind
        self.name = inst.name
        self.block = inst.block
        self.block_text, self.predictions = extract_block_text(self.block)
        self.always_exits = always_exits(self.block_text)
        self.cache_effects = [
            effect for effect in inst.inputs if isinstance(effect, parser.CacheEffect)
        ]
        self.cache_offset = sum(c.size for c in self.cache_effects)
        self.input_effects = [
            effect for effect in inst.inputs if isinstance(effect, StackEffect)
        ]
        self.output_effects = inst.outputs  # For consistency/completeness

    def write(self, out: Formatter) -> None:
        """Write one instruction, sans prologue and epilogue."""
        # Write a static assertion that a family's cache size is correct
        if family := self.family:
            if self.name == family.members[0]:
                if cache_size := family.size:
                    out.emit(
                        f"static_assert({cache_size} == "
                        f'{self.cache_offset}, "incorrect cache size");'
                    )

        # Write input stack effect variable declarations and initializations
        for i, ieffect in enumerate(reversed(self.input_effects), 1):
            src = StackEffect(f"PEEK({i})", "")
            out.declare(ieffect, src)

        # Write output stack effect variable declarations
        input_names = {ieffect.name for ieffect in self.input_effects}
        for oeffect in self.output_effects:
            if oeffect.name not in input_names:
                out.declare(oeffect, None)

        self.write_body(out, 0)

        # Skip the rest if the block always exits
        if self.always_exits:
            return

        # Write net stack growth/shrinkage
        diff = len(self.output_effects) - len(self.input_effects)
        out.stack_adjust(diff)

        # Write output stack effect assignments
        unmoved_names: set[str] = set()
        for ieffect, oeffect in zip(self.input_effects, self.output_effects):
            if ieffect.name == oeffect.name:
                unmoved_names.add(ieffect.name)
        for i, oeffect in enumerate(reversed(self.output_effects), 1):
            if oeffect.name not in unmoved_names:
                dst = StackEffect(f"PEEK({i})", "")
                out.assign(dst, oeffect)

        # Write cache effect
        if self.cache_offset:
            out.emit(f"JUMPBY({self.cache_offset});")

    def write_body(self, out: Formatter, dedent: int, cache_adjust: int = 0) -> None:
        """Write the instruction body."""
        # Write cache effect variable declarations and initializations
        cache_offset = cache_adjust
        for ceffect in self.cache_effects:
            if ceffect.name != UNUSED:
                bits = ceffect.size * BITS_PER_CODE_UNIT
                if bits == 64:
                    # NOTE: We assume that 64-bit data in the cache
                    # is always an object pointer.
                    # If this becomes false, we need a way to specify
                    # syntactically what type the cache data is.
                    typ = "PyObject *"
                    func = "read_obj"
                else:
                    typ = f"uint{bits}_t "
                    func = f"read_u{bits}"
                out.emit(f"{typ}{ceffect.name} = {func}(&next_instr[{cache_offset}].cache);")
            cache_offset += ceffect.size
        assert cache_offset == self.cache_offset + cache_adjust

        # Write the body, substituting a goto for ERROR_IF()
        assert dedent <= 0
        extra = " " * -dedent
        for line in self.block_text:
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
                    out.write_raw(
                        f"{extra}{space}if ({cond}) goto pop_{ninputs}_{label};\n"
                    )
                else:
                    out.write_raw(f"{extra}{space}if ({cond}) goto {label};\n")
            else:
                out.write_raw(extra + line)


InstructionOrCacheEffect = Instruction | parser.CacheEffect
StackEffectMapping = list[tuple[StackEffect, StackEffect]]


@dataclasses.dataclass
class Component:
    instr: Instruction
    input_mapping: StackEffectMapping
    output_mapping: StackEffectMapping

    def write_body(self, out: Formatter, cache_adjust: int) -> None:
        with out.block(""):
            for var, ieffect in self.input_mapping:
                out.declare(ieffect, var)
            for _, oeffect in self.output_mapping:
                out.declare(oeffect, None)

            self.instr.write_body(out, dedent=-4, cache_adjust=cache_adjust)

            for var, oeffect in self.output_mapping:
                out.assign(var, oeffect)


@dataclasses.dataclass
class SuperOrMacroInstruction:
    """Common fields for super- and macro instructions."""

    name: str
    stack: list[StackEffect]
    initial_sp: int
    final_sp: int


@dataclasses.dataclass
class SuperInstruction(SuperOrMacroInstruction):
    """A super-instruction."""

    super: parser.Super
    parts: list[Component]


@dataclasses.dataclass
class MacroInstruction(SuperOrMacroInstruction):
    """A macro instruction."""

    macro: parser.Macro
    parts: list[Component | parser.CacheEffect]


class Analyzer:
    """Parse input, analyze it, and write to output."""

    filename: str
    output_filename: str
    src: str
    errors: int = 0

    def __init__(self, filename: str, output_filename: str):
        """Read the input file."""
        self.filename = filename
        self.output_filename = output_filename
        with open(filename) as f:
            self.src = f.read()

    def error(self, msg: str, node: parser.Node) -> None:
        lineno = 0
        if context := node.context:
            # Use line number of first non-comment in the node
            for token in context.owner.tokens[context.begin : context.end]:
                lineno = token.line
                if token.kind != "COMMENT":
                    break
        print(f"{self.filename}:{lineno}: {msg}", file=sys.stderr)
        self.errors += 1

    everything: list[parser.InstDef | parser.Super | parser.Macro]
    instrs: dict[str, Instruction]  # Includes ops
    supers: dict[str, parser.Super]
    super_instrs: dict[str, SuperInstruction]
    macros: dict[str, parser.Macro]
    macro_instrs: dict[str, MacroInstruction]
    families: dict[str, parser.Family]

    def parse(self) -> None:
        """Parse the source text.

        We only want the parser to see the stuff between the
        begin and end markers.
        """
        psr = parser.Parser(self.src, filename=self.filename)

        # Skip until begin marker
        while tkn := psr.next(raw=True):
            if tkn.text == BEGIN_MARKER:
                break
        else:
            raise psr.make_syntax_error(
                f"Couldn't find {BEGIN_MARKER!r} in {psr.filename}"
            )
        start = psr.getpos()

        # Find end marker, then delete everything after it
        while tkn := psr.next(raw=True):
            if tkn.text == END_MARKER:
                break
        del psr.tokens[psr.getpos() - 1 :]

        # Parse from start
        psr.setpos(start)
        self.everything = []
        self.instrs = {}
        self.supers = {}
        self.macros = {}
        self.families = {}
        while thing := psr.definition():
            match thing:
                case parser.InstDef(name=name):
                    self.instrs[name] = Instruction(thing)
                    self.everything.append(thing)
                case parser.Super(name):
                    self.supers[name] = thing
                    self.everything.append(thing)
                case parser.Macro(name):
                    self.macros[name] = thing
                    self.everything.append(thing)
                case parser.Family(name):
                    self.families[name] = thing
                case _:
                    typing.assert_never(thing)
        if not psr.eof():
            raise psr.make_syntax_error("Extra stuff at the end")

        print(
            f"Read {len(self.instrs)} instructions/ops, "
            f"{len(self.supers)} supers, {len(self.macros)} macros, "
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
        self.analyze_supers_and_macros()

    def find_predictions(self) -> None:
        """Find the instructions that need PREDICTED() labels."""
        for instr in self.instrs.values():
            targets = set(instr.predictions)
            for line in instr.block_text:
                if m := re.match(RE_PREDICTED, line):
                    targets.add(m.group(1))
            for target in targets:
                if target_instr := self.instrs.get(target):
                    target_instr.predicted = True
                else:
                    self.error(
                        f"Unknown instruction {target!r} predicted in {instr.name!r}",
                        instr.inst,  # TODO: Use better location
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
                self.error(
                    f"Family {family.name!r} has unknown members: {unknown}", family
                )
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

    def analyze_supers_and_macros(self) -> None:
        """Analyze each super- and macro instruction."""
        self.super_instrs = {}
        self.macro_instrs = {}
        for name, super in self.supers.items():
            self.super_instrs[name] = self.analyze_super(super)
        for name, macro in self.macros.items():
            self.macro_instrs[name] = self.analyze_macro(macro)

    def analyze_super(self, super: parser.Super) -> SuperInstruction:
        components = self.check_super_components(super)
        stack, initial_sp = self.stack_analysis(components)
        sp = initial_sp
        parts: list[Component] = []
        for instr in components:
            part, sp = self.analyze_instruction(instr, stack, sp)
            parts.append(part)
        final_sp = sp
        return SuperInstruction(super.name, stack, initial_sp, final_sp, super, parts)

    def analyze_macro(self, macro: parser.Macro) -> MacroInstruction:
        components = self.check_macro_components(macro)
        stack, initial_sp = self.stack_analysis(components)
        sp = initial_sp
        parts: list[Component | parser.CacheEffect] = []
        for component in components:
            match component:
                case parser.CacheEffect() as ceffect:
                    parts.append(ceffect)
                case Instruction() as instr:
                    part, sp = self.analyze_instruction(instr, stack, sp)
                    parts.append(part)
                case _:
                    typing.assert_never(component)
        final_sp = sp
        return MacroInstruction(macro.name, stack, initial_sp, final_sp, macro, parts)

    def analyze_instruction(
        self, instr: Instruction, stack: list[StackEffect], sp: int
    ) -> tuple[Component, int]:
        input_mapping: StackEffectMapping = []
        for ieffect in reversed(instr.input_effects):
            sp -= 1
            input_mapping.append((stack[sp], ieffect))
        output_mapping: StackEffectMapping = []
        for oeffect in instr.output_effects:
            output_mapping.append((stack[sp], oeffect))
            sp += 1
        return Component(instr, input_mapping, output_mapping), sp

    def check_super_components(self, super: parser.Super) -> list[Instruction]:
        components: list[Instruction] = []
        for op in super.ops:
            if op.name not in self.instrs:
                self.error(f"Unknown instruction {op.name!r}", super)
            else:
                components.append(self.instrs[op.name])
        return components

    def check_macro_components(
        self, macro: parser.Macro
    ) -> list[InstructionOrCacheEffect]:
        components: list[InstructionOrCacheEffect] = []
        for uop in macro.uops:
            match uop:
                case parser.OpName(name):
                    if name not in self.instrs:
                        self.error(f"Unknown instruction {name!r}", macro)
                    components.append(self.instrs[name])
                case parser.CacheEffect():
                    components.append(uop)
                case _:
                    typing.assert_never(uop)
        return components

    def stack_analysis(
        self, components: typing.Iterable[InstructionOrCacheEffect]
    ) -> tuple[list[StackEffect], int]:
        """Analyze a super-instruction or macro.

        Print an error if there's a cache effect (which we don't support yet).

        Return the list of variable names and the initial stack pointer.
        """
        lowest = current = highest = 0
        for thing in components:
            match thing:
                case Instruction() as instr:
                    current -= len(instr.input_effects)
                    lowest = min(lowest, current)
                    current += len(instr.output_effects)
                    highest = max(highest, current)
                case parser.CacheEffect():
                    pass
                case _:
                    typing.assert_never(thing)
        # At this point, 'current' is the net stack effect,
        # and 'lowest' and 'highest' are the extremes.
        # Note that 'lowest' may be negative.
        # TODO: Reverse the numbering.
        stack = [
            StackEffect(f"_tmp_{i+1}", "") for i in reversed(range(highest - lowest))
        ]
        return stack, -lowest

    def write_instructions(self) -> None:
        """Write instructions to output file."""
        with open(self.output_filename, "w") as f:
            # Write provenance header
            f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
            f.write(f"// from {os.path.relpath(self.filename)}\n")
            f.write(f"// Do not edit!\n")

            # Create formatter; the rest of the code uses this.
            self.out = Formatter(f, 8)

            # Write and count instructions of all kinds
            n_instrs = 0
            n_supers = 0
            n_macros = 0
            for thing in self.everything:
                match thing:
                    case parser.InstDef():
                        if thing.kind == "inst":
                            n_instrs += 1
                            self.write_instr(self.instrs[thing.name])
                    case parser.Super():
                        n_supers += 1
                        self.write_super(self.super_instrs[thing.name])
                    case parser.Macro():
                        n_macros += 1
                        self.write_macro(self.macro_instrs[thing.name])
                    case _:
                        typing.assert_never(thing)

        print(
            f"Wrote {n_instrs} instructions, {n_supers} supers, "
            f"and {n_macros} macros to {self.output_filename}",
            file=sys.stderr,
        )

    def write_instr(self, instr: Instruction) -> None:
        name = instr.name
        self.out.emit("")
        with self.out.block(f"TARGET({name})"):
            if instr.predicted:
                self.out.emit(f"PREDICTED({name});")
            instr.write(self.out)
            if not instr.always_exits:
                for prediction in instr.predictions:
                    self.out.emit(f"PREDICT({prediction});")
                self.out.emit(f"DISPATCH();")

    def write_super(self, sup: SuperInstruction) -> None:
        """Write code for a super-instruction."""
        with self.wrap_super_or_macro(sup):
            first = True
            for comp in sup.parts:
                if not first:
                    self.out.emit("NEXTOPARG();")
                    self.out.emit("JUMPBY(1);")
                first = False
                comp.write_body(self.out, 0)
                if comp.instr.cache_offset:
                    self.out.emit(f"JUMPBY({comp.instr.cache_offset});")

    def write_macro(self, mac: MacroInstruction) -> None:
        """Write code for a macro instruction."""
        with self.wrap_super_or_macro(mac):
            cache_adjust = 0
            for part in mac.parts:
                match part:
                    case parser.CacheEffect(size=size):
                        cache_adjust += size
                    case Component() as comp:
                        comp.write_body(self.out, cache_adjust)
                        cache_adjust += comp.instr.cache_offset

            if cache_adjust:
                self.out.emit(f"JUMPBY({cache_adjust});")

    @contextlib.contextmanager
    def wrap_super_or_macro(self, up: SuperOrMacroInstruction):
        """Shared boilerplate for super- and macro instructions."""
        # TODO: Somewhere (where?) make it so that if one instruction
        # has an output that is input to another, and the variable names
        # and types match and don't conflict with other instructions,
        # that variable is declared with the right name and type in the
        # outer block, rather than trusting the compiler to optimize it.
        self.out.emit("")
        with self.out.block(f"TARGET({up.name})"):
            for i, var in reversed(list(enumerate(up.stack))):
                src = None
                if i < up.initial_sp:
                    src = StackEffect(f"PEEK({up.initial_sp - i})", "")
                self.out.declare(var, src)

            yield

            self.out.stack_adjust(up.final_sp - up.initial_sp)
            for i, var in enumerate(reversed(up.stack[: up.final_sp]), 1):
                dst = StackEffect(f"PEEK({i})", "")
                self.out.assign(dst, var)

            self.out.emit(f"DISPATCH();")


def extract_block_text(block: parser.Block) -> tuple[list[str], list[str]]:
    # Get lines of text with proper dedent
    blocklines = block.text.splitlines(True)

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

    # Separate PREDICT(...) macros from end
    predictions: list[str] = []
    while blocklines and (m := re.match(r"^\s*PREDICT\((\w+)\);\s*$", blocklines[-1])):
        predictions.insert(0, m.group(1))
        blocklines.pop()

    return blocklines, predictions


def always_exits(lines: list[str]) -> bool:
    """Determine whether a block always ends in a return/goto/etc."""
    if not lines:
        return False
    line = lines[-1].rstrip()
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
    a = Analyzer(args.input, args.output)  # Raises OSError if input unreadable
    a.parse()  # Raises SyntaxError on failure
    a.analyze()  # Prints messages and sets a.errors on failure
    if a.errors:
        sys.exit(f"Found {a.errors} errors")
    a.write_instructions()  # Raises OSError if output can't be written


if __name__ == "__main__":
    main()
