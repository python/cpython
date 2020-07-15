// types.Union -- used to represent e.g. Union[int, str], int | str
#include "Python.h"
#include "pycore_object.h"
#include "structmember.h"

typedef struct {
    PyObject_HEAD;
    PyObject *args;
} unionobject;

static void
unionobject_dealloc(PyObject *self)
{
    unionobject *alias = (unionobject *)self;

    _PyObject_GC_UNTRACK(self);
    // Py_XDECREF(alias->origin);
    Py_XDECREF(alias->args);
    self->ob_type->tp_free(self);
}



// static PyObject *
// unionobject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
// {
//     printf("Hello.");
//     PyObject *origin = PyTuple_GET_ITEM(args, 0);
//     PyObject *arguments = PyTuple_GET_ITEM(args, 1);
//     return Py_Union(origin, arguments);
// }

PyTypeObject Py_UnionType = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "typing.Union",
    .tp_doc = "Represent a PEP 604 union type\n"
              "\n"
              "E.g. for int | str",
    .tp_basicsize = sizeof(unionobject),
    .tp_dealloc = unionobject_dealloc,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    // .tp_new = unionobject_new
};

PyObject *
Py_Union(PyObject *args)
{
    if (!PyTuple_Check(args)) {
        args = PyTuple_Pack(1, args);
        if (args == NULL) {
            return NULL;
        }
    }
    else {
        Py_INCREF(args);
    }

    printf("Here, creating a new union");
    unionobject *alias = PyObject_GC_New(unionobject, &Py_UnionType);
    if (alias == NULL) {
        Py_DECREF(args);
        return NULL;
    }

    PyObject_Print(args, stdout, 0);
    // alias->origin = NULL;
    // alias->params = NULL;
    alias->args = args;
    _PyObject_GC_TRACK(alias);
    return (PyObject *) alias;
    // return alias;
}
