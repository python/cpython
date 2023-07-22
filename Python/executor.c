#include "Python.h"

#include "pycore_call.h"
#include "pycore_ceval.h"
#include "pycore_dict.h"
#include "pycore_emscripten_signal.h"
#include "pycore_intrinsics.h"
#include "pycore_long.h"
#include "pycore_object.h"
#include "pycore_opcode_metadata.h"
#include "pycore_opcode_utils.h"
#include "pycore_pyerrors.h"
#include "pycore_range.h"
#include "pycore_setobject.h"     // _PySet_Update()
#include "pycore_sliceobject.h"
#include "pycore_uops.h"

#include "ceval_macros.h"


#undef ASSERT_KWNAMES_IS_NULL
#define ASSERT_KWNAMES_IS_NULL() (void)0

#undef DEOPT_IF
#define DEOPT_IF(COND, INSTNAME) \
    if ((COND)) {                \
        goto deoptimize;         \
    }

#undef ENABLE_SPECIALIZATION
#define ENABLE_SPECIALIZATION 0


_PyInterpreterFrame *
_PyUopExecute(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer)
{
#ifdef Py_DEBUG
    char *uop_debug = Py_GETENV("PYTHONUOPSDEBUG");
    int lltrace = 0;
    if (uop_debug != NULL && *uop_debug >= '0') {
        lltrace = *uop_debug - '0';  // TODO: Parse an int and all that
    }
    #define DPRINTF(level, ...) \
        if (lltrace >= (level)) { fprintf(stderr, __VA_ARGS__); }
#else
    #define DPRINTF(level, ...)
#endif

    DPRINTF(3,
            "Entering _PyUopExecute for %s (%s:%d) at byte offset %ld\n",
            PyUnicode_AsUTF8(_PyFrame_GetCode(frame)->co_qualname),
            PyUnicode_AsUTF8(_PyFrame_GetCode(frame)->co_filename),
            _PyFrame_GetCode(frame)->co_firstlineno,
            2 * (long)(frame->prev_instr + 1 -
                   (_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive));

    PyThreadState *tstate = _PyThreadState_GET();
    _PyUOpExecutorObject *self = (_PyUOpExecutorObject *)executor;

    CHECK_EVAL_BREAKER();

    OBJECT_STAT_INC(optimization_traces_executed);
    _Py_CODEUNIT *ip_offset = (_Py_CODEUNIT *)_PyFrame_GetCode(frame)->co_code_adaptive;
    int pc = 0;
    int opcode;
    int oparg;
    uint64_t operand;

    for (;;) {
        opcode = self->trace[pc].opcode;
        oparg = self->trace[pc].oparg;
        operand = self->trace[pc].operand;
        DPRINTF(3,
                "%4d: uop %s, oparg %d, operand %" PRIu64 ", stack_level %d\n",
                pc,
                opcode < 256 ? _PyOpcode_OpName[opcode] : _PyOpcode_uop_name[opcode],
                oparg,
                operand,
                (int)(stack_pointer - _PyFrame_Stackbase(frame)));
        pc++;
        OBJECT_STAT_INC(optimization_uops_executed);
        switch (opcode) {

#include "executor_cases.c.h"

            default:
            {
                fprintf(stderr, "Unknown uop %d, operand %" PRIu64 "\n", opcode, operand);
                Py_FatalError("Unknown uop");
            }

        }
    }

unbound_local_error:
    _PyEval_FormatExcCheckArg(tstate, PyExc_UnboundLocalError,
        UNBOUNDLOCAL_ERROR_MSG,
        PyTuple_GetItem(_PyFrame_GetCode(frame)->co_localsplusnames, oparg)
    );
    goto error;

pop_4_error:
    STACK_SHRINK(1);
pop_3_error:
    STACK_SHRINK(1);
pop_2_error:
    STACK_SHRINK(1);
pop_1_error:
    STACK_SHRINK(1);
error:
    // On ERROR_IF we return NULL as the frame.
    // The caller recovers the frame from cframe.current_frame.
    DPRINTF(2, "Error: [Opcode %d, operand %" PRIu64 "]\n", opcode, operand);
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return NULL;

deoptimize:
    // On DEOPT_IF we just repeat the last instruction.
    // This presumes nothing was popped from the stack (nor pushed).
    DPRINTF(2, "DEOPT: [Opcode %d, operand %" PRIu64 "]\n", opcode, operand);
    frame->prev_instr--;  // Back up to just before destination
    _PyFrame_SetStackPointer(frame, stack_pointer);
    Py_DECREF(self);
    return frame;
}
