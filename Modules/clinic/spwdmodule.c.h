/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(HAVE_GETSPNAM)

PyDoc_STRVAR(spwd_getspnam__doc__,
"getspnam($module, arg, /)\n"
"--\n"
"\n"
"Return the shadow password database entry for the given user name.\n"
"\n"
"See `help(spwd)` for more on shadow password database entries.");

#define SPWD_GETSPNAM_METHODDEF    \
    {"getspnam", (PyCFunction)spwd_getspnam, METH_O, spwd_getspnam__doc__},

static PyObject *
spwd_getspnam_impl(PyObject *module, PyObject *arg);

static PyObject *
spwd_getspnam(PyObject *module, PyObject *arg_)
{
    PyObject *return_value = NULL;
    PyObject *arg;

    if (!PyUnicode_Check(arg_)) {
        _PyArg_BadArgument("getspnam", "argument", "str", arg_);
        goto exit;
    }
    if (PyUnicode_READY(arg_) == -1) {
        goto exit;
    }
    arg = arg_;
    return_value = spwd_getspnam_impl(module, arg);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSPNAM) */

#if defined(HAVE_GETSPENT)

PyDoc_STRVAR(spwd_getspall__doc__,
"getspall($module, /)\n"
"--\n"
"\n"
"Return a list of all available shadow password database entries, in arbitrary order.\n"
"\n"
"See `help(spwd)` for more on shadow password database entries.");

#define SPWD_GETSPALL_METHODDEF    \
    {"getspall", (PyCFunction)spwd_getspall, METH_NOARGS, spwd_getspall__doc__},

static PyObject *
spwd_getspall_impl(PyObject *module);

static PyObject *
spwd_getspall(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return spwd_getspall_impl(module);
}

#endif /* defined(HAVE_GETSPENT) */

#ifndef SPWD_GETSPNAM_METHODDEF
    #define SPWD_GETSPNAM_METHODDEF
#endif /* !defined(SPWD_GETSPNAM_METHODDEF) */

#ifndef SPWD_GETSPALL_METHODDEF
    #define SPWD_GETSPALL_METHODDEF
#endif /* !defined(SPWD_GETSPALL_METHODDEF) */
/*[clinic end generated code: output=eec8d0bedcd312e5 input=a9049054013a1b77]*/
