"""Generate the list of opcode IDs.
Reads the instruction definitions from bytecodes.c.
Writes the IDs to opcode._ids.h by default.
"""

import argparse
import os.path
import sys

from analyzer import (
    Analysis,
    Instruction,
    analyze_files,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Include/opcode_ids.h"


def generate_opcode_header(filenames: str, analysis: Analysis, outfile: TextIO) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    out.emit("\n")
    instmap: dict[str, int] = {}

    # 0 is reserved for cache entries. This helps debugging.
    instmap["CACHE"] = 0

    # 17 is reserved as it is the initial value for the specializing counter.
    # This helps catch cases where we attempt to execute a cache.
    instmap["RESERVED"] = 17

    # 149 is RESUME - it is hard coded as such in Tools/build/deepfreeze.py
    instmap["RESUME"] = 149
    instmap["INSTRUMENTED_LINE"] = 254

    instrumented = [
        name for name in analysis.instructions if name.startswith("INSTRUMENTED")
    ]

    # Special case: this instruction is implemented in ceval.c
    # rather than bytecodes.c, so we need to add it explicitly
    # here (at least until we add something to bytecodes.c to
    # declare external instructions).
    instrumented.append("INSTRUMENTED_LINE")

    specialized: set[str] = set()
    no_arg: list[str] = []
    has_arg: list[str] = []

    for family in analysis.families.values():
        specialized.update(inst.name for inst in family.members)

    for inst in analysis.instructions.values():
        name = inst.name
        if name in specialized:
            continue
        if name in instrumented:
            continue
        if inst.properties.oparg:
            has_arg.append(name)
        else:
            no_arg.append(name)

    # Specialized ops appear in their own section
    # Instrumented opcodes are at the end of the valid range
    min_internal = 150
    min_instrumented = 254 - (len(instrumented) - 1)
    assert min_internal + len(specialized) < min_instrumented

    next_opcode = 1

    def add_instruction(name: str) -> None:
        nonlocal next_opcode
        if name in instmap:
            return  # Pre-defined name
        while next_opcode in instmap.values():
            next_opcode += 1
        instmap[name] = next_opcode
        next_opcode += 1

    for name in sorted(no_arg):
        add_instruction(name)
    for name in sorted(has_arg):
        add_instruction(name)
    # For compatibility
    next_opcode = min_internal
    for name in sorted(specialized):
        add_instruction(name)
    next_opcode = min_instrumented
    for name in instrumented:
        add_instruction(name)

    for op, name in enumerate(sorted(analysis.pseudos), 256):
        instmap[name] = op

    assert 255 not in instmap.values()

    out.emit(
        """#ifndef Py_OPCODE_IDS_H
#define Py_OPCODE_IDS_H
#ifdef __cplusplus
extern "C" {
#endif

/* Instruction opcodes for compiled code */
"""
    )

    def write_define(name: str, op: int) -> None:
        out.emit(f"#define {name:<38} {op:>3}\n")

    for op, name in sorted([(op, name) for (name, op) in instmap.items()]):
        write_define(name, op)

    out.emit("\n")
    write_define("HAVE_ARGUMENT", len(no_arg))
    write_define("MIN_INSTRUMENTED_OPCODE", min_instrumented)

    out.emit("\n")
    out.emit("#ifdef __cplusplus\n")
    out.emit("}\n")
    out.emit("#endif\n")
    out.emit("#endif /* !Py_OPCODE_IDS_H */\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with all opcode IDs.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
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
        generate_opcode_header(args.input, data, outfile)
