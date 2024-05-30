"""Generate the cases for the tier 2 interpreter.
Reads the instruction definitions from bytecodes.c.
Writes the cases to executor_cases.c.h, which is #included in ceval.c.
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
    emit_tokens,
    emit_to,
    REPLACEMENT_FUNCTIONS,
)
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import StackOffset, Stack, SizeMismatch

DEFAULT_OUTPUT = ROOT / "Python/executor_cases.c.h"


def declare_variable(
    var: StackItem, uop: Uop, variables: set[str], out: CWriter
) -> None:
    if var.name in variables:
        return
    type = var.type if var.type else "PyObject *"
    variables.add(var.name)
    if var.condition:
        out.emit(f"{type}{var.name} = NULL;\n")
        if uop.replicates:
            # Replicas may not use all their conditional variables
            # So avoid a compiler warning with a fake use
            out.emit(f"(void){var.name};\n")
    else:
        out.emit(f"{type}{var.name};\n")


def declare_variables(uop: Uop, out: CWriter) -> None:
    variables = {"unused"}
    for var in reversed(uop.stack.inputs):
        declare_variable(var, uop, variables, out)
    for var in uop.stack.outputs:
        declare_variable(var, uop, variables, out)


def tier2_replace_error(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction | None,
) -> None:
    out.emit_at("if ", tkn)
    out.emit(next(tkn_iter))
    emit_to(out, tkn_iter, "COMMA")
    label = next(tkn_iter).text
    next(tkn_iter)  # RPAREN
    next(tkn_iter)  # Semi colon
    out.emit(") JUMP_TO_ERROR();\n")


def tier2_replace_error_no_pop(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction | None,
) -> None:
    next(tkn_iter)  # LPAREN
    next(tkn_iter)  # RPAREN
    next(tkn_iter)  # Semi colon
    out.emit_at("JUMP_TO_ERROR();", tkn)

def tier2_replace_deopt(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    unused: Stack,
    inst: Instruction | None,
) -> None:
    out.emit_at("if ", tkn)
    out.emit(next(tkn_iter))
    emit_to(out, tkn_iter, "RPAREN")
    next(tkn_iter)  # Semi colon
    out.emit(") {\n")
    out.emit("UOP_STAT_INC(uopcode, miss);\n")
    out.emit("JUMP_TO_JUMP_TARGET();\n");
    out.emit("}\n")


def tier2_replace_exit_if(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    unused: Stack,
    inst: Instruction | None,
) -> None:
    out.emit_at("if ", tkn)
    out.emit(next(tkn_iter))
    emit_to(out, tkn_iter, "RPAREN")
    next(tkn_iter)  # Semi colon
    out.emit(") {\n")
    out.emit("UOP_STAT_INC(uopcode, miss);\n")
    out.emit("JUMP_TO_JUMP_TARGET();\n")
    out.emit("}\n")


def tier2_replace_oparg(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    unused: Stack,
    inst: Instruction | None,
) -> None:
    if not uop.name.endswith("_0") and not uop.name.endswith("_1"):
        out.emit(tkn)
        return
    amp = next(tkn_iter)
    if amp.text != "&":
        out.emit(tkn)
        out.emit(amp)
        return
    one = next(tkn_iter)
    assert one.text == "1"
    out.emit_at(uop.name[-1], tkn)


TIER2_REPLACEMENT_FUNCTIONS = REPLACEMENT_FUNCTIONS.copy()
TIER2_REPLACEMENT_FUNCTIONS["ERROR_IF"] = tier2_replace_error
TIER2_REPLACEMENT_FUNCTIONS["ERROR_NO_POP"] = tier2_replace_error_no_pop
TIER2_REPLACEMENT_FUNCTIONS["DEOPT_IF"] = tier2_replace_deopt
TIER2_REPLACEMENT_FUNCTIONS["oparg"] = tier2_replace_oparg
TIER2_REPLACEMENT_FUNCTIONS["EXIT_IF"] = tier2_replace_exit_if


def write_uop(uop: Uop, out: CWriter, stack: Stack) -> None:
    try:
        out.start_line()
        if uop.properties.oparg:
            out.emit("oparg = CURRENT_OPARG();\n")
            assert uop.properties.const_oparg < 0
        elif uop.properties.const_oparg >= 0:
            out.emit(f"oparg = {uop.properties.const_oparg};\n")
            out.emit(f"assert(oparg == CURRENT_OPARG());\n")
        for var in reversed(uop.stack.inputs):
            out.emit(stack.pop(var))
        if not uop.properties.stores_sp:
            for i, var in enumerate(uop.stack.outputs):
                out.emit(stack.push(var))
        for cache in uop.caches:
            if cache.name != "unused":
                if cache.size == 4:
                    type = cast = "PyObject *"
                else:
                    type = f"uint{cache.size*16}_t "
                    cast = f"uint{cache.size*16}_t"
                out.emit(f"{type}{cache.name} = ({cast})CURRENT_OPERAND();\n")
        emit_tokens(out, uop, stack, None, TIER2_REPLACEMENT_FUNCTIONS)
        if uop.properties.stores_sp:
            for i, var in enumerate(uop.stack.outputs):
                out.emit(stack.push(var))
    except SizeMismatch as ex:
        raise analysis_error(ex.args[0], uop.body[0])


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
    out.emit("\n")
    for name, uop in analysis.uops.items():
        if uop.properties.tier == 1:
            continue
        if uop.properties.oparg_and_1:
            out.emit(f"/* {uop.name} is split on (oparg & 1) */\n\n")
            continue
        if uop.is_super():
            continue
        why_not_viable = uop.why_not_viable()
        if why_not_viable is not None:
            out.emit(f"/* {uop.name} is not a viable micro-op for tier 2 because it {why_not_viable} */\n\n")
            continue
        out.emit(f"case {uop.name}: {{\n")
        declare_variables(uop, out)
        stack = Stack()
        write_uop(uop, out, stack)
        out.start_line()
        if not uop.properties.always_exits:
            stack.flush(out)
            if uop.properties.ends_with_eval_breaker:
                out.emit("CHECK_EVAL_BREAKER();\n")
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
