#ifndef Py_CPYTHON_FLOATOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;

// Static inline version of PyFloat_AsDouble() trading safety for speed.
// It only checks if op is a Python float object in debug mode.
static inline double _PyFloat_AS_DOUBLE(PyFloatObject *op) {
    assert(PyFloat_Check(op));
    return op->ob_fval;
}
#define PyFloat_AS_DOUBLE(op) _PyFloat_AS_DOUBLE((PyFloatObject *)(op))
