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
    always_true,
)
from cwriter import CWriter
from typing import TextIO
from lexer import Token
from stack import Local, Stack, StackError, Storage

DEFAULT_OUTPUT = ROOT / "Python/optimizer_cases.c.h"
DEFAULT_ABSTRACT_INPUT = (ROOT / "Python/optimizer_bytecodes.c").absolute().as_posix()


def validate_uop(override: Uop, uop: Uop) -> None:
    """
    Check that the overridden uop (defined in 'optimizer_bytecodes.c')
    has the same stack effects as the original uop (defined in 'bytecodes.c').

    Ensure that:
        - The number of inputs and outputs is the same.
        - The names of the inputs and outputs are the same
          (except for 'unused' which is ignored).
        - The sizes of the inputs and outputs are the same.
    """
    for stack_effect in ('inputs', 'outputs'):
        orig_effects = getattr(uop.stack, stack_effect)
        new_effects = getattr(override.stack, stack_effect)

        if len(orig_effects) != len(new_effects):
            msg = (
                f"{uop.name}: Must have the same number of {stack_effect} "
                "in bytecodes.c and optimizer_bytecodes.c "
                f"({len(orig_effects)} != {len(new_effects)})"
            )
            raise analysis_error(msg, override.body.open)

        for orig, new in zip(orig_effects, new_effects, strict=True):
            if orig.name != new.name and orig.name != "unused" and new.name != "unused":
                msg = (
                    f"{uop.name}: {stack_effect.capitalize()} must have "
                    "equal names in bytecodes.c and optimizer_bytecodes.c "
                    f"({orig.name} != {new.name})"
                )
                raise analysis_error(msg, override.body.open)

            if orig.size != new.size:
                msg = (
                    f"{uop.name}: {stack_effect.capitalize()} must have "
                    "equal sizes in bytecodes.c and optimizer_bytecodes.c "
                    f"({orig.size!r} != {new.size!r})"
                )
                raise analysis_error(msg, override.body.open)


def type_name(var: StackItem) -> str:
    if var.is_array():
        return "JitOptRef *"
    return "JitOptRef "

def stackref_type_name(var: StackItem) -> str:
    assert not var.is_array(), "Unsafe to convert a symbol to an array-like StackRef."
    return "_PyStackRef "

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

    def __init__(self, out: CWriter, labels: dict[str, Label], original_uop: Uop, stack: Stack):
        super().__init__(out, labels)
        self._replacers["REPLACE_OPCODE_IF_EVALUATES_PURE"] = self.replace_opcode_if_evaluates_pure
        self.original_uop = original_uop
        self.stack = stack

    def emit_save(self, storage: Storage) -> None:
        storage.flush(self.out)

    def emit_reload(self, storage: Storage) -> None:
        pass

    def goto_label(self, goto: Token, label: Token, storage: Storage) -> None:
        self.out.emit(goto)
        self.out.emit(label)

    def replace_opcode_if_evaluates_pure(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        assert isinstance(uop, Uop)
        input_identifiers = []
        for token in tkn_iter:
            if token.kind == "IDENTIFIER":
                input_identifiers.append(token)
            if token.kind == "SEMI":
                break

        if len(input_identifiers) == 0:
            raise analysis_error(
                "To evaluate an operation as pure, it must have at least 1 input",
                tkn
            )
        # Check that the input identifiers belong to the uop's
        # input stack effect
        uop_stack_effect_input_identifers = {inp.name for inp in uop.stack.inputs}
        for input_tkn in input_identifiers:
            if input_tkn.text not in uop_stack_effect_input_identifers:
                raise analysis_error(f"{input_tkn.text} referenced in "
                                     f"REPLACE_OPCODE_IF_EVALUATES_PURE but does not "
                                     f"exist in the base uop's input stack effects",
                                     input_tkn)
        input_identifiers_as_str = {tkn.text for tkn in input_identifiers}
        used_stack_inputs = [inp for inp in uop.stack.inputs if inp.name in input_identifiers_as_str]
        assert len(used_stack_inputs) > 0
        emitter = OptimizerConstantEmitter(self.out, {}, self.original_uop, self.stack.copy())
        emitter.emit("if (\n")
        for inp in used_stack_inputs[:-1]:
            emitter.emit(f"sym_is_safe_const(ctx, {inp.name}) &&\n")
        emitter.emit(f"sym_is_safe_const(ctx, {used_stack_inputs[-1].name})\n")
        emitter.emit(') {\n')
        # Declare variables, before they are shadowed.
        for inp in used_stack_inputs:
            if inp.used:
                emitter.emit(f"{type_name(inp)}{inp.name}_sym = {inp.name};\n")
        # Shadow the symbolic variables with stackrefs.
        for inp in used_stack_inputs:
            if inp.is_array():
                raise analysis_error("Pure evaluation cannot take array-like inputs.", tkn)
            if inp.used:
                emitter.emit(f"{stackref_type_name(inp)}{inp.name} = sym_get_const_as_stackref(ctx, {inp.name}_sym);\n")
        # Rename all output variables to stackref variant.
        for outp in self.original_uop.stack.outputs:
            if outp.is_array():
                raise analysis_error(
                    "Array output StackRefs not supported for evaluating pure ops.",
                    self.original_uop.body.open
                )
            emitter.emit(f"_PyStackRef {outp.name}_stackref;\n")


        storage = Storage.for_uop(self.stack, self.original_uop, CWriter.null(), check_liveness=False)
        # No reference management of outputs needed.
        for var in storage.outputs:
            var.in_local = True
        emitter.emit("/* Start of uop copied from bytecodes for constant evaluation */\n")
        emitter.emit_tokens(self.original_uop, storage, inst=None, emit_braces=False)
        self.out.start_line()
        emitter.emit("/* End of uop copied from bytecodes for constant evaluation */\n")
        # Finally, assign back the output stackrefs to symbolics.
        for outp in self.original_uop.stack.outputs:
            # All new stackrefs are created from new references.
            # That's how the stackref contract works.
            if not outp.peek:
                emitter.emit(f"{outp.name} = sym_new_const_steal(ctx, PyStackRef_AsPyObjectSteal({outp.name}_stackref));\n")
            else:
                emitter.emit(f"{outp.name} = sym_new_const(ctx, PyStackRef_AsPyObjectBorrow({outp.name}_stackref));\n")
        storage.flush(self.out)
        emitter.emit("break;\n")
        emitter.emit("}\n")
        return True

class OptimizerConstantEmitter(OptimizerEmitter):
    def __init__(self, out: CWriter, labels: dict[str, Label], original_uop: Uop, stack: Stack):
        super().__init__(out, labels, original_uop, stack)
        # Replace all outputs to point to their stackref versions.
        overrides = {
            outp.name: self.emit_stackref_override for outp in self.original_uop.stack.outputs
        }
        self._replacers = {**self._replacers, **overrides}
        self.cannot_escape = True

    def emit_to_with_replacement(
        self,
        out: CWriter,
        tkn_iter: TokenIterator,
        end: str,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None
    ) -> Token:
        parens = 0
        for tkn in tkn_iter:
            if tkn.kind == end and parens == 0:
                return tkn
            if tkn.kind == "LPAREN":
                parens += 1
            if tkn.kind == "RPAREN":
                parens -= 1
            if tkn.text in self._replacers:
                self._replacers[tkn.text](tkn, tkn_iter, uop, storage, inst)
            else:
                out.emit(tkn)
        raise analysis_error(f"Expecting {end}. Reached end of file", tkn)

    def emit_stackref_override(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        self.out.emit(tkn)
        self.out.emit("_stackref ")
        return True

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
        self.emit_to_with_replacement(self.out, tkn_iter, "RPAREN", uop, storage, inst)
        self.emit(") {\n")
        next(tkn_iter)  # Semi colon
        # We guarantee this will deopt in real-world code
        # via constants analysis. So just bail.
        self.emit("ctx->done = true;\n")
        self.emit("break;\n")
        self.emit("}\n")
        return not always_true(first_tkn)

    exit_if = deopt_if

    def error_if(
        self,
        tkn: Token,
        tkn_iter: TokenIterator,
        uop: CodeSection,
        storage: Storage,
        inst: Instruction | None,
    ) -> bool:
        lparen = next(tkn_iter)
        assert lparen.kind == "LPAREN"
        first_tkn = tkn_iter.peek()
        unconditional = always_true(first_tkn)
        if unconditional:
            next(tkn_iter)
            next(tkn_iter)  # RPAREN
            self.out.start_line()
        else:
            self.out.emit_at("if ", tkn)
            self.emit(lparen)
            self.emit_to_with_replacement(self.out, tkn_iter, "RPAREN", uop, storage, inst)
            self.out.emit(") {\n")
        next(tkn_iter)  # Semi colon
        storage.clear_inputs("at ERROR_IF")

        self.out.emit("goto error;\n")
        if not unconditional:
            self.out.emit("}\n")
        return not unconditional


def write_uop(
    override: Uop | None,
    uop: Uop,
    out: CWriter,
    stack: Stack,
    debug: bool,
    skip_inputs: bool,
) -> None:
    locals: dict[str, Local] = {}
    prototype = override if override else uop
    try:
        out.start_line()
        if override:
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
        if override:
            emitter = OptimizerEmitter(out, {}, uop, stack.copy())
            # No reference management of inputs needed.
            for var in storage.inputs:  # type: ignore[possibly-undefined]
                var.in_local = False
            _, storage = emitter.emit_tokens(override, storage, None, False)
            out.start_line()
            storage.flush(out)
            out.start_line()
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
            declare_variables(uop, out, skip_inputs=True)
        stack = Stack()
        write_uop(override, uop, out, stack, debug, skip_inputs=(override is None))
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
