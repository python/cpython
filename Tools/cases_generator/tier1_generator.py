"""Generate the main interpreter switch.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse

from analyzer import (
    Analysis,
    Instruction,
    Uop,
    Label,
    CodeSection,
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
    TokenIterator,
    always_true,
    emit_to,
)
from cwriter import CWriter
from typing import TextIO
from lexer import Token
from stack import Local, Stack, StackError, get_stack_effect, Storage

DEFAULT_OUTPUT = ROOT / "Python/generated_cases.c.h"


FOOTER = "#undef TIER_ONE\n"
INSTRUCTION_START_MARKER = "/* BEGIN INSTRUCTIONS */"
INSTRUCTION_END_MARKER = "/* END INSTRUCTIONS */"
LABEL_START_MARKER = "/* BEGIN LABELS */"
LABEL_END_MARKER = "/* END LABELS */"


def declare_variable(var: StackItem, out: CWriter) -> None:
    type, null = type_and_null(var)
    space = " " if type[-1].isalnum() else ""
    out.emit(f"{type}{space}{var.name};\n")


def declare_variables(inst: Instruction, out: CWriter) -> None:
    try:
        stack = get_stack_effect(inst)
    except StackError as ex:
        raise analysis_error(ex.args[0], inst.where) from None
    seen = {"unused"}
    for part in inst.parts:
        if not isinstance(part, Uop):
            continue
        for var in part.stack.inputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out)
        for var in part.stack.outputs:
            if var.used and var.name not in seen:
                seen.add(var.name)
                declare_variable(var, out)


def write_uop(
    uop: Part,
    emitter: Emitter,
    offset: int,
    stack: Stack,
    inst: Instruction,
    braces: bool,
) -> tuple[bool, int, Stack]:
    # out.emit(stack.as_comment() + "\n")
    if isinstance(uop, Skip):
        entries = "entries" if uop.size > 1 else "entry"
        emitter.emit(f"/* Skip {uop.size} cache {entries} */\n")
        return True, (offset + uop.size), stack
    if isinstance(uop, Flush):
        emitter.emit(f"// flush\n")
        stack.flush(emitter.out)
        return True, offset, stack
    locals: dict[str, Local] = {}
    emitter.out.start_line()
    if braces:
        emitter.out.emit(f"// {uop.name}\n")
        emitter.emit("{\n")
        stack._print(emitter.out)
    storage = Storage.for_uop(stack, uop, emitter.out)

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

    reachable, storage = emitter.emit_tokens(uop, storage, inst, False)
    if braces:
        emitter.out.start_line()
        emitter.emit("}\n")
    # emitter.emit(stack.as_comment() + "\n")
    return reachable, offset, storage.stack


def uses_this(inst: Instruction) -> bool:
    if inst.properties.needs_this:
        return True
    for uop in inst.parts:
        if not isinstance(uop, Uop):
            continue
        for cache in uop.caches:
            if cache.name != "unused":
                return True
    # Can't be merged into the loop above, because
    # this must strictly be performed at the end.
    for uop in inst.parts:
        if not isinstance(uop, Uop):
            continue
        for tkn in uop.body.tokens():
            if (tkn.kind == "IDENTIFIER"
                    and (tkn.text in {"DEOPT_IF", "EXIT_IF", "AT_END_EXIT_IF"})):
                return True
    return False


UNKNOWN_OPCODE_HANDLER ="""\
_PyErr_Format(tstate, PyExc_SystemError,
              "%U:%d: unknown opcode %d",
              _PyFrame_GetCode(frame)->co_filename,
              PyUnstable_InterpreterFrame_GetLine(frame),
              opcode);
JUMP_TO_LABEL(error);
"""

def generate_tier1(
    filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write("""
#ifdef TIER_TWO
    #error "This file is for Tier 1 only"
#endif
#define TIER_ONE 1
""")
    outfile.write(f"""
#if !_Py_TAIL_CALL_INTERP
#if !USE_COMPUTED_GOTOS
    dispatch_opcode:
        switch (dispatch_code)
#endif
        {{
#endif /* _Py_TAIL_CALL_INTERP */
            {INSTRUCTION_START_MARKER}
"""
    )
    generate_tier1_cases(analysis, outfile, lines)
    outfile.write(f"""
            {INSTRUCTION_END_MARKER}
#if !_Py_TAIL_CALL_INTERP
#if USE_COMPUTED_GOTOS
        _unknown_opcode:
#else
        EXTRA_CASES  // From pycore_opcode_metadata.h, a 'case' for each unused opcode
#endif
            /* Tell C compilers not to hold the opcode variable in the loop.
               next_instr points the current instruction without TARGET(). */
            opcode = next_instr->op.code;
            {UNKNOWN_OPCODE_HANDLER}

        }}

        /* This should never be reached. Every opcode should end with DISPATCH()
           or goto error. */
        Py_UNREACHABLE();
#endif /* _Py_TAIL_CALL_INTERP */
        {LABEL_START_MARKER}
""")
    out = CWriter(outfile, 2, lines)
    emitter = Emitter(out, analysis.labels)
    generate_tier1_labels(analysis, emitter)
    outfile.write(f"{LABEL_END_MARKER}\n")
    outfile.write(FOOTER)



def generate_tier1_labels(
    analysis: Analysis, emitter: Emitter
) -> None:
    emitter.emit("\n")
    # Emit tail-callable labels as function defintions
    for name, label in analysis.labels.items():
        emitter.emit(f"LABEL({name})\n")
        storage = Storage(Stack(), [], [], 0, False)
        if label.spilled:
            storage.spilled = 1
        emitter.emit_tokens(label, storage, None)
        emitter.emit("\n\n")

def get_popped(inst: Instruction, analysis: Analysis) -> str:
    stack = get_stack_effect(inst)
    return (-stack.base_offset).to_c()

def generate_tier1_cases(
    analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    out = CWriter(outfile, 2, lines)
    emitter = Emitter(out, analysis.labels)
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(f"TARGET({name}) {{\n")
        popped = get_popped(inst, analysis)
        # We need to ifdef it because this breaks platforms
        # without computed gotos/tail calling.
        out.emit(f"#if _Py_TAIL_CALL_INTERP\n")
        out.emit(f"int opcode = {name};\n")
        out.emit(f"(void)(opcode);\n")
        out.emit(f"#endif\n")
        needs_this = uses_this(inst)
        unused_guard = "(void)this_instr;\n"
        if inst.properties.needs_prev:
            out.emit(f"_Py_CODEUNIT* const prev_instr = frame->instr_ptr;\n")

        if needs_this and not inst.is_target:
            out.emit(f"_Py_CODEUNIT* const this_instr = next_instr;\n")
            out.emit(unused_guard)
        if not inst.properties.no_save_ip:
            out.emit(f"frame->instr_ptr = next_instr;\n")

        out.emit(f"next_instr += {inst.size};\n")
        out.emit(f"INSTRUCTION_STATS({name});\n")
        if inst.is_target:
            out.emit(f"PREDICTED_{name}:;\n")
            if needs_this:
                out.emit(f"_Py_CODEUNIT* const this_instr = next_instr - {inst.size};\n")
                out.emit(unused_guard)
        if inst.properties.uses_opcode:
            out.emit(f"opcode = {name};\n")
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
            insert_braces = len([p for p in inst.parts if isinstance(p, Uop)]) > 1
            reachable, offset, stack = write_uop(part, emitter, offset, stack, inst, insert_braces)
        out.start_line()
        if reachable: # type: ignore[possibly-undefined]
            stack.flush(out)
            out.emit("DISPATCH();\n")
        out.start_line()
        out.emit("}")
        out.emit("\n")


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


def generate_tier1_from_files(
    filenames: list[str], outfilename: str, lines: bool
) -> None:
    data = analyze_files(filenames)
    with open(outfilename, "w") as outfile:
        generate_tier1(filenames, data, outfile, lines)


if __name__ == "__main__":
    args = arg_parser.parse_args()
    if len(args.input) == 0:
        args.input.append(DEFAULT_INPUT)
    data = analyze_files(args.input)
    with open(args.output, "w") as outfile:
        generate_tier1(args.input, data, outfile, args.emit_line_directives)
