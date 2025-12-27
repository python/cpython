#ifndef Py_BUILD_CORE
#define Py_BUILD_CORE
#endif
#ifndef Py_BUILD_CORE_MODULE
#define Py_BUILD_CORE_MODULE
#endif

#include "../../Python/ceval.h"

#include "../../Python/ceval_macros.h"

int Test_EvalFrame_Resumes, Test_EvalFrame_Loads;

#if _Py_TAIL_CALL_INTERP
#include "test_targets.h"
#include "test_cases.c.h"
#endif

PyObject* _Py_HOT_FUNCTION
Test_EvalFrame(PyThreadState *tstate, _PyInterpreterFrame *frame, int throwflag)
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
    _PyEntryFrame entry;

    if (_Py_EnterRecursiveCallTstate(tstate, "")) {
        assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
        _PyEval_FrameClearAndPop(tstate, frame);
        return NULL;
    }

    /* Local "register" variables.
     * These are cached values from the frame and code object.  */
    _Py_CODEUNIT *next_instr;
    _PyStackRef *stack_pointer;
    entry.stack[0] = PyStackRef_NULL;
#ifdef Py_STACKREF_DEBUG
    entry.frame.f_funcobj = PyStackRef_None;
#elif defined(Py_DEBUG)
    /* Set these to invalid but identifiable values for debugging. */
    entry.frame.f_funcobj = (_PyStackRef){.bits = 0xaaa0};
    entry.frame.f_locals = (PyObject*)0xaaa1;
    entry.frame.frame_obj = (PyFrameObject*)0xaaa2;
    entry.frame.f_globals = (PyObject*)0xaaa3;
    entry.frame.f_builtins = (PyObject*)0xaaa4;
#endif
    entry.frame.f_executable = PyStackRef_None;
    entry.frame.instr_ptr = (_Py_CODEUNIT *)_Py_INTERPRETER_TRAMPOLINE_INSTRUCTIONS_PTR + 1;
    entry.frame.stackpointer = entry.stack;
    entry.frame.owner = FRAME_OWNED_BY_INTERPRETER;
    entry.frame.visited = 0;
    entry.frame.return_offset = 0;
#ifdef Py_DEBUG
    entry.frame.lltrace = 0;
#endif
    /* Push frame */
    entry.frame.previous = tstate->current_frame;
    frame->previous = &entry.frame;
    tstate->current_frame = frame;
    entry.frame.localsplus[0] = PyStackRef_NULL;
#ifdef _Py_TIER2
    if (tstate->current_executor != NULL) {
        entry.frame.localsplus[0] = PyStackRef_FromPyObjectNew(tstate->current_executor);
        tstate->current_executor = NULL;
    }
#endif

    /* support for generator.throw() */
    if (throwflag) {
        if (_Py_EnterRecursivePy(tstate)) {
            goto early_exit;
        }
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
#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_error(frame, stack_pointer, tstate, next_instr, instruction_funcptr_handler_table, 0, lastopcode);
#   else
        return _TAIL_CALL_error(frame, stack_pointer, tstate, next_instr, instruction_funcptr_handler_table, 0);
#   endif
#else
        goto error;
#endif
    }

    #if defined(_Py_TIER2) && !defined(_Py_JIT)
    /* Tier 2 interpreter state */
    _PyExecutorObject *current_executor = NULL;
    const _PyUOpInstruction *next_uop = NULL;
#endif
#if _Py_TAIL_CALL_INTERP
#   if Py_STATS
        return _TAIL_CALL_start_frame(frame, NULL, tstate, NULL, instruction_funcptr_handler_table, 0, lastopcode);
#   else
        return _TAIL_CALL_start_frame(frame, NULL, tstate, NULL, instruction_funcptr_handler_table, 0);
#   endif
#else
    goto start_frame;
#include "test_cases.c.h"
#endif


#ifdef _Py_TIER2

// Tier 2 is also here!
enter_tier_two:

#ifdef _Py_JIT
    assert(0);
#else

#undef LOAD_IP
#define LOAD_IP(UNUSED) (void)0

#ifdef Py_STATS
// Disable these macros that apply to Tier 1 stats when we are in Tier 2
#undef STAT_INC
#define STAT_INC(opname, name) ((void)0)
#undef STAT_DEC
#define STAT_DEC(opname, name) ((void)0)
#endif

#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0
#undef ENABLE_SPECIALIZATION_FT
#define ENABLE_SPECIALIZATION_FT 0

    ; // dummy statement after a label, before a declaration
    uint16_t uopcode;
#ifdef Py_STATS
    int lastuop = 0;
    uint64_t trace_uop_execution_counter = 0;
#endif

    assert(next_uop->opcode == _START_EXECUTOR);
tier2_dispatch:
    for (;;) {
        uopcode = next_uop->opcode;
#ifdef Py_DEBUG
        if (frame->lltrace >= 3) {
            dump_stack(frame, stack_pointer);
            if (next_uop->opcode == _START_EXECUTOR) {
                printf("%4d uop: ", 0);
            }
            else {
                printf("%4d uop: ", (int)(next_uop - current_executor->trace));
            }
            _PyUOpPrint(next_uop);
            printf("\n");
        }
#endif
        next_uop++;
        OPT_STAT_INC(uops_executed);
        UOP_STAT_INC(uopcode, execution_count);
        UOP_PAIR_INC(uopcode, lastuop);
#ifdef Py_STATS
        trace_uop_execution_counter++;
        ((_PyUOpInstruction  *)next_uop)[-1].execution_count++;
#endif

        switch (uopcode) {

#include "executor_cases.c.h"

            default:
#ifdef Py_DEBUG
            {
                printf("Unknown uop: ");
                _PyUOpPrint(&next_uop[-1]);
                printf(" @ %d\n", (int)(next_uop - current_executor->trace - 1));
                Py_FatalError("Unknown uop");
            }
#else
            Py_UNREACHABLE();
#endif

        }
    }

jump_to_error_target:
#ifdef Py_DEBUG
    if (frame->lltrace >= 2) {
        printf("Error: [UOp ");
        _PyUOpPrint(&next_uop[-1]);
        printf(" @ %d -> %s]\n",
               (int)(next_uop - current_executor->trace - 1),
               _PyOpcode_OpName[frame->instr_ptr->op.code]);
    }
#endif
    assert(next_uop[-1].format == UOP_FORMAT_JUMP);
    uint16_t target = uop_get_error_target(&next_uop[-1]);
    next_uop = current_executor->trace + target;
    goto tier2_dispatch;

jump_to_jump_target:
    assert(next_uop[-1].format == UOP_FORMAT_JUMP);
    target = uop_get_jump_target(&next_uop[-1]);
    next_uop = current_executor->trace + target;
    goto tier2_dispatch;

#endif  // _Py_JIT

#endif // _Py_TIER2

early_exit:
    assert(_PyErr_Occurred(tstate));
    _Py_LeaveRecursiveCallPy(tstate);
    assert(frame->owner != FRAME_OWNED_BY_INTERPRETER);
    // GH-99729: We need to unlink the frame *before* clearing it:
    _PyInterpreterFrame *dying = frame;
    frame = tstate->current_frame = dying->previous;
    _PyEval_FrameClearAndPop(tstate, dying);
    frame->return_offset = 0;
    assert(frame->owner == FRAME_OWNED_BY_INTERPRETER);
    /* Restore previous frame and exit */
    tstate->current_frame = frame->previous;
    return NULL;
}
