
/// Includes

#include <Python.h>
#include <stdlib.h>

/// Implementation of spam.system

static PyObject *
spam_system(PyObject *self, PyObject *arg)
{
    const char *command = PyUnicode_AsUTF8(arg);
    if (command == NULL) {
        return NULL;
    }
    int status = system(command);
    return PyLong_FromLong(status);
}


/// Module method table

static PyMethodDef spam_methods[] = {
    {
        .ml_name="system",
        .ml_meth=spam_system,
        .ml_flags=METH_O,
        .ml_doc=PyDoc_STR("Execute a shell command."),
    },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

/// ABI information

PyABIInfo_VAR(abi_info);


/// Module slot table

static PyModuleDef_Slot spam_module[] = {
    {Py_mod_abi, &abi_info},
    {Py_mod_name, "spam"},
    {Py_mod_doc, PyDoc_STR("A wonderful module with an example function")},
    {Py_mod_methods, spam_methods},
    {0, NULL}  /* Sentinel */
};


/// Export hook prototype

PyMODEXPORT_FUNC PyModExport_spam(void);

/// Module export hook

PyMODEXPORT_FUNC
PyModExport_spam(void)
{
    return spam_module;
}


/// Legacy initialization function

PyMODINIT_FUNC PyInit_spam(void);

PyMODINIT_FUNC
PyInit_spam(void)
{
    PyErr_SetString(
        PyExc_SystemError,
        "incompatible extension, need Python 3.15 or higher");
    return NULL;
}
