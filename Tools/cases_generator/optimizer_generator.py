"""Generate the cases for the tier 2 optimizer.
Reads the instruction definitions from bytecodes.c and optimizer_bytecodes.c
Writes the cases to optimizer_cases.c.h, which is #included in Python/optimizer_analysis.c.
"""

import argparse

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    analyze_files,
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
from stack import Stack, SizeMismatch, UNUSED

DEFAULT_OUTPUT = ROOT / "Python/optimizer_cases.c.h"
DEFAULT_ABSTRACT_INPUT = (ROOT / "Python/optimizer_bytecodes.c").absolute().as_posix()


def validate_uop(override: Uop, uop: Uop) -> None:
    # To do
    pass


def type_name(var: StackItem) -> str:
    if var.is_array():
        return f"_Py_UopsSymbol **"
    if var.type:
        return var.type
    return f"_Py_UopsSymbol *"


def declare_variables(uop: Uop, out: CWriter, skip_inputs: bool) -> None:
    variables = {"unused"}
    if not skip_inputs:
        for var in reversed(uop.stack.inputs):
            if var.name not in variables:
                variables.add(var.name)
                if var.condition:
                    out.emit(f"{type_name(var)}{var.name} = NULL;\n")
                else:
                    out.emit(f"{type_name(var)}{var.name};\n")
    for var in uop.stack.outputs:
        if var.peek:
            continue
        if var.name not in variables:
            variables.add(var.name)
            if var.condition:
                out.emit(f"{type_name(var)}{var.name} = NULL;\n")
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
                out.emit(f"{var.name}[_i] = sym_new_unknown(ctx);\n")
                out.emit(f"if ({var.name}[_i] == NULL) goto out_of_space;\n")
                out.emit("}\n")
            elif var.name == "null":
                out.emit(f"{var.name} = sym_new_null(ctx);\n")
                out.emit(f"if ({var.name} == NULL) goto out_of_space;\n")
            else:
                out.emit(f"{var.name} = sym_new_unknown(ctx);\n")
                out.emit(f"if ({var.name} == NULL) goto out_of_space;\n")


def write_uop(
    override: Uop | None,
    uop: Uop,
    out: CWriter,
    stack: Stack,
    debug: bool,
    skip_inputs: bool,
) -> None:
    try:
        prototype = override if override else uop
        is_override = override is not None
        out.start_line()
        for var in reversed(prototype.stack.inputs):
            res = stack.pop(var)
            if not skip_inputs:
                out.emit(res)
        if not prototype.properties.stores_sp:
            for i, var in enumerate(prototype.stack.outputs):
                res = stack.push(var)
                if not var.peek or is_override:
                    out.emit(res)
        if debug:
            args = []
            for var in prototype.stack.inputs:
                if not var.peek or is_override:
                    args.append(var.name)
            out.emit(f'DEBUG_PRINTF({", ".join(args)});\n')
        if override:
            for cache in uop.caches:
                if cache.name != "unused":
                    if cache.size == 4:
                        type = cast = "PyObject *"
                    else:
                        type = f"uint{cache.size*16}_t "
                        cast = f"uint{cache.size*16}_t"
                    out.emit(f"{type}{cache.name} = ({cast})this_instr->operand;\n")
        if override:
            replacement_funcs = {
                "DECREF_INPUTS": decref_inputs,
                "SYNC_SP": replace_sync_sp,
            }
            emit_tokens(out, override, stack, None, replacement_funcs)
        else:
            emit_default(out, uop)

        if prototype.properties.stores_sp:
            for i, var in enumerate(prototype.stack.outputs):
                if not var.peek or is_override:
                    out.emit(stack.push(var))
        out.start_line()
        stack.flush(out, cast_type="_Py_UopsSymbol *")
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
    base_uop_names = set([uop.name for uop in base.uops.values()])
    for abstract_uop_name in abstract.uops:
        assert abstract_uop_name in base_uop_names,\
            f"All abstract uops should override base uops, but {abstract_uop_name} is not."

    for uop in base.uops.values():
        override: Uop | None = None
        if uop.name in abstract.uops:
            override = abstract.uops[uop.name]
            validate_uop(override, uop)
        if uop.properties.tier == 1:
            continue
        if uop.replicates:
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


def generate_tier2_abstract_from_files(
    filenames: list[str], outfilename: str, debug: bool=False
) -> None:
    assert len(filenames) == 2, "Need a base file and an abstract cases file."
    base = analyze_files([filenames[0]])
    abstract = analyze_files([filenames[1]])
    with open(outfilename, "w") as outfile:
        generate_abstract_interpreter(filenames, abstract, base, outfile, debug)


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the tier 2 interpreter.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)


arg_parser.add_argument("input", nargs='*', help="Abstract interpreter definition file")

arg_parser.add_argument(
    "base", nargs="*", help="The base instruction definition file(s)"
)

arg_parser.add_argument("-d", "--debug", help="Insert debug calls", action="store_true")

if __name__ == "__main__":
    args = arg_parser.parse_args()
    if not args.input:
        args.base.append(DEFAULT_INPUT)
        args.input.append(DEFAULT_ABSTRACT_INPUT)
    else:
        args.base.append(args.input[-1])
        args.input.pop()
    abstract = analyze_files(args.input)
    base = analyze_files(args.base)
    with open(args.output, "w") as outfile:
        generate_abstract_interpreter(args.input, abstract, base, outfile, args.debug)
