/*[clinic input]
preserve
[clinic start generated code]*/

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
/*[clinic end generated code: output=44002184a5d9dbb9 input=a9049054013a1b77]*/
