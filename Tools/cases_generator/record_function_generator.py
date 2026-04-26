
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
    if not record_name.startswith("_RECORD_"):
        return record_name
    slot_name = record_name.removeprefix("_RECORD_")
    if slot_name == "BOUND_METHOD":
        return "CALLABLE"
    base_name, separator, _ = slot_name.partition("_")
    if separator:
        return base_name
    return slot_name


def get_record_slot_map(
    inst_name: str,
    source_records: list[str],
    own_records: list[str],
    record_slot_keys: dict[str, str],
) -> list[int]:
    used = [False] * len(source_records)
    slot_map: list[int] = []
    for own_record in own_records:
        own_slot = record_slot_keys[own_record]
        for index, source_record in enumerate(source_records):
            if used[index]:
                continue
            if record_slot_keys[source_record] == own_slot:
                used[index] = True
                slot_map.append(index)
                break
        else:
            raise ValueError(
                f"Instruction {inst_name} has no compatible family slot for "
                f"{own_record} in {source_records}"
            )
    return slot_map

def generate_recorder_functions(filenames: list[str], analysis: Analysis, out: CWriter) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
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
    record_function_indexes: dict[str, int] = dict()
    record_table: dict[str, list[str]] = {}
    record_consumer_table: dict[str, tuple[list[int], bool]] = {}
    record_uop_names = [
        name
        for name, uop in analysis.uops.items()
        if uop.properties.records_value
    ]
    record_slot_keys = {
        name: get_record_slot_kind(name) for name in record_uop_names
    }
    index = 1
    for inst in analysis.instructions.values():
        source_inst = inst
        own_records = [
            part.name for part in inst.parts if part.properties.records_value
        ]
        if inst.family is not None:
            # TRACE_RECORD runs before execution, but specialization may
            # rewrite the opcode before translation. Use the family head's
            # raw recording shape so every member records the same layout.
            family_head = analysis.instructions[inst.family.name]
            if family_head.properties.records_value:
                source_inst = family_head
        is_family_override = source_inst is not inst
        records: list[str] = []
        for part in source_inst.parts:
            if not part.properties.records_value:
                continue
            if part.name not in record_function_indexes:
                record_function_indexes[part.name] = index
                index += 1
            records.append(part.name)
        if records:
            if len(records) > MAX_RECORDED_VALUES:
                raise ValueError(
                    f"Instruction {inst.name} has {len(records)} recording ops, "
                    f"exceeds MAX_RECORDED_VALUES ({MAX_RECORDED_VALUES})"
                )
            record_table[inst.name] = records
            if own_records:
                slot_map = get_record_slot_map(
                    inst.name,
                    records,
                    own_records,
                    record_slot_keys,
                )
                needs_transform = is_family_override and any(
                    own_record != records[slot]
                    for own_record, slot in zip(own_records, slot_map)
                )
                record_consumer_table[inst.name] = (slot_map, needs_transform)
    func_count = len(record_function_indexes)

    for name, index in record_function_indexes.items():
        out.emit(f"#define {name}_INDEX {index}\n")
    out.emit("\n")
    out.emit("const _PyOpcodeRecordEntry _PyOpcode_RecordEntries[256] = {\n")
    for inst_name, record_names in record_table.items():
        indices = ", ".join(f"{name}_INDEX" for name in record_names)
        out.emit(f"    [{inst_name}] = {{{len(record_names)}, {{{indices}}}}},\n")
    out.emit("};\n\n")
    out.emit("const _PyOpcodeRecordSlotMap _PyOpcode_RecordSlotMaps[256] = {\n")
    for inst_name, (slots, needs_transform) in record_consumer_table.items():
        slot_list = ", ".join(str(slot) for slot in slots)
        out.emit(
            f"    [{inst_name}] = {{{len(slots)}, {int(needs_transform)}, {{{slot_list}}}}},\n"
        )
    out.emit("};\n\n")
    out.emit(f"const _Py_RecordFuncPtr _PyOpcode_RecordFunctions[{func_count+1}] = {{\n")
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
