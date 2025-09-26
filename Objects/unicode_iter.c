/* Unicode Iterator */

#include "Python.h"
#include "pycore_ceval.h"         // _PyEval_GetBuiltin()
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_unicodeobject.h" // _PyUnicode_CHECK()


typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyObject *it_seq;    /* Set to NULL when iterator is exhausted */
} unicodeiterobject;

static void
unicodeiter_dealloc(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    _PyObject_GC_UNTRACK(it);
    Py_XDECREF(it->it_seq);
    PyObject_GC_Del(it);
}

static int
unicodeiter_traverse(PyObject *op, visitproc visit, void *arg)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_VISIT(it->it_seq);
    return 0;
}

static PyObject *
unicodeiter_next(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    PyObject *seq;

    assert(it != NULL);
    seq = it->it_seq;
    if (seq == NULL)
        return NULL;
    assert(_PyUnicode_CHECK(seq));

    if (it->it_index < PyUnicode_GET_LENGTH(seq)) {
        int kind = PyUnicode_KIND(seq);
        const void *data = PyUnicode_DATA(seq);
        Py_UCS4 chr = PyUnicode_READ(kind, data, it->it_index);
        it->it_index++;
        return _PyUnicode_FromOrdinal(chr);
    }

    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

static PyObject *
unicode_ascii_iter_next(PyObject *op)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    assert(it != NULL);
    PyObject *seq = it->it_seq;
    if (seq == NULL) {
        return NULL;
    }
    assert(_PyUnicode_CHECK(seq));
    assert(PyUnicode_IS_COMPACT_ASCII(seq));
    if (it->it_index < PyUnicode_GET_LENGTH(seq)) {
        const void *data = ((void*)(_PyASCIIObject_CAST(seq) + 1));
        Py_UCS1 chr = (Py_UCS1)PyUnicode_READ(PyUnicode_1BYTE_KIND,
                                              data, it->it_index);
        it->it_index++;
        return (PyObject*)&_Py_SINGLETON(strings).ascii[chr];
    }
    it->it_seq = NULL;
    Py_DECREF(seq);
    return NULL;
}

PyObject*
_PyUnicode_Iter(PyObject *seq)
{
    unicodeiterobject *it;

    if (!PyUnicode_Check(seq)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_IS_COMPACT_ASCII(seq)) {
        it = PyObject_GC_New(unicodeiterobject, &_PyUnicodeASCIIIter_Type);
    }
    else {
        it = PyObject_GC_New(unicodeiterobject, &PyUnicodeIter_Type);
    }
    if (it == NULL)
        return NULL;
    it->it_index = 0;
    it->it_seq = Py_NewRef(seq);
    _PyObject_GC_TRACK(it);
    return (PyObject *)it;
}

static PyObject *
unicodeiter_len(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_ssize_t len = 0;
    if (it->it_seq)
        len = PyUnicode_GET_LENGTH(it->it_seq) - it->it_index;
    return PyLong_FromSsize_t(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyObject *
unicodeiter_reduce(PyObject *op, PyObject *Py_UNUSED(ignored))
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    PyObject *iter = _PyEval_GetBuiltin(&_Py_ID(iter));

    /* _PyEval_GetBuiltin can invoke arbitrary code,
     * call must be before access of iterator pointers.
     * see issue #101765 */

    if (it->it_seq != NULL) {
        return Py_BuildValue("N(O)n", iter, it->it_seq, it->it_index);
    } else {
        PyObject *u = _PyUnicode_GetEmpty();
        if (u == NULL) {
            Py_XDECREF(iter);
            return NULL;
        }
        return Py_BuildValue("N(N)", iter, u);
    }
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static PyObject *
unicodeiter_setstate(PyObject *op, PyObject *state)
{
    unicodeiterobject *it = (unicodeiterobject *)op;
    Py_ssize_t index = PyLong_AsSsize_t(state);
    if (index == -1 && PyErr_Occurred())
        return NULL;
    if (it->it_seq != NULL) {
        if (index < 0)
            index = 0;
        else if (index > PyUnicode_GET_LENGTH(it->it_seq))
            index = PyUnicode_GET_LENGTH(it->it_seq); /* iterator truncated */
        it->it_index = index;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(setstate_doc, "Set state information for unpickling.");

static PyMethodDef unicodeiter_methods[] = {
    {"__length_hint__", unicodeiter_len, METH_NOARGS, length_hint_doc},
    {"__reduce__",      unicodeiter_reduce, METH_NOARGS, reduce_doc},
    {"__setstate__",    unicodeiter_setstate, METH_O, setstate_doc},
    {NULL,      NULL}       /* sentinel */
};

PyTypeObject PyUnicodeIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "str_iterator",         /* tp_name */
    sizeof(unicodeiterobject),      /* tp_basicsize */
    0,                  /* tp_itemsize */
    /* methods */
    unicodeiter_dealloc,/* tp_dealloc */
    0,                  /* tp_vectorcall_offset */
    0,                  /* tp_getattr */
    0,                  /* tp_setattr */
    0,                  /* tp_as_async */
    0,                  /* tp_repr */
    0,                  /* tp_as_number */
    0,                  /* tp_as_sequence */
    0,                  /* tp_as_mapping */
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    PyObject_GenericGetAttr,        /* tp_getattro */
    0,                  /* tp_setattro */
    0,                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    0,                  /* tp_doc */
    unicodeiter_traverse, /* tp_traverse */
    0,                  /* tp_clear */
    0,                  /* tp_richcompare */
    0,                  /* tp_weaklistoffset */
    PyObject_SelfIter,          /* tp_iter */
    unicodeiter_next,   /* tp_iternext */
    unicodeiter_methods,            /* tp_methods */
    0,
};

PyTypeObject _PyUnicodeASCIIIter_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "str_ascii_iterator",
    .tp_basicsize = sizeof(unicodeiterobject),
    .tp_dealloc = unicodeiter_dealloc,
    .tp_getattro = PyObject_GenericGetAttr,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .tp_traverse = unicodeiter_traverse,
    .tp_iter = PyObject_SelfIter,
    .tp_iternext = unicode_ascii_iter_next,
    .tp_methods = unicodeiter_methods,
};
