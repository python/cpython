#include "pyconfig.h"   // Py_GIL_DISABLED
#ifdef Py_GIL_DISABLED
#  define Py_TARGET_ABI3T 0x030f0000
#else
   // Need limited C API 3.6.1 for PySlice_Unpack() and PySlice_AdjustIndices()
   // and for PySlice_GetIndicesEx() implemented as a macro.
#  define Py_LIMITED_API 0x03060100
#endif

#include "parts.h"
#include "util.h"


static PyObject *
slice_check(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    return PyLong_FromLong(PySlice_Check(obj));
}

static PyObject *
slice_new(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *start, *stop, *step;

    if (!PyArg_ParseTuple(args, "OOO", &start, &stop, &step)) {
        return NULL;
    }
    NULLABLE(start);
    NULLABLE(stop);
    NULLABLE(step);
    return PySlice_New(start, stop, step);
}

/* Returns the (start, stop, step) triple on success.  If PySlice_GetIndices()
 * fails without setting an exception, returns None. */
static PyObject *
slice_getindices(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *slice;
    Py_ssize_t length;
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "On", &slice, &length)) {
        return NULL;
    }
    NULLABLE(slice);
    if (PySlice_GetIndices(slice, length, &start, &stop, &step) < 0) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        Py_RETURN_NONE;
    }
    assert(!PyErr_Occurred());
    assert(start != UNINITIALIZED_SIZE);
    assert(stop != UNINITIALIZED_SIZE);
    assert(step != UNINITIALIZED_SIZE);
    return Py_BuildValue("nnn", start, stop, step);
}

/* Test PySlice_GetIndicesEx() implemented as a macro using PySlice_Unpack()
 * and PySlice_AdjustIndices(). */
static PyObject *
slice_getindicesex_macro(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *slice;
    Py_ssize_t length = UNINITIALIZED_SIZE;
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;
    Py_ssize_t slicelength = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "On", &slice, &length)) {
        return NULL;
    }
    NULLABLE(slice);
    if (PySlice_GetIndicesEx(slice, length,
                             &start, &stop, &step, &slicelength) < 0) {
        assert(PyErr_Occurred());
        /* The macro sets the slice length to 0 on error. */
        assert(slicelength == 0);
        return NULL;
    }
    assert(!PyErr_Occurred());
    assert(start != UNINITIALIZED_SIZE);
    assert(stop != UNINITIALIZED_SIZE);
    assert(step != UNINITIALIZED_SIZE);
    assert(slicelength != UNINITIALIZED_SIZE);
    return Py_BuildValue("nnnn", start, stop, step, slicelength);
}

/* Same as slice_getindicesex_macro(), but the length is the size of a sequence.
 * The macro evaluates it after calling PySlice_Unpack(), which can execute
 * arbitrary Python code and resize the sequence. */
static PyObject *
slice_getindicesex_seq_macro(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *slice, *seq;
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;
    Py_ssize_t slicelength = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "OO", &slice, &seq)) {
        return NULL;
    }
    NULLABLE(slice);
    NULLABLE(seq);
    if (PySlice_GetIndicesEx(slice, Py_SIZE(seq),
                             &start, &stop, &step, &slicelength) < 0) {
        assert(PyErr_Occurred());
        assert(slicelength == 0);
        return NULL;
    }
    assert(!PyErr_Occurred());
    return Py_BuildValue("nnnn", start, stop, step, slicelength);
}

static PyObject *
slice_unpack(PyObject *Py_UNUSED(module), PyObject *slice)
{
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;

    NULLABLE(slice);
    if (PySlice_Unpack(slice, &start, &stop, &step) < 0) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    assert(start != UNINITIALIZED_SIZE);
    assert(stop != UNINITIALIZED_SIZE);
    assert(step != UNINITIALIZED_SIZE);
    return Py_BuildValue("nnn", start, stop, step);
}

static PyObject *
slice_adjustindices(PyObject *Py_UNUSED(module), PyObject *args)
{
    Py_ssize_t length, start, stop, step;

    if (!PyArg_ParseTuple(args, "nnnn", &length, &start, &stop, &step)) {
        return NULL;
    }
    Py_ssize_t slicelength = PySlice_AdjustIndices(length, &start, &stop, step);
    assert(!PyErr_Occurred());
    return Py_BuildValue("nnn", slicelength, start, stop);
}

#undef PySlice_GetIndicesEx

/* Test the deprecated PySlice_GetIndicesEx() function.  It is still exported
 * for the stable ABI and used if Py_LIMITED_API is older than 3.5.4. */
static PyObject *
slice_getindicesex_func(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *slice;
    Py_ssize_t length;
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;
    Py_ssize_t slicelength = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "On", &slice, &length)) {
        return NULL;
    }
    NULLABLE(slice);
// Ignore deprecation warnings
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    int res = PySlice_GetIndicesEx(slice, length,
                                   &start, &stop, &step, &slicelength);
_Py_COMP_DIAG_POP
    if (res < 0) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    assert(start != UNINITIALIZED_SIZE);
    assert(stop != UNINITIALIZED_SIZE);
    assert(step != UNINITIALIZED_SIZE);
    assert(slicelength != UNINITIALIZED_SIZE);
    return Py_BuildValue("nnnn", start, stop, step, slicelength);
}


/* Same as slice_getindicesex_seq_macro(), but using the deprecated function.
 * The length is evaluated before the call. */
static PyObject *
slice_getindicesex_seq_func(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *slice, *seq;
    Py_ssize_t start = UNINITIALIZED_SIZE;
    Py_ssize_t stop = UNINITIALIZED_SIZE;
    Py_ssize_t step = UNINITIALIZED_SIZE;
    Py_ssize_t slicelength = UNINITIALIZED_SIZE;

    if (!PyArg_ParseTuple(args, "OO", &slice, &seq)) {
        return NULL;
    }
    NULLABLE(slice);
    NULLABLE(seq);
// Ignore deprecation warnings
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS
    int res = PySlice_GetIndicesEx(slice, Py_SIZE(seq),
                                   &start, &stop, &step, &slicelength);
_Py_COMP_DIAG_POP
    if (res < 0) {
        assert(PyErr_Occurred());
        return NULL;
    }
    assert(!PyErr_Occurred());
    return Py_BuildValue("nnnn", start, stop, step, slicelength);
}


static PyMethodDef test_methods[] = {
    {"slice_check", slice_check, METH_O},
    {"slice_new", slice_new, METH_VARARGS},
    {"slice_getindices", slice_getindices, METH_VARARGS},
    {"slice_getindicesex_macro", slice_getindicesex_macro, METH_VARARGS},
    {"slice_getindicesex_seq_macro", slice_getindicesex_seq_macro, METH_VARARGS},
    {"slice_getindicesex_func", slice_getindicesex_func,
     METH_VARARGS},
    {"slice_getindicesex_seq_func", slice_getindicesex_seq_func,
     METH_VARARGS},
    {"slice_unpack", slice_unpack, METH_O},
    {"slice_adjustindices", slice_adjustindices, METH_VARARGS},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_Slice(PyObject *m)
{
    return PyModule_AddFunctions(m, test_methods);
}
