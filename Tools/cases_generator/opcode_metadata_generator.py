"""Generate opcode metadata.
Reads the instruction definitions from bytecodes.c.
Writes the metadata to pycore_opcode_metadata.h by default.
"""

import argparse

from analyzer import (
    Analysis,
    Instruction,
    PseudoInstruction,
    analyze_files,
    Uop,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    cflags,
)
from cwriter import CWriter
from dataclasses import dataclass
from typing import TextIO
from stack import Stack, get_stack_effect, get_stack_effects

# Constants used instead of size for macro expansions.
# Note: 1, 2, 4 must match actual cache entry sizes.
OPARG_KINDS = {
    "OPARG_SIMPLE": 0,
    "OPARG_CACHE_1": 1,
    "OPARG_CACHE_2": 2,
    "OPARG_CACHE_4": 4,
    "OPARG_TOP": 5,
    "OPARG_BOTTOM": 6,
    "OPARG_SAVE_RETURN_OFFSET": 7,
    # Skip 8 as the other powers of 2 are sizes
    "OPARG_REPLACED": 9,
    "OPERAND1_1": 10,
    "OPERAND1_2": 11,
    "OPERAND1_4": 12,
}

FLAGS = [
    "ARG",
    "CONST",
    "NAME",
    "JUMP",
    "FREE",
    "LOCAL",
    "EVAL_BREAK",
    "DEOPT",
    "ERROR",
    "ESCAPES",
    "EXIT",
    "PURE",
    "PASSTHROUGH",
    "OPARG_AND_1",
    "ERROR_NO_POP",
    "NO_SAVE_IP",
]


def generate_flag_macros(out: CWriter) -> None:
    for i, flag in enumerate(FLAGS):
        out.emit(f"#define HAS_{flag}_FLAG ({1<<i})\n")
    for i, flag in enumerate(FLAGS):
        out.emit(
            f"#define OPCODE_HAS_{flag}(OP) (_PyOpcode_opcode_metadata[OP].flags & (HAS_{flag}_FLAG))\n"
        )
    out.emit("\n")


def generate_oparg_macros(out: CWriter) -> None:
    for name, value in OPARG_KINDS.items():
        out.emit(f"#define {name} {value}\n")
    out.emit("\n")


def emit_stack_effect_function(
    out: CWriter, direction: str, data: list[tuple[str, str]]
) -> None:
    out.emit(f"extern int _PyOpcode_num_{direction}(int opcode, int oparg);\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(f"int _PyOpcode_num_{direction}(int opcode, int oparg)  {{\n")
    out.emit("switch(opcode) {\n")
    for name, effect in data:
        out.emit(f"case {name}:\n")
        out.emit(f"    return {effect};\n")
    out.emit("default:\n")
    out.emit("    return -1;\n")
    out.emit("}\n")
    out.emit("}\n\n")
    out.emit("#endif\n\n")


def generate_stack_effect_functions(analysis: Analysis, out: CWriter) -> None:
    popped_data: list[tuple[str, str]] = []
    pushed_data: list[tuple[str, str]] = []

    def add(inst: Instruction | PseudoInstruction) -> None:
        stack = get_stack_effect(inst)
        popped = (-stack.base_offset).to_c()
        pushed = (stack.top_offset - stack.base_offset).to_c()
        popped_data.append((inst.name, popped))
        pushed_data.append((inst.name, pushed))

    for inst in analysis.instructions.values():
        add(inst)
    for pseudo in analysis.pseudos.values():
        add(pseudo)

    emit_stack_effect_function(out, "popped", sorted(popped_data))
    emit_stack_effect_function(out, "pushed", sorted(pushed_data))

    generate_max_stack_effect_function(analysis, out)


def emit_max_stack_effect_function(
    out: CWriter, effects: list[tuple[str, list[str]]]
) -> None:
    out.emit("extern int _PyOpcode_max_stack_effect(int opcode, int oparg, int *effect);\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(f"int _PyOpcode_max_stack_effect(int opcode, int oparg, int *effect)  {{\n")
    out.emit("switch(opcode) {\n")
    for name, exprs in effects:
        out.emit(f"case {name}: {{\n")
        if len(exprs) == 1:
            out.emit(f"*effect = {exprs[0]};\n")
        elif len(exprs) == 2:
            out.emit(f"*effect = Py_MAX({exprs[0]}, {exprs[1]});\n")
        else:
            assert len(exprs) > 2
            out.emit(f"int max_eff = Py_MAX({exprs[0]}, {exprs[1]});\n")
            for expr in exprs[2:]:
                out.emit(f"max_eff = Py_MAX(max_eff, {expr});\n")
            out.emit(f"*effect = max_eff;\n")
        out.emit(f"return 0;\n")
        out.emit("}\n")
    out.emit("default:\n")
    out.emit("    return -1;\n")
    out.emit("}\n")
    out.emit("}\n\n")
    out.emit("#endif\n\n")


@dataclass
class MaxStackEffectSet:
    int_effect: int | None
    cond_effects: set[str]

    def __init__(self) -> None:
        self.int_effect = None
        self.cond_effects = set()

    def add(self, stack: Stack) -> None:
        top_off = stack.top_offset
        top_off_int = top_off.as_int()
        if top_off_int is not None:
            if self.int_effect is None or top_off_int > self.int_effect:
                self.int_effect = top_off_int
        else:
            self.cond_effects.add(top_off.to_c())

    def update(self, other: "MaxStackEffectSet") -> None:
        if self.int_effect is None:
            if other.int_effect is not None:
                self.int_effect = other.int_effect
        elif other.int_effect is not None:
            self.int_effect = max(self.int_effect, other.int_effect)
        self.cond_effects.update(other.cond_effects)


def generate_max_stack_effect_function(analysis: Analysis, out: CWriter) -> None:
    """Generate a function that returns the maximum stack effect of an
    instruction while it is executing.

    Specialized instructions that are composed of uops may have a greater stack
    effect during instruction execution than the net stack effect of the
    instruction if the uops pass values on the stack.
    """
    effects: dict[str, MaxStackEffectSet] = {}

    def add(inst: Instruction | PseudoInstruction) -> None:
        inst_effect = MaxStackEffectSet()
        for stack in get_stack_effects(inst):
            inst_effect.add(stack)
        effects[inst.name] = inst_effect

    # Collect unique stack effects for each instruction
    for inst in analysis.instructions.values():
        add(inst)
    for pseudo in analysis.pseudos.values():
        add(pseudo)

    # Merge the effects of all specializations in a family into the generic
    # instruction
    for family in analysis.families.values():
        for inst in family.members:
            effects[family.name].update(effects[inst.name])

    data: list[tuple[str, list[str]]] = []
    for name, effs in sorted(effects.items(), key=lambda kv: kv[0]):
        exprs = []
        if effs.int_effect is not None:
            exprs.append(str(effs.int_effect))
        exprs.extend(sorted(effs.cond_effects))
        data.append((name, exprs))
    emit_max_stack_effect_function(out, data)


def generate_is_pseudo(analysis: Analysis, out: CWriter) -> None:
    """Write the IS_PSEUDO_INSTR macro"""
    out.emit("\n\n#define IS_PSEUDO_INSTR(OP)  ( \\\n")
    for op in analysis.pseudos:
        out.emit(f"((OP) == {op}) || \\\n")
    out.emit("0")
    out.emit(")\n\n")


def get_format(inst: Instruction) -> str:
    if inst.properties.oparg:
        format = "INSTR_FMT_IB"
    else:
        format = "INSTR_FMT_IX"
    if inst.size > 1:
        format += "C"
    format += "0" * (inst.size - 2)
    return format


def generate_instruction_formats(analysis: Analysis, out: CWriter) -> None:
    # Compute the set of all instruction formats.
    formats: set[str] = set()
    for inst in analysis.instructions.values():
        formats.add(get_format(inst))
    # Generate an enum for it
    out.emit("enum InstructionFormat {\n")
    next_id = 1
    for format in sorted(formats):
        out.emit(f"{format} = {next_id},\n")
        next_id += 1
    out.emit("};\n\n")


def generate_deopt_table(analysis: Analysis, out: CWriter) -> None:
    out.emit("extern const uint8_t _PyOpcode_Deopt[256];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint8_t _PyOpcode_Deopt[256] = {\n")
    deopts: list[tuple[str, str]] = []
    for inst in analysis.instructions.values():
        deopt = inst.name
        if inst.family is not None:
            deopt = inst.family.name
        deopts.append((inst.name, deopt))
    for name, deopt in sorted(deopts):
        out.emit(f"[{name}] = {deopt},\n")
    out.emit("};\n\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")


def generate_cache_table(analysis: Analysis, out: CWriter) -> None:
    out.emit("extern const uint8_t _PyOpcode_Caches[256];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const uint8_t _PyOpcode_Caches[256] = {\n")
    for inst in analysis.instructions.values():
        if inst.family and inst.family.name != inst.name:
            continue
        if inst.name.startswith("INSTRUMENTED"):
            continue
        if inst.size > 1:
            out.emit(f"[{inst.name}] = {inst.size-1},\n")
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_name_table(analysis: Analysis, out: CWriter) -> None:
    table_size = 256 + len(analysis.pseudos)
    out.emit(f"extern const char *_PyOpcode_OpName[{table_size}];\n")
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(f"const char *_PyOpcode_OpName[{table_size}] = {{\n")
    names = list(analysis.instructions) + list(analysis.pseudos)
    for name in sorted(names):
        out.emit(f'[{name}] = "{name}",\n')
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_metadata_table(analysis: Analysis, out: CWriter) -> None:
    table_size = 256 + len(analysis.pseudos)
    out.emit("struct opcode_metadata {\n")
    out.emit("uint8_t valid_entry;\n")
    out.emit("uint8_t instr_format;\n")
    out.emit("uint16_t flags;\n")
    out.emit("};\n\n")
    out.emit(
        f"extern const struct opcode_metadata _PyOpcode_opcode_metadata[{table_size}];\n"
    )
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(
        f"const struct opcode_metadata _PyOpcode_opcode_metadata[{table_size}] = {{\n"
    )
    for inst in sorted(analysis.instructions.values(), key=lambda t: t.name):
        out.emit(
            f"[{inst.name}] = {{ true, {get_format(inst)}, {cflags(inst.properties)} }},\n"
        )
    for pseudo in sorted(analysis.pseudos.values(), key=lambda t: t.name):
        flags = cflags(pseudo.properties)
        for flag in pseudo.flags:
            if flags == "0":
                flags = f"{flag}_FLAG"
            else:
                flags += f" | {flag}_FLAG"
        out.emit(f"[{pseudo.name}] = {{ true, -1, {flags} }},\n")
    out.emit("};\n")
    out.emit("#endif\n\n")


def generate_expansion_table(analysis: Analysis, out: CWriter) -> None:
    expansions_table: dict[str, list[tuple[str, str, int]]] = {}
    for inst in sorted(analysis.instructions.values(), key=lambda t: t.name):
        offset: int = 0  # Cache effect offset
        expansions: list[tuple[str, str, int]] = []  # [(name, size, offset), ...]
        if inst.is_super():
            pieces = inst.name.split("_")
            assert len(pieces) == 4, f"{inst.name} doesn't look like a super-instr"
            name1 = "_".join(pieces[:2])
            name2 = "_".join(pieces[2:])
            assert name1 in analysis.instructions, f"{name1} doesn't match any instr"
            assert name2 in analysis.instructions, f"{name2} doesn't match any instr"
            instr1 = analysis.instructions[name1]
            instr2 = analysis.instructions[name2]
            assert (
                len(instr1.parts) == 1
            ), f"{name1} is not a good superinstruction part"
            assert (
                len(instr2.parts) == 1
            ), f"{name2} is not a good superinstruction part"
            expansions.append((instr1.parts[0].name, "OPARG_TOP", 0))
            expansions.append((instr2.parts[0].name, "OPARG_BOTTOM", 0))
        elif not is_viable_expansion(inst):
            continue
        else:
            for part in inst.parts:
                size = part.size
                if isinstance(part, Uop):
                    # Skip specializations
                    if "specializing" in part.annotations:
                        continue
                    # Add the primary expansion.
                    fmt = "OPARG_SIMPLE"
                    if part.name == "_SAVE_RETURN_OFFSET":
                        fmt = "OPARG_SAVE_RETURN_OFFSET"
                    elif part.caches:
                        fmt = str(part.caches[0].size)
                    if "replaced" in part.annotations:
                        fmt = "OPARG_REPLACED"
                    expansions.append((part.name, fmt, offset))
                    if len(part.caches) > 1:
                        # Add expansion for the second operand
                        internal_offset = 0
                        for cache in part.caches[:-1]:
                            internal_offset += cache.size
                        expansions.append((part.name, f"OPERAND1_{part.caches[-1].size}", offset+internal_offset))
                offset += part.size
        expansions_table[inst.name] = expansions
    max_uops = max(len(ex) for ex in expansions_table.values())
    out.emit(f"#define MAX_UOP_PER_EXPANSION {max_uops}\n")
    out.emit("struct opcode_macro_expansion {\n")
    out.emit("int nuops;\n")
    out.emit(
        "struct { int16_t uop; int8_t size; int8_t offset; } uops[MAX_UOP_PER_EXPANSION];\n"
    )
    out.emit("};\n")
    out.emit(
        "extern const struct opcode_macro_expansion _PyOpcode_macro_expansion[256];\n\n"
    )
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit("const struct opcode_macro_expansion\n")
    out.emit("_PyOpcode_macro_expansion[256] = {\n")
    for inst_name, expansions in expansions_table.items():
        uops = [
            f"{{ {name}, {size}, {offset} }}" for (name, size, offset) in expansions
        ]
        out.emit(
            f'[{inst_name}] = {{ .nuops = {len(expansions)}, .uops = {{ {", ".join(uops)} }} }},\n'
        )
    out.emit("};\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n\n")


def is_viable_expansion(inst: Instruction) -> bool:
    "An instruction can be expanded if all its parts are viable for tier 2"
    for part in inst.parts:
        if isinstance(part, Uop):
            # Skip specializing and replaced uops
            if "specializing" in part.annotations:
                continue
            if "replaced" in part.annotations:
                continue
            if part.properties.tier == 1 or not part.is_viable():
                return False
    return True


def generate_extra_cases(analysis: Analysis, out: CWriter) -> None:
    out.emit("#define EXTRA_CASES \\\n")
    valid_opcodes = set(analysis.opmap.values())
    for op in range(256):
        if op not in valid_opcodes:
            out.emit(f"    case {op}: \\\n")
    out.emit("        ;\n")


def generate_pseudo_targets(analysis: Analysis, out: CWriter) -> None:
    table_size = len(analysis.pseudos)
    max_targets = max(len(pseudo.targets) for pseudo in analysis.pseudos.values())
    out.emit("struct pseudo_targets {\n")
    out.emit(f"uint8_t as_sequence;\n")
    out.emit(f"uint8_t targets[{max_targets + 1}];\n")
    out.emit("};\n")
    out.emit(
        f"extern const struct pseudo_targets _PyOpcode_PseudoTargets[{table_size}];\n"
    )
    out.emit("#ifdef NEED_OPCODE_METADATA\n")
    out.emit(
        f"const struct pseudo_targets _PyOpcode_PseudoTargets[{table_size}] = {{\n"
    )
    for pseudo in analysis.pseudos.values():
        as_sequence = "1" if pseudo.as_sequence else "0"
        targets = ["0"] * (max_targets + 1)
        for i, target in enumerate(pseudo.targets):
            targets[i] = target.name
        out.emit(f"[{pseudo.name}-256] = {{ {as_sequence}, {{ {', '.join(targets)} }} }},\n")
    out.emit("};\n\n")
    out.emit("#endif // NEED_OPCODE_METADATA\n")
    out.emit("static inline bool\n")
    out.emit("is_pseudo_target(int pseudo, int target) {\n")
    out.emit(f"if (pseudo < 256 || pseudo >= {256+table_size}) {{\n")
    out.emit(f"return false;\n")
    out.emit("}\n")
    out.emit(
        f"for (int i = 0; _PyOpcode_PseudoTargets[pseudo-256].targets[i]; i++) {{\n"
    )
    out.emit(
        f"if (_PyOpcode_PseudoTargets[pseudo-256].targets[i] == target) return true;\n"
    )
    out.emit("}\n")
    out.emit(f"return false;\n")
    out.emit("}\n\n")


def generate_opcode_metadata(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 0, False)
    with out.header_guard("Py_CORE_OPCODE_METADATA_H"):
        out.emit("#ifndef Py_BUILD_CORE\n")
        out.emit('#  error "this header requires Py_BUILD_CORE define"\n')
        out.emit("#endif\n\n")
        out.emit("#include <stdbool.h>              // bool\n")
        out.emit('#include "opcode_ids.h"\n')
        generate_is_pseudo(analysis, out)
        out.emit('#include "pycore_uop_ids.h"\n')
        generate_stack_effect_functions(analysis, out)
        generate_instruction_formats(analysis, out)
        table_size = 256 + len(analysis.pseudos)
        out.emit("#define IS_VALID_OPCODE(OP) \\\n")
        out.emit(f"    (((OP) >= 0) && ((OP) < {table_size}) && \\\n")
        out.emit("     (_PyOpcode_opcode_metadata[(OP)].valid_entry))\n\n")
        generate_flag_macros(out)
        generate_oparg_macros(out)
        generate_metadata_table(analysis, out)
        generate_expansion_table(analysis, out)
        generate_name_table(analysis, out)
        generate_cache_table(analysis, out)
        generate_deopt_table(analysis, out)
        generate_extra_cases(analysis, out)
        generate_pseudo_targets(analysis, out)


arg_parser = argparse.ArgumentParser(
    description="Generate the header file with opcode metadata.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)


DEFAULT_OUTPUT = ROOT / "Include/internal/pycore_opcode_metadata.h"


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
        generate_opcode_metadata(args.input, data, outfile)
