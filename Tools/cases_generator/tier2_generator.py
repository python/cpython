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
    get_uop_cache_depths,
    is_large,
    MAX_CACHED_REGISTER,
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

    def __init__(self, out: CWriter, labels: dict[str, Label], exit_cache_depth: int):
        super().__init__(out, labels)
        self._replacers["oparg"] = self.oparg
        self._replacers["IP_OFFSET_OF"] = self.ip_offset_of
        self.exit_cache_depth = exit_cache_depth

    def goto_error(self, offset: int, storage: Storage) -> str:
        # To do: Add jump targets for popping values.
        if offset != 0:
            storage.copy().flush(self.out)
        else:
            storage.stack.copy().flush(self.out)
        self.emit("SET_CURRENT_CACHED_VALUES(0);\n")
        return "JUMP_TO_ERROR();"

    def exit_if(
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
        storage = storage.copy()
        self.cache_items(storage.stack, self.exit_cache_depth, False)
        storage.stack.flush(self.out)
        self.emit("JUMP_TO_JUMP_TARGET();\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    periodic_if = deopt_if = exit_if

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

    def ip_offset_of(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        assert uop.name.startswith("_GUARD_IP")
        # LPAREN
        next(tkn_iter)
        tok = next(tkn_iter)
        self.emit(f" OFFSET_OF_{tok.text};\n")
        # RPAREN
        next(tkn_iter)
        # SEMI
        next(tkn_iter)
        return True

    def tier2_to_tier2(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        assert self.exit_cache_depth == 0, uop.name
        self.cache_items(storage.stack, self.exit_cache_depth, False)
        storage.flush(self.out)
        self.out.emit(tkn)
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        self.emit(lparen)
        emit_to(self.out, tkn_iter, "RPAREN")
        self.out.emit(")")
        return False

    goto_tier_one = tier2_to_tier2

    def exit_if_after(
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
        storage = storage.copy()
        storage.clear_inputs("in AT_END_EXIT_IF")
        self.cache_items(storage.stack, self.exit_cache_depth, False)
        storage.flush(self.out)
        self.emit("JUMP_TO_JUMP_TARGET();\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    def cache_items(self, stack: Stack, cached_items: int, zero_regs: bool) -> None:
        self.out.start_line()
        i = cached_items
        while i > 0:
            self.out.start_line()
            item = StackItem(f"_tos_cache{i-1}", "", False, True)
            stack.pop(item, self.out)
            i -= 1
        if zero_regs:
            # TO DO -- For compilers that support it,
            # replace this with a "clobber" to tell
            # the compiler that these values are unused
            # without having to emit any code.
            for i in range(cached_items, MAX_CACHED_REGISTER):
                self.out.emit(f"_tos_cache{i} = PyStackRef_ZERO_BITS;\n")
        self.emit(f"SET_CURRENT_CACHED_VALUES({cached_items});\n")


def write_uop(uop: Uop, emitter: Tier2Emitter, stack: Stack, cached_items: int = 0) -> tuple[bool, Stack]:
    locals: dict[str, Local] = {}
    zero_regs = is_large(uop) or uop.properties.escapes
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
                bits = cache.size*16
                if cache.size == 4:
                    type = cast = "PyObject *"
                else:
                    type = f"uint{bits}_t "
                    cast = f"uint{bits}_t"
                emitter.emit(f"{type}{cache.name} = ({cast})CURRENT_OPERAND{idx}_{bits}();\n")
                idx += 1
        reachable, storage = emitter.emit_tokens(uop, storage, None, False)
        if reachable:
            storage.stack._print(emitter.out)
            emitter.cache_items(storage.stack, cached_items, zero_regs)
            storage.flush(emitter.out)
        return reachable, storage.stack
    except StackError as ex:
        raise analysis_error(ex.args[0], uop.body.open) from None

SKIPS = ("_EXTENDED_ARG",)

def is_for_iter_test(uop: Uop) -> bool:
    return uop.name in (
        "_GUARD_NOT_EXHAUSTED_RANGE", "_GUARD_NOT_EXHAUSTED_LIST",
        "_GUARD_NOT_EXHAUSTED_TUPLE", "_FOR_ITER_TIER_TWO"
    )

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
    out.emit("\n")

    for name, uop in analysis.uops.items():
        if uop.properties.tier == 1:
            continue
        if uop.is_super():
            continue
        if uop.properties.records_value:
            continue
        why_not_viable = uop.why_not_viable()
        if why_not_viable is not None:
            out.emit(
                f"/* {uop.name} is not a viable micro-op for tier 2 because it {why_not_viable} */\n\n"
            )
            continue
        for inputs, outputs, exit_depth in get_uop_cache_depths(uop):
            emitter = Tier2Emitter(out, analysis.labels, exit_depth)
            out.emit(f"case {uop.name}_r{inputs}{outputs}: {{\n")
            out.emit(f"CHECK_CURRENT_CACHED_VALUES({inputs});\n")
            out.emit("assert(WITHIN_STACK_BOUNDS_IGNORING_CACHE());\n")
            declare_variables(uop, out)
            stack = Stack()
            stack.push_cache([f"_tos_cache{i}" for i in range(inputs)], out)
            stack._print(out)
            reachable, stack = write_uop(uop, emitter, stack, outputs)
            out.start_line()
            if reachable:
                out.emit("assert(WITHIN_STACK_BOUNDS_IGNORING_CACHE());\n")
                if not uop.properties.always_exits:
                    out.emit("break;\n")
            out.start_line()
            out.emit("}")
            out.emit("\n\n")
    out.emit("\n")
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
