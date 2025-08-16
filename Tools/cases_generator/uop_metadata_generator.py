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
    MAX_CACHED_REGISTER,
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

def uop_cache_info(uop: Uop) -> list[str] | None:
    if uop.name == "_SPILL_OR_RELOAD":
        return None
    default = "{ -1, -1, -1 },\n"
    table_size = MAX_CACHED_REGISTER + 1
    entries = [ default ] * table_size
    low = MAX_CACHED_REGISTER+1
    high = -1
    defined = [ False ] * 4
    for inputs, outputs, exit_depth in get_uop_cache_depths(uop):
        entries[inputs] = f"{{ {outputs}, {exit_depth}, {uop.name}_r{inputs}{outputs} }},\n"
        if inputs < low:
            low = inputs
        if inputs > high:
            high = inputs
    best = [ str(low if i < low else (high if high < i else i)) for i in range(MAX_CACHED_REGISTER+1) ]

    return [ f".best = {{ {', '.join(best)} }},\n", ".entries = {\n",  ] + entries + [ "},\n" ]

CACHING_INFO_DECL = """
typedef struct _pyuop_tos_cache_entry {
    /* input depth is implicit in position */
    int8_t output;
    int8_t exit;
    uint16_t opcode;
} _PyUopTOSentry;

typedef struct _PyUopCachingInfo {
    uint8_t best[MAX_CACHED_REGISTER + 1];
    _PyUopTOSentry entries[MAX_CACHED_REGISTER + 1];
} _PyUopCachingInfo;

extern const _PyUopCachingInfo _PyUop_Caching[MAX_UOP_ID+1];
"""


def generate_names_and_flags(analysis: Analysis, out: CWriter) -> None:
    out.emit(f"#define MAX_CACHED_REGISTER {MAX_CACHED_REGISTER}\n")
    out.emit("extern const uint16_t _PyUop_Flags[MAX_UOP_ID+1];\n")
    out.emit("typedef struct _rep_range { uint8_t start; uint8_t stop; } ReplicationRange;\n")
    out.emit("extern const ReplicationRange _PyUop_Replication[MAX_UOP_ID+1];\n")
    out.emit("extern const char * const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1];\n\n")
    out.emit("extern int _PyUop_num_popped(int opcode, int oparg);\n")
    out.emit(CACHING_INFO_DECL)
    out.emit(f"extern const uint16_t _PyUop_SpillsAndReloads[{MAX_CACHED_REGISTER+1}][{MAX_CACHED_REGISTER+1}];\n")
    out.emit("extern const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1];\n\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint16_t _PyUop_Flags[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            out.emit(f"[{uop.name}] = {cflags(uop.properties)},\n")

    out.emit("};\n\n")
    out.emit("const ReplicationRange _PyUop_Replication[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.replicated:
            assert(uop.replicated.step == 1)
            out.emit(f"[{uop.name}] = {{ {uop.replicated.start}, {uop.replicated.stop} }},\n")

    out.emit("};\n\n")
    out.emit("const _PyUopCachingInfo _PyUop_Caching[MAX_UOP_ID+1] = {\n")
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            info = uop_cache_info(uop)
            if info is not None:
                out.emit(f"[{uop.name}] = {{\n")
                for line in info:
                    out.emit(line)
                out.emit("},\n")
    out.emit("};\n\n")
    out.emit("const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1] = {\n");
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            for inputs, outputs, _ in get_uop_cache_depths(uop):
                out.emit(f"[{uop.name}_r{inputs}{outputs}] = {uop.name},\n")
    out.emit("};\n\n")
    out.emit(f"const uint16_t _PyUop_SpillsAndReloads[{MAX_CACHED_REGISTER+1}][{MAX_CACHED_REGISTER+1}] = {{\n")
    for i in range(MAX_CACHED_REGISTER+1):
        for j in range(MAX_CACHED_REGISTER+1):
            if i != j:
                out.emit(f"[{i}][{j}] = _SPILL_OR_RELOAD_r{i}{j},\n")
    out.emit("};\n\n")
    out.emit("const char *const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1] = {\n")
    for uop in sorted(analysis.uops.values(), key=lambda t: t.name):
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            out.emit(f'[{uop.name}] = "{uop.name}",\n')
            for inputs, outputs, _ in get_uop_cache_depths(uop):
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
