#ifndef Py_TESTLIMITEDCAPI_PARTS_H
#define Py_TESTLIMITEDCAPI_PARTS_H

// Always enable assertions
#undef NDEBUG

#include "pyconfig.h"   // Py_GIL_DISABLED

// Use the limited C API
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
   // need limited C API version 3.5 for PyModule_AddFunctions()
#  define Py_LIMITED_API 0x03050000
#endif

// Make sure that the internal C API cannot be used.
#undef Py_BUILD_CORE_MODULE
#undef Py_BUILD_CORE_BUILTIN

#include "Python.h"

#ifdef Py_BUILD_CORE
#  error "Py_BUILD_CORE macro must not be defined"
#endif

int _PyTestLimitedCAPI_Init_Abstract(PyObject *module);
int _PyTestLimitedCAPI_Init_ByteArray(PyObject *module);
int _PyTestLimitedCAPI_Init_Bytes(PyObject *module);
int _PyTestLimitedCAPI_Init_Complex(PyObject *module);
int _PyTestLimitedCAPI_Init_Dict(PyObject *module);
int _PyTestLimitedCAPI_Init_Eval(PyObject *module);
int _PyTestLimitedCAPI_Init_Float(PyObject *module);
int _PyTestLimitedCAPI_Init_HeaptypeRelative(PyObject *module);
int _PyTestLimitedCAPI_Init_Import(PyObject *module);
int _PyTestLimitedCAPI_Init_Object(PyObject *module);
int _PyTestLimitedCAPI_Init_List(PyObject *module);
int _PyTestLimitedCAPI_Init_Long(PyObject *module);
int _PyTestLimitedCAPI_Init_PyOS(PyObject *module);
int _PyTestLimitedCAPI_Init_Set(PyObject *module);
int _PyTestLimitedCAPI_Init_Sys(PyObject *module);
int _PyTestLimitedCAPI_Init_Tuple(PyObject *module);
int _PyTestLimitedCAPI_Init_Unicode(PyObject *module);
int _PyTestLimitedCAPI_Init_VectorcallLimited(PyObject *module);
int _PyTestLimitedCAPI_Init_File(PyObject *module);

#endif // Py_TESTLIMITEDCAPI_PARTS_H
