#ifndef Py_BUILD_CORE
#define Py_BUILD_CORE
#endif
#ifndef Py_BUILD_CORE_MODULE
#define Py_BUILD_CORE_MODULE
#endif

#include "../../Python/ceval.h"

#include "../../Python/ceval_macros.h"

#undef IS_PEP523_HOOKED
#define IS_PEP523_HOOKED(tstate) (tstate->interp->eval_frame != NULL && !tstate->interp->eval_frame_allow_specialization)

int Test_EvalFrame_Resumes, Test_EvalFrame_Loads;

#ifdef _Py_TIER2
static int
stop_tracing_and_jit(PyThreadState *tstate, _PyControlRecord *frame)
{
    (void)(frame);
    // Don't actually JIT-compile in this test eval-frame, but we still must
    // finalize the tracer so the thread-global is_tracing flag is reset.
    // Otherwise a trace started inside this duplicated interpreter loop
    // (reachable under low JIT thresholds, e.g. PYTHON_JIT_STRESS=1) would
    // leave is_tracing stuck true and permanently disable the JIT.
    _PyJit_FinalizeTracing(tstate, 0);
    return 0;
}
#endif

#if _Py_TAIL_CALL_INTERP
#include "test_targets.h"
#include "test_cases.c.h"
#endif

PyObject* _Py_HOT_FUNCTION
Test_EvalFrame(PyThreadState *tstate, _PyControlRecord *frame, int throwflag)
{
    _Py_EnsureTstateNotNULL(tstate);
    check_invalid_reentrancy();
    CALL_STAT_INC(pyeval_calls);

#if USE_COMPUTED_GOTOS && !_Py_TAIL_CALL_INTERP
/* Import the static jump table */
#include "test_targets.h"
    void **opcode_targets = opcode_targets_table;
#endif

#ifdef Py_STATS
    int lastopcode = 0;
#endif
#if !_Py_TAIL_CALL_INTERP
    uint8_t opcode;        /* Current opcode */
    int oparg;         /* Current opcode argument, if any */
    assert(tstate->current_frame == NULL || tstate->current_frame->stackpointer != NULL);
#if !USE_COMPUTED_GOTOS
    uint8_t tracing_mode = 0;
    uint8_t dispatch_code;
#endif
#endif
    _PyInterpreterDataFrame *fp = frame->data;

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame, fp);
        return NULL;
    }

    /* Local "register" variables.
     * These are cached values from the frame and code object.  */
    _Py_CODEUNIT *next_instr;
    _PyStackRef *stack_pointer;
    char c_frame;
    frame->c_frame = &c_frame;
    frame->state = CONTROL_NATIVE;
    tstate->current_frame = frame;

    /* support for generator.throw() */
    if (throwflag) {
#ifdef Py_GIL_DISABLED
        /* Load thread-local bytecode */
        if (frame->tlbc_index != ((_PyThreadStateImpl *)tstate)->tlbc_index) {
            _Py_CODEUNIT *bytecode =
                _PyEval_GetExecutableCode(tstate, _PyFrame_GetCode(frame));
            if (bytecode == NULL) {
                goto early_exit;
            }
            ptrdiff_t off = frame->instr_ptr - _PyFrame_GetBytecode(frame);
            frame->tlbc_index = ((_PyThreadStateImpl *)tstate)->tlbc_index;
            frame->instr_ptr = bytecode + off;
        }
#endif
        /* Because this avoids the RESUME, we need to update instrumentation */
        _Py_Instrument(_PyFrame_GetCode(frame), tstate->interp);
        next_instr = frame->instr_ptr;
        monitor_throw(tstate, frame, next_instr);
        stack_pointer = _PyFrame_GetStackPointer(frame);
        _PyFrame_StackPointerInvalidate(frame);
#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_error(frame, fp, stack_pointer, tstate, next_instr, instruction_funcptr_handler_table, 0, lastopcode);
#   else
        return _TAIL_CALL_error(frame, fp, stack_pointer, tstate, next_instr, instruction_funcptr_handler_table, 0);
#   endif
#else
        goto error;
#endif
    }

#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_start_frame(frame, fp, NULL, tstate, NULL, instruction_funcptr_handler_table, 0, lastopcode);
#   else
        return _TAIL_CALL_start_frame(frame, fp, NULL, tstate, NULL, instruction_funcptr_handler_table, 0);
#   endif
#else
    goto start_frame;
#include "test_cases.c.h"
#endif


early_exit:
    assert(_PyErr_Occurred(tstate));
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyControlRecord *dying = frame;
    frame = tstate->current_frame = _PyFrame_GetPrevious(dying);
    _PyEval_FrameClearAndPop(tstate, dying, fp);
    return NULL;
}
