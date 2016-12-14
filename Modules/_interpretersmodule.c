
/* interpreters module */
/* low-level access to interpreter primitives */

#include "Python.h"


static PyMethodDef module_functions[] = {
    {NULL, NULL}           /* sentinel */
};


/* initialization function */

PyDoc_STRVAR(module_doc,
"This module provides primitive operations to manage Python interpreters.\n\
The 'interpreters' module provides a more convenient interface.");

static struct PyModuleDef interpretersmodule = {
    PyModuleDef_HEAD_INIT,
    "_interpreters",
    module_doc,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__interpreters(void)
{
    PyObject *module;

    module = PyModule_Create(&interpretersmodule);
    if (module == NULL)
        return NULL;


    return module;
}
