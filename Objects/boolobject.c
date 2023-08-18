#include "Python.h"
#include "pycore_long.h"
#include "pycore_modsupport.h"
#include "pycore_object.h"
#include "pycore_runtime.h"

static PyObject *const Py_TrueObj = &Py_TrueStruct;
static PyObject *const Py_FalseObj = &Py_FalseStruct;

static PyObject *bool_repr(PyObject *self) {
    return self == Py_TrueObj ? &_Py_ID(True) : &_Py_ID(False);
}

PyObject *PyBool_FromLong(long ok) {
    return ok ? Py_TrueObj : Py_FalseObj;
}

static PyObject *bool_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    PyObject *x = Py_FalseObj;
    long ok;

    if (!_PyArg_NoKeywords("bool", kwds))
        return NULL;

    if (!PyArg_UnpackTuple(args, "bool", 0, 1, &x))
        return NULL;

    ok = PyObject_IsTrue(x);
    if (ok < 0)
        return NULL;

    return PyBool_FromLong(ok);
}

static PyObject *bool_vectorcall(PyObject *type, PyObject * const*args,
                                 size_t nargsf, PyObject *kwnames) {
    long ok = 0;

    if (!_PyArg_NoKwnames("bool", kwnames)) {
        return NULL;
    }

    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    if (!_PyArg_CheckPositional("bool", nargs, 0, 1)) {
        return NULL;
    }

    if (nargs) {
        ok = PyObject_IsTrue(args[0]);
        if (ok < 0) {
            return NULL;
        }
    }

    return PyBool_FromLong(ok);
}

static PyObject *bool_invert(PyObject *v) {
    return PyLong_Type.tp_as_number->nb_invert(v);
}

static PyObject *bool_and(PyObject *a, PyObject *b) {
    return PyBool_FromLong((a == Py_TrueObj) & (b == Py_TrueObj));
}

static PyObject *bool_or(PyObject *a, PyObject *b) {
    return PyBool_FromLong((a == Py_TrueObj) | (b == Py_TrueObj));
}

static PyObject *bool_xor(PyObject *a, PyObject *b) {
    return PyBool_FromLong((a == Py_TrueObj) ^ (b == Py_TrueObj));
}

PyTypeObject PyBool_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bool",
    offsetof(struct _longobject, long_value.ob_digit),
    sizeof(digit),
    bool_dealloc,
    0,
    0,
    bool_repr,
    &bool_as_number,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    &PyLong_Type,
    0,
    0,
    0,
    0,
    0,
    0,
    bool_new,
    .tp_vectorcall = bool_vectorcall,
};

struct _longobject _Py_FalseStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    { .lv_tag = _PyLong_FALSE_TAG, { 0 } }
};

struct _longobject _Py_TrueStruct = {
    PyObject_HEAD_INIT(&PyBool_Type)
    { .lv_tag = _PyLong_TRUE_TAG, { 1 } }
};

PyDoc_STRVAR(bool_doc,
"bool(x) -> bool\n\
\n\
Returns True when the argument x is true, False otherwise.\n\
The builtins True and False are the only two instances of the class bool.\n\
The class bool is a subclass of the class int, and cannot be subclassed.");

// More code can follow, including module initialization and other functions.
