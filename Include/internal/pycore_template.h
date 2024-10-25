#ifndef Py_INTERNAL_TEMPLATE_H
#define Py_INTERNAL_TEMPLATE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_stackref.h"    // _PyStackRef

extern PyTypeObject _PyTemplate_Type;

PyAPI_FUNC(PyObject *) _PyTemplate_FromValues(PyObject **values, Py_ssize_t n);
PyAPI_FUNC(PyObject *) _PyTemplate_FromListStackRef(_PyStackRef ref);

#ifdef __cplusplus
}
#endif

#endif
