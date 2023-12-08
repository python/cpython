"""Generate the main interpreter switch.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import os.path
import sys

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    Part,
    analyze_files,
    Skip,
    StackItem,
    analysis_error,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
)
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import StackOffset


DEFAULT_OUTPUT = ROOT / "Python/generated_cases.c.h"


FOOTER = "#undef TIER_ONE\n"


class SizeMismatch(Exception):
    pass


class Stack:
    def __init__(self) -> None:
        self.top_offset = StackOffset()
        self.base_offset = StackOffset()
        self.peek_offset = StackOffset()
        self.variables: list[StackItem] = []
        self.defined: set[str] = set()

    def pop(self, var: StackItem) -> str:
        self.top_offset.pop(var)
        if not var.peek:
            self.peek_offset.pop(var)
        indirect = "&" if var.is_array() else ""
        if self.variables:
            popped = self.variables.pop()
            if popped.size != var.size:
                raise SizeMismatch(
                    f"Size mismatch when popping '{popped.name}' from stack to assign to {var.name}. "
                    f"Expected {var.size} got {popped.size}"
                )
            if popped.name == var.name:
                return ""
            elif popped.name == "unused":
                self.defined.add(var.name)
                return (
                    f"{var.name} = {indirect}stack_pointer[{self.top_offset.to_c()}];\n"
                )
            elif var.name == "unused":
                return ""
            else:
                self.defined.add(var.name)
                return f"{var.name} = {popped.name};\n"
        self.base_offset.pop(var)
        if var.name == "unused":
            return ""
        else:
            self.defined.add(var.name)
        assign = f"{var.name} = {indirect}stack_pointer[{self.base_offset.to_c()}];"
        if var.condition:
            return f"if ({var.condition}) {{ {assign} }}\n"
        return f"{assign}\n"

    def push(self, var: StackItem) -> str:
        self.variables.append(var)
        if var.is_array() and var.name not in self.defined and var.name != "unused":
            c_offset = self.top_offset.to_c()
            self.top_offset.push(var)
            self.defined.add(var.name)
            return f"{var.name} = &stack_pointer[{c_offset}];\n"
        else:
            self.top_offset.push(var)
            return ""

    def flush(self, out: CWriter) -> None:
        for var in self.variables:
            if not var.peek:
                if var.name != "unused" and not var.is_array():
                    if var.condition:
                        out.emit(f" if ({var.condition}) ")
                    out.emit(
                        f"stack_pointer[{self.base_offset.to_c()}] = {var.name};\n"
                    )
            self.base_offset.push(var)
        if self.base_offset.to_c() != self.top_offset.to_c():
            print("base", self.base_offset.to_c(), "top", self.top_offset.to_c())
            assert False
        number = self.base_offset.to_c()
        if number != "0":
            out.emit(f"stack_pointer += {number};\n")
        self.variables = []
        self.base_offset.clear()
        self.top_offset.clear()
        self.peek_offset.clear()

    def as_comment(self) -> str:
        return f"/* Variables: {[v.name for v in self.variables]}. Base offset: {self.base_offset.to_c()}. Top offset: {self.top_offset.to_c()} */"


def declare_variables(inst: Instruction, out: CWriter) -> None:
    variables = {"unused"}
    for uop in inst.parts:
        if isinstance(uop, Uop):
            for var in reversed(uop.stack.inputs):
                if var.name not in variables:
                    type = var.type if var.type else "PyObject *"
                    variables.add(var.name)
                    if var.condition:
                        out.emit(f"{type}{var.name} = NULL;\n")
                    else:
                        out.emit(f"{type}{var.name};\n")
            for var in uop.stack.outputs:
                if var.name not in variables:
                    variables.add(var.name)
                    type = var.type if var.type else "PyObject *"
                    if var.condition:
                        out.emit(f"{type}{var.name} = NULL;\n")
                    else:
                        out.emit(f"{type}{var.name};\n")


def emit_to(out: CWriter, tkn_iter: Iterator[Token], end: str) -> None:
    parens = 0
    for tkn in tkn_iter:
        if tkn.kind == end and parens == 0:
            return
        if tkn.kind == "LPAREN":
            parens += 1
        if tkn.kind == "RPAREN":
            parens -= 1
        out.emit(tkn)


def replace_deopt(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    unused: Stack,
    inst: Instruction,
) -> None:
    out.emit_at("DEOPT_IF", tkn)
    out.emit(next(tkn_iter))
    emit_to(out, tkn_iter, "RPAREN")
    next(tkn_iter)  # Semi colon
    out.emit(", ")
    assert inst.family is not None
    out.emit(inst.family.name)
    out.emit(");\n")


def replace_error(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction,
) -> None:
    out.emit_at("if ", tkn)
    out.emit(next(tkn_iter))
    emit_to(out, tkn_iter, "COMMA")
    label = next(tkn_iter).text
    next(tkn_iter)  # RPAREN
    next(tkn_iter)  # Semi colon
    out.emit(") ")
    c_offset = stack.peek_offset.to_c()
    try:
        offset = -int(c_offset)
        close = ";\n"
    except ValueError:
        offset = None
        out.emit(f"{{ stack_pointer += {c_offset}; ")
        close = "; }\n"
    out.emit("goto ")
    if offset:
        out.emit(f"pop_{offset}_")
    out.emit(label)
    out.emit(close)


def replace_decrefs(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction,
) -> None:
    next(tkn_iter)
    next(tkn_iter)
    next(tkn_iter)
    out.emit_at("", tkn)
    for var in uop.stack.inputs:
        if var.name == "unused" or var.name == "null" or var.peek:
            continue
        if var.size != "1":
            out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
            out.emit(f"Py_DECREF({var.name}[_i]);\n")
            out.emit("}\n")
        elif var.condition:
            out.emit(f"Py_XDECREF({var.name});\n")
        else:
            out.emit(f"Py_DECREF({var.name});\n")


def replace_store_sp(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction,
) -> None:
    next(tkn_iter)
    next(tkn_iter)
    next(tkn_iter)
    out.emit_at("", tkn)
    stack.flush(out)
    out.emit("_PyFrame_SetStackPointer(frame, stack_pointer);\n")


def replace_check_eval_breaker(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction,
) -> None:
    next(tkn_iter)
    next(tkn_iter)
    next(tkn_iter)
    if not uop.properties.ends_with_eval_breaker:
        out.emit_at("CHECK_EVAL_BREAKER();", tkn)


REPLACEMENT_FUNCTIONS = {
    "DEOPT_IF": replace_deopt,
    "ERROR_IF": replace_error,
    "DECREF_INPUTS": replace_decrefs,
    "CHECK_EVAL_BREAKER": replace_check_eval_breaker,
    "STORE_SP": replace_store_sp,
}


# Move this to formatter
def emit_tokens(out: CWriter, uop: Uop, stack: Stack, inst: Instruction) -> None:
    tkns = uop.body[1:-1]
    if not tkns:
        return
    tkn_iter = iter(tkns)
    out.start_line()
    for tkn in tkn_iter:
        if tkn.kind == "IDENTIFIER" and tkn.text in REPLACEMENT_FUNCTIONS:
            REPLACEMENT_FUNCTIONS[tkn.text](out, tkn, tkn_iter, uop, stack, inst)
        else:
            out.emit(tkn)


def write_uop(
    uop: Part, out: CWriter, offset: int, stack: Stack, inst: Instruction, braces: bool
) -> int:
    # out.emit(stack.as_comment() + "\n")
    if isinstance(uop, Skip):
        entries = "entries" if uop.size > 1 else "entry"
        out.emit(f"/* Skip {uop.size} cache {entries} */\n")
        return offset + uop.size
    try:
        out.start_line()
        if braces:
            out.emit(f"// {uop.name}\n")
        for var in reversed(uop.stack.inputs):
            out.emit(stack.pop(var))
        if braces:
            out.emit("{\n")
        if not uop.properties.stores_sp:
            for i, var in enumerate(uop.stack.outputs):
                out.emit(stack.push(var))
        for cache in uop.caches:
            if cache.name != "unused":
                if cache.size == 4:
                    type = "PyObject *"
                    reader = "read_obj"
                else:
                    type = f"uint{cache.size*16}_t "
                    reader = f"read_u{cache.size*16}"
                out.emit(
                    f"{type}{cache.name} = {reader}(&this_instr[{offset}].cache);\n"
                )
            offset += cache.size
        emit_tokens(out, uop, stack, inst)
        if uop.properties.stores_sp:
            for i, var in enumerate(uop.stack.outputs):
                out.emit(stack.push(var))
        if braces:
            out.start_line()
            out.emit("}\n")
        # out.emit(stack.as_comment() + "\n")
        return offset
    except SizeMismatch as ex:
        raise analysis_error(ex.args[0], uop.body[0])


def uses_this(inst: Instruction) -> bool:
    if inst.properties.needs_this:
        return True
    for uop in inst.parts:
        if isinstance(uop, Skip):
            continue
        for cache in uop.caches:
            if cache.name != "unused":
                return True
    return False


def generate_tier1(
    filenames: str, analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
        """
#ifdef TIER_TWO
    #error "This file is for Tier 1 only"
#endif
#define TIER_ONE 1
"""
    )
    out = CWriter(outfile, 2, lines)
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        needs_this = uses_this(inst)
        out.emit("\n")
        out.emit(f"TARGET({name}) {{\n")
        if needs_this and not inst.is_target:
            out.emit(f"_Py_CODEUNIT *this_instr = frame->instr_ptr = next_instr;\n")
        else:
            out.emit(f"frame->instr_ptr = next_instr;\n")
        out.emit(f"next_instr += {inst.size};\n")
        out.emit(f"INSTRUCTION_STATS({name});\n")
        if inst.is_target:
            out.emit(f"PREDICTED({name});\n")
            if needs_this:
                out.emit(f"_Py_CODEUNIT *this_instr = next_instr - {inst.size};\n")
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
            offset = write_uop(part, out, offset, stack, inst, len(inst.parts) > 1)
        out.start_line()
        if not inst.parts[-1].properties.always_exits:
            stack.flush(out)
            if inst.parts[-1].properties.ends_with_eval_breaker:
                out.emit("CHECK_EVAL_BREAKER();\n")
            out.emit("DISPATCH();\n")
        out.start_line()
        out.emit("}")
        out.emit("\n")
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
