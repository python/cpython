/* Cell object implementation */

#include "Python.h"
#include "pycore_cell.h"          // PyCell_GetRef()
#include "pycore_object.h"

#include "clinic/cellobject.c.h"

#define _PyCell_CAST(op) _Py_CAST(PyCellObject*, (op))

/*[clinic input]
class cell "PyCellObject *" "&PyCell_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3d6baba8cf810af0]*/

PyObject *
PyCell_New(PyObject *obj)
{
    PyCellObject *op;

    op = (PyCellObject *)PyObject_GC_New(PyCellObject, &PyCell_Type);
    if (op == NULL)
        return NULL;
    op->ob_ref = Py_XNewRef(obj);

    _PyObject_GC_TRACK(op);
    return (PyObject *)op;
}

/*[clinic input]
@vectorcall
@classmethod
@text_signature "([contents])"
cell.__new__ as cell_new

    contents: object = NULL
    /

Create a new cell object.

  contents
    the contents of the cell. If not specified, the cell will be empty,
    and further attempts to access its cell_contents attribute will
    raise a ValueError.
[clinic start generated code]*/

static PyObject *
cell_new_impl(PyTypeObject *type, PyObject *contents)
/*[clinic end generated code: output=0b7db264f0621bb4 input=4460339a82243798]*/
{
    return PyCell_New(contents);
}

PyObject *
PyCell_Get(PyObject *op)
{
    if (!PyCell_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyCell_GetRef((PyCellObject *)op);
}

int
PyCell_Set(PyObject *op, PyObject *value)
{
    if (!PyCell_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    PyCell_SetTakeRef((PyCellObject *)op, Py_XNewRef(value));
    return 0;
}

static void
cell_dealloc(PyObject *self)
{
    PyCellObject *op = _PyCell_CAST(self);
    _PyObject_GC_UNTRACK(op);
    Py_XDECREF(op->ob_ref);
    PyObject_GC_Del(op);
}

static PyObject *
cell_compare_impl(PyObject *a, PyObject *b, int op)
{
    if (a != NULL && b != NULL) {
        return PyObject_RichCompare(a, b, op);
    }
    else {
        Py_RETURN_RICHCOMPARE(b == NULL, a == NULL, op);
    }
}

static PyObject *
cell_richcompare(PyObject *a, PyObject *b, int op)
{
    /* neither argument should be NULL, unless something's gone wrong */
    assert(a != NULL && b != NULL);

    /* both arguments should be instances of PyCellObject */
    if (!PyCell_Check(a) || !PyCell_Check(b)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    PyObject *a_ref = PyCell_GetRef((PyCellObject *)a);
    PyObject *b_ref = PyCell_GetRef((PyCellObject *)b);

    /* compare cells by contents; empty cells come before anything else */
    PyObject *res = cell_compare_impl(a_ref, b_ref, op);

    Py_XDECREF(a_ref);
    Py_XDECREF(b_ref);
    return res;
}

static PyObject *
cell_repr(PyObject *self)
{
    PyObject *ref = PyCell_GetRef((PyCellObject *)self);
    if (ref == NULL) {
        return PyUnicode_FromFormat("<cell at %p: empty>", self);
    }
    PyObject *res = PyUnicode_FromFormat("<cell at %p: %.80s object at %p>",
                                         self, Py_TYPE(ref)->tp_name, ref);
    Py_DECREF(ref);
    return res;
}

static int
cell_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyCellObject *op = _PyCell_CAST(self);
    Py_VISIT(op->ob_ref);
    return 0;
}

static int
cell_clear(PyObject *self)
{
    PyCellObject *op = _PyCell_CAST(self);
    Py_CLEAR(op->ob_ref);
    return 0;
}

static PyObject *
cell_get_contents(PyObject *self, void *closure)
{
    PyCellObject *op = _PyCell_CAST(self);
    PyObject *res = PyCell_GetRef(op);
    if (res == NULL) {
        PyErr_SetString(PyExc_ValueError, "Cell is empty");
        return NULL;
    }
    return res;
}

static int
cell_set_contents(PyObject *self, PyObject *obj, void *Py_UNUSED(ignored))
{
    PyCellObject *cell = _PyCell_CAST(self);
    Py_XINCREF(obj);
    PyCell_SetTakeRef((PyCellObject *)cell, obj);
    return 0;
}

static PyGetSetDef cell_getsetlist[] = {
    {"cell_contents", cell_get_contents, cell_set_contents, NULL},
    {NULL} /* sentinel */
};

PyTypeObject PyCell_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "cell",
    sizeof(PyCellObject),
    0,
    cell_dealloc,                               /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    cell_repr,                                  /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,    /* tp_flags */
    cell_new__doc__,                            /* tp_doc */
    cell_traverse,                              /* tp_traverse */
    cell_clear,                                 /* tp_clear */
    cell_richcompare,                           /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    cell_getsetlist,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    cell_new,                                   /* tp_new */
    0,                                          /* tp_free */
    .tp_vectorcall = cell_vectorcall,
};
