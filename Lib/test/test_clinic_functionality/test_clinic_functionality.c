#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "clinic/test_clinic_functionality.c.h"


/*[clinic input]
module  clinic_functional_tester
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2ee8b0b242501b11]*/

/*[clinic input]
gh_32092_oob

    pos1: object
    pos2: object
    *varargs: object
    kw1: object = None
    kw2: object = None

Proof-of-concept of GH-32092 OOB bug.

Array index out-of-bound bug in function
`_PyArg_UnpackKeywordsWithVararg` .

Calling this function by gh_32092_oob(1, 2, 3, 4, kw1=5, kw2=6)
to trigger this bug (crash).
Expected return: (1, 2, (3, 4), 5, 6)

[clinic start generated code]*/

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2)
/*[clinic end generated code: output=ee259c130054653f input=91d8e227acf93b02]*/
{
    PyObject *tuple = PyTuple_New(5);;
    PyTuple_SET_ITEM(tuple, 0, Py_NewRef(pos1));
    PyTuple_SET_ITEM(tuple, 1, Py_NewRef(pos2));
    PyTuple_SET_ITEM(tuple, 2, Py_NewRef(varargs));
    PyTuple_SET_ITEM(tuple, 3, Py_NewRef(kw1));
    PyTuple_SET_ITEM(tuple, 4, Py_NewRef(kw2));
    return tuple;
}

/*[clinic input]
gh_32092_kw_pass

    pos: object
    *args: object
    kw: object = None

Proof-of-concept of GH-32092 keyword args passing bug.

The calculation of `noptargs` in AC-generated function
`builtin_kw_pass_poc` is incorrect.

Calling this function by gh_32092_kw_pass(1, 2, 3)
to trigger this bug (crash).
Expected return: (1, (2, 3))

[clinic start generated code]*/

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw)
/*[clinic end generated code: output=4a2bbe4f7c8604e9 input=c51b7572ac09f193]*/
{
    PyObject *tuple = PyTuple_New(3);;
    PyTuple_SET_ITEM(tuple, 0, Py_NewRef(pos));
    PyTuple_SET_ITEM(tuple, 1, Py_NewRef(args));
    PyTuple_SET_ITEM(tuple, 2, Py_NewRef(kw));
    return tuple;
}

static PyMethodDef tester_methods[] = {
    GH_32092_OOB_METHODDEF
    GH_32092_KW_PASS_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef clinic_functional_tester_module = {
        PyModuleDef_HEAD_INIT,
        "clinic_functional_tester",
        NULL,
        0,
        tester_methods,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit_test_clinic_functionality(void)
{
    return PyModule_Create(&clinic_functional_tester_module);
}
