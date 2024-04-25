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
    {"getgrgid", _PyCFunction_CAST(grp_getgrgid), METH_FASTCALL|METH_KEYWORDS, grp_getgrgid__doc__},

static PyObject *
grp_getgrgid_impl(PyObject *module, PyObject *id);

static PyObject *
grp_getgrgid(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"id", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "getgrgid", 0};
    PyObject *argsbuf[1];
    PyObject *id;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    id = args[0];
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
    {"getgrnam", _PyCFunction_CAST(grp_getgrnam), METH_FASTCALL|METH_KEYWORDS, grp_getgrnam__doc__},

static PyObject *
grp_getgrnam_impl(PyObject *module, PyObject *name);

static PyObject *
grp_getgrnam(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "getgrnam", 0};
    PyObject *argsbuf[1];
    PyObject *name;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("getgrnam", "argument 'name'", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    name = args[0];
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
/*[clinic end generated code: output=ba680465f71ed779 input=a9049054013a1b77]*/
