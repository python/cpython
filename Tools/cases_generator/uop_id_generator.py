"""Generate the list of uop IDs.
Reads the instruction definitions from bytecodes.c.
Writes the IDs to pycore_uop_ids.h by default.
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


DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_uop_ids.h"


OMIT = {"_CACHE", "_RESERVED", "_EXTENDED_ARG"}


def generate_uop_ids(
    filenames: list[str], analysis: Analysis, outfile: TextIO, distinct_namespace: bool
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    out.emit(
        """#ifndef Py_CORE_UOP_IDS_H
#define Py_CORE_UOP_IDS_H
#ifdef __cplusplus
extern "C" {
#endif

"""
    )

    next_id = 1 if distinct_namespace else 300
    # These two are first by convention
    out.emit(f"#define _EXIT_TRACE {next_id}\n")
    next_id += 1
    out.emit(f"#define _SET_IP {next_id}\n")
    next_id += 1
    PRE_DEFINED = {"_EXIT_TRACE", "_SET_IP"}

    for uop in analysis.uops.values():
        if uop.name in PRE_DEFINED:
            continue
        # TODO: We should omit all tier-1 only uops, but
        # generate_cases.py still generates code for those.
        if uop.name in OMIT:
            continue
        if uop.implicitly_created and not distinct_namespace:
            out.emit(f"#define {uop.name} {uop.name[1:]}\n")
        else:
            out.emit(f"#define {uop.name} {next_id}\n")
            next_id += 1

    out.emit("\n")
    out.emit("#ifdef __cplusplus\n")
    out.emit("}\n")
    out.emit("#endif\n")
    out.emit("#endif /* !Py_OPCODE_IDS_H */\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with all uop IDs.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)
arg_parser.add_argument(
    "-n",
    "--namespace",
    help="Give uops a distinct namespace",
    action="store_true",
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
        generate_uop_ids(args.input, data, outfile, args.namespace)
