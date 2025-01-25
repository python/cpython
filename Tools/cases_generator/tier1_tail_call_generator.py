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
    TokenIterator,
)

from analyzer import (
    Analysis,
    Instruction,
    analyze_files,
    Uop,
)

from tier1_generator import (
    write_single_inst
)

from lexer import Token

from stack import Storage

DEFAULT_INPUT = ROOT / "Python/bytecodes.c"
DEFAULT_OUTPUT = ROOT / "Python/generated_tail_call_handlers.c.h"

FOOTER = "#undef TIER_ONE\n#undef IN_TAIL_CALL_INTERP\n"

class TailCallEmitter(Emitter):

    def __init__(self, out: CWriter, analysis: Analysis):
        super().__init__(out)
        self.analysis = analysis

    def go_to_instruction(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        next(tkn_iter)
        name = next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        assert name.kind == "IDENTIFIER"
        self.emit("\n")
        inst = self.analysis.instructions[name.text]
        fam = None
        # Search for the family (if any)
        for family_name, family in self.analysis.families.items():
            if inst.name == family_name:
                fam = family
                break
        size = fam.size if fam is not None else 0
        self.emit(f"Py_MUSTTAIL return (INSTRUCTION_TABLE[{name.text}])(frame, stack_pointer, tstate, next_instr - 1 - {size}, opcode, oparg);\n")
        return True

class TailCallLabelsEmitter(Emitter):
    def __init__(self, out: CWriter):
        super().__init__(out)
        self._replacers = {
            'goto': self.goto,
        }

    def go_to_instruction(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        next(tkn_iter)
        name = next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        assert name.kind == "IDENTIFIER"
        self.emit("\n")
        inst = self.analysis.instructions[name.text]
        fam = None
        # Search for the family (if any)
        for family_name, family in self.analysis.families.items():
            if inst.name == family_name:
                fam = family
                break
        size = fam.size if fam is not None else 0
        self.emit(f"Py_MUSTTAIL return (INSTRUCTION_TABLE[{name.text}])(frame, stack_pointer, tstate, next_instr - 1 - {size}, opcode, oparg);\n")
        return True

    def goto(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        # Only labels need to replace their gotos with tail calls.
        # We can't do this in normal code due to GCC's escape analysis
        # complaining.
        name = next(tkn_iter)
        next(tkn_iter)
        assert name.kind == "IDENTIFIER"
        self.out.emit("\n")
        self.emit(f"TAIL_CALL({name.text});\n")



def function_proto(name: str) -> str:
    return f"Py_PRESERVE_NONE_CC static PyObject *_TAIL_CALL_{name}(TAIL_CALL_PARAMS)"

def generate_label_handlers(
    analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    out = CWriter(outfile, 0, lines)
    emitter = TailCallLabelsEmitter(out)
    emitter.emit("\n")
    for name in analysis.labels:
        emitter.emit(f"{function_proto(name)};\n")
    emitter.emit("\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"{function_proto(name)}\n")
        emitter.emit_tokens_simple(label.body)

        emitter.emit("\n")
        emitter.emit("\n")


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

    generate_label_handlers(analysis, outfile, lines)

    emitter = TailCallEmitter(out, analysis)
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
        for err_label in analysis.labels.keys():
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
