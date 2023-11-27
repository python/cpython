#ifndef Py_TESTCAPI_PARTS_H
#define Py_TESTCAPI_PARTS_H

#include "pyconfig.h"  // for Py_TRACE_REFS

// Figure out if Limited API is available for this build. If it isn't we won't
// build tests for it.
// Currently, only Py_TRACE_REFS disables Limited API.
#ifdef Py_TRACE_REFS
#undef LIMITED_API_AVAILABLE
#else
#define LIMITED_API_AVAILABLE 1
#endif

// Always enable assertions
#undef NDEBUG

#if !defined(LIMITED_API_AVAILABLE) && defined(Py_LIMITED_API)
// Limited API being unavailable means that with Py_LIMITED_API defined
// we can't even include Python.h.
// Do nothing; the .c file that defined Py_LIMITED_API should also do nothing.

#else

#include "Python.h"

int _PyTestCapi_Init_Vectorcall(PyObject *module);
int _PyTestCapi_Init_Heaptype(PyObject *module);
int _PyTestCapi_Init_Abstract(PyObject *module);
int _PyTestCapi_Init_ByteArray(PyObject *module);
int _PyTestCapi_Init_Bytes(PyObject *module);
int _PyTestCapi_Init_Unicode(PyObject *module);
int _PyTestCapi_Init_GetArgs(PyObject *module);
int _PyTestCapi_Init_PyTime(PyObject *module);
int _PyTestCapi_Init_DateTime(PyObject *module);
int _PyTestCapi_Init_Docstring(PyObject *module);
int _PyTestCapi_Init_Mem(PyObject *module);
int _PyTestCapi_Init_Watchers(PyObject *module);
int _PyTestCapi_Init_Long(PyObject *module);
int _PyTestCapi_Init_Float(PyObject *module);
int _PyTestCapi_Init_Complex(PyObject *module);
int _PyTestCapi_Init_Numbers(PyObject *module);
int _PyTestCapi_Init_Dict(PyObject *module);
int _PyTestCapi_Init_Set(PyObject *module);
int _PyTestCapi_Init_List(PyObject *module);
int _PyTestCapi_Init_Tuple(PyObject *module);
int _PyTestCapi_Init_Structmember(PyObject *module);
int _PyTestCapi_Init_Exceptions(PyObject *module);
int _PyTestCapi_Init_Code(PyObject *module);
int _PyTestCapi_Init_Buffer(PyObject *module);
int _PyTestCapi_Init_PyOS(PyObject *module);
int _PyTestCapi_Init_File(PyObject *module);
int _PyTestCapi_Init_Codec(PyObject *module);
int _PyTestCapi_Init_Immortal(PyObject *module);
int _PyTestCapi_Init_GC(PyObject *module);
int _PyTestCapi_Init_Sys(PyObject *module);

#ifdef LIMITED_API_AVAILABLE
int _PyTestCapi_Init_VectorcallLimited(PyObject *module);
int _PyTestCapi_Init_HeaptypeRelative(PyObject *module);
#endif // LIMITED_API_AVAILABLE

#endif
#endif // Py_TESTCAPI_PARTS_H
