/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_copy_deepcopy__doc__,
"deepcopy($module, x, memo=None, /)\n"
"--\n"
"\n"
"Create a deep copy of x\n"
"\n"
"See the documentation for the copy module for details.");

#define _COPY_DEEPCOPY_METHODDEF    \
    {"deepcopy", _PyCFunction_CAST(_copy_deepcopy), METH_FASTCALL, _copy_deepcopy__doc__},

static PyObject *
_copy_deepcopy_impl(PyObject *module, PyObject *x, PyObject *memo);

static PyObject *
_copy_deepcopy(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *x;
    PyObject *memo = Py_None;

    if (!_PyArg_CheckPositional("deepcopy", nargs, 1, 2)) {
        goto exit;
    }
    x = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    memo = args[1];
skip_optional:
    return_value = _copy_deepcopy_impl(module, x, memo);

exit:
    return return_value;
}
/*[clinic end generated code: output=c1d30b4875fef931 input=a9049054013a1b77]*/
