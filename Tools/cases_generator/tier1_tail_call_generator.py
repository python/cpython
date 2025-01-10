"""Generate the main interpreter tail call handlers.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_tail_call_handlers.c.h, which is #included in ceval.c.
"""

import argparse
import re

from typing import TextIO

from generators_common import (
    ROOT,
    write_header,
    CWriter,
    Emitter,
)

from analyzer import (
    Analysis,
    analyze_files,
)

from tier1_generator import (
    write_single_inst
)

DEFAULT_INPUT = ROOT / "Python/generated_tail_call_handlers.c.h"
DEFAULT_OUTPUT = ROOT / "Python/generated_tail_call_handlers.c.h"

DEFAULT_CEVAL_INPUT = ROOT / "Python/ceval.c"

FOOTER = "#undef TIER_ONE\n#undef IN_TAIL_CALL_INTERP\n"

TARGET_LABEL = "TAIL_CALL_TARGET"

def generate_label_handlers(outfile: TextIO):
    out = CWriter(outfile, 0, False)
    with open(DEFAULT_CEVAL_INPUT, "r") as fp:
        str_in = fp.read()
        # https://stackoverflow.com/questions/8303488/regex-to-match-any-character-including-new-lines
        eval_framedefault = re.findall("_PyEval_EvalFrameDefault\(.*\)\n({[\s\S]*\/\* END_BASE_INTERPRETER \*\/)", str_in)[0]
    function_protos = re.findall(f"{TARGET_LABEL}\((\w+)\):", eval_framedefault)
    for proto in function_protos:
        out.emit(f"{function_proto(proto)};\n")
    lines = iter(eval_framedefault[eval_framedefault.find(TARGET_LABEL):].split("\n"))
    next(lines)
    for i in range(len(function_protos)):
        curr_proto = function_protos[i]
        fallthrough_proto = function_protos[i + 1] if i + 1 < len(function_protos) else None
        out.emit(function_proto(curr_proto))
        out.emit("\n")
        out.emit("{\n")
        out.emit("int opcode = next_instr->op.code;\n")
        out.emit("(void)opcode;\n")
        for line in lines:
            if TARGET_LABEL in line:
                break
            if label := re.findall("goto (\w+);", line):
                out.emit(f"CEVAL_GOTO({label[0]});\n")
            else:
                out.emit(line)
                out.emit("\n")
        if fallthrough_proto:
            out.emit(f"CEVAL_GOTO({fallthrough_proto});\n")
        out.emit("}\n")


def function_proto(name: str) -> str:
    return f"Py_PRESERVE_NONE_CC static PyObject *_TAIL_CALL_{name}(TAIL_CALL_PARAMS)"

def generate_tier1(
    filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
        """
#ifndef Py_TAIL_CALL_INTERP
    #error "This file is for tail-calling interpreter only."
#endif
#define TIER_ONE 1
#define IN_TAIL_CALL_INTERP 1
"""
    )
    out = CWriter(outfile, 0, lines)
    out.emit("static inline PyObject *_TAIL_CALL_shim(TAIL_CALL_PARAMS);\n")
    out.emit("static py_tail_call_funcptr INSTRUCTION_TABLE[256];\n");

    generate_label_handlers(outfile)

    emitter = Emitter(out)
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(function_proto(name))
        out.emit("{\n")
        if analysis.opmap[name] >= analysis.min_instrumented:
            out.emit(f"int opcode = {name};\n")
        else:
            out.emit("int opcode = next_instr->op.code;\n")
        # Some instructions don't use opcode.
        out.emit(f"(void)(opcode);\n")
        out.emit("{\n")
        write_single_inst(out, emitter, name, inst)
        if not inst.parts[-1].properties.always_exits:
            out.emit("DISPATCH();\n")
        out.start_line()
        out.emit("}\n")
        out.emit("}\n")

    out.emit("\n")

    # Emit unknown opcode handler.
    out.emit(function_proto("UNKNOWN_OPCODE"))
    out.emit("{\n")
    out.emit("""
int opcode = next_instr->op.code;
_PyErr_Format(tstate, PyExc_SystemError,
              "%U:%d: unknown opcode %d",
              _PyFrame_GetCode(frame)->co_filename,
              PyUnstable_InterpreterFrame_GetLine(frame),
              opcode);    
""")
    out.emit("CEVAL_GOTO(error);")
    out.emit("}\n")

    out.emit("static py_tail_call_funcptr INSTRUCTION_TABLE[256] = {\n")
    for name in sorted(analysis.instructions.keys()):
        out.emit(f"[{name}] = _TAIL_CALL_{name},\n")
    named_values = analysis.opmap.values()
    for rest in range(256):
        if rest not in named_values:
            out.emit(f"[{rest}] = _TAIL_CALL_UNKNOWN_OPCODE,\n")
    out.emit("};\n")
    outfile.write(FOOTER)


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "-l", "--emit-line-directives", help="Emit #line directives", action="store_true"
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
        generate_tier1(args.input, data, outfile, args.emit_line_directives)
