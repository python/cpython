#include <Python.h>

typedef struct {
    PyObject_HEAD
    /* Type-specific fields go here. */
} NoddyObject;

static PyTypeObject NoddyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "noddy.Noddy",
    .tp_doc = "Noddy objects",
    .tp_basicsize = sizeof(NoddyObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
};

static PyModuleDef noddymodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "noddy",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_noddy(void)
{
    PyObject *m;
    if (PyType_Ready(&NoddyType) < 0)
        return NULL;

    m = PyModule_Create(&noddymodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&NoddyType);
    PyModule_AddObject(m, "Noddy", (PyObject *) &NoddyType);
    return m;
}
