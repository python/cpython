#include "Python.h"

/* Always enable assertions */
#undef NDEBUG

int _PyTestCapi_Init_Vectorcall(PyObject *module);
int _PyTestCapi_Init_VectorcallLimited(PyObject *module);
int _PyTestCapi_Init_Heaptype(PyObject *module);
int _PyTestCapi_Init_Unicode(PyObject *module);
