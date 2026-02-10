/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

#if defined(HAVE_GETRUSAGE)

PyDoc_STRVAR(resource_getrusage__doc__,
"getrusage($module, who, /)\n"
"--\n"
"\n");

#define RESOURCE_GETRUSAGE_METHODDEF    \
    {"getrusage", (PyCFunction)resource_getrusage, METH_O, resource_getrusage__doc__},

static PyObject *
resource_getrusage_impl(PyObject *module, int who);

static PyObject *
resource_getrusage(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int who;

    who = PyLong_AsInt(arg);
    if (who == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = resource_getrusage_impl(module, who);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETRUSAGE) */

PyDoc_STRVAR(resource_getrlimit__doc__,
"getrlimit($module, resource, /)\n"
"--\n"
"\n");

#define RESOURCE_GETRLIMIT_METHODDEF    \
    {"getrlimit", (PyCFunction)resource_getrlimit, METH_O, resource_getrlimit__doc__},

static PyObject *
resource_getrlimit_impl(PyObject *module, int resource);

static PyObject *
resource_getrlimit(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int resource;

    resource = PyLong_AsInt(arg);
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = resource_getrlimit_impl(module, resource);

exit:
    return return_value;
}

PyDoc_STRVAR(resource_setrlimit__doc__,
"setrlimit($module, resource, limits, /)\n"
"--\n"
"\n");

#define RESOURCE_SETRLIMIT_METHODDEF    \
    {"setrlimit", _PyCFunction_CAST(resource_setrlimit), METH_FASTCALL, resource_setrlimit__doc__},

static PyObject *
resource_setrlimit_impl(PyObject *module, int resource, PyObject *limits);

static PyObject *
resource_setrlimit(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int resource;
    PyObject *limits;

    if (!_PyArg_CheckPositional("setrlimit", nargs, 2, 2)) {
        goto exit;
    }
    resource = PyLong_AsInt(args[0]);
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    limits = args[1];
    return_value = resource_setrlimit_impl(module, resource, limits);

exit:
    return return_value;
}

#if defined(HAVE_PRLIMIT)

PyDoc_STRVAR(resource_prlimit__doc__,
"prlimit($module, pid, resource, limits=None, /)\n"
"--\n"
"\n");

#define RESOURCE_PRLIMIT_METHODDEF    \
    {"prlimit", _PyCFunction_CAST(resource_prlimit), METH_FASTCALL, resource_prlimit__doc__},

static PyObject *
resource_prlimit_impl(PyObject *module, pid_t pid, int resource,
                      PyObject *limits);

static PyObject *
resource_prlimit(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int resource;
    PyObject *limits = Py_None;

    if (!_PyArg_CheckPositional("prlimit", nargs, 2, 3)) {
        goto exit;
    }
    pid = PyLong_AsPid(args[0]);
    if (pid == -1 && PyErr_Occurred()) {
        goto exit;
    }
    resource = PyLong_AsInt(args[1]);
    if (resource == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    limits = args[2];
skip_optional:
    return_value = resource_prlimit_impl(module, pid, resource, limits);

exit:
    return return_value;
}

#endif /* defined(HAVE_PRLIMIT) */

PyDoc_STRVAR(resource_getpagesize__doc__,
"getpagesize($module, /)\n"
"--\n"
"\n");

#define RESOURCE_GETPAGESIZE_METHODDEF    \
    {"getpagesize", (PyCFunction)resource_getpagesize, METH_NOARGS, resource_getpagesize__doc__},

static int
resource_getpagesize_impl(PyObject *module);

static PyObject *
resource_getpagesize(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = resource_getpagesize_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#ifndef RESOURCE_GETRUSAGE_METHODDEF
    #define RESOURCE_GETRUSAGE_METHODDEF
#endif /* !defined(RESOURCE_GETRUSAGE_METHODDEF) */

#ifndef RESOURCE_PRLIMIT_METHODDEF
    #define RESOURCE_PRLIMIT_METHODDEF
#endif /* !defined(RESOURCE_PRLIMIT_METHODDEF) */
/*[clinic end generated code: output=8e905b2f5c35170e input=a9049054013a1b77]*/
