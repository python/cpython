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
from tier1_generator import generate_tier1_cases

DEFAULT_OUTPUT = ROOT / "Python/generated_tracer_cases.c.h"

class TracerEmitter(Emitter):
    out: CWriter
    labels: dict[str, Label]
    _replacers: dict[str, ReplacementFunctionType]
    cannot_escape: bool

    def __init__(self, out: CWriter, labels: dict[str, Label], cannot_escape: bool = False):
        super().__init__(out, labels, cannot_escape, is_tracing=True)
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
        if "specializing" in uop.annotations:
            self.emit("TRACING_SPECIALIZE_DISPATCH_SAME_OPARG")
        else:
            self.emit(tkn)
        emit_to(self.out, tkn_iter, "SEMI")
        return False

    def record_jump_taken(
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

    def deopt_if(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
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
        self.emit(f"JUMP_TO_PREDICTED(TRACING_{family_name});\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    exit_if = deopt_if

def generate_tracer_cases(
    analysis: Analysis, out: CWriter
) -> None:
    out.emit(f"#ifdef _Py_TIER2 /* BEGIN TRACING INSTRUCTIONS */\n")
    generate_tier1_cases(analysis, out, TracerEmitter(out, analysis.labels), is_tracing=True)
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
