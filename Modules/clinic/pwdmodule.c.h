/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(pwd_getpwuid__doc__,
"getpwuid($module, uidobj, /)\n"
"--\n"
"\n"
"Return the password database entry for the given numeric user ID.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWUID_METHODDEF    \
    {"getpwuid", (PyCFunction)pwd_getpwuid, METH_O, pwd_getpwuid__doc__},

PyDoc_STRVAR(pwd_getpwnam__doc__,
"getpwnam($module, arg, /)\n"
"--\n"
"\n"
"Return the password database entry for the given user name.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWNAM_METHODDEF    \
    {"getpwnam", (PyCFunction)pwd_getpwnam, METH_O, pwd_getpwnam__doc__},

static PyObject *
pwd_getpwnam_impl(PyModuleDef *module, PyObject *arg);

static PyObject *
pwd_getpwnam(PyModuleDef *module, PyObject *arg_)
{
    PyObject *return_value = NULL;
    PyObject *arg;

    if (!PyArg_Parse(arg_, "U:getpwnam", &arg))
        goto exit;
    return_value = pwd_getpwnam_impl(module, arg);

exit:
    return return_value;
}

#if defined(HAVE_GETPWENT)

PyDoc_STRVAR(pwd_getpwall__doc__,
"getpwall($module, /)\n"
"--\n"
"\n"
"Return a list of all available password database entries, in arbitrary order.\n"
"\n"
"See help(pwd) for more on password database entries.");

#define PWD_GETPWALL_METHODDEF    \
    {"getpwall", (PyCFunction)pwd_getpwall, METH_NOARGS, pwd_getpwall__doc__},

static PyObject *
pwd_getpwall_impl(PyModuleDef *module);

static PyObject *
pwd_getpwall(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return pwd_getpwall_impl(module);
}

#endif /* defined(HAVE_GETPWENT) */

#ifndef PWD_GETPWALL_METHODDEF
    #define PWD_GETPWALL_METHODDEF
#endif /* !defined(PWD_GETPWALL_METHODDEF) */
/*[clinic end generated code: output=2ed0ecf34fd3f98f input=a9049054013a1b77]*/
