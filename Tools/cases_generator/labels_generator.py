"""Generate the main interpreter labels.
Reads the label definitions from bytecodes.c.
Writes the labels to generated_labels.c.h, which is #included in ceval.c.
"""

import argparse

from analyzer import (
    Analysis,
    Instruction,
    Uop,
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
)
from cwriter import CWriter
from typing import TextIO
from stack import Local, Stack, StackError, get_stack_effect, Storage


DEFAULT_OUTPUT = ROOT / "Python/generated_labels.c.h"



def declare_variable(var: StackItem, out: CWriter) -> None:
    type, null = type_and_null(var)
    space = " " if type[-1].isalnum() else ""
    if var.condition:
        out.emit(f"{type}{space}{var.name} = {null};\n")
    else:
        out.emit(f"{type}{space}{var.name};\n")


def declare_variables(inst: Instruction, out: CWriter) -> None:
    try:
        stack = get_stack_effect(inst)
    except StackError as ex:
        raise analysis_error(ex.args[0], inst.where) from None
    required = set(stack.defined)
    required.discard("unused")
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        for var in part.stack.inputs:
            if var.name in required:
                required.remove(var.name)
                declare_variable(var, out)
        for var in part.stack.outputs:
            if var.name in required:
                required.remove(var.name)
                declare_variable(var, out)


def write_uop(
    uop: Part,
    emitter: Emitter,
    offset: int,
    stack: Stack,
    inst: Instruction,
    braces: bool,
) -> tuple[int, Stack]:
    # out.emit(stack.as_comment() + "\n")
    if isinstance(uop, Skip):
        entries = "entries" if uop.size > 1 else "entry"
        emitter.emit(f"/* Skip {uop.size} cache {entries} */\n")
        return (offset + uop.size), stack
    if isinstance(uop, Flush):
        emitter.emit(f"// flush\n")
        stack.flush(emitter.out)
        return offset, stack
    try:
        locals: dict[str, Local] = {}
        emitter.out.start_line()
        if braces:
            emitter.out.emit(f"// {uop.name}\n")
            emitter.emit("{\n")
        code_list, storage = Storage.for_uop(stack, uop)
        emitter._print_storage(storage)
        for code in code_list:
            emitter.emit(code)

        for cache in uop.caches:
            if cache.name != "unused":
                if cache.size == 4:
                    type = "PyObject *"
                    reader = "read_obj"
                else:
                    type = f"uint{cache.size*16}_t "
                    reader = f"read_u{cache.size*16}"
                emitter.emit(
                    f"{type}{cache.name} = {reader}(&this_instr[{offset}].cache);\n"
                )
                if inst.family is None:
                    emitter.emit(f"(void){cache.name};\n")
            offset += cache.size

        storage = emitter.emit_tokens(uop, storage, inst)
        if braces:
            emitter.out.start_line()
            emitter.emit("}\n")
        # emitter.emit(stack.as_comment() + "\n")
        return offset, storage.stack
    except StackError as ex:
        raise analysis_error(ex.args[0], uop.body[0])


def uses_this(inst: Instruction) -> bool:
    if inst.properties.needs_this:
        return True
    for uop in inst.parts:
        if not isinstance(uop, Uop):
            continue
        for cache in uop.caches:
            if cache.name != "unused":
                return True
    return False


def generate_labels(
    filenames: list[str], analysis: Analysis, outfile: TextIO
) -> None:
    write_header(__file__, filenames, outfile)
    out = CWriter(outfile, 2, False)
    out.emit("\n")
    for name, label in analysis.labels.items():
        out.emit(f"{name}:\n")
        for tkn in label.body:
            out.emit(tkn)
        out.emit("\n")
        out.emit("\n")
    out.emit("\n")


arg_parser = argparse.ArgumentParser(
    description="Generate the code for the interpreter labels.",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter,
)

arg_parser.add_argument(
    "-o", "--output", type=str, help="Generated code", default=DEFAULT_OUTPUT
)

arg_parser.add_argument(
    "input", nargs=argparse.REMAINDER, help="Instruction definition file(s)"
)


def generate_labels_from_files(
    filenames: list[str], outfilename: str
) -> None:
    data = analyze_files(filenames)
    with open(outfilename, "w") as outfile:
        generate_labels(filenames, data, outfile)


if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_labels(args.input, data, outfile)
