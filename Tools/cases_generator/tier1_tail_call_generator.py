"""Generate the main interpreter tail call handlers.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_tail_call_handlers.c.h, which is #included in ceval.c.
"""

import argparse

from typing import TextIO

from generators_common import (
    ROOT,
    write_header,
    CWriter,
    Emitter,
    TokenIterator,
    emit_to,
    always_true,
)

from analyzer import (
    Analysis,
    Instruction,
    analyze_files,
    Uop,
    Label,
)

from tier1_generator import (
    write_single_inst,
    generate_tier1_labels,
    INSTRUCTION_START_MARKER,
    INSTRUCTION_END_MARKER,
)

from lexer import Token

from stack import Storage

DEFAULT_INPUT = ROOT / "Python/bytecodes.c"
DEFAULT_OUTPUT = ROOT / "Python/generated_tail_call_handlers.c.h"
DEFAULT_LABELS_OUTPUT = ROOT / "Python/generated_tail_call_labels.c.h"

PRELUDE = """
#ifndef Py_TAIL_CALL_INTERP
    #error "This file is for tail-calling interpreter only."
#endif
#define TIER_ONE 1
"""
FOOTER = "#undef TIER_ONE\n"

class TailCallEmitter(Emitter):

    def __init__(self, out: CWriter, analysis: Analysis):
        super().__init__(out)
        self.analysis = analysis

    def go_to_instruction(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop | Label,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        next(tkn_iter)
        name = next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        assert name.kind == "IDENTIFIER"
        self.out.start_line()
        self.emit(f"Py_MUSTTAIL return (INSTRUCTION_TABLE[{name.text}])(frame, stack_pointer, tstate, this_instr, opcode, oparg);\n")
        return True

    def deopt_if(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop | Label,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.start_line()
        self.out.emit("if (")
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        first_tkn = tkn_iter.peek()
        emit_to(self.out, tkn_iter, "RPAREN")
        self.emit(") {\n")
        next(tkn_iter)  # Semi colon
        assert inst is not None
        assert inst.family is not None
        family_name = inst.family.name
        self.emit(f"UPDATE_MISS_STATS({family_name});\n")
        self.emit(f"assert(_PyOpcode_Deopt[opcode] == ({family_name}));\n")
        self.emit(f"Py_MUSTTAIL return _TAIL_CALL_{family_name}(frame, stack_pointer, tstate, this_instr, opcode, oparg);\n")
        self.emit("}\n")
        return not always_true(first_tkn)


    exit_if = deopt_if

class TailCallLabelsEmitter(Emitter):
    def __init__(self, out: CWriter):
        super().__init__(out)
        self._replacers = {
            'goto': self.goto,
        }

    def goto(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop | Label,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        # Only labels need to replace their gotos with tail calls.
        # We can't do this in normal code due to GCC's escape analysis
        # complaining.
        name = next(tkn_iter)
        next(tkn_iter)
        assert name.kind == "IDENTIFIER"
        self.out.start_line()
        self.emit(f"TAIL_CALL({name.text});\n")
        return True


class TailCallCevalLabelsEmitter(Emitter):
    def __init__(self, out: CWriter):
        super().__init__(out)
        self._replacers = {
            'DISPATCH': self.dispatch,
        }

    def dispatch(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop | Label,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        # Replace DISPATCH with _TAIL_CALL_entry(...)
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        self.out.start_line()
        self.emit("return _TAIL_CALL_entry(frame, stack_pointer, tstate, next_instr, 0, 0);\n")
        return True


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
    out.emit(f"{INSTRUCTION_START_MARKER}\n")
    for name, label in analysis.labels.items():
        emitter.emit(f"{function_proto(name)}\n")
        emitter.emit_label(label)

        emitter.emit("\n")
        emitter.emit("\n")


def uses_this(inst: Instruction) -> bool:
    if inst.properties.needs_this:
        return True
    for uop in inst.parts:
        if not isinstance(uop, Uop):
            continue
        for cache in uop.caches:
            if cache.name != "unused":
                return True
        for tkn in uop.body:
            if (tkn.kind == "IDENTIFIER"
                    and (tkn.text in {"DEOPT_IF", "EXIT_IF", "GO_TO_INSTRUCTION"})):
                return True
    return False

def generate_tier1(
    filenames: list[str], analysis: Analysis, outfile: TextIO, labels_outfile: TextIO,  lines: bool
) -> None:
    # Write labels required for main ceval
    write_header(__file__, filenames, labels_outfile)
    labels_outfile.write(PRELUDE)
    out = CWriter(labels_outfile, 2, lines)
    emitter = TailCallCevalLabelsEmitter(out)
    generate_tier1_labels(analysis, emitter)
    labels_outfile.write(FOOTER)
    write_header(__file__, filenames, outfile)
    outfile.write(PRELUDE)
    out = CWriter(outfile, 0, lines)
    out.emit("static inline PyObject *_TAIL_CALL_entry(TAIL_CALL_PARAMS);\n")
    out.emit("static py_tail_call_funcptr INSTRUCTION_TABLE[256];\n");

    generate_label_handlers(analysis, outfile, lines)

    emitter = TailCallEmitter(out, analysis)
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(function_proto(name))
        out.emit(" {\n")
        # We wrap this with a block to signal to GCC that the local variables
        # are dead at the tail call site.
        # Otherwise, GCC 15's escape analysis may think there are
        # escaping locals.
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=118430#c1
        out.emit("{\n")
        write_single_inst(out, emitter, name, inst, uses_this)
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

    out.emit(f"{INSTRUCTION_END_MARKER}\n")

    # Emit unknown opcode handler.
    out.emit(function_proto("UNKNOWN_OPCODE"))
    out.emit(" {\n")
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

def generate_tier1_from_files(
    filenames: list[str], out_filename: str, out_label_filename: str, lines: bool
) -> None:
    data = analyze_files(filenames)
    with open(out_filename, "w") as outfile, open(out_label_filename, "w") as labels_outfile:
        generate_tier1(filenames, data, outfile, labels_outfile, lines)


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter switch.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "-lo", "--labels-output", type=str, help="Generated labels", default=DEFAULT_LABELS_OUTPUT
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
    with open(args.output, "w") as outfile, open(args.labels_output, 'w') as labels_outfile:
        generate_tier1(args.input, data, outfile, labels_outfile, args.emit_line_directives)
