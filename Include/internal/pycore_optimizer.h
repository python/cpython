#ifndef Py_INTERNAL_OPTIMIZER_H
#define Py_INTERNAL_OPTIMIZER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_uops.h"          // _PyUOpInstruction

int _Py_uop_analyze_and_optimize(PyCodeObject *code,
    _PyUOpInstruction *trace, int trace_len, int curr_stackentries);

extern PyTypeObject _PyCounterExecutor_Type;
#define _PyCounterExecutor_Check(op) Py_IS_TYPE((op), &_PyCounterExecutor_Type)
extern PyTypeObject _PyCounterOptimizer_Type;
#define _PyCounterOptimizer_Check(op) Py_IS_TYPE((op), &_PyCounterOptimizer_Type)
extern PyTypeObject _PyDefaultOptimizer_Type;
#define _PyDefaultOptimizer_Check(op) Py_IS_TYPE((op), &_PyDefaultOptimizer_Type)
extern PyTypeObject _PyUOpExecutor_Type;
#define _PyUOpExecutor_Check(op) Py_IS_TYPE((op), &_PyUOpExecutor_Type)
extern PyTypeObject _PyUOpOptimizer_Type;
#define _PyUOpOptimizer_Check(op) Py_IS_TYPE((op), &_PyUOpOptimizer_Type)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OPTIMIZER_H */
