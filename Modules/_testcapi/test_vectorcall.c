#include "testcapimodule_parts.h"
#include <stddef.h>  // offsetof


/* Test PEP 590 - Vectorcall */

static int
fastcall_args(PyObject *args, PyObject ***stack, Py_ssize_t *nargs)
{
    if (args == Py_None) {
        *stack = NULL;
        *nargs = 0;
    }
    else if (PyTuple_Check(args)) {
        *stack = ((PyTupleObject *)args)->ob_item;
        *nargs = PyTuple_GET_SIZE(args);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "args must be None or a tuple");
        return -1;
    }
    return 0;
}


static PyObject *
test_pyobject_fastcall(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args;
    PyObject **stack;
    Py_ssize_t nargs;

    if (!PyArg_ParseTuple(args, "OO", &func, &func_args)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }
    return _PyObject_FastCall(func, stack, nargs);
}

static PyObject *
test_pyobject_fastcalldict(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args, *kwargs;
    PyObject **stack;
    Py_ssize_t nargs;

    if (!PyArg_ParseTuple(args, "OOO", &func, &func_args, &kwargs)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }

    if (kwargs == Py_None) {
        kwargs = NULL;
    }
    else if (!PyDict_Check(kwargs)) {
        PyErr_SetString(PyExc_TypeError, "kwnames must be None or a dict");
        return NULL;
    }

    return PyObject_VectorcallDict(func, stack, nargs, kwargs);
}

static PyObject *
test_pyobject_vectorcall(PyObject *self, PyObject *args)
{
    PyObject *func, *func_args, *kwnames = NULL;
    PyObject **stack;
    Py_ssize_t nargs, nkw;

    if (!PyArg_ParseTuple(args, "OOO", &func, &func_args, &kwnames)) {
        return NULL;
    }

    if (fastcall_args(func_args, &stack, &nargs) < 0) {
        return NULL;
    }

    if (kwnames == Py_None) {
        kwnames = NULL;
    }
    else if (PyTuple_Check(kwnames)) {
        nkw = PyTuple_GET_SIZE(kwnames);
        if (nargs < nkw) {
            PyErr_SetString(PyExc_ValueError, "kwnames longer than args");
            return NULL;
        }
        nargs -= nkw;
    }
    else {
        PyErr_SetString(PyExc_TypeError, "kwnames must be None or a tuple");
        return NULL;
    }
    return PyObject_Vectorcall(func, stack, nargs, kwnames);
}

static PyObject *
test_pyvectorcall_call(PyObject *self, PyObject *args)
{
    PyObject *func;
    PyObject *argstuple;
    PyObject *kwargs = NULL;

    if (!PyArg_ParseTuple(args, "OO|O", &func, &argstuple, &kwargs)) {
        return NULL;
    }

    if (!PyTuple_Check(argstuple)) {
        PyErr_SetString(PyExc_TypeError, "args must be a tuple");
        return NULL;
    }
    if (kwargs != NULL && !PyDict_Check(kwargs)) {
        PyErr_SetString(PyExc_TypeError, "kwargs must be a dict");
        return NULL;
    }

    return PyVectorcall_Call(func, argstuple, kwargs);
}

static PyMethodDef TestMethods[] = {
    {"pyobject_fastcall", test_pyobject_fastcall, METH_VARARGS},
    {"pyobject_fastcalldict", test_pyobject_fastcalldict, METH_VARARGS},
    {"pyobject_vectorcall", test_pyobject_vectorcall, METH_VARARGS},
    {"pyvectorcall_call", test_pyvectorcall_call, METH_VARARGS},
    {NULL},
};


typedef struct {
    PyObject_HEAD
    vectorcallfunc vectorcall;
} MethodDescriptorObject;

static PyObject *
MethodDescriptor_vectorcall(PyObject *callable, PyObject *const *args,
                            size_t nargsf, PyObject *kwnames)
{
    /* True if using the vectorcall function in MethodDescriptorObject
     * but False for MethodDescriptor2Object */
    MethodDescriptorObject *md = (MethodDescriptorObject *)callable;
    return PyBool_FromLong(md->vectorcall != NULL);
}

static PyObject *
MethodDescriptor_new(PyTypeObject* type, PyObject* args, PyObject *kw)
{
    MethodDescriptorObject *op = (MethodDescriptorObject *)type->tp_alloc(type, 0);
    op->vectorcall = MethodDescriptor_vectorcall;
    return (PyObject *)op;
}

static PyObject *
func_descr_get(PyObject *func, PyObject *obj, PyObject *type)
{
    if (obj == Py_None || obj == NULL) {
        Py_INCREF(func);
        return func;
    }
    return PyMethod_New(func, obj);
}

static PyObject *
nop_descr_get(PyObject *func, PyObject *obj, PyObject *type)
{
    Py_INCREF(func);
    return func;
}

static PyObject *
call_return_args(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_INCREF(args);
    return args;
}

static PyTypeObject MethodDescriptorBase_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethodDescriptorBase",
    sizeof(MethodDescriptorObject),
    .tp_new = MethodDescriptor_new,
    .tp_call = PyVectorcall_Call,
    .tp_vectorcall_offset = offsetof(MethodDescriptorObject, vectorcall),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
                Py_TPFLAGS_METHOD_DESCRIPTOR | Py_TPFLAGS_HAVE_VECTORCALL,
    .tp_descr_get = func_descr_get,
};

static PyTypeObject MethodDescriptorDerived_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethodDescriptorDerived",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
};

static PyTypeObject MethodDescriptorNopGet_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethodDescriptorNopGet",
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_call = call_return_args,
    .tp_descr_get = nop_descr_get,
};

typedef struct {
    MethodDescriptorObject base;
    vectorcallfunc vectorcall;
} MethodDescriptor2Object;

static PyObject *
MethodDescriptor2_new(PyTypeObject* type, PyObject* args, PyObject *kw)
{
    MethodDescriptor2Object *op = PyObject_New(MethodDescriptor2Object, type);
    op->base.vectorcall = NULL;
    op->vectorcall = MethodDescriptor_vectorcall;
    return (PyObject *)op;
}

static PyTypeObject MethodDescriptor2_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethodDescriptor2",
    sizeof(MethodDescriptor2Object),
    .tp_new = MethodDescriptor2_new,
    .tp_call = PyVectorcall_Call,
    .tp_vectorcall_offset = offsetof(MethodDescriptor2Object, vectorcall),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_VECTORCALL,
};


int
_PyTestCapi_Init_Vectorcall(PyObject *m) {
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    if (PyType_Ready(&MethodDescriptorBase_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, &MethodDescriptorBase_Type) < 0) {
        return -1;
    }

    MethodDescriptorDerived_Type.tp_base = &MethodDescriptorBase_Type;
    if (PyType_Ready(&MethodDescriptorDerived_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, &MethodDescriptorDerived_Type) < 0) {
        return -1;
    }

    MethodDescriptorNopGet_Type.tp_base = &MethodDescriptorBase_Type;
    if (PyType_Ready(&MethodDescriptorNopGet_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, &MethodDescriptorNopGet_Type) < 0) {
        return -1;
    }

    MethodDescriptor2_Type.tp_base = &MethodDescriptorBase_Type;
    if (PyType_Ready(&MethodDescriptor2_Type) < 0) {
        return -1;
    }
    if (PyModule_AddType(m, &MethodDescriptor2_Type) < 0) {
        return -1;
    }

    return 0;
}
