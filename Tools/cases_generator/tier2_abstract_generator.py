"""Generate the cases for the tier 2 abstract interpreter.
Reads the instruction definitions from bytecodes.c.
Writes the cases to abstract_interp_cases.c.h, which is #included in optimizer_analysis.c
"""

import argparse
import os.path
import sys
import dataclasses

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
from tier2_generator import tier2_replace_error
from cwriter import CWriter
from typing import TextIO, Iterator
from lexer import Token
from stack import StackOffset, Stack, SizeMismatch, UNUSED

DEFAULT_OUTPUT = ROOT / "Python/abstract_interp_cases.c.h"

SPECIALLY_HANDLED_ABSTRACT_INSTR = {
    "LOAD_FAST",
    "LOAD_FAST_CHECK",
    "LOAD_FAST_AND_CLEAR",
    "LOAD_CONST",
    "STORE_FAST",
    "STORE_FAST_MAYBE_NULL",
    "COPY",
    "POP_TOP",
    "PUSH_NULL",
    "SWAP",
    "END_SEND",
    # Frame stuff
    "_PUSH_FRAME",
    "_POP_FRAME",
    "_SHRINK_STACK",
}

NO_CONST_OR_TYPE_EVALUATE = {
    "_RESUME_CHECK",
    "_GUARD_GLOBALS_VERSION",
    "_GUARD_BUILTINS_VERSION",
    "_CHECK_MANAGED_OBJECT_HAS_VALUES",
    "_CHECK_PEP_523",
    "_CHECK_STACK_SPACE",
    "_INIT_CALL_PY_EXACT_ARGS",
}


def declare_variables(
    uop: Uop,
    out: CWriter,
    default_type: str = "_Py_UOpsSymbolicExpression *",
    skip_inputs: bool = False,
    skip_peeks: bool = False,
) -> None:
    variables = set(UNUSED)
    if not skip_inputs:
        for var in reversed(uop.stack.inputs):
            if skip_peeks and var.peek:
                continue
            if var.name not in variables:
                type = var.type if var.type else default_type
                if var.size > "1" and type == "PyObject **":
                    type = "_Py_UOpsSymbolicExpression **"
                variables.add(var.name)
                if var.condition:
                    out.emit(f"{type}{var.name} = NULL;\n")
                else:
                    out.emit(f"{type}{var.name};\n")
    for var in uop.stack.outputs:
        if skip_peeks and var.peek:
            continue
        if var.name not in variables:
            variables.add(var.name)
            type = var.type if var.type else default_type
            if var.condition:
                out.emit(f"{type}{var.name} = NULL;\n")
            else:
                out.emit(f"{type}{var.name};\n")


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
    out.emit(") goto error;\n")


TIER2_REPLACEMENT_FUNCTIONS = REPLACEMENT_FUNCTIONS.copy()
TIER2_REPLACEMENT_FUNCTIONS["ERROR_IF"] = tier2_replace_error
TIER2_REPLACEMENT_FUNCTIONS["DEOPT_IF"] = tier2_replace_deopt


def _write_body_abstract_interp_impure_uop(
    mangled_uop: Uop, uop: Uop, out: CWriter, stack: Stack
) -> None:
    # Simply make all outputs effects unknown

    for var in mangled_uop.stack.outputs:
        if var.name in UNUSED or var.peek:
            continue

        if var.size == "1":
            out.emit(f"{var.name} = sym_init_unknown(ctx);\n")
            out.emit(f"if({var.name} == NULL) goto error;\n")
            if var.name in ("null", "__null_"):
                out.emit(f"sym_set_type({var.name}, NULL_TYPE, 0);\n")
        else:
            out.emit(
                f"for (int case_gen_i = 0; case_gen_i < {var.size}; case_gen_i++) {{\n"
            )
            out.emit(f"{var.name}[case_gen_i] = sym_init_unknown(ctx);\n")
            out.emit(f"if({var.name}[case_gen_i] == NULL) goto error;\n")
            out.emit("}\n")


def mangle_uop_names(uop: Uop) -> Uop:
    uop = dataclasses.replace(uop)
    new_stack = dataclasses.replace(uop.stack)
    new_stack.inputs = [
        dataclasses.replace(var, name=f"__{var.name}_") for var in uop.stack.inputs
    ]
    new_stack.outputs = [
        dataclasses.replace(var, name=f"__{var.name}_") for var in uop.stack.outputs
    ]
    uop.stack = new_stack
    return uop


# Returns a tuple of a pointer to an array of subexpressions, the length of said array
# and a string containing the join of all other subexpressions obtained from stack input.
# This grabs variadic inputs that depend on things like oparg or cache
def get_subexpressions(input_vars: list[StackItem]) -> tuple[str, int, str]:
    arr_var = [(var.name, var) for var in input_vars if var.size > "1"]
    assert len(arr_var) <= 1, "Can have at most one array input from oparg/cache"
    arr_var_name = arr_var[0][0] if len(arr_var) == 1 else None
    arr_var_size = (arr_var[0][1].size or 0) if arr_var_name is not None else 0
    if arr_var_name is not None:
        input_vars.remove(arr_var[0][1])
    var = ", ".join([v.name for v in input_vars])
    if var:
        var = ", " + var
    return arr_var_name, arr_var_size, var


def new_sym(
    constant: str | None,
    arr_var_size: int | str | None,
    arr_var_name: str | None,
    subexpresion_count: int | str,
    subexpressions: str,
) -> str:
    return (
        f"_Py_UOpsSymbolicExpression_New("
        f"ctx, *inst, {constant or 'NULL'}, "
        f"{arr_var_size or 0}, {arr_var_name or 'NULL'}, "
        f"{subexpresion_count} {subexpressions});"
    )


def _write_body_abstract_interp_pure_uop(
    mangled_uop: Uop, uop: Uop, out: CWriter, stack: Stack
) -> None:
    arr_var_name, arr_var_size, subexpressions = get_subexpressions(
        mangled_uop.stack.inputs
    )

    assert (
        len(uop.stack.outputs) == 1
    ), f"Currently we only support 1 stack output for pure ops: {uop}"
    # uop is mandatory - we cannot const evaluate it
    sym = new_sym(
        None, arr_var_size, arr_var_name, len(mangled_uop.stack.inputs), subexpressions
    )
    if uop.name in NO_CONST_OR_TYPE_EVALUATE:
        out.emit(f"{mangled_uop.stack.outputs[0].name} = {sym}")
        return

    # Constant prop only handles one output, and no variadic inputs.
    # Perhaps in the future we can support these.
    if all(input.size == "1" for input in uop.stack.inputs):
        # We can try a constant evaluation
        out.emit("// Constant evaluation\n")
        predicates = " && ".join(
            [
                f"is_const({var.name})"
                for var in mangled_uop.stack.inputs
                if var.name not in UNUSED
            ]
        )

        out.emit(f"if ({predicates or 0}){{\n")
        declare_variables(uop, out, default_type="PyObject *")
        for var, mangled_var in zip(uop.stack.inputs, mangled_uop.stack.inputs):
            out.emit(f"{var.name} = get_const({mangled_var.name});\n")
        emit_tokens(out, uop, stack, None, TIER2_REPLACEMENT_FUNCTIONS)
        out.emit("\n")
        maybe_const_val = new_sym(
            f"(PyObject *){uop.stack.outputs[0].name}",
            None,
            None,
            len(mangled_uop.stack.inputs),
            subexpressions,
        )
        out.emit(f"{mangled_uop.stack.outputs[0].name} = {maybe_const_val}\n")

        out.emit("}\n")
        out.emit("else {\n")
        sym = new_sym(None, None, None, len(mangled_uop.stack.inputs), subexpressions)
        out.emit(f"{mangled_uop.stack.outputs[0].name} = {sym}\n")
        out.emit("}\n")
    else:
        sym = new_sym(None, None, None, len(mangled_uop.stack.inputs), subexpressions)
        out.emit(f"{mangled_uop.stack.outputs[0].name} = {sym}\n")

    out.emit(f"if ({mangled_uop.stack.outputs[0].name} == NULL) goto error;\n")

    # Perform type propagation
    if (typ := uop.stack.outputs[0].type_prop) is not None:
        typname, aux = typ
        aux = "0" if aux is None else aux
        out.emit("// Type propagation\n")
        out.emit(
            f"sym_set_type({mangled_uop.stack.outputs[0].name}, {typname}, (uint32_t){aux});"
        )


def _write_body_abstract_interp_guard_uop(
    mangled_uop: Uop, uop: Uop, out: CWriter, stack: Stack
) -> None:
    # 1. Attempt to perform guard elimination
    # 2. Type propagate for guard success
    if uop.name in NO_CONST_OR_TYPE_EVALUATE:
        out.emit("goto guard_required;")
        return

    for cache in uop.caches:
        if cache.name not in UNUSED:
            if cache.size == 4:
                type = cast = "PyObject *"
            else:
                type = f"uint{cache.size*16}_t "
                cast = f"uint{cache.size*16}_t"
            out.emit(f"{type}{cache.name} = ({cast})CURRENT_OPERAND();\n")

    out.emit("// Constant evaluation \n")
    predicates_str = " && ".join(
        [
            f"is_const({var.name})"
            for var in mangled_uop.stack.inputs
            if var.name not in UNUSED
        ]
    )
    if predicates_str:
        out.emit(f"if ({predicates_str}) {{\n")
        declare_variables(uop, out, default_type="PyObject *")
        for var, mangled_var in zip(uop.stack.inputs, mangled_uop.stack.inputs):
            if var.name in UNUSED:
                continue
            out.emit(f"{var.name} = get_const({mangled_var.name});\n")
        emit_tokens(out, uop, stack, None, TIER2_REPLACEMENT_FUNCTIONS)
        out.emit("\n")
        # Guard elimination - if we are successful, don't add it to the symexpr!
        out.emit('DPRINTF(3, "const eliminated guard\\n");\n')
        out.emit("break;\n")
        out.emit("}\n")

    # Does the input specify typed inputs?
    if not any(output_var.type_prop for output_var in mangled_uop.stack.outputs):
        return
    # If the input types already match, eliminate the guard
    # Read the cache information to check the auxiliary type information
    predicates = []
    propagates = []

    assert len(mangled_uop.stack.outputs) == len(
        mangled_uop.stack.inputs
    ), "guards must have same number of args"
    assert [
        output == input_
        for output, input_ in zip(mangled_uop.stack.outputs, mangled_uop.stack.inputs)
    ], "guards must forward their stack values"
    for output_var in mangled_uop.stack.outputs:
        if output_var.name in UNUSED:
            continue
        if (typ := output_var.type_prop) is not None:
            typname, aux = typ
            aux = "0" if aux is None else aux
            # Check that the input type information match (including auxiliary info)
            predicates.append(
                f"sym_matches_type((_Py_UOpsSymbolicExpression *){output_var.name}, {typname}, (uint32_t){aux})"
            )
            # Propagate mode - set the types
            propagates.append(
                f"sym_set_type((_Py_UOpsSymbolicExpression *){output_var.name}, {typname}, (uint32_t){aux})"
            )

    out.emit("// Type guard elimination \n")
    out.emit(f"if ({' && '.join(predicates)}){{\n")
    out.emit('DPRINTF(2, "type propagation eliminated guard\\n");\n')
    out.emit("break;\n")
    out.emit("}\n")
    # Else we need the guard
    out.emit("else {\n")
    out.emit("// Type propagation \n")
    for prop in propagates:
        out.emit(f"{prop};\n")
    out.emit("goto guard_required;\n")
    out.emit("}\n")


def write_abstract_uop(mangled_uop: Uop, uop: Uop, out: CWriter, stack: Stack) -> None:
    try:
        out.start_line()
        for var in reversed(mangled_uop.stack.inputs):
            is_impure = (
                not mangled_uop.properties.pure and not mangled_uop.properties.guard
            )
            old_var_name = var.name
            # code smell, but basically impure ops don't use any of their inputs
            if is_impure:
                var.name = "unused"
            out.emit(stack.pop(var))
            var.name = old_var_name
        if not mangled_uop.properties.stores_sp:
            for i, var in enumerate(mangled_uop.stack.outputs):
                out.emit(stack.push(var))
        # emit_tokens(out, uop, stack, None, TIER2_REPLACEMENT_FUNCTIONS)
        if uop.properties.pure:
            _write_body_abstract_interp_pure_uop(mangled_uop, uop, out, stack)
        elif uop.properties.guard:
            _write_body_abstract_interp_guard_uop(mangled_uop, uop, out, stack)
        else:
            _write_body_abstract_interp_impure_uop(mangled_uop, uop, out, stack)
    except SizeMismatch as ex:
        raise analysis_error(ex.args[0], uop.body[0])


SKIPS = ("_EXTENDED_ARG",)


def generate_tier2_abstract(
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
        if name in SPECIALLY_HANDLED_ABSTRACT_INSTR:
            continue
        if uop.properties.tier_one_only:
            continue
        if uop.is_super():
            continue
        if not uop.is_viable():
            out.emit(f"/* {uop.name} is not a viable micro-op for tier 2 */\n\n")
            continue
        out.emit(f"case {uop.name}: {{\n")
        mangled_uop = mangle_uop_names(uop)
        is_impure = not mangled_uop.properties.pure and not mangled_uop.properties.guard
        declare_variables(mangled_uop, out, skip_inputs=is_impure, skip_peeks=is_impure)
        stack = Stack()
        write_abstract_uop(mangled_uop, uop, out, stack)
        out.start_line()
        if not uop.properties.always_exits:
            stack.flush(out)
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
        generate_tier2_abstract(args.input, data, outfile, args.emit_line_directives)
