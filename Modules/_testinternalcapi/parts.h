#ifndef Py_TESTINTERNALCAPI_PARTS_H
#define Py_TESTINTERNALCAPI_PARTS_H

// Always enable assertions
#undef NDEBUG

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

int _PyTestInternalCapi_Init_Lock(PyObject *module);
int _PyTestInternalCapi_Init_PyTime(PyObject *module);
int _PyTestInternalCapi_Init_Set(PyObject *module);
int _PyTestInternalCapi_Init_Complex(PyObject *module);
int _PyTestInternalCapi_Init_CriticalSection(PyObject *module);

#endif // Py_TESTINTERNALCAPI_PARTS_H
