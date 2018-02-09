/* Accumulator struct implementation */

#include "Python.h"
#include "accu.h"

static PyObject *
join_list_unicode(PyObject *lst)
{
    /* return ''.join(lst) */
    PyObject *sep, *ret;
    sep = PyUnicode_FromStringAndSize("", 0);
    ret = PyUnicode_Join(sep, lst);
    Py_DECREF(sep);
    return ret;
}

int
_PyAccu_Init(_PyAccu *acc)
{
    /* Lazily allocated */
    acc->large = NULL;
    acc->small = PyList_New(0);
    if (acc->small == NULL)
        return -1;
    return 0;
}

static int
flush_accumulator(_PyAccu *acc)
{
    Py_ssize_t nsmall = PyList_GET_SIZE(acc->small);
    if (nsmall) {
        int ret;
        PyObject *joined;
        if (acc->large == NULL) {
            acc->large = PyList_New(0);
            if (acc->large == NULL)
                return -1;
        }
        joined = join_list_unicode(acc->small);
        if (joined == NULL)
            return -1;
        if (PyList_SetSlice(acc->small, 0, nsmall, NULL)) {
            Py_DECREF(joined);
            return -1;
        }
        ret = PyList_Append(acc->large, joined);
        Py_DECREF(joined);
        return ret;
    }
    return 0;
}

int
_PyAccu_Accumulate(_PyAccu *acc, PyObject *unicode)
{
    Py_ssize_t nsmall;
    assert(PyUnicode_Check(unicode));

    if (PyList_Append(acc->small, unicode))
        return -1;
    nsmall = PyList_GET_SIZE(acc->small);
    /* Each item in a list of unicode objects has an overhead (in 64-bit
     * builds) of:
     *   - 8 bytes for the list slot
     *   - 56 bytes for the header of the unicode object
     * that is, 64 bytes.  100000 such objects waste more than 6 MiB
     * compared to a single concatenated string.
     */
    if (nsmall < 100000)
        return 0;
    return flush_accumulator(acc);
}

PyObject *
_PyAccu_FinishAsList(_PyAccu *acc)
{
    int ret;
    PyObject *res;

    ret = flush_accumulator(acc);
    Py_CLEAR(acc->small);
    if (ret) {
        Py_CLEAR(acc->large);
        return NULL;
    }
    res = acc->large;
    acc->large = NULL;
    return res;
}

PyObject *
_PyAccu_Finish(_PyAccu *acc)
{
    PyObject *list, *res;
    if (acc->large == NULL) {
        list = acc->small;
        acc->small = NULL;
    }
    else {
        list = _PyAccu_FinishAsList(acc);
        if (!list)
            return NULL;
    }
    res = join_list_unicode(list);
    Py_DECREF(list);
    return res;
}

void
_PyAccu_Destroy(_PyAccu *acc)
{
    Py_CLEAR(acc->small);
    Py_CLEAR(acc->large);
}
