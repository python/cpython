/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_py_fopen__doc__,
"py_fopen($module, path, mode, /)\n"
"--\n"
"\n"
"Call Py_fopen(), fread(256) and Py_fclose(). Return read bytes.");

#define _TESTCAPI_PY_FOPEN_METHODDEF    \
    {"py_fopen", _PyCFunction_CAST(_testcapi_py_fopen), METH_FASTCALL, _testcapi_py_fopen__doc__},

static PyObject *
_testcapi_py_fopen_impl(PyObject *module, PyObject *path, const char *mode);

static PyObject *
_testcapi_py_fopen(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *path;
    const char *mode;

    if (!_PyArg_CheckPositional("py_fopen", nargs, 2, 2)) {
        goto exit;
    }
    path = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("py_fopen", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t mode_length;
    mode = PyUnicode_AsUTF8AndSize(args[1], &mode_length);
    if (mode == NULL) {
        goto exit;
    }
    if (strlen(mode) != (size_t)mode_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _testcapi_py_fopen_impl(module, path, mode);

exit:
    return return_value;
}
/*[clinic end generated code: output=c9fe964c3e5a0c32 input=a9049054013a1b77]*/
