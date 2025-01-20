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

DEFAULT_INPUT = ROOT / "Python/bytecodes.c"
DEFAULT_OUTPUT = ROOT / "Python/generated_tail_call_handlers.c.h"

DEFAULT_CEVAL_INPUT = ROOT / "Python/ceval.c"

FOOTER = "#undef TIER_ONE\n#undef IN_TAIL_CALL_INTERP\n"

TARGET_LABEL = "TAIL_CALL_TARGET"

def generate_label_handlers(infile: TextIO, outfile: TextIO) -> list[str]:
    out = CWriter(outfile, 0, False)
    str_in = infile.read()
    # https://stackoverflow.com/questions/8303488/regex-to-match-any-character-including-new-lines
    eval_framedefault = re.findall(r"_PyEval_EvalFrameDefault\(.*\)\n({[\s\S]*\/\* END_BASE_INTERPRETER \*\/)", str_in)[0]
    function_protos = re.findall(rf"{TARGET_LABEL}\((\w+)\):", eval_framedefault)
    for proto in function_protos:
        out.emit(f"{function_proto(proto)};\n")
    out.emit("\n")
    lines = iter(eval_framedefault[eval_framedefault.find(TARGET_LABEL):].split("\n"))
    next(lines)
    for i in range(len(function_protos)):
        curr_proto = function_protos[i]
        fallthrough_proto = function_protos[i + 1] if i + 1 < len(function_protos) else None
        out.emit(function_proto(curr_proto))
        out.emit("\n")
        out.emit("{\n")
        for line in lines:
            if TARGET_LABEL in line:
                break
            if label := re.findall(r"goto (\w+);", line):
                out.emit(f"TAIL_CALL({label[0]});\n")
            else:
                out.emit_text(line)
                out.emit("\n")
        if fallthrough_proto:
            out.emit(f"TAIL_CALL({fallthrough_proto});\n")
        out.emit("}\n")
        out.emit("\n")
    return function_protos

# For unit testing.
def generate_label_handlers_from_files(
    infilename: str, outfilename: str
) -> None:
    with open(infilename, "r") as infile, open(outfilename, "w") as outfile:
        generate_label_handlers(infile, outfile)



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

    with open(DEFAULT_CEVAL_INPUT, "r") as infile:
        err_labels = generate_label_handlers(infile, outfile)

    emitter = Emitter(out)
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(function_proto(name))
        out.emit("{\n")
        # We wrap this with a block to signal to GCC that the local variables
        # are dead at the tail call site.
        # Otherwise, GCC 15's escape analysis may think there are
        # escaping locals.
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118430#c1
        out.emit("{\n")
        write_single_inst(out, emitter, name, inst)
        out.emit("}\n")
        if not inst.parts[-1].properties.always_exits:
            out.emit("DISPATCH();\n")
        # Note: this produces 2 jumps, but a tail call directly
        # at the branch also produces the same.
        # Furthermore, this is required to make GCC 15's escape analysis happy
        # as written above.
        for err_label in err_labels:
            out.emit(f"{err_label}:\n")
            out.emit(f"TAIL_CALL({err_label});\n")
        out.start_line()
        out.emit("}\n")

    out.emit("\n")

    # Emit unknown opcode handler.
    out.emit(function_proto("UNKNOWN_OPCODE"))
    out.emit("{\n")
    out.emit("{\n")
    out.emit("""
_PyErr_Format(tstate, PyExc_SystemError,
              "%U:%d: unknown opcode %d",
              _PyFrame_GetCode(frame)->co_filename,
              PyUnstable_InterpreterFrame_GetLine(frame),
              opcode);
""")
    out.emit("}\n")
    out.emit("TAIL_CALL(error);\n")
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
