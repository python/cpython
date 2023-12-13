"""Generate uop metedata.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to pycore_uop_metadata.h by default.
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
    cflags,
)
from cwriter import CWriter
from typing import TextIO


DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_uop_metadata.h"

def generate_names_and_flags(
    analysis: Analysis, out: CWriter
) -> None:
    out.emit("extern const uint16_t _PyUop_Flags[MAX_UOP_ID+1];\n")
    out.emit("extern const char * const _PyOpcode_uop_name[MAX_UOP_ID+1];\n\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint16_t _PyUop_Flags[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        out.emit(f"[{uop.name}] = {cflags(uop.properties)},\n")

    out.emit("};\n\n")
    out.emit("const char *const _PyOpcode_uop_name[MAX_UOP_ID+1] = {\n")
    names = [uop.name for uop in analysis.uops.values()]
    for name in sorted(names):
        out.emit(f'[{name}] = "{name}",\n')
    out.emit("};\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")

def generate_expansion_table(
    analysis: Analysis, out: CWriter
) -> None:
    out.emit("extern const struct opcode_macro_expansion\n")
    out.emit("_PyOpcode_macro_expansion[OPCODE_MACRO_EXPANSION_SIZE];\n\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const struct opcode_macro_expansion\n")
    out.emit("_PyOpcode_macro_expansion[OPCODE_MACRO_EXPANSION_SIZE] = {\n")

    
    out.emit("};\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")
    
def generate_uop_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_UOP_METADATA_H"):
        out.emit('#include <stdint.h>\n')
        out.emit('#include "pycore_uop_ids.h"\n')
        generate_names_and_flags(analysis, out)
        generate_expansion_table(analysis, out)

arg_parser = argparse.ArgumentParser(
    description="Generate the header file with uop metadata.",
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
        generate_uop_metadata(args.input, data, outfile)
