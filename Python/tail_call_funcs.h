
#define TARGET(X) uintptr_t FUNC_##X(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer, _Py_CODEUNIT *next_instr, int opcode, int oparg, ret_state *state)

const uintptr_t pop_1_error = 1;
const uintptr_t pop_2_error = 2;
const uintptr_t pop_3_error = 3;
const uintptr_t pop_4_error = 4;
const uintptr_t error = 5;
const uintptr_t exception_unwind = 6;
const uintptr_t start_frame = 7;
const uintptr_t resume_frame = 8;

#define STORE_RET_STATE {\
    state->tstate = tstate;\
    state->frame = frame;\
    state->stack_pointer = stack_pointer;\
    state->next_instr = next_instr;\
    state->opcode = opcode;\
    state->oparg = oparg;\
}

#define RESTORE_RET_STATE {\
    tstate = s.tstate;\
    frame = s.frame;\
    stack_pointer = s.stack_pointer;\
    next_instr = s.next_instr;\
    opcode = s.opcode;\
    oparg = s.oparg;\
}

#define CEVAL_GOTO(X) do { STORE_RET_STATE return (uintptr_t)X; } while(0)

#include "generated_cases.c.h"

uintptr_t FUNC_unknown_opcode(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer, _Py_CODEUNIT *next_instr, int opcode, int oparg, ret_state *state) {
    opcode = next_instr->op.code;
    _PyErr_Format(tstate, PyExc_SystemError,
                    "%U:%d: unknown opcode %d",
                    _PyFrame_GetCode(frame)->co_filename,
                    PyUnstable_InterpreterFrame_GetLine(frame),
                    opcode);
    CEVAL_GOTO(error);
}

uintptr_t FUNC_INSTRUMENTED_LINE(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer, _Py_CODEUNIT *next_instr, int opcode, int oparg, ret_state *state) {
    _Py_CODEUNIT *prev = frame->instr_ptr;
    _Py_CODEUNIT *here = frame->instr_ptr = next_instr;
    int original_opcode = 0;
    if (tstate->tracing) {
        PyCodeObject *code = _PyFrame_GetCode(frame);
        original_opcode = code->_co_monitoring->lines[(int)(here - _PyCode_CODE(code))].original_opcode;
    } else {
        _PyFrame_SetStackPointer(frame, stack_pointer);
        original_opcode = _Py_call_instrumentation_line(
                tstate, frame, here, prev);
        stack_pointer = _PyFrame_GetStackPointer(frame);
        if (original_opcode < 0) {
            next_instr = here+1;
            CEVAL_GOTO(error);
        }
        next_instr = frame->instr_ptr;
        if (next_instr != here) {
            DISPATCH();
        }
    }
    if (_PyOpcode_Caches[original_opcode]) {
        _PyBinaryOpCache *cache = (_PyBinaryOpCache *)(next_instr+1);
        /* Prevent the underlying instruction from specializing
            * and overwriting the instrumentation. */
        PAUSE_ADAPTIVE_COUNTER(cache->counter);
    }
    opcode = original_opcode;
    DISPATCH_GOTO();
}

#undef goto
#undef return
#undef TARGET

#undef DISPATCH_GOTO
#define DISPATCH_GOTO() OPCODE_FUNC_PTR_TYPE next = opcode_funcs[opcode]; \
                        uintptr_t res = next(tstate, frame, stack_pointer, next_instr, opcode, oparg, &s); \
                        RESTORE_RET_STATE;\
                        switch(res) {\
                        case pop_1_error: goto pop_1_error;\
                        case pop_2_error: goto pop_2_error;\
                        case pop_3_error: goto pop_3_error;\
                        case pop_4_error: goto pop_4_error;\
                        case error: goto error;\
                        case exception_unwind: goto exception_unwind;\
                        case start_frame: goto start_frame;\
                        case resume_frame: goto resume_frame;\
                        default: return (PyObject*)res;\
                        }
