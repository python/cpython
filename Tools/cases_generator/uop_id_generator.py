"""Generate the list of uop IDs.
Reads the instruction definitions from bytecodes.c.
Writes the IDs to pycore_uop_ids.h by default.
"""

import argparse

from analyzer import (
    Analysis,
    analyze_files,
    get_uop_cache_depths,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_uop_ids.h"


def generate_uop_ids(
    filenames: list[str], analysis: Analysis, outfile: TextIO, distinct_namespace: bool
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_UOP_IDS_H"):
        start_id = 1 if distinct_namespace else 300
        # next_id only tracks the sequentially numbered ids so that the
        # MAX_UOP_ID / MAX_UOP_REGS_ID macros below can be emitted as plain
        # integer literals (they are used in preprocessor #if conditions,
        # which cannot see enum constants).
        next_id = start_id
        PRE_DEFINED = {"_EXIT_TRACE", "_SET_IP"}

        uops = [(uop.name, uop) for uop in analysis.uops.values()]
        # Sort so that _BASE comes immediately before _BASE_0, etc.  Split the
        # uops into those that get their own sequential id and those that are
        # aliased to the matching tier 1 opcode id.
        sequential_uops: list[str] = []
        aliased_uops: list[str] = []
        for name, uop in sorted(uops):
            if name in PRE_DEFINED or uop.is_super() or uop.properties.tier == 1:
                continue
            if uop.implicitly_created and not distinct_namespace and not uop.replicated:
                aliased_uops.append(name)
            else:
                sequential_uops.append(name)

        # Emit the ids as an enum with automatic numbering.  This way adding or
        # removing a uop only changes its own line (plus the MAX_* macros)
        # instead of renumbering, and therefore rewriting, every line below it.
        out.emit("enum {\n")
        # _EXIT_TRACE and _SET_IP are first by convention.
        out.emit(f"_EXIT_TRACE = {start_id},\n")
        next_id += 1
        out.emit("_SET_IP,\n")
        next_id += 1
        for name in sequential_uops:
            out.emit(f"{name},\n")
            next_id += 1
        out.emit("};\n")
        out.emit(f"#define MAX_UOP_ID {next_id - 1}\n")
        out.emit("\n")

        # Uops implicitly created from a tier 1 instruction share that
        # instruction's opcode id.  These reference the tier 1 opcode name, so
        # they never need renumbering; emit them as plain macros.
        for name in aliased_uops:
            out.emit(f"#define {name} {name[1:]}\n")
        out.emit("\n")

        # The "register" variants are numbered immediately after the base uops.
        reg_labels: list[str] = []
        for name, uop in sorted(uops):
            if uop.properties.tier == 1:
                continue
            if uop.properties.records_value:
                continue
            for inputs, outputs, _ in sorted(get_uop_cache_depths(uop)):
                reg_labels.append(f"{name}_r{inputs}{outputs}")

        if reg_labels:
            out.emit("enum {\n")
            for i, label in enumerate(reg_labels):
                if i == 0:
                    out.emit(f"{label} = MAX_UOP_ID + 1,\n")
                else:
                    out.emit(f"{label},\n")
                next_id += 1
            out.emit("};\n")
        out.emit(f"#define MAX_UOP_REGS_ID {next_id - 1}\n")


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
