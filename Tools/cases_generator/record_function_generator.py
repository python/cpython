
import argparse

from analyzer import (
    Analysis,
    Instruction,
    analyze_files,
    CodeSection,
)

from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    emit_to,
    TokenIterator,
)

from cwriter import CWriter

from tier1_generator import write_uop, Emitter, declare_variable
from typing import TextIO
from lexer import Token
from stack import Stack, Storage

DEFAULT_OUTPUT = ROOT / "Python/recorder_functions.c.h"

# Must match MAX_RECORDED_VALUES in Include/internal/pycore_optimizer.h.
MAX_RECORDED_VALUES = 3

# Map `_RECORD_*` uops to the helper that converts a raw family-recorded
# value to the form the specialized member consumes.
_RECORD_TRANSFORM_HELPERS: dict[str, str] = {
    "_RECORD_TOS_TYPE": "record_trace_transform_to_type",
    "_RECORD_NOS_TYPE": "record_trace_transform_to_type",
    "_RECORD_NOS_GEN_FUNC": "record_trace_transform_gen_func",
    "_RECORD_3OS_GEN_FUNC": "record_trace_transform_gen_func",
    "_RECORD_BOUND_METHOD": "record_trace_transform_bound_method",
}

# Recorder uops whose slot kind differs from the leading word of their name.
_RECORD_SLOT_KIND_OVERRIDES: dict[str, str] = {
    "_RECORD_BOUND_METHOD": "CALLABLE",
}


class RecorderEmitter(Emitter):
    def __init__(self, out: CWriter):
        super().__init__(out, {})
        self._replacers["RECORD_VALUE"] = self.record_value

    def record_value(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        lparen = next(tkn_iter)
        self.out.start_line()
        self.emit("*recorded_value = (PyObject *)")
        emit_to(self.out, tkn_iter, "RPAREN")
        next(tkn_iter)  # Semi colon
        self.emit(";\n")
        self.emit("Py_INCREF(*recorded_value);\n")
        return True


def get_record_slot_kind(record_name: str) -> str:
    if record_name in _RECORD_SLOT_KIND_OVERRIDES:
        return _RECORD_SLOT_KIND_OVERRIDES[record_name]
    if not record_name.startswith("_RECORD_"):
        return record_name
    return record_name.removeprefix("_RECORD_").partition("_")[0]


def get_instruction_record_names(inst: Instruction) -> list[str]:
    return [part.name for part in inst.parts if part.properties.records_value]


def get_family_record_names(
    family_head: Instruction,
    family_members: list[Instruction],
    instruction_records: dict[str, list[str]],
    record_slot_keys: dict[str, str],
) -> list[str]:
    member_records = [instruction_records[m.name] for m in family_members]
    all_member_names = {n for names in member_records for n in names}
    records: list[str] = []
    slot_index: dict[str, int] = {}

    def add(name: str) -> None:
        kind = record_slot_keys[name]
        # Prefer the raw recorder if any member uses it; otherwise the given form.
        raw = f"_RECORD_{kind}"
        source = raw if raw in all_member_names else name
        existing = slot_index.get(kind)
        if existing is None:
            slot_index[kind] = len(records)
            records.append(source)
        elif records[existing] != source:
            raise ValueError(
                f"Family {family_head.name} has incompatible recorders for "
                f"slot {kind}: {records[existing]} and {source}"
            )

    for names in member_records:
        for name in names:
            add(name)
    # Family head supplies any slots no member exercises.
    for name in instruction_records[family_head.name]:
        if record_slot_keys[name] not in slot_index:
            slot_index[record_slot_keys[name]] = len(records)
            records.append(name)
    return records


def get_record_consumer_layout(
    inst_name: str,
    source_records: list[str],
    own_records: list[str],
    record_slot_keys: dict[str, str],
) -> tuple[list[int], int]:
    used = [False] * len(source_records)
    slot_map: list[int] = []
    transform_mask = 0
    for i, own in enumerate(own_records):
        own_kind = record_slot_keys[own]
        for j, src in enumerate(source_records):
            if not used[j] and record_slot_keys[src] == own_kind:
                used[j] = True
                slot_map.append(j)
                if src != own:
                    transform_mask |= 1 << i
                break
        else:
            raise ValueError(
                f"Instruction {inst_name} has no compatible family slot for "
                f"{own} in {source_records}"
            )
    return slot_map, transform_mask

def generate_recorder_functions(filenames: list[str], analysis: Analysis, out: CWriter) -> None:
    write_header(__file__, filenames, out.out)
    out.out.write(
        """
#ifdef TIER_ONE
    #error "This file is for Tier 2 only"
#endif
"""
    )
    args = "_PyInterpreterFrame *frame, _PyStackRef *stack_pointer, int oparg, PyObject **recorded_value"
    emitter = RecorderEmitter(out)
    nop = analysis.instructions["NOP"]
    for uop in analysis.uops.values():
        if not uop.properties.records_value:
            continue
        out.emit(f"void _PyOpcode_RecordFunction{uop.name[7:]}({args}) {{\n")
        seen = {"unused"}
        for var in uop.stack.inputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out)
        stack = Stack()
        write_uop(uop, emitter, 0, stack, nop, False)
        out.start_line()
        out.emit("}")
        out.emit("\n\n")

def generate_recorder_tables(analysis: Analysis, out: CWriter) -> None:
    instruction_records = {
        inst.name: get_instruction_record_names(inst)
        for inst in analysis.instructions.values()
    }
    record_uop_names = [
        name for name, uop in analysis.uops.items() if uop.properties.records_value
    ]
    record_slot_keys = {name: get_record_slot_kind(name) for name in record_uop_names}
    family_record_table = {
        family.name: get_family_record_names(
            analysis.instructions[family.name],
            family.members,
            instruction_records,
            record_slot_keys,
        )
        for family in analysis.families.values()
    }

    record_table: dict[str, list[str]] = {}
    record_consumer_table: dict[str, tuple[list[int], int]] = {}
    record_function_indexes: dict[str, int] = {}
    for inst in analysis.instructions.values():
        own_records = instruction_records[inst.name]
        # TRACE_RECORD runs before execution, but specialization may rewrite
        # the opcode before translation. Record the raw family shape (union
        # of head + members) so any opcode in the family can be translated
        # from the same recorded layout.
        family = inst.family or analysis.families.get(inst.name)
        records = family_record_table[family.name] if family is not None else own_records
        if not records:
            continue
        if len(records) > MAX_RECORDED_VALUES:
            raise ValueError(
                f"Instruction {inst.name} has {len(records)} recording ops, "
                f"exceeds MAX_RECORDED_VALUES ({MAX_RECORDED_VALUES})"
            )
        record_table[inst.name] = records
        for name in records:
            if name not in record_function_indexes:
                record_function_indexes[name] = len(record_function_indexes) + 1
        if own_records:
            record_consumer_table[inst.name] = get_record_consumer_layout(
                inst.name, records, own_records, record_slot_keys
            )

    for name, index in record_function_indexes.items():
        out.emit(f"#define {name}_INDEX {index}\n")
    out.emit("\n")

    out.emit("const _PyOpcodeRecordEntry _PyOpcode_RecordEntries[256] = {\n")
    for inst_name, records in record_table.items():
        indices = ", ".join(f"{name}_INDEX" for name in records)
        out.emit(f"    [{inst_name}] = {{{len(records)}, {{{indices}}}}},\n")
    out.emit("};\n\n")

    out.emit("const _PyOpcodeRecordSlotMap _PyOpcode_RecordSlotMaps[256] = {\n")
    for inst_name, (slots, mask) in record_consumer_table.items():
        slot_list = ", ".join(str(s) for s in slots)
        out.emit(
            f"    [{inst_name}] = {{{len(slots)}, {mask}, {{{slot_list}}}}},\n"
        )
    out.emit("};\n\n")

    out.emit(
        f"const _Py_RecordFuncPtr _PyOpcode_RecordFunctions"
        f"[{len(record_function_indexes) + 1}] = {{\n"
    )
    out.emit("    [0] = NULL,\n")
    for name in record_function_indexes:
        out.emit(f"    [{name}_INDEX] = _PyOpcode_RecordFunction{name[7:]},\n")
    out.emit("};\n")
    generate_record_transform_dispatcher(record_uop_names, out)


def generate_record_transform_dispatcher(
    record_uop_names: list[str], out: CWriter
) -> None:
    """Emit a switch that converts a family-recorded value for a recorder uop.

    Only `_RECORD_*` uops that need conversion get a case; the default
    returns the input value unchanged. Helpers live in Python/optimizer.c.
    """
    cases: dict[str, list[str]] = {}
    for record_name in record_uop_names:
        helper = _RECORD_TRANSFORM_HELPERS.get(record_name)
        if helper is None:
            continue
        cases.setdefault(helper, []).append(record_name)
    out.emit("\n")
    out.emit(
        "PyObject *\n"
        "_PyOpcode_RecordTransformValue(int uop, PyObject *value)\n"
        "{\n"
    )
    out.emit("    switch (uop) {\n")
    for helper, names in cases.items():
        for name in names:
            out.emit(f"        case {name}:\n")
        out.emit(f"            return {helper}(value);\n")
    out.emit("        default:\n")
    out.emit("            return value;\n")
    out.emit("    }\n")
    out.emit("}\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the record functions for the trace recorder.",
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
        out = CWriter(outfile, 0, False)
        generate_recorder_functions(args.input, data, out)
        generate_recorder_tables(data, out)
