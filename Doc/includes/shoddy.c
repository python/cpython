#include <Python.h>

typedef struct {
    PyListObject list;
    int state;
} ShoddyObject;

static PyObject *
Shoddy_increment(ShoddyObject *self, PyObject *unused)
{
    self->state++;
    return PyLong_FromLong(self->state);
}

static PyMethodDef Shoddy_methods[] = {
    {"increment", (PyCFunction) Shoddy_increment, METH_NOARGS,
     PyDoc_STR("increment state counter")},
    {NULL},
};

static int
Shoddy_init(ShoddyObject *self, PyObject *args, PyObject *kwds)
{
    if (PyList_Type.tp_init((PyObject *) self, args, kwds) < 0)
        return -1;
    self->state = 0;
    return 0;
}

static PyTypeObject ShoddyType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "shoddy.Shoddy",
    .tp_doc = "Shoddy objects",
    .tp_basicsize = sizeof(ShoddyObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_init = (initproc) Shoddy_init,
    .tp_methods = Shoddy_methods,
};

static PyModuleDef shoddymodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "shoddy",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_shoddy(void)
{
    PyObject *m;
    ShoddyType.tp_base = &PyList_Type;
    if (PyType_Ready(&ShoddyType) < 0)
        return NULL;

    m = PyModule_Create(&shoddymodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&ShoddyType);
    PyModule_AddObject(m, "Shoddy", (PyObject *) &ShoddyType);
    return m;
}
