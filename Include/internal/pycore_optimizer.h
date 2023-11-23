#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_uops.h"          // _PyUopInstruction

int _Py_uop_analyze_and_optimize(PyCodeObject *code,
    _PyUopInstruction *trace, int trace_len, int curr_stackentries);

extern PyTypeObject _PyCounterExecutor_Type;
extern PyTypeObject _PyCounterOptimizer_Type;
extern PyTypeObject _PyDefaultOptimizer_Type;
extern PyTypeObject _PyUopExecutor_Type;
extern PyTypeObject _PyUopOptimizer_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
