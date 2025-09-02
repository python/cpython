#include "parts.h"


static PyObject *
test_PyOS_mystrnicmp(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(PyOS_mystrnicmp("", "", 0) == 0);
    assert(PyOS_mystrnicmp("", "", 1) == 0);

    assert(PyOS_mystrnicmp("insert", "ins", 3) == 0);
    assert(PyOS_mystrnicmp("ins", "insert", 3) == 0);
    assert(PyOS_mystrnicmp("insect", "insert", 3) == 0);

    assert(PyOS_mystrnicmp("insert", "insert", 6) == 0);
    assert(PyOS_mystrnicmp("Insert", "insert", 6) == 0);
    assert(PyOS_mystrnicmp("INSERT", "insert", 6) == 0);
    assert(PyOS_mystrnicmp("insert", "insert", 10) == 0);

    assert(PyOS_mystrnicmp("invert", "insert", 6) == ('v' - 's'));
    assert(PyOS_mystrnicmp("insert", "invert", 6) == ('s' - 'v'));
    assert(PyOS_mystrnicmp("insert", "ins\0rt", 6) == 'e');

    // GH-21845
    assert(PyOS_mystrnicmp("insert\0a", "insert\0b", 8) == 0);

    Py_RETURN_NONE;
}

static PyObject *
test_PyOS_mystricmp(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    assert(PyOS_mystricmp("", "") == 0);
    assert(PyOS_mystricmp("insert", "insert") == 0);
    assert(PyOS_mystricmp("Insert", "insert") == 0);
    assert(PyOS_mystricmp("INSERT", "insert") == 0);
    assert(PyOS_mystricmp("insert", "ins") == 'e');
    assert(PyOS_mystricmp("ins", "insert") == -'e');

    // GH-21845
    assert(PyOS_mystricmp("insert", "ins\0rt") == 'e');
    assert(PyOS_mystricmp("invert", "insert") == ('v' - 's'));

    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"test_PyOS_mystrnicmp", test_PyOS_mystrnicmp, METH_NOARGS, NULL},
    {"test_PyOS_mystricmp", test_PyOS_mystricmp, METH_NOARGS, NULL},
    {NULL},
};

int
_PyTestLimitedCAPI_Init_PyOS(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }

    return 0;
}
