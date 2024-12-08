"""Generate the cases for the tier 2 partial evaluator.
Reads the instruction definitions from bytecodes.c, optimizer_bytecodes.c and partial_evaluator_bytecodes.c
Writes the cases to partial_evaluator_cases.c.h, which is #included in Python/optimizer_analysis.c.
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
    Emitter,
    TokenIterator,
)
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import Local, Stack, StackError, Storage

DEFAULT_OUTPUT = ROOT / "Python/partial_evaluator_cases.c.h"
DEFAULT_ABSTRACT_INPUT = (ROOT / "Python/partial_evaluator_bytecodes.c").absolute().as_posix()

def validate_uop(override: Uop, uop: Uop) -> None:
    if len(override.stack.inputs) != len(uop.stack.inputs):
        assert False, f"Uop {uop.name} input length don't match."
    if len(override.stack.outputs) != len(uop.stack.outputs):
        assert False, f"Uop {uop.name} output length don't match."
    for override_input, uop_input in zip(override.stack.inputs, uop.stack.inputs):
        if override_input.name != uop_input.name:
            assert False, f"Uop {uop.name} input names don't match."
        if override_input.size != uop_input.size:
            assert False, f"Uop {uop.name} input sizes don't match."
    for override_output, uop_output in zip(override.stack.outputs, uop.stack.outputs):
        if override_output.name != uop_output.name:
            assert False, f"Uop {uop.name} output names don't match."
        if override_output.size != uop_output.size:
            assert False, f"Uop {uop.name} output sizes don't match."



def type_name(var: StackItem) -> str:
    if var.is_array():
        return f"_Py_UopsPESlot *"
    return f"_Py_UopsPESlot "

def var_name(var: StackItem, unused_count: int) -> tuple[str, int]:
    if var.name == "unused":
        name = f"unused_{unused_count}"
        unused_count += 1
    else:
        name = var.name
    return name, unused_count


def declare_variables(uop: Uop, out: CWriter) -> None:
    variables = set()
    unused_count = 0
    for var in reversed(uop.stack.inputs):
        if var.name not in variables:
            name, unused_count = var_name(var, unused_count)
            variables.add(name)
            if var.condition:
                out.emit(f"{type_name(var)}{name} = (_Py_UopsPESlot){{NULL, 0}};\n")
            else:
                out.emit(f"{type_name(var)}{name};\n")
    for var in uop.stack.outputs:
        if var.name not in variables:
            name, unused_count = var_name(var, unused_count)
            variables.add(name)
            if var.condition:
                out.emit(f"{type_name(var)}{name} = (_Py_UopsPESlot){{NULL, 0}};\n")
            else:
                out.emit(f"{type_name(var)}{name};\n")




def emit_default(out: CWriter, uop: Uop, stack: Stack) -> None:
    out.emit("MATERIALIZE_INST();\n")
    unused_count = 0
    for var in reversed(uop.stack.inputs):
        name, unused_count = var_name(var, unused_count)
        old_var_name = var.name
        var.name = name
        assign, _ = stack.pop(var, assign_unused=True)
        var.name = old_var_name
        out.emit(assign)
        if var.is_array():
            out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
            out.emit(f"materialize(&{name}[_i]);\n")
            out.emit("}\n")
        else:
            out.emit(f"materialize(&{name});\n")
    top_offset = stack.top_offset.copy()
    for var in uop.stack.outputs:
        if var.is_array() and not var.peek and not var.name == "unused":
            c_offset = top_offset.to_c()
            out.emit(f"{var.name} = &stack_pointer[{c_offset}];\n")
        top_offset.push(var)
    for var in uop.stack.outputs:
        local = Local.undefined(var)
        stack.push(local)
        if var.name != "unused" and not var.peek:
            local.defined = True
            if var.is_array():
                if var.size == "1":
                    out.emit(f"{var.name}[0] = sym_new_not_null(ctx);\n")
                else:
                    out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
                    out.emit(f"{var.name}[_i] = sym_new_not_null(ctx);\n")
                    out.emit("}\n")
            elif var.name == "null":
                out.emit(f"{var.name} = sym_new_null(ctx);\n")
            else:
                out.emit(f"{var.name} = sym_new_not_null(ctx);\n")
    if uop.properties.escapes:
        out.emit("materialize_ctx(ctx);\n")


class Tier2PEEmitter(Emitter):
    def __init__(self, out: CWriter):
        super().__init__(out)
        self._replacers["MATERIALIZE_INPUTS"] = self.materialize_inputs

    def materialize_inputs(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: Uop,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        next(tkn_iter)
        next(tkn_iter)
        next(tkn_iter)
        self.out.emit_at("", tkn)
        for var in uop.stack.inputs:
            if var.size:
                if var.size == "1":
                    self.out.emit(f"materialize(&{var.name}[0]);\n")
                else:
                    self.out.emit(f"for (int _i = {var.size}; --_i >= 0;) {{\n")
                    self.out.emit(f"materialize(&{var.name}[_i]);\n")
                    self.out.emit("}\n")
            elif var.condition:
                if var.condition == "1":
                    self.out.emit(f"materialize(&{var.name});\n")
                elif var.condition != "0":
                    self.out.emit(f"materialize(&{var.name});\n")
            else:
                self.out.emit(f"materialize(&{var.name});\n")
        return False

    def emit_save(self, storage: Storage) -> None:
        storage.flush(self.out)

    def emit_reload(self, storage: Storage) -> None:
        pass

def write_uop(
    override: Uop | None,
    uop: Uop,
    out: CWriter,
    stack: Stack,
    debug: bool,
) -> None:
    prototype = override if override else uop
    try:
        out.start_line()
        if override:
            code_list, storage = Storage.for_uop(stack, prototype, extract_bits=False)
            for code in code_list:
                out.emit(code)
        if debug:
            args = []
            for input in prototype.stack.inputs:
                if not input.peek or override:
                    args.append(input.name)
            out.emit(f'DEBUG_PRINTF({", ".join(args)});\n')
        if override:
            for cache in uop.caches:
                if cache.name != "unused":
                    if cache.size == 4:
                        type = cast = "PyObject *"
                    else:
                        type = f"uint{cache.size*16}_t "
                        cast = f"uint{cache.size*16}_t"
                    out.emit(f"{type}{cache.name} = ({cast})this_instr->operand0;\n")
        if override:
            emitter = Tier2PEEmitter(out)
            # No reference management of inputs needed.
            for var in storage.inputs:  # type: ignore[possibly-undefined]
                var.defined = False
            base_offset = stack.base_offset.copy()
            for input in reversed(uop.stack.inputs):
                c_offset = base_offset.to_c()
                if input.is_array():
                    out.emit(f"{input.name} = &stack_pointer[{c_offset}];\n")
                else:
                    out.emit(f"{input.name} = stack_pointer[{c_offset}];\n")
                base_offset.push(input)
            storage = emitter.emit_tokens(override, storage, None)
            out.start_line()
            storage.flush(out, cast_type="", extract_bits=False)
        else:
            emit_default(out, uop, stack)
            out.start_line()
            stack.flush(out, cast_type="", extract_bits=False)
    except StackError as ex:
        raise analysis_error(ex.args[0], prototype.body[0]) # from None


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
        assert (
            abstract_uop_name in base_uop_names
        ), f"All abstract uops should override base uops, but {abstract_uop_name} is not."

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
        declare_variables(uop, out)
        stack = Stack()
        write_uop(override, uop, out, stack, debug)
        out.start_line()
        out.emit("break;\n")
        out.emit("}")
        out.emit("\n\n")


def generate_tier2_abstract_from_files(
    filenames: list[str], outfilename: str, debug: bool = False
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


arg_parser.add_argument("input", nargs="*", help="Abstract interpreter definition file")

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
