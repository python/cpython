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
    CodeSection,
    Label,
)
from generators_common import (
    DEFAULT_INPUT,
    ROOT,
    write_header,
    Emitter,
    TokenIterator,
    emit_to,
    skip_to,
)
from cwriter import CWriter
from typing import TextIO
from lexer import Token
from stack import Local, Stack, StackError, Storage

DEFAULT_OUTPUT = ROOT / "Python/optimizer_cases.c.h"
DEFAULT_ABSTRACT_INPUT = (ROOT / "Python/optimizer_bytecodes.c").absolute().as_posix()


def validate_uop(override: Uop, uop: Uop) -> None:
    # To do
    pass


def type_name(var: StackItem) -> str:
    if var.is_array():
        return f"JitOptSymbol **"
    if var.type:
        return var.type
    return f"JitOptSymbol *"


def declare_variables(uop: Uop, out: CWriter, skip_inputs: bool) -> None:
    variables = {"unused"}
    if not skip_inputs:
        for var in reversed(uop.stack.inputs):
            if var.used and var.name not in variables:
                variables.add(var.name)
                out.emit(f"{type_name(var)}{var.name};\n")
    for var in uop.stack.outputs:
        if var.peek:
            continue
        if var.name not in variables:
            variables.add(var.name)
            out.emit(f"{type_name(var)}{var.name};\n")


def decref_inputs(
    out: CWriter,
    tkn: Token,
    tkn_iter: TokenIterator,
    uop: Uop,
    stack: Stack,
    inst: Instruction | None,
) -> None:
    next(tkn_iter)
    next(tkn_iter)
    next(tkn_iter)
    out.emit_at("", tkn)


def emit_default(out: CWriter, uop: Uop, stack: Stack) -> None:
    null = CWriter.null()
    for var in reversed(uop.stack.inputs):
        stack.pop(var, null)
    offset = stack.base_offset - stack.physical_sp
    for var in uop.stack.outputs:
        if var.is_array() and not var.peek and not var.name == "unused":
            c_offset = offset.to_c()
            out.emit(f"{var.name} = &stack_pointer[{c_offset}];\n")
        offset = offset.push(var)
    for var in uop.stack.outputs:
        local = Local.undefined(var)
        stack.push(local)
        if var.name != "unused" and not var.peek:
            local.in_local = True
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


class OptimizerEmitter(Emitter):

    def emit_save(self, storage: Storage) -> None:
        storage.flush(self.out)

    def emit_reload(self, storage: Storage) -> None:
        pass

    def goto_label(self, goto: Token, label: Token, storage: Storage) -> None:
        self.out.emit(goto)
        self.out.emit(label)

class OptimizerConstantEmitter(OptimizerEmitter):
    def __init__(self, out: CWriter, labels: dict[str, Label]):
        super().__init__(out, labels)
        overrides = {
            "PyStackRef_AsPyObjectBorrow": self.emit_stackref_borrow,
            "PyStackRef_CLOSE_SPECIALIZED": self.emit_nothing,
            "PyStackRef_CLOSE": self.emit_nothing,
            "PyStackRef_FromPyObjectSteal": self.emit_stackref_steal,
            "PyStackRef_IsNull": self.emit_stackref_null,
            "PyStackRef_IsFalse": self.emit_stackref_isfalse,
            "PyStackRef_IsTrue": self.emit_stackref_istrue,
            "PyStackRef_False": self.emit_stackref_false,
            "PyStackRef_True": self.emit_stackref_true,
            "assert": self.emit_nothing,
        }
        self._replacers = {**self._replacers, **overrides}

    def emit_stackref_borrow(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        next(tkn_iter)
        self.out.emit(" sym_get_const(ctx, ")
        rparen = emit_to(self.out, tkn_iter, "RPAREN")
        self.emit(rparen)
        return True

    def emit_stackref_steal(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        next(tkn_iter)
        self.out.emit(" sym_new_const_steal(ctx, ")
        rparen = emit_to(self.out, tkn_iter, "RPAREN")
        self.emit(rparen)
        return True

    def emit_nothing(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        while (tkn := next(tkn_iter)).kind != "SEMI":
            pass
        return True

    def emit_stackref_null(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        next(tkn_iter)
        self.out.emit(" sym_is_null(")
        rparen = emit_to(self.out, tkn_iter, "RPAREN")
        self.emit(rparen)
        return True

    def emit_stackref_isfalse(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        next(tkn_iter)
        name = next(tkn_iter)
        assert name.kind == "IDENTIFIER", \
            "PyStackRef_IsFalse(target), target must be a simple identifier"
        self.out.emit(f"(sym_is_const(ctx, {name.text}) && "
                      f"Py_IsFalse(sym_get_const(ctx, {name.text})))")
        next(tkn_iter)
        return True

    def emit_stackref_istrue(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        next(tkn_iter)
        name = next(tkn_iter)
        assert name.kind == "IDENTIFIER", \
            "PyStackRef_IsTrue(target), target must be a simple identifier"
        self.out.emit(f"(sym_is_const(ctx, {name.text}_o) && "
                      f"Py_IsTrue(sym_get_const(ctx, {name.text}_o)))")
        next(tkn_iter)
        return True

    def emit_stackref_false(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        name = tkn
        assert name.kind == "IDENTIFIER", \
            "PyStackRef_False must be a simple identifier"
        self.out.emit(f"sym_new_const(ctx, Py_False)")
        return True

    def emit_stackref_true(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ):
        name = tkn
        assert name.kind == "IDENTIFIER", \
            "PyStackRef_True must be a simple identifier"
        self.out.emit(f"sym_new_const(ctx, Py_True)")
        return True

def write_uop(
    override: Uop | None,
    uop: Uop,
    out: CWriter,
    stack: Stack,
    debug: bool,
) -> None:
    locals: dict[str, Local] = {}
    prototype = override if override else uop
    try:
        out.start_line()
        if override or uop.properties.pure:
            storage = Storage.for_uop(stack, prototype, out, check_liveness=False)
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
        if override or uop.properties.pure:
            # No reference management of inputs needed.
            for var in storage.inputs:  # type: ignore[possibly-undefined]
                var.in_local = False
            if uop.properties.pure:
                emitter = OptimizerConstantEmitter(out, {})
                emitter.emit("if (\n")
                for inp in uop.stack.inputs:
                    emitter.emit(f"sym_is_const(ctx, {inp.name}) &&\n")
                emitter.emit("1) {\n")
                _, storage = emitter.emit_tokens(uop, storage, None, False, is_abstract=True)
                out.start_line()
                emitter.emit("}\n")
                emitter.emit("else {\n")
            out.start_line()
            if override:
                emitter = OptimizerEmitter(out, {})
                _, storage = emitter.emit_tokens(override, storage, None, False, is_abstract=True)
            else:
                emit_default(out, uop, stack)
            out.start_line()
            if uop.properties.pure:
                emitter.emit("}\n")
            storage.flush(out)
        else:
            emit_default(out, uop, stack)
            out.start_line()
            stack.flush(out)
    except StackError as ex:
        raise analysis_error(ex.args[0], prototype.body.open) # from None


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
        if override:
            declare_variables(override, out, skip_inputs=False)
        else:
            declare_variables(uop, out, skip_inputs=not uop.properties.pure)
        stack = Stack(extract_bits=False, cast_type="JitOptSymbol *")
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
