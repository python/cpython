/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_testcapi_pyfile_newstdprinter__doc__,
"pyfile_newstdprinter($module, fd, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_NEWSTDPRINTER_METHODDEF    \
    {"pyfile_newstdprinter", (PyCFunction)_testcapi_pyfile_newstdprinter, METH_O, _testcapi_pyfile_newstdprinter__doc__},

static PyObject *
_testcapi_pyfile_newstdprinter_impl(PyObject *module, int fd);

static PyObject *
_testcapi_pyfile_newstdprinter(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_pyfile_newstdprinter_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_py_fopen__doc__,
"py_fopen($module, path, mode, /)\n"
"--\n"
"\n"
"Call Py_fopen(), fread(256) and Py_fclose(). Return read bytes.");

#define _TESTCAPI_PY_FOPEN_METHODDEF    \
    {"py_fopen", _PyCFunction_CAST(_testcapi_py_fopen), METH_FASTCALL, _testcapi_py_fopen__doc__},

static PyObject *
_testcapi_py_fopen_impl(PyObject *module, PyObject *path, const char *mode,
                        Py_ssize_t mode_length);

static PyObject *
_testcapi_py_fopen(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *path;
    const char *mode;
    Py_ssize_t mode_length;

    if (!_PyArg_ParseStack(args, nargs, "Oz#:py_fopen",
        &path, &mode, &mode_length)) {
        goto exit;
    }
    return_value = _testcapi_py_fopen_impl(module, path, mode, mode_length);

exit:
    return return_value;
}
/*[clinic end generated code: output=e943bbd7f181d079 input=a9049054013a1b77]*/
