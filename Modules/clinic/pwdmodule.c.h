/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


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
"getpwnam($module, name, /)\n"
"--\n"
"\n"
"Return the password database entry for the given user name.\n"
"\n"
"See `help(pwd)` for more on password database entries.");

#define PWD_GETPWNAM_METHODDEF    \
    {"getpwnam", (PyCFunction)pwd_getpwnam, METH_O, pwd_getpwnam__doc__},

static PyObject *
pwd_getpwnam_impl(PyObject *module, PyObject *name);

static PyObject *
pwd_getpwnam(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *name;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("getpwnam", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    name = arg;
    return_value = pwd_getpwnam_impl(module, name);

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
pwd_getpwall_impl(PyObject *module);

static PyObject *
pwd_getpwall(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return pwd_getpwall_impl(module);
}

#endif /* defined(HAVE_GETPWENT) */

#ifndef PWD_GETPWALL_METHODDEF
    #define PWD_GETPWALL_METHODDEF
#endif /* !defined(PWD_GETPWALL_METHODDEF) */
/*[clinic end generated code: output=a95bc08653cda56b input=a9049054013a1b77]*/
