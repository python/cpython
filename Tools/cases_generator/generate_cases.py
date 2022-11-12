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


class Analyzer:
    """Parse input, analyze it, and write to output."""

    filename: str
    src: str

    def __init__(self, filename: str):
        """Read the input file."""
        self.filename = filename
        with open(filename) as f:
            self.src = f.read()

    instrs: dict[str, parser.InstDef]
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
            raise psr.make_syntax_error(f"Couldn't find {marker!r} in {psr.filename}")

        # Parse until end marker
        self.instrs = {}
        self.supers = {}
        self.families = {}
        while (tkn := psr.peek(raw=True)) and tkn.text != END_MARKER:
            if instr := psr.inst_def():
                self.instrs[instr.name] = instr
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
        self.compute_cache_offsets()
        self.compute_stack_inputs()
        self.compute_stack_outputs()
        self.map_families()
        errors = self.check_families()
        if errors:
            sys.exit(f"Found {errors} errors")

    predictions: set[str] = set()

    def find_predictions(self) -> None:
        """Find the instructions that need PREDICTED() labels."""
        self.predictions = set()
        for instr in self.instrs.values():
            for target in re.findall(RE_PREDICTED, instr.block.text):
                self.predictions.add(target)

    cache_offsets: dict[str, int]

    def compute_cache_offsets(self) -> None:
        """Compute the amount of cache space used per instruction."""
        self.cache_offsets = {}
        for instr in self.instrs.values():
            cache_offset = 0
            for effect in instr.inputs:
                if isinstance(effect, parser.CacheEffect):
                    cache_offset += effect.size
            self.cache_offsets[instr.name] = cache_offset

    stack_inputs: dict[str, int]

    def compute_stack_inputs(self) -> None:
        """Compute the number of stack items consumed per instruction."""
        self.stack_inputs = {}
        for instr in self.instrs.values():
            stack_input = 0
            for effect in instr.inputs:
                if isinstance(effect, parser.StackEffect):
                    stack_input += 1
            self.stack_inputs[instr.name] = stack_input

    stack_outputs: dict[str, int]

    def compute_stack_outputs(self) -> None:
        """Compute the number of stack items produced per instruction."""
        self.stack_outputs = {}
        for instr in self.instrs.values():
            stack_output = len(instr.outputs)
            self.stack_outputs[instr.name] = stack_output

    family: dict[str, parser.Family]  # instruction name -> family

    def map_families(self) -> None:
        """Make instruction names back to their family, if they have one."""
        self.family = {}
        for family in self.families.values():
            for member in family.members:
                self.family[member] = family

    def check_families(self) -> int:
        """Check each family:

        - Must have at least 2 members
        - All members must be known instructions
        - All members must have the same cache, input and output effects
        """
        errors = 0
        for family in self.families.values():
            if len(family.members) < 2:
                print(f"Family {family.name!r} has insufficient members")
                errors += 1
            members = [member for member in family.members if member in self.instrs]
            if members != family.members:
                unknown = set(family.members) - set(members)
                print(f"Family {family.name!r} has unknown members: {unknown}")
                errors += 1
            if len(members) < 2:
                continue
            head = members[0]
            cache = self.cache_offsets[head]
            input = self.stack_inputs[head]
            output = self.stack_outputs[head]
            for member in members[1:]:
                c = self.cache_offsets[member]
                i = self.stack_inputs[member]
                o = self.stack_outputs[member]
                if (c, i, o) != (cache, input, output):
                    errors += 1
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
        return errors

    indent: str = " " * 8

    def write_instructions(self, filename: str) -> None:
        """Write instructions to output file."""
        indent = self.indent
        with open(filename, "w") as f:
            # Write provenance header
            f.write(f"// This file is generated by {os.path.relpath(__file__)}\n")
            f.write(f"// from {os.path.relpath(self.filename)}\n")
            f.write(f"// Do not edit!\n")

            # Write regular instructions
            for name, instr in self.instrs.items():
                f.write(f"\n{indent}TARGET({name}) {{\n")
                if name in self.predictions:
                    f.write(f"{indent}    PREDICTED({name});\n")
                self.write_instr(f, instr)
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
                    self.write_instr(f, instr, dedent=-4)
                    f.write(f"    {indent}}}\n")
                f.write(f"{indent}    DISPATCH();\n")
                f.write(f"{indent}}}\n")

            print(
                f"Wrote {len(self.instrs)} instructions and "
                f"{len(self.supers)} super-instructions to {filename}",
                file=sys.stderr,
            )

    def write_instr(
        self, f: typing.TextIO, instr: parser.InstDef, dedent: int = 0
    ) -> None:
        """Write one instruction, sans prologue and epilogue."""
        indent = self.indent
        if dedent < 0:
            indent += " " * -dedent  # DO WE NEED THIS?

        # Get cache offset and maybe assert that it is correct
        cache_offset = self.cache_offsets.get(instr.name, 0)
        if family := self.family.get(instr.name):
            if instr.name == family.members[0]:
                if cache_size := family.size:
                    f.write(
                        f"{indent}    static_assert({cache_size} == "
                        f'{cache_offset}, "incorrect cache size");\n'
                    )

        # Separate cache effects from input stack effects
        cache = [
            effect for effect in instr.inputs if isinstance(effect, parser.CacheEffect)
        ]
        stack = [
            effect for effect in instr.inputs if isinstance(effect, parser.StackEffect)
        ]

        # Write cache effect variable declarations
        for ceffect in cache:
            if ceffect.name != "unused":
                bits = ceffect.size * 16
                f.write(
                    f"{indent}    PyObject *{ceffect.name} = "
                    f"read{bits}(next_instr + {cache_offset});\n"
                )

        # Write input stack effect variable declarations and initializations
        for i, seffect in enumerate(reversed(stack), 1):
            if seffect.name != "unused":
                f.write(f"{indent}    PyObject *{seffect.name} = PEEK({i});\n")

        # Write output stack effect variable declarations
        for seffect in instr.outputs:
            if seffect.name != "unused":
                f.write(f"{indent}    PyObject *{seffect.name};\n")

        self.write_instr_body(f, instr, dedent, len(stack))

        # Skip the rest if the block always exits
        if always_exits(instr.block):
            return

        # Write net stack growth/shrinkage
        diff = len(instr.outputs) - len(stack)
        if diff > 0:
            f.write(f"{indent}    STACK_GROW({diff});\n")
        elif diff < 0:
            f.write(f"{indent}    STACK_SHRINK({-diff});\n")

        # Write output stack effect assignments
        input_names = [seffect.name for seffect in stack]
        for i, output in enumerate(reversed(instr.outputs), 1):
            if output.name not in input_names and output.name != "unused":
                f.write(f"{indent}    POKE({i}, {output.name});\n")

        # Write cache effect
        if cache_offset:
            f.write(f"{indent}    next_instr += {cache_offset};\n")

    def write_instr_body(
        self, f: typing.TextIO, instr: parser.InstDef, dedent: int, ninputs: int
    ) -> None:
        """Write the instruction body."""

        # Get lines of text with proper dedelt
        blocklines = instr.block.to_text(dedent=dedent).splitlines(True)

        # Remove blank lines from both ends
        while blocklines and not blocklines[0].strip():
            blocklines.pop(0)
        while blocklines and not blocklines[-1].strip():
            blocklines.pop()

        # Remove leading '{' and trailing '}'
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
                # ERROR_IF() must remove the inputs from the stack.
                # The code block is responsible for DECREF()ing them.
                if ninputs:
                    f.write(f"{space}if ({cond}) goto pop_{ninputs}_{label};\n")
                else:
                    f.write(f"{space}if ({cond}) goto {label};\n")
            else:
                f.write(line)


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
    a.write_instructions(args.output)  # Raises OSError if file can't be written


if __name__ == "__main__":
    main()
