/* Interpolation object implementation */
#include "Python.h"
#include <stddef.h>

#include "pycore_initconfig.h"  // _PyStatus_OK
#include "pycore_typeobject.h"  // _PyStaticType_InitBuiltin
#include "pycore_stackref.h"    // _PyStackRef
#include "pycore_object.h"      // _PyObject_GC_TRACK

#include "pycore_template.h"

typedef struct {
    PyObject_HEAD
    PyObject *args;
} templateobject;

static templateobject *
template_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    templateobject *self = (templateobject *) type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }

    static char *kwlist[] = {"args", NULL};
    PyObject *selfargs;

    if (PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist,
                                    &selfargs) < 0) {
        Py_DECREF(self);
        return NULL;
    }

    Py_XSETREF(self->args, Py_NewRef(selfargs));
    return self;
}

static void
template_dealloc(templateobject *self)
{
    Py_CLEAR(self->args);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
template_repr(templateobject *self)
{
    return PyUnicode_FromFormat("%s(%R)",
                                _PyType_Name(Py_TYPE(self)),
                                self->args);
}

static PyMemberDef template_members[] = {
    {"args", Py_T_OBJECT_EX, offsetof(templateobject, args), Py_READONLY, "Args"},
    {NULL}
};

PyTypeObject _PyTemplate_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "templatelib.Template",
    .tp_doc = PyDoc_STR("Template object"),
    .tp_basicsize = sizeof(templateobject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | _Py_TPFLAGS_MATCH_SELF,
    .tp_new = (newfunc) template_new,
    .tp_dealloc = (destructor) template_dealloc,
    .tp_repr = (reprfunc) template_repr,
    .tp_members = template_members,
};

PyObject *
_PyTemplate_Create(PyObject **values, Py_ssize_t oparg)
{
    PyObject *tuple = PyTuple_New(oparg);
    if (!tuple) {
        return NULL;
    }

    for (Py_ssize_t i = 0; i < oparg; i++) {
        PyTuple_SET_ITEM(tuple, i, Py_NewRef(values[i]));
    }

    PyObject *template = PyObject_CallOneArg((PyObject *) &_PyTemplate_Type, tuple);
    Py_DECREF(tuple);
    return template;
}
