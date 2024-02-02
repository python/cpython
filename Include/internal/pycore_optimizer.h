#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_uop_ids.h"

#define TRACE_STACK_SIZE 5

int _Py_uop_analyze_and_optimize(_PyInterpreterFrame *frame,
    _PyUOpInstruction *trace, int trace_len, int curr_stackentries,
    _PyBloomFilter *dependencies);


static void
clear_strong_refs_in_uops(_PyUOpInstruction *trace, Py_ssize_t uop_len)
{
    for (Py_ssize_t i = 0; i < uop_len; i++) {
        if (trace[i].opcode == _LOAD_CONST_INLINE ||
            trace[i].opcode == _LOAD_CONST_INLINE_WITH_NULL) {
            PyObject *c = (PyObject*)trace[i].operand;
            Py_CLEAR(c);
        }
        if (trace[i].opcode == _JUMP_ABSOLUTE ||
            trace[i].opcode == _JUMP_TO_TOP ||
            trace[i].opcode == _EXIT_TRACE) {
            return;
        }
    }
}


extern PyTypeObject _PyCounterExecutor_Type;
extern PyTypeObject _PyCounterOptimizer_Type;
extern PyTypeObject _PyDefaultOptimizer_Type;
extern PyTypeObject _PyUOpExecutor_Type;
extern PyTypeObject _PyUOpOptimizer_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
