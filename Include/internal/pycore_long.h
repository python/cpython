#ifndef Py_INTERNAL_LONG_H
#define Py_INTERNAL_LONG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_global_objects.h"  // _PY_NSMALLNEGINTS
#include "pycore_runtime.h"       // _PyRuntime


/* runtime lifecycle */

extern PyStatus _PyLong_InitTypes(PyInterpreterState *);
extern void _PyLong_FiniTypes(PyInterpreterState *interp);


/* other API */

#define _PyLong_SMALL_INTS _Py_SINGLETON(small_ints)

// _PyLong_GetZero() and _PyLong_GetOne() must always be available
#if _PY_NSMALLPOSINTS < 2
#  error "_PY_NSMALLPOSINTS must be greater than 1"
#endif

// Return a borrowed reference to the zero singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetZero(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS]; }

// Return a borrowed reference to the one singleton.
// The function cannot return NULL.
static inline PyObject* _PyLong_GetOne(void)
{ return (PyObject *)&_PyLong_SMALL_INTS[_PY_NSMALLNEGINTS+1]; }

PyObject *_PyLong_Add(PyLongObject *left, PyLongObject *right);
PyObject *_PyLong_Multiply(PyLongObject *left, PyLongObject *right);
PyObject *_PyLong_Subtract(PyLongObject *left, PyLongObject *right);

/* Used by Python/mystrtoul.c, _PyBytes_FromHex(),
   _PyBytes_DecodeEscape(), etc. */
PyAPI_DATA(unsigned char) _PyLong_DigitValue[256];

/* Format the object based on the format_spec, as defined in PEP 3101
   (Advanced String Formatting). */
PyAPI_FUNC(int) _PyLong_FormatAdvancedWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    PyObject *format_spec,
    Py_ssize_t start,
    Py_ssize_t end);

PyAPI_FUNC(int) _PyLong_FormatWriter(
    _PyUnicodeWriter *writer,
    PyObject *obj,
    int base,
    int alternate);

PyAPI_FUNC(char*) _PyLong_FormatBytesWriter(
    _PyBytesWriter *writer,
    char *str,
    PyObject *obj,
    int base,
    int alternate);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LONG_H */
