#include "parts.h"
#include "clinic/vectorcall.c.h"

#include "structmember.h"           // PyMemberDef
#include <stddef.h>                 // offsetof


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
override_vectorcall(PyObject *callable, PyObject *const *args, size_t nargsf,
                    PyObject *kwnames)
{
    return PyUnicode_FromString("overridden");
}

static PyObject *
function_setvectorcall(PyObject *self, PyObject *func)
{
    if (!PyFunction_Check(func)) {
        PyErr_SetString(PyExc_TypeError, "'func' must be a function");
        return NULL;
    }
    PyFunction_SetVectorcall((PyFunctionObject *)func, (vectorcallfunc)override_vectorcall);
    Py_RETURN_NONE;
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

PyObject *
VectorCallClass_tpcall(PyObject *self, PyObject *args, PyObject *kwargs) {
    return PyUnicode_FromString("tp_call");
}

PyObject *
VectorCallClass_vectorcall(PyObject *callable,
                            PyObject *const *args,
                            size_t nargsf,
                            PyObject *kwnames) {
    return PyUnicode_FromString("vectorcall");
}

/*[clinic input]
module _testcapi
class _testcapi.VectorCallClass "PyObject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8423a8e919f2f0df]*/

/*[clinic input]
_testcapi.VectorCallClass.set_vectorcall

    type: object(subclass_of="&PyType_Type", type="PyTypeObject *")
    /

Set self's vectorcall function for `type` to one that returns "vectorcall"
[clinic start generated code]*/

static PyObject *
_testcapi_VectorCallClass_set_vectorcall_impl(PyObject *self,
                                              PyTypeObject *type)
/*[clinic end generated code: output=b37f0466f15da903 input=840de66182c7d71a]*/
{
    if (!PyObject_TypeCheck(self, type)) {
        return PyErr_Format(
            PyExc_TypeError,
            "expected %s instance",
            PyType_GetName(type));
    }
    if (!type->tp_vectorcall_offset) {
        return PyErr_Format(
            PyExc_TypeError,
            "type %s has no vectorcall offset",
            PyType_GetName(type));
    }
    *(vectorcallfunc*)((char*)self + type->tp_vectorcall_offset) = (
        VectorCallClass_vectorcall);
    Py_RETURN_NONE;
}

PyMethodDef VectorCallClass_methods[] = {
    _TESTCAPI_VECTORCALLCLASS_SET_VECTORCALL_METHODDEF
    {NULL, NULL}
};

PyMemberDef VectorCallClass_members[] = {
    {"__vectorcalloffset__", T_PYSSIZET, 0/* set later */, READONLY},
    {NULL}
};

PyType_Slot VectorCallClass_slots[] = {
    {Py_tp_call, VectorCallClass_tpcall},
    {Py_tp_members, VectorCallClass_members},
    {Py_tp_methods, VectorCallClass_methods},
    {0},
};

/*[clinic input]
_testcapi.make_vectorcall_class

    base: object(subclass_of="&PyType_Type", type="PyTypeObject *") = NULL
    /

Create a class whose instances return "tpcall" when called.

When the "set_vectorcall" method is called on an instance, a vectorcall
function that returns "vectorcall" will be installed.
[clinic start generated code]*/

static PyObject *
_testcapi_make_vectorcall_class_impl(PyObject *module, PyTypeObject *base)
/*[clinic end generated code: output=16dcfc3062ddf968 input=f72e01ccf52de2b4]*/
{
    if (!base) {
        base = (PyTypeObject *)&PyBaseObject_Type;
    }
    VectorCallClass_members[0].offset = base->tp_basicsize;
    PyType_Spec spec = {
        .name = "_testcapi.VectorcallClass",
        .basicsize = (int)(base->tp_basicsize + sizeof(vectorcallfunc)),
        .flags = Py_TPFLAGS_DEFAULT
            | Py_TPFLAGS_HAVE_VECTORCALL
            | Py_TPFLAGS_BASETYPE,
        .slots = VectorCallClass_slots,
    };

    return PyType_FromSpecWithBases(&spec, (PyObject *)base);
}

/*[clinic input]
_testcapi.has_vectorcall_flag -> bool

    type: object(subclass_of="&PyType_Type", type="PyTypeObject *")
    /

Return true iff Py_TPFLAGS_HAVE_VECTORCALL is set on the class.
[clinic start generated code]*/

static int
_testcapi_has_vectorcall_flag_impl(PyObject *module, PyTypeObject *type)
/*[clinic end generated code: output=3ae8d1374388c671 input=8eee492ac548749e]*/
{
    return PyType_HasFeature(type, Py_TPFLAGS_HAVE_VECTORCALL);
}

static PyMethodDef TestMethods[] = {
    {"pyobject_fastcall", test_pyobject_fastcall, METH_VARARGS},
    {"pyobject_fastcalldict", test_pyobject_fastcalldict, METH_VARARGS},
    {"pyobject_vectorcall", test_pyobject_vectorcall, METH_VARARGS},
    {"function_setvectorcall", function_setvectorcall, METH_O},
    {"pyvectorcall_call", test_pyvectorcall_call, METH_VARARGS},
    _TESTCAPI_MAKE_VECTORCALL_CLASS_METHODDEF
    _TESTCAPI_HAS_VECTORCALL_FLAG_METHODDEF
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
        return Py_NewRef(func);
    }
    return PyMethod_New(func, obj);
}

static PyObject *
nop_descr_get(PyObject *func, PyObject *obj, PyObject *type)
{
    return Py_NewRef(func);
}

static PyObject *
call_return_args(PyObject *self, PyObject *args, PyObject *kwargs)
{
    return Py_NewRef(args);
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
