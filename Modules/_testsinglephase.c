
/* Testing module for single-phase initialization of extension modules
 */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_namespace.h"     // _PyNamespace_New()


/* Function of two integers returning integer */

PyDoc_STRVAR(testexport_foo_doc,
"foo(i,j)\n\
\n\
Return the sum of i and j.");

static PyObject *
testexport_foo(PyObject *self, PyObject *args)
{
    long i, j;
    long res;
    if (!PyArg_ParseTuple(args, "ll:foo", &i, &j))
        return NULL;
    res = i + j;
    return PyLong_FromLong(res);
}


static PyMethodDef TestMethods[] = {
    {"foo",             testexport_foo,         METH_VARARGS,
        testexport_foo_doc},
    {NULL,              NULL}           /* sentinel */
};


static struct PyModuleDef _testsinglephase = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testsinglephase",
    .m_doc = PyDoc_STR("Test module _testsinglephase (main)"),
    .m_size = -1,  // no module state
    .m_methods = TestMethods,
};


PyMODINIT_FUNC
PyInit__testsinglephase(void)
{
    PyObject *module = PyModule_Create(&_testsinglephase);
    if (module == NULL) {
        return NULL;
    }

    /* Add an exception type */
    PyObject *temp = PyErr_NewException("_testsinglephase.error", NULL, NULL);
    if (temp == NULL) {
        goto error;
    }
    if (PyModule_AddObject(module, "error", temp) != 0) {
        Py_DECREF(temp);
        goto error;
    }

    if (PyModule_AddIntConstant(module, "int_const", 1969) != 0) {
        goto error;
    }

    if (PyModule_AddStringConstant(module, "str_const", "something different") != 0) {
        goto error;
    }

    return module;

error:
    Py_DECREF(module);
    return NULL;
}
