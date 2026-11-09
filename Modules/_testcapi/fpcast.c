/*
 * Tests for Python/wasm_trampoline.c
 *
 * Third party extensions frequently cast functions with the "wrong" number of
 * arguments to PyCFunction, getter, setter or ternaryfunc. On native targets
 * this works by accident. On WebAssembly, call_indirect checks the signature
 * and traps, so CPython routes these calls through a trampoline that detects
 * the real signature.
 */

#include "parts.h"

static PyObject *
zero(void)
{
    Py_RETURN_NONE;
}

static PyObject *
one(PyObject *self)
{
    Py_RETURN_NONE;
}

static PyObject *
two(PyObject *self, PyObject *args)
{
    Py_RETURN_NONE;
}

static PyObject *
three(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_RETURN_NONE;
}

// Using this as a handler should raise a SystemError.
static PyObject *
four(PyObject *self, PyObject *a, PyObject *b, PyObject *c)
{
    Py_RETURN_NONE;
}

static int
set_two(PyObject *self, PyObject *value)
{
    return 0;
}

static PyObject *last_set_value = NULL;

static int
set_three(PyObject *self, PyObject *value, void *closure)
{
    Py_XSETREF(last_set_value, Py_XNewRef(value));
    return 0;
}

static PyObject *
get_last_set_value(PyObject *self, PyObject *Py_UNUSED(args))
{
    if (last_set_value == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(last_set_value);
}

#define FPCAST_METHODS(prefix)                                              \
    {prefix "noargs0", _PyCFunction_CAST(zero), METH_NOARGS},               \
    {prefix "noargs1", _PyCFunction_CAST(one), METH_NOARGS},                \
    {prefix "noargs2", _PyCFunction_CAST(two), METH_NOARGS},                \
    {prefix "noargs3", _PyCFunction_CAST(three), METH_NOARGS},              \
    {prefix "noargs4", _PyCFunction_CAST(four), METH_NOARGS},               \
                                                                            \
    {prefix "o0", _PyCFunction_CAST(zero), METH_O},                         \
    {prefix "o1", _PyCFunction_CAST(one), METH_O},                          \
    {prefix "o2", _PyCFunction_CAST(two), METH_O},                          \
    {prefix "o3", _PyCFunction_CAST(three), METH_O},                        \
                                                                            \
    {prefix "varargs0", _PyCFunction_CAST(zero), METH_VARARGS},             \
    {prefix "varargs1", _PyCFunction_CAST(one), METH_VARARGS},              \
    {prefix "varargs2", _PyCFunction_CAST(two), METH_VARARGS},              \
    {prefix "varargs3", _PyCFunction_CAST(three), METH_VARARGS},            \
                                                                            \
    {prefix "kwargs0", _PyCFunction_CAST(zero),                             \
     METH_VARARGS | METH_KEYWORDS},                                         \
    {prefix "kwargs1", _PyCFunction_CAST(one),                              \
     METH_VARARGS | METH_KEYWORDS},                                         \
    {prefix "kwargs2", _PyCFunction_CAST(two),                              \
     METH_VARARGS | METH_KEYWORDS},                                         \
    {prefix "kwargs3", _PyCFunction_CAST(three),                            \
     METH_VARARGS | METH_KEYWORDS}

static PyMethodDef test_methods[] = {
    FPCAST_METHODS("fpcast_"),
    {"fpcast_last_set_value", get_last_set_value, METH_NOARGS},
    {NULL},
};

static PyMethodDef type_methods[] = {
    FPCAST_METHODS(""),
    {NULL},
};

static PyGetSetDef type_getset[] = {
    {"getset0", .get = _Py_FUNC_CAST(getter, zero)},
    {"getset1", .get = _Py_FUNC_CAST(getter, one),
                .set = _Py_FUNC_CAST(setter, set_two)},
    {"getset2", .get = _Py_FUNC_CAST(getter, two),
                .set = _Py_FUNC_CAST(setter, set_three)},
    {"getset4", .get = _Py_FUNC_CAST(getter, four),
                .set = _Py_FUNC_CAST(setter, four)},
    {NULL},
};

static PyTypeObject FpcastTestType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_testcapi.FpcastTestType",
    .tp_basicsize = sizeof(PyObject),
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = type_methods,
    .tp_getset = type_getset,
    .tp_new = PyType_GenericNew,
};

#define CALLABLE_TYPE(N, FUNC)                                  \
    static PyTypeObject FpcastCallable##N = {                   \
        PyVarObject_HEAD_INIT(NULL, 0)                          \
        .tp_name = "_testcapi.FpcastCallable" #N,               \
        .tp_basicsize = sizeof(PyObject),                       \
        .tp_flags = Py_TPFLAGS_DEFAULT,                         \
        .tp_call = _Py_FUNC_CAST(ternaryfunc, FUNC),            \
        .tp_new = PyType_GenericNew,                            \
    };

CALLABLE_TYPE(0, zero)
CALLABLE_TYPE(1, one)
CALLABLE_TYPE(2, two)
CALLABLE_TYPE(3, three)
CALLABLE_TYPE(4, four)

#undef CALLABLE_TYPE

int
_PyTestCapi_Init_Fpcast(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    PyTypeObject *types[] = {
        &FpcastTestType,
        &FpcastCallable0,
        &FpcastCallable1,
        &FpcastCallable2,
        &FpcastCallable3,
        &FpcastCallable4,
    };
    for (size_t i = 0; i < Py_ARRAY_LENGTH(types); i++) {
        if (PyModule_AddType(mod, types[i]) < 0) {
            return -1;
        }
    }
    return 0;
}
