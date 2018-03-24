/* Bisection algorithms. Drop in replacement for bisect.py

Converted to C by Dmitry Vasiliev (dima at hlabs.spb.ru).
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

static int
ssize_t_converter(PyObject *obj, void *ptr)
{
    Py_ssize_t val;

    val = PyLong_AsSsize_t(obj);
    if (val == -1 && PyErr_Occurred()) {
        return 0;
    }
    *(Py_ssize_t *)ptr = val;
    return 1;
}

static int
optional_ssize_t_converter(PyObject *obj, void *ptr)
{
    if (obj != Py_None) {
        return ssize_t_converter(obj, ptr);
    }
    else {
        *(Py_ssize_t *)ptr = -1;
        return 1;
    }
}


#include "clinic/_bisectmodule.c.h"

/*[clinic input]
module bisect
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d0e256c42a9e4c13]*/

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

/*[python input]
class hi_parameter_converter(CConverter):
    type = 'Py_ssize_t'
    converter = 'optional_ssize_t_converter'

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=53c711121eb30d3f]*/


/*[clinic input]
bisect.bisect_right

    a: object
    x: object
    lo: Py_ssize_t(c_default='0') = 0
    hi: hi_parameter(c_default='-1') = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e <= x, and all e in
a[i:] have e > x.  So if x already appears in the list, i points just
beyond the rightmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.
[clinic start generated code]*/

static PyObject *
bisect_bisect_right_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi)
/*[clinic end generated code: output=a2fb3e3261e46954 input=94e1e505e7f1ced8]*/
{
    Py_ssize_t index;

    index = internal_bisect_right(a, x, lo, hi);
    if (index < 0)
        return NULL;
    return PyLong_FromSsize_t(index);
}

/*[clinic input]
bisect.insort_right

    a: object
    x: object
    lo: Py_ssize_t(c_default='0') = 0
    hi: hi_parameter(c_default='-1') = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the right of the rightmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

[clinic start generated code]*/

static PyObject *
bisect_insort_right_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi)
/*[clinic end generated code: output=6e0b99c731a11c1a input=6790b22da4643197]*/
{
    PyObject *result;
    Py_ssize_t index;

    index = internal_bisect_right(a, x, lo, hi);
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(a)) {
        if (PyList_Insert(a, index, x) < 0)
            return NULL;
    } else {
        result = _PyObject_CallMethodId(a, &PyId_insert, "nO", index, x);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

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

/*[clinic input]
bisect.bisect_left

    a: object
    x: object
    lo: Py_ssize_t(c_default='0') = 0
    hi: hi_parameter(c_default='-1') = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e < x, and all e in
a[i:] have e >= x.  So if x already appears in the list, i points just
before the leftmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

[clinic start generated code]*/

static PyObject *
bisect_bisect_left_impl(PyObject *module, PyObject *a, PyObject *x,
                        Py_ssize_t lo, Py_ssize_t hi)
/*[clinic end generated code: output=27a0228c4a0a5fa2 input=fc1e8f6081ccfd7c]*/
{
    Py_ssize_t index;

    index = internal_bisect_left(a, x, lo, hi);
    if (index < 0)
        return NULL;
    return PyLong_FromSsize_t(index);
}

/*[clinic input]
bisect.insort_left

    a: object
    x: object
    lo: Py_ssize_t(c_default='0') = 0
    hi: hi_parameter(c_default='-1') = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the left of the leftmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.

[clinic start generated code]*/

static PyObject *
bisect_insort_left_impl(PyObject *module, PyObject *a, PyObject *x,
                        Py_ssize_t lo, Py_ssize_t hi)
/*[clinic end generated code: output=aa0228af6970ec52 input=582456c4727c5716]*/
{
    PyObject *result;
    Py_ssize_t index;

    index = internal_bisect_left(a, x, lo, hi);
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(a)) {
        if (PyList_Insert(a, index, x) < 0)
            return NULL;
    } else {
        result = _PyObject_CallMethodId(a, &PyId_insert, "nO", index, x);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

static PyMethodDef bisect_methods[] = {
    BISECT_BISECT_RIGHT_METHODDEF
    BISECT_INSORT_RIGHT_METHODDEF
    BISECT_BISECT_LEFT_METHODDEF
    BISECT_INSORT_LEFT_METHODDEF
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
