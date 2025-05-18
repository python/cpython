/* Test Vectorcall in the limited API */

// Need limited C API version 3.12 for PyObject_Vectorcall()
#include "pyconfig.h"   // Py_GIL_DISABLED
#if !defined(Py_GIL_DISABLED) && !defined(Py_LIMITED_API)
#  define Py_LIMITED_API 0x030c0000
#endif

#include <stddef.h>                         // offsetof

#include "parts.h"
#include "clinic/vectorcall_limited.c.h"

/*[clinic input]
module _testlimitedcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2700057f9c1135ba]*/

static PyObject *
LimitedVectorCallClass_tpcall(PyObject *self, PyObject *args, PyObject *kwargs) {
    return PyUnicode_FromString("tp_call called");
}

static PyObject *
LimitedVectorCallClass_vectorcall(PyObject *callable,
                            PyObject *const *args,
                            size_t nargsf,
                            PyObject *kwnames) {
    return PyUnicode_FromString("vectorcall called");
}

static PyObject *
LimitedVectorCallClass_new(PyTypeObject *tp, PyObject *a, PyObject *kw)
{
    PyObject *self = ((allocfunc)PyType_GetSlot(tp, Py_tp_alloc))(tp, 0);
    if (!self) {
        return NULL;
    }
    *(vectorcallfunc*)((char*)self + sizeof(PyObject)) = (
        LimitedVectorCallClass_vectorcall);
    return self;
}

/*[clinic input]
_testlimitedcapi.call_vectorcall

    callable: object
    /
[clinic start generated code]*/

static PyObject *
_testlimitedcapi_call_vectorcall(PyObject *module, PyObject *callable)
/*[clinic end generated code: output=9cbb7832263a8eef input=0743636c12dccb28]*/
{
    PyObject *args[3] = { NULL, NULL, NULL };
    PyObject *kwname = NULL, *kwnames = NULL, *result = NULL;

    args[1] = PyUnicode_FromString("foo");
    if (!args[1]) {
        goto leave;
    }

    args[2] = PyUnicode_FromString("bar");
    if (!args[2]) {
        goto leave;
    }

    kwname = PyUnicode_InternFromString("baz");
    if (!kwname) {
        goto leave;
    }

    kwnames = PyTuple_New(1);
    if (!kwnames) {
        goto leave;
    }

    if (PyTuple_SetItem(kwnames, 0, kwname)) {
        goto leave;
    }

    result = PyObject_Vectorcall(
        callable,
        args + 1,
        1 | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames
    );

leave:
    Py_XDECREF(args[1]);
    Py_XDECREF(args[2]);
    Py_XDECREF(kwnames);

    return result;
}

/*[clinic input]
_testlimitedcapi.call_vectorcall_method

    callable: object
    /
[clinic start generated code]*/

static PyObject *
_testlimitedcapi_call_vectorcall_method(PyObject *module, PyObject *callable)
/*[clinic end generated code: output=4558323a46cc09eb input=a736f7dbf15f1be5]*/
{
    PyObject *args[3] = { NULL, NULL, NULL };
    PyObject *name = NULL, *kwname = NULL,
             *kwnames = NULL, *result = NULL;

    name = PyUnicode_FromString("f");
    if (!name) {
        goto leave;
    }

    args[0] = callable;
    args[1] = PyUnicode_FromString("foo");
    if (!args[1]) {
        goto leave;
    }

    args[2] = PyUnicode_FromString("bar");
    if (!args[2]) {
        goto leave;
    }

    kwname = PyUnicode_InternFromString("baz");
    if (!kwname) {
        goto leave;
    }

    kwnames = PyTuple_New(1);
    if (!kwnames) {
        goto leave;
    }

    if (PyTuple_SetItem(kwnames, 0, kwname)) {
        goto leave;
    }


    result = PyObject_VectorcallMethod(
        name,
        args,
        2 | PY_VECTORCALL_ARGUMENTS_OFFSET,
        kwnames
    );

leave:
    Py_XDECREF(name);
    Py_XDECREF(args[1]);
    Py_XDECREF(args[2]);
    Py_XDECREF(kwnames);

    return result;
}

static PyMemberDef LimitedVectorCallClass_members[] = {
    {"__vectorcalloffset__", Py_T_PYSSIZET, sizeof(PyObject), Py_READONLY},
    {NULL}
};

static PyType_Slot LimitedVectorallClass_slots[] = {
    {Py_tp_new, LimitedVectorCallClass_new},
    {Py_tp_call, LimitedVectorCallClass_tpcall},
    {Py_tp_members, LimitedVectorCallClass_members},
    {0},
};

static PyType_Spec LimitedVectorCallClass_spec = {
    .name = "_testlimitedcapi.LimitedVectorCallClass",
    .basicsize = (int)(sizeof(PyObject) + sizeof(vectorcallfunc)),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_HAVE_VECTORCALL
        | Py_TPFLAGS_BASETYPE,
    .slots = LimitedVectorallClass_slots,
};

typedef struct {
    vectorcallfunc vfunc;
} LimitedRelativeVectorCallStruct;

static PyObject *
LimitedRelativeVectorCallClass_new(PyTypeObject *tp, PyObject *a, PyObject *kw)
{
    PyObject *self = ((allocfunc)PyType_GetSlot(tp, Py_tp_alloc))(tp, 0);
    if (!self) {
        return NULL;
    }
    LimitedRelativeVectorCallStruct *data = PyObject_GetTypeData(self, tp);
    data->vfunc = LimitedVectorCallClass_vectorcall;
    return self;
}


static PyType_Spec LimitedRelativeVectorCallClass_spec = {
    .name = "_testlimitedcapi.LimitedRelativeVectorCallClass",
    .basicsize = -(int)sizeof(LimitedRelativeVectorCallStruct),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_HAVE_VECTORCALL,
    .slots = (PyType_Slot[]) {
        {Py_tp_new, LimitedRelativeVectorCallClass_new},
        {Py_tp_call, LimitedVectorCallClass_tpcall},
        {Py_tp_members, (PyMemberDef[]){
            {"__vectorcalloffset__", Py_T_PYSSIZET,
             offsetof(LimitedRelativeVectorCallStruct, vfunc),
             Py_READONLY | Py_RELATIVE_OFFSET},
            {NULL}
        }},
        {0}
    },
};

static PyMethodDef TestMethods[] = {
    _TESTLIMITEDCAPI_CALL_VECTORCALL_METHODDEF
    _TESTLIMITEDCAPI_CALL_VECTORCALL_METHOD_METHODDEF
    {NULL},
};

int
_PyTestLimitedCAPI_Init_VectorcallLimited(PyObject *m)
{
    if (PyModule_AddFunctions(m, TestMethods) < 0) {
        return -1;
    }

    PyObject *LimitedVectorCallClass = PyType_FromModuleAndSpec(
        m, &LimitedVectorCallClass_spec, NULL);
    if (!LimitedVectorCallClass) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)LimitedVectorCallClass) < 0) {
        return -1;
    }
    Py_DECREF(LimitedVectorCallClass);

    PyObject *LimitedRelativeVectorCallClass = PyType_FromModuleAndSpec(
        m, &LimitedRelativeVectorCallClass_spec, NULL);
    if (!LimitedRelativeVectorCallClass) {
        return -1;
    }
    if (PyModule_AddType(m, (PyTypeObject *)LimitedRelativeVectorCallClass) < 0) {
        return -1;
    }
    Py_DECREF(LimitedRelativeVectorCallClass);

    return 0;
}
