#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

/* Always enable assertions */
#undef NDEBUG

#define PY_SSIZE_T_CLEAN

#include "Python.h"

#include "clinic/_testclinic.c.h"

/*[clinic input]
module  _testclinic
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=d4981b80d6efdb12]*/

/*[clinic input]
test_empty_function

[clinic start generated code]*/

static PyObject *
test_empty_function_impl(PyObject *module)
/*[clinic end generated code: output=0f8aeb3ddced55cb input=0dd7048651ad4ae4]*/
{
    Py_RETURN_NONE;
};

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

[clinic start generated code]*/

static PyObject *
gh_32092_oob_impl(PyObject *module, PyObject *pos1, PyObject *pos2,
                  PyObject *varargs, PyObject *kw1, PyObject *kw2)
/*[clinic end generated code: output=ee259c130054653f input=568c6276e3fdef62]*/
{
    Py_RETURN_NONE;
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

[clinic start generated code]*/

static PyObject *
gh_32092_kw_pass_impl(PyObject *module, PyObject *pos, PyObject *args,
                      PyObject *kw)
/*[clinic end generated code: output=4a2bbe4f7c8604e9 input=5bfe6191e1e7a2fb]*/
{
    Py_RETURN_NONE;
}

static PyMethodDef tester_methods[] = {
    TEST_EMPTY_FUNCTION_METHODDEF
    GH_32092_OOB_METHODDEF
    GH_32092_KW_PASS_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef _testclinic_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testclinic",
    .m_size = 0,
    .m_methods = tester_methods,
};

PyMODINIT_FUNC
PyInit__testclinic(void)
{
    return PyModule_Create(&_testclinic_module);
}
