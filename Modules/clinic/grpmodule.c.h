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
    {"getgrgid", (PyCFunction)grp_getgrgid, METH_VARARGS|METH_KEYWORDS, grp_getgrgid__doc__},

static PyObject *
grp_getgrgid_impl(PyModuleDef *module, PyObject *id);

static PyObject *
grp_getgrgid(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"id", NULL};
    PyObject *id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:getgrgid", _keywords,
        &id))
        goto exit;
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
    {"getgrnam", (PyCFunction)grp_getgrnam, METH_VARARGS|METH_KEYWORDS, grp_getgrnam__doc__},

static PyObject *
grp_getgrnam_impl(PyModuleDef *module, PyObject *name);

static PyObject *
grp_getgrnam(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"name", NULL};
    PyObject *name;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "U:getgrnam", _keywords,
        &name))
        goto exit;
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
grp_getgrall_impl(PyModuleDef *module);

static PyObject *
grp_getgrall(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return grp_getgrall_impl(module);
}
/*[clinic end generated code: output=5191c25600afb1bd input=a9049054013a1b77]*/
