"""Generate the cases for the tier 2 interpreter.
Reads the instruction definitions from bytecodes.c.
Writes the cases to executor_cases.c.h, which is #included in ceval.c.
"""

import argparse

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    Label,
    CodeSection,
    analyze_files,
    StackItem,
    analysis_error,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    emit_to,
    write_header,
    type_and_null,
    Emitter,
    TokenIterator,
    always_true,
)
from cwriter import CWriter
from typing import TextIO
from lexer import Token
from stack import Local, Stack, StackError, Storage

DEFAULT_OUTPUT = ROOT / "Python/executor_cases.c.h"


def declare_variable(
    var: StackItem, uop: Uop, seen: set[str], out: CWriter
) -> None:
    if not var.used or var.name in seen:
        return
    seen.add(var.name)
    type, null = type_and_null(var)
    space = " " if type[-1].isalnum() else ""
    out.emit(f"{type}{space}{var.name};\n")


def declare_variables(uop: Uop, out: CWriter) -> None:
    stack = Stack()
    null = CWriter.null()
    for var in reversed(uop.stack.inputs):
        stack.pop(var, null)
    for var in uop.stack.outputs:
        stack.push(Local.undefined(var))
    seen = {"unused"}
    for var in reversed(uop.stack.inputs):
        declare_variable(var, uop, seen, out)
    for var in uop.stack.outputs:
        declare_variable(var, uop, seen, out)


class Tier2Emitter(Emitter):

    def __init__(self, out: CWriter, labels: dict[str, Label]):
        super().__init__(out, labels)
        self._replacers["oparg"] = self.oparg

    def goto_error(self, offset: int, storage: Storage) -> str:
        # To do: Add jump targets for popping values.
        if offset != 0:
            storage.copy().flush(self.out)
        return f"JUMP_TO_ERROR();"

    def deopt_if(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.emit_at("if ", tkn)
        lparen = next(tkn_iter)
        self.emit(lparen)
        assert lparen.kind == "LPAREN"
        first_tkn = tkn_iter.peek()
        emit_to(self.out, tkn_iter, "RPAREN")
        next(tkn_iter)  # Semi colon
        self.emit(") {\n")
        self.emit("UOP_STAT_INC(uopcode, miss);\n")
        self.emit("JUMP_TO_JUMP_TARGET();\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    def exit_if(  # type: ignore[override]
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.emit_at("if ", tkn)
        lparen = next(tkn_iter)
        self.emit(lparen)
        first_tkn = tkn_iter.peek()
        emit_to(self.out, tkn_iter, "RPAREN")
        next(tkn_iter)  # Semi colon
        self.emit(") {\n")
        self.emit("UOP_STAT_INC(uopcode, miss);\n")
        self.emit("JUMP_TO_JUMP_TARGET();\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    def oparg(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        if not uop.name.endswith("_0") and not uop.name.endswith("_1"):
            self.emit(tkn)
            return True
        amp = next(tkn_iter)
        if amp.text != "&":
            self.emit(tkn)
            self.emit(amp)
            return True
        one = next(tkn_iter)
        assert one.text == "1"
        self.out.emit_at(uop.name[-1], tkn)
        return True


def write_uop(uop: Uop, emitter: Emitter, stack: Stack) -> Stack:
    locals: dict[str, Local] = {}
    try:
        emitter.out.start_line()
        if uop.properties.oparg:
            emitter.emit("oparg = CURRENT_OPARG();\n")
            assert uop.properties.const_oparg < 0
        elif uop.properties.const_oparg >= 0:
            emitter.emit(f"oparg = {uop.properties.const_oparg};\n")
            emitter.emit(f"assert(oparg == CURRENT_OPARG());\n")
        storage = Storage.for_uop(stack, uop, emitter.out)
        idx = 0
        for cache in uop.caches:
            if cache.name != "unused":
                if cache.size == 4:
                    type = cast = "PyObject *"
                else:
                    type = f"uint{cache.size*16}_t "
                    cast = f"uint{cache.size*16}_t"
                emitter.emit(f"{type}{cache.name} = ({cast})CURRENT_OPERAND{idx}();\n")
                idx += 1
        _, storage = emitter.emit_tokens(uop, storage, None, False)
        storage.flush(emitter.out)
    except StackError as ex:
        raise analysis_error(ex.args[0], uop.body.open) from None
    return storage.stack

SKIPS = ("_EXTENDED_ARG",)


def generate_tier2(
    filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
        """
#ifdef TIER_ONE
    #error "This file is for Tier 2 only"
#endif
#define TIER_TWO 2
"""
    )
    out = CWriter(outfile, 2, lines)
    emitter = Tier2Emitter(out, analysis.labels)
    out.emit("\n")
    for name, uop in analysis.uops.items():
        if uop.properties.tier == 1:
            continue
        if uop.is_super():
            continue
        why_not_viable = uop.why_not_viable()
        if why_not_viable is not None:
            out.emit(
                f"/* {uop.name} is not a viable micro-op for tier 2 because it {why_not_viable} */\n\n"
            )
            continue
        out.emit(f"case {uop.name}: {{\n")
        declare_variables(uop, out)
        stack = Stack()
        stack = write_uop(uop, emitter, stack)
        out.start_line()
        if not uop.properties.always_exits:
            out.emit("break;\n")
        out.start_line()
        out.emit("}")
        out.emit("\n\n")
    outfile.write("#undef TIER_TWO\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the tier 2 interpreter.",
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
        generate_tier2(args.input, data, outfile, args.emit_line_directives)
