"""Generate the main interpreter tail call.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
"""

import argparse
import re

from typing import TextIO

from generators_common import (
    ROOT,
    write_header,
)

from tier1_generator import (
    write_single_inst
)

DEFAULT_INPUT = ROOT / "Python/generated_cases.c.h"
DEFAULT_OUTPUT = ROOT / "Python/generated_cases_tail_call.c.h"


"""Generate the main interpreter tail calls.
Reads the instruction definitions from bytecodes.c.
Writes the cases to generated_cases.c.h, which is #included in ceval.c.
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
from stack import Stack


DEFAULT_OUTPUT = ROOT / "Python/generated_cases_tail_call.c.h"


FOOTER = "#undef TIER_ONE\n"


def generate_tier1(
        filenames: list[str], analysis: Analysis, outfile: TextIO, lines: bool
) -> None:
    write_header(__file__, filenames, outfile)
    outfile.write(
        """
#ifndef Py_TAIL_CALL_INTERP
    #error "This file is for tail-calling interpreter only."
#endif
#define TIER_ONE 1
"""
    )
    out = CWriter(outfile, 0, lines)
    out.emit("static py_tail_call_funcptr INSTRUCTION_TABLE[256];\n");
    emitter = Emitter(out)
    out.emit("\n")
    for name, inst in sorted(analysis.instructions.items()):
        out.emit("\n")
        out.emit(f"""
#ifdef LLTRACE
__attribute__((preserve_none)) PyObject *
_TAIL_CALL_{name}(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer,
                 PyThreadState *tstate, _Py_CODEUNIT *next_instr, int oparg, _PyInterpreterFrame* entry_frame, int lltrace)
#else
__attribute__((preserve_none)) PyObject *
_TAIL_CALL_{name}(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer,
                 PyThreadState *tstate, _Py_CODEUNIT *next_instr, int oparg, _PyInterpreterFrame* entry_frame)
#endif
""")
        out.emit("{\n")
        # out.emit(f'printf("{name}\\n");\n')
        out.emit("int opcode = next_instr->op.code;\n")
        out.emit("{\n")
        write_single_inst(out, emitter, name, inst)
        if not inst.parts[-1].properties.always_exits:
            out.emit("DISPATCH();\n")
        out.emit("""
pop_4_error:
    STACK_SHRINK(1);
pop_3_error:
    STACK_SHRINK(1);
pop_2_error:
    STACK_SHRINK(1);
pop_1_error:
    STACK_SHRINK(1);
error:
        /* Double-check exception status. */
#ifdef NDEBUG
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_SystemError,
                             "error return without exception set");
        }
#else
        assert(_PyErr_Occurred(tstate));
#endif

        /* Log traceback info. */
        assert(frame != entry_frame);
        if (!_PyFrame_IsIncomplete(frame)) {
            PyFrameObject *f = _PyFrame_GetFrameObject(frame);
            if (f != NULL) {
                PyTraceBack_Here(f);
            }
        }
        _PyEval_MonitorRaise(tstate, frame, next_instr-1);
exception_unwind:
        {
            /* We can't use frame->instr_ptr here, as RERAISE may have set it */
            int offset = INSTR_OFFSET()-1;
            int level, handler, lasti;
            if (get_exception_handler(_PyFrame_GetCode(frame), offset, &level, &handler, &lasti) == 0) {
                // No handlers, so exit.
                assert(_PyErr_Occurred(tstate));

                /* Pop remaining stack entries. */
                _PyStackRef *stackbase = _PyFrame_Stackbase(frame);
                while (stack_pointer > stackbase) {
                    PyStackRef_XCLOSE(POP());
                }
                assert(STACK_LEVEL() == 0);
                _PyFrame_SetStackPointer(frame, stack_pointer);
                monitor_unwind(tstate, frame, next_instr-1);
                goto exit_unwind;
            }

            assert(STACK_LEVEL() >= level);
            _PyStackRef *new_top = _PyFrame_Stackbase(frame) + level;
            while (stack_pointer > new_top) {
                PyStackRef_XCLOSE(POP());
            }
            if (lasti) {
                int frame_lasti = _PyInterpreterFrame_LASTI(frame);
                PyObject *lasti = PyLong_FromLong(frame_lasti);
                if (lasti == NULL) {
                    goto exception_unwind;
                }
                PUSH(PyStackRef_FromPyObjectSteal(lasti));
            }

            /* Make the raw exception data
                available to the handler,
                so a program can emulate the
                Python main loop. */
            PyObject *exc = _PyErr_GetRaisedException(tstate);
            PUSH(PyStackRef_FromPyObjectSteal(exc));
            next_instr = _PyFrame_GetBytecode(frame) + handler;

            if (monitor_handled(tstate, frame, next_instr, exc) < 0) {
                goto exception_unwind;
            }
            /* Resume normal execution */
#ifdef LLTRACE
            if (lltrace >= 5) {
                lltrace_resume_frame(frame);
            }
#endif
            DISPATCH();
        }
    }

exit_unwind:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame != entry_frame);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    _PyEval_FrameClearAndPop(tstate, dying);
    frame->return_offset = 0;
    if (frame == entry_frame) {
        /* Restore previous frame and exit */
        tstate->current_frame = frame->previous;
        tstate->c_recursion_remaining += PY_EVAL_C_STACK_UNITS;
        return NULL;
    }

resume_with_error:
    next_instr = frame->instr_ptr;
    stack_pointer = _PyFrame_GetStackPointer(frame);
    goto error;
        
""")
        out.start_line()
        out.emit("}\n")
    out.emit("static py_tail_call_funcptr INSTRUCTION_TABLE[256] = {\n")
    for name in sorted(analysis.instructions.keys()):
        out.emit(f"[{name}] = _TAIL_CALL_{name},\n")
    out.emit("};\n")
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
