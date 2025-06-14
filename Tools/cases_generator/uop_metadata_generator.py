"""Generate uop metadata.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to pycore_uop_metadata.h by default.
"""

import argparse

from analyzer import (
    Analysis,
    analyze_files,
    get_uop_cache_depths,
    Uop,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    cflags,
)
from stack import Stack
from cwriter import CWriter
from typing import TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_uop_metadata.h"

def uop_cache_info(uop: Uop) -> str | None:
    if uop.name == "_SPILL_OR_RELOAD":
        return None
    min_inputs = 4
    uops = [ "0" ] * 4
    for inputs, outputs in get_uop_cache_depths(uop):
        delta = outputs - inputs
        uops[inputs] = f"{uop.name}_r{inputs}{outputs}"
    for i in range(4):
        if uops[i] != "0":
            max_inputs = i
            if i < min_inputs:
                min_inputs = i
    max_inputs, delta  # type: ignore[possibly-undefined]
    return f"{{ {min_inputs}, {max_inputs}, {delta}, {{ {', '.join(uops)} }} }}"


def generate_names_and_flags(analysis: Analysis, out: CWriter) -> None:
    out.emit("extern const uint16_t _PyUop_Flags[MAX_UOP_ID+1];\n")
    out.emit("extern const uint8_t _PyUop_Replication[MAX_UOP_ID+1];\n")
    out.emit("extern const char * const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1];\n\n")
    out.emit("extern int _PyUop_num_popped(int opcode, int oparg);\n\n")
    out.emit("typedef struct _pyuop_info {\n")
    out.emit("int8_t min_input; int8_t max_input;\n")
    out.emit("int8_t delta; uint16_t opcodes[4];\n")
    out.emit("} _PyUopCachingInfo;\n")
    out.emit("extern const _PyUopCachingInfo _PyUop_Caching[MAX_UOP_ID+1];\n\n")
    out.emit("extern const uint16_t _PyUop_SpillsAndReloads[4][4];\n")
    out.emit("extern const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1];\n\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint16_t _PyUop_Flags[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            out.emit(f"[{uop.name}] = {cflags(uop.properties)},\n")

    out.emit("};\n\n")
    out.emit("const uint8_t _PyUop_Replication[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.replicated:
            out.emit(f"[{uop.name}] = {uop.replicated},\n")

    out.emit("};\n\n")
    out.emit("const _PyUopCachingInfo _PyUop_Caching[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            info = uop_cache_info(uop)
            if info is not None:
                out.emit(f"[{uop.name}] = {info},\n")
    out.emit("};\n\n")
    out.emit("const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1] = {\n");
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            for inputs, outputs in get_uop_cache_depths(uop):
                out.emit(f"[{uop.name}_r{inputs}{outputs}] = {uop.name},\n")
    out.emit("};\n\n")
    out.emit("const uint16_t _PyUop_SpillsAndReloads[4][4] = {\n")
    for i in range(4):
        for j in range(4):
            if i != j:
                out.emit(f"[{i}][{j}] = _SPILL_OR_RELOAD_r{i}{j},\n")
    out.emit("};\n\n")
    out.emit("const char *const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1] = {\n")
    for uop in sorted(analysis.uops.values(), key=lambda t: t.name):
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            out.emit(f'[{uop.name}] = "{uop.name}",\n')
            for inputs, outputs in get_uop_cache_depths(uop):
                out.emit(f'[{uop.name}_r{inputs}{outputs}] = "{uop.name}_r{inputs}{outputs}",\n')
    out.emit("};\n")
    out.emit("int _PyUop_num_popped(int opcode, int oparg)\n{\n")
    out.emit("switch(opcode) {\n")
    null = CWriter.null()
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            stack = Stack()
            for var in reversed(uop.stack.inputs):
                if var.peek:
                    break
                stack.pop(var, null)
            popped = (-stack.base_offset).to_c()
            out.emit(f"case {uop.name}:\n")
            out.emit(f"    return {popped};\n")
    out.emit("default:\n")
    out.emit("    return -1;\n")
    out.emit("}\n")
    out.emit("}\n\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")


def generate_uop_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_UOP_METADATA_H"):
        out.emit("#include <stdint.h>\n")
        out.emit('#include "pycore_uop_ids.h"\n')
        generate_names_and_flags(analysis, out)


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
