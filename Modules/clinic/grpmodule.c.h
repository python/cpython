/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

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

static PyObject *
grp_getgrgid(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"id", NULL};
    PyObject *id;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:getgrgid", _keywords,
        &id))
        goto exit;
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = grp_getgrgid_impl(module, id);
    Py_END_CRITICAL_SECTION();

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

static PyObject *
grp_getgrnam(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"name", NULL};
    PyObject *name;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "U:getgrnam", _keywords,
        &name))
        goto exit;
    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = grp_getgrnam_impl(module, name);
    Py_END_CRITICAL_SECTION();

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
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(module);
    return_value = grp_getgrall_impl(module);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=dfcdb3a6951a1805 input=a9049054013a1b77]*/
