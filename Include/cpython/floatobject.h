#ifndef Py_CPYTHON_FLOATOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;

// Macro version of PyFloat_AsDouble() trading safety for speed.
// It doesn't check if op is a double object.
#define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)
