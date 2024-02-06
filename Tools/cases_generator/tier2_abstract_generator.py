"""Generate the cases for the tier 2 redundancy eliminator/abstract interpreter.
Reads the instruction definitions from bytecodes.c. and tier2_redundancy_eliminator.bytecodes.c
Writes the cases to abstract_interp_cases.c.h, which is #included in Python/optimizer_analysis.c.
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
    replace_sync_sp,
)
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import StackOffset, Stack, SizeMismatch, UNUSED

DEFAULT_OUTPUT = ROOT / "Python/abstract_interp_cases.c.h"
DEFAULT_ABSTRACT_INPUT = ROOT / "Python/tier2_redundancy_eliminator_bytecodes.c"


def validate_uop(override: Uop, uop: Uop) -> None:
    # To do
    pass


def type_name(var: StackItem) -> str:
    if var.is_array():
        return f"_Py_UOpsSymType **"
    if var.type:
        return var.type
    return f"_Py_UOpsSymType *"


def declare_variables(uop: Uop, out: CWriter, skip_inputs: bool) -> None:
    variables = {"unused"}
    if not skip_inputs:
        for var in reversed(uop.stack.inputs):
            if var.name not in variables:
                variables.add(var.name)
                if var.condition:
                    out.emit(f"{type_name(var)}{var.name} = sym_init_null(ctx);\n")
                    out.emit(f"if({var.name}) {{goto error;}}\n")
                else:
                    out.emit(f"{type_name(var)}{var.name};\n")
    has_type_prop = any(output.type_prop is not None for output in uop.stack.outputs)
    for var in uop.stack.outputs:
        if var.peek and not has_type_prop:
            continue
        if var.name not in variables:
            variables.add(var.name)
            if var.condition:
                out.emit(f"{type_name(var)}{var.name} = sym_init_null(ctx);\n")
                out.emit(f"if({var.name}) {{goto error;}}\n")
            else:
                out.emit(f"{type_name(var)}{var.name};\n")


def decref_inputs(
    out: CWriter,
    tkn: Token,
    tkn_iter: Iterator[Token],
    uop: Uop,
    stack: Stack,
    inst: Instruction | None,
) -> None:
    next(tkn_iter)
    next(tkn_iter)
    next(tkn_iter)
    out.emit_at("", tkn)


def emit_default(out: CWriter, uop: Uop) -> None:
    for i, var in enumerate(uop.stack.outputs):
        if var.name != "unused" and not var.peek:
            if var.is_array():
                out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
                out.emit(f"{var.name}[_i] = sym_init_unknown(ctx);\n")
                out.emit(f"if ({var.name}[_i] == NULL) goto error;\n")
                out.emit("}\n")
            elif var.name == "null":
                out.emit(f"{var.name} = sym_init_unknown(ctx);\n")
                out.emit(f"if ({var.name} == NULL) goto error;\n")
            else:
                out.emit(f"{var.name} = sym_init_unknown(ctx);\n")
                out.emit(f"if ({var.name} == NULL) goto error;\n")


def write_uop(
    override: Uop | None,
    uop: Uop,
    out: CWriter,
    stack: Stack,
    debug: bool,
    skip_inputs: bool,
) -> None:
    try:
        has_type_prop = any(
            output.type_prop is not None for output in (override or uop).stack.outputs
        )
        prototype = override if override else uop
        is_override = override is not None
        out.start_line()
        for var in reversed(prototype.stack.inputs):
            if not skip_inputs or (has_type_prop and var.peek):
                out.emit(stack.pop(var))
        if not prototype.properties.stores_sp:
            for i, var in enumerate(prototype.stack.outputs):
                if not var.peek or is_override or has_type_prop:
                    out.emit(stack.push(var))
        if debug:
            args = []
            for var in prototype.stack.inputs:
                if not var.peek or is_override:
                    args.append(var.name)
            out.emit(f'DEBUG_PRINTF({", ".join(args)});\n')
        if override or has_type_prop:
            for cache in uop.caches:
                if cache.name != "unused":
                    if cache.size == 4:
                        type = cast = "PyObject *"
                    else:
                        type = f"uint{cache.size*16}_t "
                        cast = f"uint{cache.size*16}_t"
                    out.emit(f"{type}{cache.name} = ({cast})inst->operand;\n")
        if override:
            replacement_funcs = {
                "DECREF_INPUTS": decref_inputs,
                "SYNC_SP": replace_sync_sp,
            }
            emit_tokens(out, override, stack, None, replacement_funcs)
        else:
            emit_default(out, uop)
        # Type propagation
        out.emit("\n")
        for output in (override or uop).stack.outputs:
            if typ := output.type_prop:
                typname, refinement = typ
                refinement = refinement or "0"
                out.emit(f"sym_set_type({output.name}, {typname}, {refinement});\n")
            else:
                # Silence compiler unused variable warnings.
                if has_type_prop and output.name not in UNUSED:
                    out.emit(f"(void){output.name};\n")

        if prototype.properties.stores_sp:
            for i, var in enumerate(prototype.stack.outputs):
                if not var.peek or is_override:
                    out.emit(stack.push(var))
        out.start_line()
        stack.flush(out, cast_type="_Py_UOpsSymType *")
    except SizeMismatch as ex:
        raise analysis_error(ex.args[0], uop.body[0])


SKIPS = ("_EXTENDED_ARG",)


def generate_abstract_interpreter(
    filenames: list[str],
    abstract: Analysis,
    base: Analysis,
    outfile: TextIO,
    debug: bool,
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 2, False)
    out.emit("\n")
    for uop in base.uops.values():
        override: Uop | None = None
        if uop.name in abstract.uops:
            override = abstract.uops[uop.name]
            validate_uop(override, uop)
        if uop.properties.tier_one_only:
            continue
        if uop.is_super():
            continue
        if not uop.is_viable():
            out.emit(f"/* {uop.name} is not a viable micro-op for tier 2 */\n\n")
            continue
        out.emit(f"case {uop.name}: {{\n")
        if override:
            declare_variables(override, out, skip_inputs=False)
        else:
            declare_variables(uop, out, skip_inputs=True)
        stack = Stack()
        write_uop(override, uop, out, stack, debug, skip_inputs=(override is None))
        out.start_line()
        out.emit("break;\n")
        out.emit("}")
        out.emit("\n\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the tier 2 interpreter.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)


arg_parser.add_argument("input", nargs=1, help="Abstract interpreter definition file")

arg_parser.add_argument(
    "base", nargs=argparse.REMAINDER, help="The base instruction definition file(s)"
)

arg_parser.add_argument("-d", "--debug", help="Insert debug calls", action="store_true")

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.base) == 0:
        args.input.append(DEFAULT_INPUT)
        args.input.append(DEFAULT_ABSTRACT_INPUT)
    abstract = analyze_files(args.input)
    base = analyze_files(args.base)
    with open(args.output, "w") as outfile:
        generate_abstract_interpreter(args.input, abstract, base, outfile, args.debug)
