/* This file needs to be kept in sync with the tutorial
 * at Doc/extending/first-extension-module.rst
 */

/// Includes

#include <Python.h>
#include <stdlib.h>     // for system()

/// Implementation of spam.system

static PyObject *
spam_system(PyObject *self, PyObject *arg)
{
   const char *command = PyUnicode_AsUTF8AndSize(arg, NULL);
   if (command == NULL) {
      return NULL;
   }
   int status = system(command);
   PyObject *result = PyLong_FromLong(status);
   return result;
}

/// Module method table

static PyMethodDef spam_methods[] = {
    {
        .ml_name="system",
        .ml_meth=spam_system,
        .ml_flags=METH_O,
        .ml_doc="Execute a shell command.",
    },
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

/// Module slot table

static PyModuleDef_Slot spam_slots[] = {
    {Py_mod_name, "spam"},
    {Py_mod_doc, "A wonderful module with an example function"},
    {Py_mod_methods, spam_methods},
    {0, NULL}
};

/// Export hook prototype

PyMODEXPORT_FUNC PyModExport_spam(void);

/// Module export hook

PyMODEXPORT_FUNC
PyModExport_spam(void)
{
   return spam_slots;
}
