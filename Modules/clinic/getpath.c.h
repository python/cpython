/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(getpath_abspath__doc__,
"abspath($module, path, /)\n"
"--\n"
"\n"
"Return the absolute path.");

#define GETPATH_ABSPATH_METHODDEF    \
    {"abspath", (PyCFunction)getpath_abspath, METH_O, getpath_abspath__doc__},

static PyObject *
getpath_abspath_impl(PyObject *module, const wchar_t *path);

static PyObject *
getpath_abspath(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const wchar_t *path = NULL;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("abspath", "argument", "str", arg);
        goto exit;
    }
    path = PyUnicode_AsWideCharString(arg, NULL);
    if (path == NULL) {
        goto exit;
    }
    return_value = getpath_abspath_impl(module, path);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_basename__doc__,
"basename($module, path, /)\n"
"--\n"
"\n"
"Return the final component of the path.");

#define GETPATH_BASENAME_METHODDEF    \
    {"basename", (PyCFunction)getpath_basename, METH_O, getpath_basename__doc__},

static PyObject *
getpath_basename_impl(PyObject *module, PyObject *path);

static PyObject *
getpath_basename(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("basename", "argument", "str", arg);
        goto exit;
    }
    path = arg;
    return_value = getpath_basename_impl(module, path);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_dirname__doc__,
"dirname($module, path, /)\n"
"--\n"
"\n"
"Return the directory component of the path.");

#define GETPATH_DIRNAME_METHODDEF    \
    {"dirname", (PyCFunction)getpath_dirname, METH_O, getpath_dirname__doc__},

static PyObject *
getpath_dirname_impl(PyObject *module, PyObject *path);

static PyObject *
getpath_dirname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("dirname", "argument", "str", arg);
        goto exit;
    }
    path = arg;
    return_value = getpath_dirname_impl(module, path);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isabs__doc__,
"isabs($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is absolute.");

#define GETPATH_ISABS_METHODDEF    \
    {"isabs", (PyCFunction)getpath_isabs, METH_O, getpath_isabs__doc__},

static int
getpath_isabs_impl(PyObject *module, const wchar_t *path);

static PyObject *
getpath_isabs(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const wchar_t *path = NULL;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("isabs", "argument", "str", arg);
        goto exit;
    }
    path = PyUnicode_AsWideCharString(arg, NULL);
    if (path == NULL) {
        goto exit;
    }
    _return_value = getpath_isabs_impl(module, path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_hassuffix__doc__,
"hassuffix($module, path, suffix, /)\n"
"--\n"
"\n"
"Return True if the path ends with the suffix, ignoring the case.");

#define GETPATH_HASSUFFIX_METHODDEF    \
    {"hassuffix", _PyCFunction_CAST(getpath_hassuffix), METH_FASTCALL, getpath_hassuffix__doc__},

static PyObject *
getpath_hassuffix_impl(PyObject *module, PyObject *pathobj,
                       PyObject *suffixobj);

static PyObject *
getpath_hassuffix(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *pathobj;
    PyObject *suffixobj;

    if (!_PyArg_CheckPositional("hassuffix", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("hassuffix", "argument 1", "str", args[0]);
        goto exit;
    }
    pathobj = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("hassuffix", "argument 2", "str", args[1]);
        goto exit;
    }
    suffixobj = args[1];
    return_value = getpath_hassuffix_impl(module, pathobj, suffixobj);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isdir__doc__,
"isdir($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is a directory.");

#define GETPATH_ISDIR_METHODDEF    \
    {"isdir", (PyCFunction)getpath_isdir, METH_O, getpath_isdir__doc__},

static int
getpath_isdir_impl(PyObject *module, const wchar_t *path);

static PyObject *
getpath_isdir(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const wchar_t *path = NULL;
    int _return_value;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("isdir", "argument", "str", arg);
        goto exit;
    }
    path = PyUnicode_AsWideCharString(arg, NULL);
    if (path == NULL) {
        goto exit;
    }
    _return_value = getpath_isdir_impl(module, path);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyBool_FromLong((long)_return_value);

exit:
    /* Cleanup for path */
    PyMem_Free((void *)path);

    return return_value;
}

PyDoc_STRVAR(getpath_isfile__doc__,
"isfile($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is a regular file.");

#define GETPATH_ISFILE_METHODDEF    \
    {"isfile", (PyCFunction)getpath_isfile, METH_O, getpath_isfile__doc__},

static PyObject *
getpath_isfile_impl(PyObject *module, PyObject *pathobj);

static PyObject *
getpath_isfile(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pathobj;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("isfile", "argument", "str", arg);
        goto exit;
    }
    pathobj = arg;
    return_value = getpath_isfile_impl(module, pathobj);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_isxfile__doc__,
"isxfile($module, path, /)\n"
"--\n"
"\n"
"Return True if the path is an executable file.");

#define GETPATH_ISXFILE_METHODDEF    \
    {"isxfile", (PyCFunction)getpath_isxfile, METH_O, getpath_isxfile__doc__},

static PyObject *
getpath_isxfile_impl(PyObject *module, PyObject *pathobj);

static PyObject *
getpath_isxfile(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pathobj;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("isxfile", "argument", "str", arg);
        goto exit;
    }
    pathobj = arg;
    return_value = getpath_isxfile_impl(module, pathobj);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_joinpath__doc__,
"joinpath($module, /, *args)\n"
"--\n"
"\n"
"Join the path components.");

#define GETPATH_JOINPATH_METHODDEF    \
    {"joinpath", _PyCFunction_CAST(getpath_joinpath), METH_FASTCALL, getpath_joinpath__doc__},

static PyObject *
getpath_joinpath_impl(PyObject *module, PyObject *args);

static PyObject *
getpath_joinpath(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *__clinic_args = NULL;

    __clinic_args = PyTuple_FromArray(args, nargs);
    if (__clinic_args == NULL) {
        goto exit;
    }
    return_value = getpath_joinpath_impl(module, __clinic_args);

exit:
    /* Cleanup for args */
    Py_XDECREF(__clinic_args);

    return return_value;
}

PyDoc_STRVAR(getpath_readlines__doc__,
"readlines($module, path, /)\n"
"--\n"
"\n"
"Return the lines of the file.");

#define GETPATH_READLINES_METHODDEF    \
    {"readlines", (PyCFunction)getpath_readlines, METH_O, getpath_readlines__doc__},

static PyObject *
getpath_readlines_impl(PyObject *module, PyObject *pathobj);

static PyObject *
getpath_readlines(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pathobj;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("readlines", "argument", "str", arg);
        goto exit;
    }
    pathobj = arg;
    return_value = getpath_readlines_impl(module, pathobj);

exit:
    return return_value;
}

PyDoc_STRVAR(getpath_realpath__doc__,
"realpath($module, path, /)\n"
"--\n"
"\n"
"Resolve a symlinked file.");

#define GETPATH_REALPATH_METHODDEF    \
    {"realpath", (PyCFunction)getpath_realpath, METH_O, getpath_realpath__doc__},

static PyObject *
getpath_realpath_impl(PyObject *module, PyObject *pathobj);

static PyObject *
getpath_realpath(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *pathobj;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("realpath", "argument", "str", arg);
        goto exit;
    }
    pathobj = arg;
    return_value = getpath_realpath_impl(module, pathobj);

exit:
    return return_value;
}
/*[clinic end generated code: output=176f5c505fa66eff input=a9049054013a1b77]*/
