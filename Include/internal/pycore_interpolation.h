#ifndef Py_INTERNAL_INTERPOLATION_H
#define Py_INTERNAL_INTERPOLATION_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_stackref.h"    // _PyStackRef

extern int _conversion_converter(PyObject *arg, PyObject **Py_UNUSED(unused));

extern PyTypeObject _PyInterpolation_Type;

PyAPI_FUNC(PyObject *) _PyInterpolation_FromStackRefSteal(_PyStackRef *values);

extern PyStatus _PyInterpolation_InitTypes(PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif

#endif
