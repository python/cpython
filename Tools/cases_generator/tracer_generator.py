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

class TracerEmitter(Emitter):
    out: CWriter
    labels: dict[str, Label]
    _replacers: dict[str, ReplacementFunctionType]
    cannot_escape: bool

    def __init__(self, out: CWriter, labels: dict[str, Label], cannot_escape: bool = False):
        super().__init__(out, labels, cannot_escape)
        self._replacers = {
            **self._replacers,
            "DISPATCH": self.dispatch,
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
):
    # Import inside to avoid circular imports.
    from tier1_generator import generate_tier1_cases
    out.emit(f"#ifdef _Py_TIER2 /* BEGIN TRACING INSTRUCTIONS */\n")
    generate_tier1_cases(analysis, out, TracerEmitter(out, analysis.labels), is_tracing=True)
    out.emit(f"#endif /* END TRACING INSTRUCTIONS */\n")