/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testcapi_pyfile_getline__doc__,
"pyfile_getline($module, file, n, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_GETLINE_METHODDEF    \
    {"pyfile_getline", (PyCFunction)(void(*)(void))_testcapi_pyfile_getline, METH_FASTCALL, _testcapi_pyfile_getline__doc__},

static PyObject *
_testcapi_pyfile_getline_impl(PyObject *module, PyObject *file, int n);

static PyObject *
_testcapi_pyfile_getline(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *file;
    int n;

    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "pyfile_getline expected 2 arguments, got %zd", nargs);
        goto exit;
    }
    file = args[0];
    n = PyLong_AsInt(args[1]);
    if (n == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_pyfile_getline_impl(module, file, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyfile_writeobject__doc__,
"pyfile_writeobject($module, obj, file, flags, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_WRITEOBJECT_METHODDEF    \
    {"pyfile_writeobject", (PyCFunction)(void(*)(void))_testcapi_pyfile_writeobject, METH_FASTCALL, _testcapi_pyfile_writeobject__doc__},

static PyObject *
_testcapi_pyfile_writeobject_impl(PyObject *module, PyObject *obj,
                                  PyObject *file, int flags);

static PyObject *
_testcapi_pyfile_writeobject(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    PyObject *file;
    int flags;

    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "pyfile_writeobject expected 3 arguments, got %zd", nargs);
        goto exit;
    }
    obj = args[0];
    file = args[1];
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_pyfile_writeobject_impl(module, obj, file, flags);

exit:
    return return_value;
}

PyDoc_STRVAR(_testcapi_pyobject_asfiledescriptor__doc__,
"pyobject_asfiledescriptor($module, obj, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYOBJECT_ASFILEDESCRIPTOR_METHODDEF    \
    {"pyobject_asfiledescriptor", (PyCFunction)_testcapi_pyobject_asfiledescriptor, METH_O, _testcapi_pyobject_asfiledescriptor__doc__},
/*[clinic end generated code: output=ea572aaaa01aec7b input=a9049054013a1b77]*/
