
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
    func_count = 0
    nop = analysis.instructions["NOP"]
    function_table: dict[str, int] = dict()
    for name, uop in analysis.uops.items():
        if not uop.properties.records_value:
            continue
        func_count += 1
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
    index = 1
    for inst in analysis.instructions.values():
        if not inst.properties.records_value:
            continue
        records: list[str] = []
        for part in inst.parts:
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
    func_count = len(record_function_indexes)

    for name, index in record_function_indexes.items():
        out.emit(f"#define {name}_INDEX {index}\n")
    out.emit("\n")
    out.emit("const _PyOpcodeRecordEntry _PyOpcode_RecordEntries[256] = {\n")
    for inst_name, record_names in record_table.items():
        indices = ", ".join(f"{name}_INDEX" for name in record_names)
        out.emit(f"    [{inst_name}] = {{{len(record_names)}, {{{indices}}}}},\n")
    out.emit("};\n\n")
    out.emit(f"const _Py_RecordFuncPtr _PyOpcode_RecordFunctions[{func_count+1}] = {{\n")
    out.emit("    [0] = NULL,\n")
    for name in record_function_indexes:
        out.emit(f"    [{name}_INDEX] = _PyOpcode_RecordFunction{name[7:]},\n")
    out.emit("};\n")


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
