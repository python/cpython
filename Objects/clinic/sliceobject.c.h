/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

static PyObject *
slice_new_impl(PyTypeObject *type, PyObject *start, PyObject *stop,
               PyObject *step);

static PyObject *
slice_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = &PySlice_Type;
    PyObject *start;
    PyObject *stop = NULL;
    PyObject *step = NULL;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("slice", kwargs)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("slice", PyTuple_GET_SIZE(args), 1, 3)) {
        goto exit;
    }
    start = PyTuple_GET_ITEM(args, 0);
    if (PyTuple_GET_SIZE(args) < 2) {
        goto skip_optional;
    }
    stop = PyTuple_GET_ITEM(args, 1);
    if (PyTuple_GET_SIZE(args) < 3) {
        goto skip_optional;
    }
    step = PyTuple_GET_ITEM(args, 2);
skip_optional:
    return_value = slice_new_impl(type, start, stop, step);

exit:
    return return_value;
}

static PyObject *
slice_vectorcall(PyObject *type, PyObject *const *args,
    size_t nargsf, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    Py_ssize_t nargs = PyVectorcall_NARGS(nargsf);
    PyObject *start;
    PyObject *stop = NULL;
    PyObject *step = NULL;

    assert(Py_Is(_PyType_CAST(type), &PySlice_Type));
    /* Make sure the type object is immutable: the generated
     * vectorcall doesn't deal e.g. with users reassigning __init__. */
    assert(PyType_HasFeature(_PyType_CAST(type), Py_TPFLAGS_IMMUTABLETYPE));
    if (!_PyArg_NoKwnames("slice", kwnames)) {
        goto exit;
    }
    if (!_PyArg_CheckPositional("slice", nargs, 1, 3)) {
        goto exit;
    }
    start = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    stop = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    step = args[2];
skip_optional:
    return_value = slice_new_impl(_PyType_CAST(type), start, stop, step);

exit:
    return return_value;
}
/*[clinic end generated code: output=3547c23a3ee3cd86 input=a9049054013a1b77]*/
