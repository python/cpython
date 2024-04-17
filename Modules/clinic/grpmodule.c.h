/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(grp_getgrgid__doc__,
"getgrgid($module, /, id)\n"
"--\n"
"\n"
"Return the group database entry for the given numeric group ID.\n"
"\n"
"If id is not valid, raise KeyError.");

#define GRP_GETGRGID_METHODDEF    \
    {"getgrgid", (PyCFunction)(void(*)(void))grp_getgrgid, METH_VARARGS|METH_KEYWORDS, grp_getgrgid__doc__},

static PyObject *
grp_getgrgid_impl(PyObject *module, PyObject *id);

// Emit compiler warnings when we get to Python 3.16.
#if PY_VERSION_HEX >= 0x031000C0
#  error "Update the clinic input of 'grp.getgrgid'."
#elif PY_VERSION_HEX >= 0x031000A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'grp.getgrgid'.")
#  else
#    warning "Update the clinic input of 'grp.getgrgid'."
#  endif
#endif

static PyObject *
grp_getgrgid(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"id", NULL};
    Py_ssize_t nargs = PyTuple_Size(args);
    PyObject *id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:getgrgid", _keywords,
        &id))
        goto exit;
    if (nargs < 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'id' to grp.getgrgid() is deprecated. "
                "Parameter 'id' will become positional-only in Python 3.16.", 1))
        {
            goto exit;
        }
    }
    return_value = grp_getgrgid_impl(module, id);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrnam__doc__,
"getgrnam($module, /, name)\n"
"--\n"
"\n"
"Return the group database entry for the given group name.\n"
"\n"
"If name is not valid, raise KeyError.");

#define GRP_GETGRNAM_METHODDEF    \
    {"getgrnam", (PyCFunction)(void(*)(void))grp_getgrnam, METH_VARARGS|METH_KEYWORDS, grp_getgrnam__doc__},

static PyObject *
grp_getgrnam_impl(PyObject *module, PyObject *name);

// Emit compiler warnings when we get to Python 3.16.
#if PY_VERSION_HEX >= 0x031000C0
#  error "Update the clinic input of 'grp.getgrnam'."
#elif PY_VERSION_HEX >= 0x031000A0
#  ifdef _MSC_VER
#    pragma message ("Update the clinic input of 'grp.getgrnam'.")
#  else
#    warning "Update the clinic input of 'grp.getgrnam'."
#  endif
#endif

static PyObject *
grp_getgrnam(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"name", NULL};
    Py_ssize_t nargs = PyTuple_Size(args);
    PyObject *name;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "U:getgrnam", _keywords,
        &name))
        goto exit;
    if (nargs < 1) {
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "Passing keyword argument 'name' to grp.getgrnam() is deprecated."
                " Parameter 'name' will become positional-only in Python 3.16.", 1))
        {
            goto exit;
        }
    }
    return_value = grp_getgrnam_impl(module, name);

exit:
    return return_value;
}

PyDoc_STRVAR(grp_getgrall__doc__,
"getgrall($module, /)\n"
"--\n"
"\n"
"Return a list of all available group entries, in arbitrary order.\n"
"\n"
"An entry whose name starts with \'+\' or \'-\' represents an instruction\n"
"to use YP/NIS and may not be accessible via getgrnam or getgrgid.");

#define GRP_GETGRALL_METHODDEF    \
    {"getgrall", (PyCFunction)grp_getgrall, METH_NOARGS, grp_getgrall__doc__},

static PyObject *
grp_getgrall_impl(PyObject *module);

static PyObject *
grp_getgrall(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return grp_getgrall_impl(module);
}
/*[clinic end generated code: output=1097d5192981d137 input=a9049054013a1b77]*/
