#ifndef Py_TESTLIMITEDCAPI_PARTS_H
#define Py_TESTLIMITEDCAPI_PARTS_H

// Always enable assertions
#undef NDEBUG

#include "pyconfig.h"   // Py_GIL_DISABLED

// Use the limited C API
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030c0000  // 3.12
#endif

// Make sure that the internal C API cannot be used.
#undef Py_BUILD_CORE_MODULE
#undef Py_BUILD_CORE_BUILTIN

#include "Python.h"

#ifdef Py_BUILD_CORE
#  error "Py_BUILD_CORE macro must not be defined"
#endif

int _PyTestCapi_Init_VectorcallLimited(PyObject *module);
int _PyTestCapi_Init_HeaptypeRelative(PyObject *module);

#endif // Py_TESTLIMITEDCAPI_PARTS_H
