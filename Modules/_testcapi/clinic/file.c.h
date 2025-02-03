/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_testcapi_pyfile_getline__doc__,
"pyfile_getline($module, file, n, /)\n"
"--\n"
"\n");

#define _TESTCAPI_PYFILE_GETLINE_METHODDEF    \
    {"pyfile_getline", _PyCFunction_CAST(_testcapi_pyfile_getline), METH_FASTCALL, _testcapi_pyfile_getline__doc__},

static PyObject *
_testcapi_pyfile_getline_impl(PyObject *module, PyObject *file, int n);

static PyObject *
_testcapi_pyfile_getline(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *file;
    int n;

    if (!_PyArg_CheckPositional("pyfile_getline", nargs, 2, 2)) {
        goto exit;
    }
    file = args[0];
    n = _PyLong_AsInt(args[1]);
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
    {"pyfile_writeobject", _PyCFunction_CAST(_testcapi_pyfile_writeobject), METH_FASTCALL, _testcapi_pyfile_writeobject__doc__},

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

    if (!_PyArg_CheckPositional("pyfile_writeobject", nargs, 3, 3)) {
        goto exit;
    }
    obj = args[0];
    file = args[1];
    flags = _PyLong_AsInt(args[2]);
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

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testcapi_pyfile_newstdprinter_impl(module, fd);

exit:
    return return_value;
}
/*[clinic end generated code: output=bb69f2c213a490f6 input=a9049054013a1b77]*/
