/* Bisection algorithms. Drop in replacement for bisect.py

Converted to C by Dmitry Vasiliev (dima at hlabs.spb.ru).
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

/*[clinic input]
module _bisect
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=4d56a2b2033b462b]*/

#include "clinic/_bisectmodule.c.h"

typedef struct {
    PyObject *str_insert;
} bisect_state;

static inline bisect_state*
get_bisect_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (bisect_state *)state;
}

static inline Py_ssize_t
internal_bisect_right(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi,
                      PyObject* key)
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
        if (key != Py_None) {
            PyObject *newitem = PyObject_CallOneArg(key, litem);
            if (newitem == NULL) {
                Py_DECREF(litem);
                return -1;
            }
            Py_SETREF(litem, newitem);
        }
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

/*[clinic input]
_bisect.bisect_right -> Py_ssize_t

    a: object
    x: object
    lo: Py_ssize_t = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e <= x, and all e in
a[i:] have e > x.  So if x already appears in the list, a.insert(i, x) will
insert just after the rightmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.
[clinic start generated code]*/

static Py_ssize_t
_bisect_bisect_right_impl(PyObject *module, PyObject *a, PyObject *x,
                          Py_ssize_t lo, Py_ssize_t hi, PyObject *key)
/*[clinic end generated code: output=3a4bc09cc7c8a73d input=40fcc5afa06ae593]*/
{
    return internal_bisect_right(a, x, lo, hi, key);
}

/*[clinic input]
_bisect.insort_right

    a: object
    x: object
    lo: Py_ssize_t = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the right of the rightmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.
[clinic start generated code]*/

static PyObject *
_bisect_insort_right_impl(PyObject *module, PyObject *a, PyObject *x,
                          Py_ssize_t lo, Py_ssize_t hi, PyObject *key)
/*[clinic end generated code: output=ac3bf26d07aedda2 input=44e1708e26b7b802]*/
{
    PyObject *result, *key_x;
    Py_ssize_t index;

    if (key == Py_None) {
        index = internal_bisect_right(a, x, lo, hi, key);
    } else {
        key_x = PyObject_CallOneArg(key, x);
        if (key_x == NULL) {
            return NULL;
        }
        index = internal_bisect_right(a, key_x, lo, hi, key);
        Py_DECREF(key_x);
    }
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(a)) {
        if (PyList_Insert(a, index, x) < 0)
            return NULL;
    }
    else {
        bisect_state *state = get_bisect_state(module);
        result = _PyObject_CallMethod(a, state->str_insert, "nO", index, x);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

static inline Py_ssize_t
internal_bisect_left(PyObject *list, PyObject *item, Py_ssize_t lo, Py_ssize_t hi,
                     PyObject *key)
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
        if (key != Py_None) {
            PyObject *newitem = PyObject_CallOneArg(key, litem);
            if (newitem == NULL) {
                Py_DECREF(litem);
                return -1;
            }
            Py_SETREF(litem, newitem);
        }
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
_bisect.bisect_left -> Py_ssize_t

    a: object
    x: object
    lo: Py_ssize_t = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Return the index where to insert item x in list a, assuming a is sorted.

The return value i is such that all e in a[:i] have e < x, and all e in
a[i:] have e >= x.  So if x already appears in the list, a.insert(i, x) will
insert just before the leftmost x already there.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.
[clinic start generated code]*/

static Py_ssize_t
_bisect_bisect_left_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi, PyObject *key)
/*[clinic end generated code: output=70749d6e5cae9284 input=90dd35b50ceb05e3]*/
{
    return internal_bisect_left(a, x, lo, hi, key);
}


/*[clinic input]
_bisect.insort_left

    a: object
    x: object
    lo: Py_ssize_t = 0
    hi: Py_ssize_t(c_default='-1', accept={int, NoneType}) = None
    *
    key: object = None

Insert item x in list a, and keep it sorted assuming a is sorted.

If x is already in a, insert it to the left of the leftmost x.

Optional args lo (default 0) and hi (default len(a)) bound the
slice of a to be searched.
[clinic start generated code]*/

static PyObject *
_bisect_insort_left_impl(PyObject *module, PyObject *a, PyObject *x,
                         Py_ssize_t lo, Py_ssize_t hi, PyObject *key)
/*[clinic end generated code: output=b1d33e5e7ffff11e input=3ab65d8784f585b1]*/
{
    PyObject *result, *key_x;
    Py_ssize_t index;

    if (key == Py_None) {
        index = internal_bisect_left(a, x, lo, hi, key);
    } else {
        key_x = PyObject_CallOneArg(key, x);
        if (key_x == NULL) {
            return NULL;
        }
        index = internal_bisect_left(a, key_x, lo, hi, key);
        Py_DECREF(key_x);
    }
    if (index < 0)
        return NULL;
    if (PyList_CheckExact(a)) {
        if (PyList_Insert(a, index, x) < 0)
            return NULL;
    } else {
        bisect_state *state = get_bisect_state(module);
        result = _PyObject_CallMethod(a, state->str_insert, "nO", index, x);
        if (result == NULL)
            return NULL;
        Py_DECREF(result);
    }

    Py_RETURN_NONE;
}

static PyMethodDef bisect_methods[] = {
    _BISECT_BISECT_RIGHT_METHODDEF
    _BISECT_INSORT_RIGHT_METHODDEF
    _BISECT_BISECT_LEFT_METHODDEF
    _BISECT_INSORT_LEFT_METHODDEF
    {NULL, NULL} /* sentinel */
};

PyDoc_STRVAR(module_doc,
"Bisection algorithms.\n\
\n\
This module provides support for maintaining a list in sorted order without\n\
having to sort the list after each insertion. For long lists of items with\n\
expensive comparison operations, this can be an improvement over the more\n\
common approach.\n");

static int
bisect_clear(PyObject *module)
{
    bisect_state *state = get_bisect_state(module);
    Py_CLEAR(state->str_insert);
    return 0;
}

static void
bisect_free(void *module)
{
    bisect_clear((PyObject *)module);
}

static int
bisect_modexec(PyObject *m)
{
    bisect_state *state = get_bisect_state(m);
    state->str_insert = PyUnicode_InternFromString("insert");
    if (state->str_insert == NULL) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot bisect_slots[] = {
    {Py_mod_exec, bisect_modexec},
    {0, NULL}
};

static struct PyModuleDef _bisectmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_bisect",
    .m_size = sizeof(bisect_state),
    .m_doc = module_doc,
    .m_methods = bisect_methods,
    .m_slots = bisect_slots,
    .m_clear = bisect_clear,
    .m_free = bisect_free,
};

PyMODINIT_FUNC
PyInit__bisect(void)
{
    return PyModuleDef_Init(&_bisectmodule);
}
