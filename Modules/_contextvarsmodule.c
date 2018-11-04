#include "Python.h"

#include "clinic/_contextvarsmodule.c.h"

/*[clinic input]
module _contextvars
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a0955718c8b8cea6]*/


/*[clinic input]
_contextvars.copy_context
[clinic start generated code]*/

static PyObject *
_contextvars_copy_context_impl(PyObject *module)
/*[clinic end generated code: output=1fcd5da7225c4fa9 input=89bb9ae485888440]*/
{
    return PyContext_CopyCurrent();
}


PyDoc_STRVAR(module_doc, "Context Variables");

static PyMethodDef _contextvars_methods[] = {
    _CONTEXTVARS_COPY_CONTEXT_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef _contextvarsmodule = {
    PyModuleDef_HEAD_INIT,      /* m_base */
    "_contextvars",             /* m_name */
    module_doc,                 /* m_doc */
    -1,                         /* m_size */
    _contextvars_methods,       /* m_methods */
    NULL,                       /* m_slots */
    NULL,                       /* m_traverse */
    NULL,                       /* m_clear */
    NULL,                       /* m_free */
};

PyMODINIT_FUNC
PyInit__contextvars(void)
{
    PyObject *m = PyModule_Create(&_contextvarsmodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&PyContext_Type);
    if (PyModule_AddObject(m, "Context",
                           (PyObject *)&PyContext_Type) < 0)
    {
        Py_DECREF(&PyContext_Type);
        return NULL;
    }

    Py_INCREF(&PyContextVar_Type);
    if (PyModule_AddObject(m, "ContextVar",
                           (PyObject *)&PyContextVar_Type) < 0)
    {
        Py_DECREF(&PyContextVar_Type);
        return NULL;
    }

    Py_INCREF(&PyContextToken_Type);
    if (PyModule_AddObject(m, "Token",
                           (PyObject *)&PyContextToken_Type) < 0)
    {
        Py_DECREF(&PyContextToken_Type);
        return NULL;
    }

    return m;
}
