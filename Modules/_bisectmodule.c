/* Bisection algorithms. Drop in replacement for bisect.py

Converted to C by Dmitry Vasiliev (dima at hlabs.spb.ru).
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

_Py_IDENTIFIER(insert);

static Py_ssize_t
internal_bisect_right(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi)
{
    PyObject *litem;
    Py_ssize_t mid;
    int res;

    if (lo < 0) {
        PyErr_SetString(PyExc_ValueError, "lo must be non-negative");
        return -1;
    }
    if (hi == -1) {
        hi = PySequence_Size(list);
        if (hi < 0)
            return -1;
    }
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        litem = PySequence_GetItem(list, mid);
        if (litem == NULL)
            return -1;
        res = PyObject_RichCompareBool(item, litem, Py_LT);
        Py_DECREF(litem);
        if (res < 0)
            return -1;
        if (res)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}

static PyObject *
bisect_right(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *list, *item;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    Py_ssize_t index;
    static char *keywords[] = {"a", "x", "lo", "hi", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|nn:bisect_right",
        keywords, &list, &item, &lo, &hi))
        return NULL;
    index = internal_bisect_right(list, item, lo, hi);
    if (index < 0)
        return NULL;
    return PyLong_FromSsize_t(index);
}

PyDoc_STRVAR(bisect_right_doc,
"bisect_right(a, x[, lo[, hi]]) -> index\n\
\n\
Return the index where to insert item x in list a, assuming a is sorted.\n\
\n\
The return value i is such that all e in a[:i] have e <= x, and all e in\n\
a[i:] have e > x.  So if x already appears in the list, i points just\n\
beyond the rightmost x already there\n\
\n\
Optional args lo (default 0) and hi (default len(a)) bound the\n\
slice of a to be searched.\n");

static PyObject *
insort_right(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *list, *item, *result;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    Py_ssize_t index;
    static char *keywords[] = {"a", "x", "lo", "hi", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|nn:insort_right",
        keywords, &list, &item, &lo, &hi))
        return NULL;
    index = internal_bisect_right(list, item, lo, hi);
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(list)) {
        if (PyList_Insert(list, index, item) < 0)
            return NULL;
    } else {
        result = _PyObject_CallMethodId(list, &PyId_insert, "nO", index, item);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(insort_right_doc,
"insort_right(a, x[, lo[, hi]])\n\
\n\
Insert item x in list a, and keep it sorted assuming a is sorted.\n\
\n\
If x is already in a, insert it to the right of the rightmost x.\n\
\n\
Optional args lo (default 0) and hi (default len(a)) bound the\n\
slice of a to be searched.\n");

static Py_ssize_t
internal_bisect_left(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi)
{
    PyObject *litem;
    Py_ssize_t mid;
    int res;

    if (lo < 0) {
        PyErr_SetString(PyExc_ValueError, "lo must be non-negative");
        return -1;
    }
    if (hi == -1) {
        hi = PySequence_Size(list);
        if (hi < 0)
            return -1;
    }
    while (lo < hi) {
        /* The (size_t)cast ensures that the addition and subsequent division
           are performed as unsigned operations, avoiding difficulties from
           signed overflow.  (See issue 13496.) */
        mid = ((size_t)lo + hi) / 2;
        litem = PySequence_GetItem(list, mid);
        if (litem == NULL)
            return -1;
        res = PyObject_RichCompareBool(litem, item, Py_LT);
        Py_DECREF(litem);
        if (res < 0)
            return -1;
        if (res)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

static PyObject *
bisect_left(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *list, *item;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    Py_ssize_t index;
    static char *keywords[] = {"a", "x", "lo", "hi", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|nn:bisect_left",
        keywords, &list, &item, &lo, &hi))
        return NULL;
    index = internal_bisect_left(list, item, lo, hi);
    if (index < 0)
        return NULL;
    return PyLong_FromSsize_t(index);
}

PyDoc_STRVAR(bisect_left_doc,
"bisect_left(a, x[, lo[, hi]]) -> index\n\
\n\
Return the index where to insert item x in list a, assuming a is sorted.\n\
\n\
The return value i is such that all e in a[:i] have e < x, and all e in\n\
a[i:] have e >= x.  So if x already appears in the list, i points just\n\
before the leftmost x already there.\n\
\n\
Optional args lo (default 0) and hi (default len(a)) bound the\n\
slice of a to be searched.\n");

static PyObject *
insort_left(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *list, *item, *result;
    Py_ssize_t lo = 0;
    Py_ssize_t hi = -1;
    Py_ssize_t index;
    static char *keywords[] = {"a", "x", "lo", "hi", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|nn:insort_left",
        keywords, &list, &item, &lo, &hi))
        return NULL;
    index = internal_bisect_left(list, item, lo, hi);
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(list)) {
        if (PyList_Insert(list, index, item) < 0)
            return NULL;
    } else {
        result = _PyObject_CallMethodId(list, &PyId_insert, "nO", index, item);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(insort_left_doc,
"insort_left(a, x[, lo[, hi]])\n\
\n\
Insert item x in list a, and keep it sorted assuming a is sorted.\n\
\n\
If x is already in a, insert it to the left of the leftmost x.\n\
\n\
Optional args lo (default 0) and hi (default len(a)) bound the\n\
slice of a to be searched.\n");

PyDoc_STRVAR(bisect_doc, "Alias for bisect_right().\n");
PyDoc_STRVAR(insort_doc, "Alias for insort_right().\n");

static PyMethodDef bisect_methods[] = {
    {"bisect_right", (PyCFunction)bisect_right,
        METH_VARARGS|METH_KEYWORDS, bisect_right_doc},
    {"bisect", (PyCFunction)bisect_right,
        METH_VARARGS|METH_KEYWORDS, bisect_doc},
    {"insort_right", (PyCFunction)insort_right,
        METH_VARARGS|METH_KEYWORDS, insort_right_doc},
    {"insort", (PyCFunction)insort_right,
        METH_VARARGS|METH_KEYWORDS, insort_doc},
    {"bisect_left", (PyCFunction)bisect_left,
        METH_VARARGS|METH_KEYWORDS, bisect_left_doc},
    {"insort_left", (PyCFunction)insort_left,
        METH_VARARGS|METH_KEYWORDS, insort_left_doc},
    {NULL, NULL} /* sentinel */
};

PyDoc_STRVAR(module_doc,
"Bisection algorithms.\n\
\n\
This module provides support for maintaining a list in sorted order without\n\
having to sort the list after each insertion. For long lists of items with\n\
expensive comparison operations, this can be an improvement over the more\n\
common approach.\n");


static struct PyModuleDef _bisectmodule = {
    PyModuleDef_HEAD_INIT,
    "_bisect",
    module_doc,
    -1,
    bisect_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__bisect(void)
{
    return PyModule_Create(&_bisectmodule);
}
