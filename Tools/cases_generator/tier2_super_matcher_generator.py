"""Generate the cases for the matcher of the tier 2 super instructions.
"""

import argparse
import os.path
import sys

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    Part,
    analyze_files,
    Skip,
    StackItem,
    analysis_error,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    emit_tokens,
    emit_to,
    REPLACEMENT_FUNCTIONS,
)
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import StackOffset, Stack, SizeMismatch

DEFAULT_OUTPUT = ROOT / "Python/uop_super_matcher_cases.c.h"


def generate_tier2(
    filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
        """
#ifdef TIER_ONE
    #error "This file is for Tier 2 only"
#endif
#define TIER_TWO 2
"""
    )
    out = CWriter(outfile, 2, lines)
    out.emit("\n")
    first_uops: dict[str, list[Instruction]] = {}
    # Extract out common first uops
    for name, super_uop in analysis.super_uops.items():
        first_uop = super_uop.parts[0]
        if first_uop.name in first_uops:
            first_uops[first_uop.name].append(super_uop)
            continue
        first_uops[first_uop.name] = [super_uop]
    for first_uop_name, sub_op in first_uops.items():
        depth = 0
        out.emit(f"case {first_uop_name}: ")
        out.emit("{\n")
        depth += 1
        for super_uop in sub_op:
            middle_uops = super_uop.parts[1:-1]
            last_uop = super_uop.parts[-1]

            oparg = 0
            operand = 0
            for idx, part in enumerate(super_uop.parts):
                if part.properties.oparg:
                    oparg = idx
                if len(part.caches) > 0 and part.caches[0] != "unused":
                    operand = idx

            for part in middle_uops:
                out.emit(f"switch (this_instr[{depth}].opcode) {{\n")
                out.emit(f"case {part.name}: ")
                out.emit("{\n")
                depth += 1
            out.emit(f"if (this_instr[{depth}].opcode == {last_uop.name}) {{\n")
            out.emit(f'DPRINTF(2, "Inserting super {super_uop.name}\\n");')
            out.emit(f"REPLACE_OP(this_instr, {super_uop.name}, this_instr[{oparg}].oparg, this_instr[{operand}].operand);\n")
            out.emit(f"for (int i = 1; i < {depth + 1}; i++) {{ REPLACE_OP((&this_instr[i]), _NOP, 0, 0); }}")
            out.emit(f"this_instr += {depth + 1};\n")
            out.emit("break;\n")
            out.emit("}\n")
            for part in middle_uops:
                out.start_line()
                out.emit("break;\n")
                out.start_line()
                out.emit("}\n")
                out.emit("}\n")
                out.start_line()

        out.emit(f"this_instr += 1;\n")
        out.emit("break;\n")
        out.emit("}")
        out.emit("\n\n")

    outfile.write("#undef TIER_TWO\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the tier 2 interpreter.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "-l", "--emit-line-directives", help="Emit #line directives", action="store_true"
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_tier2(args.input, data, outfile, args.emit_line_directives)
