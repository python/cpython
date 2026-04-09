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
    MAX_GENERATED_CACHED_REGISTER,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    cflags,
)
from stack import Stack
from cwriter import CWriter
from typing import Callable, List, TextIO

DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_uop_metadata.h"


def uop_cache_info(uop: Uop, max_cached_register: int) -> list[str] | None:
    if uop.name == "_SPILL_OR_RELOAD":
        return None
    if uop.properties.records_value:
        return None
    default = "{ -1, -1, -1 },\n"
    table_size = max_cached_register + 1
    entries = [ default ] * table_size
    low = max_cached_register + 1
    high = -1
    for inputs, outputs, exit_depth in get_uop_cache_depths(
        uop, max_cached_register=max_cached_register
    ):
        entries[inputs] = f"{{ {outputs}, {exit_depth}, {uop.name}_r{inputs}{outputs} }},\n"
        if inputs < low:
            low = inputs
        if inputs > high:
            high = inputs
    best = [
        str(low if i < low else (high if high < i else i))
        for i in range(max_cached_register + 1)
    ]

    return [ f".best = {{ {', '.join(best)} }},\n", ".entries = {\n",  ] + entries + [ "},\n" ]


CACHING_INFO_DECL = """
typedef struct _pyuop_tos_cache_entry {
    /* input depth is implicit in position */
    int8_t output;
    int8_t exit;
    int16_t opcode;
} _PyUopTOSentry;
"""


def emit_runtime_tables(out: CWriter, max_cached_register: int) -> None:
    out.emit(
        f"typedef struct _PyUopCachingInfo_{max_cached_register} {{\n"
    )
    out.emit(f"    uint8_t best[{max_cached_register + 1}];\n")
    out.emit(f"    _PyUopTOSentry entries[{max_cached_register + 1}];\n")
    out.emit(f"}} _PyUopCachingInfo_{max_cached_register};\n")
    out.emit(
        f"extern const _PyUopCachingInfo_{max_cached_register} "
        f"_PyUop_Caching_{max_cached_register}[MAX_UOP_ID+1];\n"
    )
    out.emit(
        f"extern const uint16_t _PyUop_SpillsAndReloads_{max_cached_register}"
        f"[{max_cached_register + 1}][{max_cached_register + 1}];\n"
    )


def emit_runtime_table_selector(out: CWriter) -> None:
    first = True
    for max_cached_register in range(
        MAX_CACHED_REGISTER, MAX_GENERATED_CACHED_REGISTER + 1
    ):
        directive = "#if" if first else "#elif"
        out.emit(f"{directive} MAX_CACHED_REGISTER == {max_cached_register}\n")
        out.emit(
            f"typedef _PyUopCachingInfo_{max_cached_register} _PyUopCachingInfo;\n"
        )
        out.emit(
            f"#define _PyUop_Caching _PyUop_Caching_{max_cached_register}\n"
        )
        out.emit(
            f"#define _PyUop_SpillsAndReloads "
            f"_PyUop_SpillsAndReloads_{max_cached_register}\n"
        )
        first = False
    out.emit("#else\n")
    out.emit('#error "Unsupported MAX_CACHED_REGISTER value"\n')
    out.emit("#endif\n")
    out.emit("extern const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1];\n\n")


def generate_runtime_metadata(
    analysis: Analysis, out: CWriter, max_cached_register: int
) -> None:
    out.emit(
        f"const _PyUopCachingInfo_{max_cached_register} "
        f"_PyUop_Caching_{max_cached_register}[MAX_UOP_ID+1] = {{\n"
    )
    for uop in analysis.uops.values():
        if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
            info = uop_cache_info(uop, max_cached_register)
            if info is not None:
                out.emit(f"[{uop.name}] = {{\n")
                for line in info:
                    out.emit(line)
                out.emit("},\n")
    out.emit("};\n\n")
    out.emit(
        f"const uint16_t _PyUop_SpillsAndReloads_{max_cached_register}"
        f"[{max_cached_register + 1}][{max_cached_register + 1}] = {{\n"
    )
    for i in range(max_cached_register + 1):
        for j in range(max_cached_register + 1):
            if i != j:
                out.emit(f"[{i}][{j}] = _SPILL_OR_RELOAD_r{i}{j},\n")
    out.emit("};\n\n")


def emit_exact_match_dispatch(
    out: CWriter, emitter: Callable[[int], None]
) -> None:
    first = True
    for max_cached_register in range(
        MAX_CACHED_REGISTER, MAX_GENERATED_CACHED_REGISTER + 1
    ):
        directive = "#if" if first else "#elif"
        out.emit(f"{directive} MAX_CACHED_REGISTER == {max_cached_register}\n")
        emitter(max_cached_register)
        first = False
    out.emit("#else\n")
    out.emit('#error "Unsupported MAX_CACHED_REGISTER value"\n')
    out.emit("#endif\n\n")


def generate_names_and_flags(analysis: Analysis, out: CWriter) -> None:
    out.emit("extern const uint32_t _PyUop_Flags[MAX_UOP_ID+1];\n")
    out.emit("typedef struct _rep_range { uint8_t start; uint8_t stop; } ReplicationRange;\n")
    out.emit("extern const ReplicationRange _PyUop_Replication[MAX_UOP_ID+1];\n")
    out.emit("extern const char * const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1];\n\n")
    out.emit("extern int _PyUop_num_popped(int opcode, int oparg);\n")
    out.emit(CACHING_INFO_DECL)
    for max_cached_register in range(
        MAX_CACHED_REGISTER, MAX_GENERATED_CACHED_REGISTER + 1
    ):
        emit_runtime_tables(out, max_cached_register)
    emit_runtime_table_selector(out)
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint32_t _PyUop_Flags[MAX_UOP_ID+1] = {\n")
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
    emit_exact_match_dispatch(
        out,
        lambda max_cached_register: generate_runtime_metadata(
            analysis, out, max_cached_register
        ),
    )
    def emit_uncached_for_target(max_cached_register: int) -> None:
        out.emit("const uint16_t _PyUop_Uncached[MAX_UOP_REGS_ID+1] = {\n")
        for uop in analysis.uops.values():
            if (
                uop.is_viable()
                and uop.properties.tier != 1
                and not uop.is_super()
                and not uop.properties.records_value
            ):
                for inputs, outputs, _ in get_uop_cache_depths(
                    uop, max_cached_register=max_cached_register
                ):
                    out.emit(f"[{uop.name}_r{inputs}{outputs}] = {uop.name},\n")
        out.emit("};\n")

    emit_exact_match_dispatch(out, emit_uncached_for_target)

    def emit_opname_for_target(max_cached_register: int) -> None:
        out.emit("const char *const _PyOpcode_uop_name[MAX_UOP_REGS_ID+1] = {\n")
        for uop in sorted(analysis.uops.values(), key=lambda t: t.name):
            if uop.is_viable() and uop.properties.tier != 1 and not uop.is_super():
                out.emit(f'[{uop.name}] = "{uop.name}",\n')
                if not uop.properties.records_value:
                    for inputs, outputs, _ in get_uop_cache_depths(
                        uop, max_cached_register=max_cached_register
                    ):
                        out.emit(
                            f'[{uop.name}_r{inputs}{outputs}] = '
                            f'"{uop.name}_r{inputs}{outputs}",\n'
                        )
        out.emit("};\n")

    emit_exact_match_dispatch(out, emit_opname_for_target)
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
