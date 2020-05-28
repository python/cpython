/*[clinic input]
preserve
[clinic start generated code]*/

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

    who = _PyLong_AsInt(arg);
    if (who == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = resource_getrusage_impl(module, who);

exit:
    return return_value;
}

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

    resource = _PyLong_AsInt(arg);
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
    {"setrlimit", (PyCFunction)(void(*)(void))resource_setrlimit, METH_FASTCALL, resource_setrlimit__doc__},

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
    resource = _PyLong_AsInt(args[0]);
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
"prlimit(pid, resource, [limits])");

#define RESOURCE_PRLIMIT_METHODDEF    \
    {"prlimit", (PyCFunction)resource_prlimit, METH_VARARGS, resource_prlimit__doc__},

static PyObject *
resource_prlimit_impl(PyObject *module, pid_t pid, int resource,
                      int group_right_1, PyObject *limits);

static PyObject *
resource_prlimit(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    pid_t pid;
    int resource;
    int group_right_1 = 0;
    PyObject *limits = NULL;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "" _Py_PARSE_PID "i:prlimit", &pid, &resource)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "" _Py_PARSE_PID "iO:prlimit", &pid, &resource, &limits)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "resource.prlimit requires 2 to 3 arguments");
            goto exit;
    }
    return_value = resource_prlimit_impl(module, pid, resource, group_right_1, limits);

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

#ifndef RESOURCE_PRLIMIT_METHODDEF
    #define RESOURCE_PRLIMIT_METHODDEF
#endif /* !defined(RESOURCE_PRLIMIT_METHODDEF) */
/*[clinic end generated code: output=ad190fb33d647d1e input=a9049054013a1b77]*/
