import argparse

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    Label,
    CodeSection,
    Part,
    analyze_files,
    Skip,
    Flush,
    analysis_error,
    StackItem,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    type_and_null,
    Emitter,
    TokenIterator,
    always_true,
    emit_to,
    ReplacementFunctionType,
)
from cwriter import CWriter
from typing import TextIO
from lexer import Token
from stack import Local, Stack, StackError, get_stack_effect, Storage
from tier1_generator import get_popped, declare_variables, write_uop

DEFAULT_OUTPUT = ROOT / "Python/generated_tracer_cases.c.h"

class TracerEmitter(Emitter):
    out: CWriter
    labels: dict[str, Label]
    _replacers: dict[str, ReplacementFunctionType]
    cannot_escape: bool

    def __init__(self, out: CWriter, labels: dict[str, Label], cannot_escape: bool = False):
        super().__init__(out, labels, cannot_escape, jump_prefix="TRACING_")
        self._replacers = {
            **self._replacers,
            "DISPATCH": self.dispatch,
            "DISPATCH_INLINED": self.dispatch_inlined,
            "DISPATCH_SAME_OPARG": self.dispatch_same_oparg,
        }

    def dispatch(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        if storage.spilled:
            raise analysis_error("stack_pointer needs reloading before dispatch", tkn)
        storage.stack.flush(self.out)
        self.out.start_line()
        self.emit("TRACING_DISPATCH")
        return False

    def dispatch_inlined(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        if storage.spilled:
            raise analysis_error("stack_pointer needs reloading before dispatch", tkn)
        storage.stack.flush(self.out)
        self.out.start_line()
        self.emit("TRACING_DISPATCH_INLINED")
        return False

    def dispatch_same_oparg(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        if storage.spilled:
            raise analysis_error("stack_pointer needs reloading before dispatch", tkn)
        storage.stack.flush(self.out)
        self.out.start_line()
        if isinstance(uop, Uop) and "specializing" in uop.annotations:
            self.emit("TRACING_SPECIALIZE_DISPATCH_SAME_OPARG")
        else:
            self.emit(tkn)
        emit_to(self.out, tkn_iter, "SEMI")
        return False

    def record_dynamic_jump_taken(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.emit(tkn)
        emit_to(self.out, tkn_iter, "SEMI")
        self.out.emit(";\n")
        return True

def generate_tier1_tracer_cases(
    analysis: Analysis, out: CWriter, emitter: Emitter
) -> None:
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(f"TRACING_TARGET({name}) {{\n")
        out.emit(f"assert(IS_JIT_TRACING());\n")
        # We need to ifdef it because this breaks platforms
        # without computed gotos/tail calling.
        out.emit(f"#if _Py_TAIL_CALL_INTERP\n")
        out.emit(f"int opcode = {name};\n")
        out.emit(f"(void)(opcode);\n")
        out.emit(f"#endif\n")
        unused_guard = "(void)this_instr;\n"
        if inst.properties.needs_prev:
            out.emit(f"_Py_CODEUNIT* const prev_instr = frame->instr_ptr;\n")
        if not inst.is_target:
            out.emit(f"_Py_CODEUNIT* const this_instr = next_instr;\n")
            out.emit(unused_guard)
        if not inst.properties.no_save_ip:
            out.emit(f"frame->instr_ptr = next_instr;\n")

        out.emit(f"next_instr += {inst.size};\n")
        out.emit(f"INSTRUCTION_STATS({name});\n")
        if inst.is_target:
            out.emit(f"PREDICTED_TRACING_{name}:;\n")
            out.emit(f"_Py_CODEUNIT* const this_instr = next_instr - {inst.size};\n")
            out.emit(unused_guard)
        # This is required so that the predicted ops reflect the correct opcode.
        out.emit(f"opcode = {name};\n")
        out.emit(f"PyCodeObject *old_code = (PyCodeObject *)PyStackRef_AsPyObjectBorrow(frame->f_executable);\n")
        out.emit(f"(void)old_code;\n")
        out.emit(f"PyFunctionObject *old_func = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(frame->f_funcobj);\n")
        out.emit(f"(void)old_func;\n")
        out.emit(f"int _jump_taken = false;\n")
        out.emit(f"(void)_jump_taken;\n")
        out.emit(f"int _old_stack_level = !PyStackRef_IsNull(frame->f_executable) ? STACK_LEVEL() : 0;\n")
        out.emit(f"(void)(_old_stack_level);\n")
        if inst.family is not None:
            out.emit(
                f"static_assert({inst.family.size} == {inst.size-1}"
                ', "incorrect cache size");\n'
            )
        declare_variables(inst, out)
        offset = 1  # The instruction itself
        stack = Stack()
        for part in inst.parts:
            # Only emit braces if more than one uop
            insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
            reachable, offset, stack = write_uop(part, emitter, offset, stack, inst, insert_braces)
        out.start_line()
        if reachable: # type: ignore[possibly-undefined]
            stack.flush(out)
            out.emit(f"TRACING_DISPATCH();\n")
        out.start_line()
        out.emit("}")
        out.emit("\n")


def generate_tracer_cases(
    analysis: Analysis, out: CWriter
) -> None:
    out.emit(f"#ifdef _Py_TIER2 /* BEGIN TRACING INSTRUCTIONS */\n")
    generate_tier1_tracer_cases(analysis, out, TracerEmitter(out, analysis.labels))
    out.emit(f"#endif /* END TRACING INSTRUCTIONS */\n")

def generate_tracer(
    filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 2, lines)
    out.emit("#define TIER_ONE 1\n")
    out.emit("#define TRACING_JIT 1\n")
    generate_tracer_cases(analysis, out)
    out.emit("#undef TRACING_JIT\n")
    out.emit("#undef TIER_ONE\n")

# For use in unittest
def generate_tracer_from_files(
    filenames: list[str], outfilename: str, lines: bool
) -> None:
    data = analyze_files(filenames)
    with open(outfilename, "w") as outfile:
        generate_tracer(filenames, data, outfile, lines)


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
        generate_tracer(args.input, data, outfile, args.emit_line_directives)
