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
    {"getspnam", (PyCFunction)spwd_getspnam, METH_VARARGS, spwd_getspnam__doc__},

static PyObject *
spwd_getspnam_impl(PyModuleDef *module, PyObject *arg);

static PyObject *
spwd_getspnam(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *arg;

    if (!PyArg_ParseTuple(args,
        "U:getspnam",
        &arg))
        goto exit;
    return_value = spwd_getspnam_impl(module, arg);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETSPNAM) */

#ifndef SPWD_GETSPNAM_METHODDEF
    #define SPWD_GETSPNAM_METHODDEF
#endif /* !defined(SPWD_GETSPNAM_METHODDEF) */

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
spwd_getspall_impl(PyModuleDef *module);

static PyObject *
spwd_getspall(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return spwd_getspall_impl(module);
}

#endif /* defined(HAVE_GETSPENT) */

#ifndef SPWD_GETSPALL_METHODDEF
    #define SPWD_GETSPALL_METHODDEF
#endif /* !defined(SPWD_GETSPALL_METHODDEF) */
/*[clinic end generated code: output=41fec4a15b0cd2a0 input=a9049054013a1b77]*/
