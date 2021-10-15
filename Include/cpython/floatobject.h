#ifndef Py_CPYTHON_FLOATOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;

// Macro version of PyFloat_AsDouble() trading safety for speed.
// It only checks if op is a double object in debug mode.
#define _PyFloat_CAST(op) (assert(PyFloat_Check(op)), (PyFloatObject *)(op))
#define PyFloat_AS_DOUBLE(op) _Py_RVALUE(_PyFloat_CAST(op)->ob_fval)
