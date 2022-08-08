#define Py_LIMITED_API 0x030c0000 // 3.12
#include "parts.h"
#include "structmember.h"         // PyMemberDef

/* Test Vectorcall in the limited API */

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
LimitedVectorCallClass_new(PyTypeObject *tp, PyTypeObject *a, PyTypeObject *kw)
{
    PyObject *self = ((allocfunc)PyType_GetSlot(tp, Py_tp_alloc))(tp, 0);
    if (!self) {
        return NULL;
    }
    *(vectorcallfunc*)((char*)self + sizeof(PyObject)) = (
        LimitedVectorCallClass_vectorcall);
    return self;
}

static PyMemberDef LimitedVectorCallClass_members[] = {
    {"__vectorcalloffset__", T_PYSSIZET, sizeof(PyObject), READONLY},
    {NULL}
};

static PyType_Slot LimitedVectorallClass_slots[] = {
    {Py_tp_new, LimitedVectorCallClass_new},
    {Py_tp_call, LimitedVectorCallClass_tpcall},
    {Py_tp_members, LimitedVectorCallClass_members},
    {0},
};

static PyType_Spec LimitedVectorCallClass_spec = {
    .name = "_testcapi.LimitedVectorCallClass",
    .basicsize = (int)(sizeof(PyObject) + sizeof(vectorcallfunc)),
    .flags = Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_HAVE_VECTORCALL
        | Py_TPFLAGS_BASETYPE,
    .slots = LimitedVectorallClass_slots,
};

static PyMethodDef TestMethods[] = {
    /* Add module methods here.
     * (Empty list left here as template/example, since using
     * PyModule_AddFunctions isn't very common.)
     */
    {NULL},
};

int
_PyTestCapi_Init_VectorcallLimited(PyObject *m) {
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

    return 0;
}
