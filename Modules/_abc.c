/* ABCMeta implementation */

#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(_abc__doc__,
"_abc module contains (presumably faster) implementation of ABCMeta");
#define DEFERRED_ADDRESS(ADDR) 0

typedef struct {
    PyHeapTypeObject tp;
    PyObject *abc_registry;
    /*PyObject *abc_cache;
    PyObject *abc_negative_cache;
    int abc_negative_cache_version;*/
} abc;

static void
abcmeta_dealloc(abc *tp)
{
    Py_CLEAR(tp->abc_registry);
    PyType_Type.tp_dealloc((PyObject *)tp);
}

static int
abcmeta_traverse(PyObject *self, visitproc visit, void *arg)
{
    Py_VISIT(((abc *)self)->abc_registry);
    return PyType_Type.tp_traverse(self, visit, arg);
}

static int
abcmeta_clear(abc *tp)
{
    PySet_Clear(tp->abc_registry);
    return PyType_Type.tp_clear((PyObject *)tp);
}

static PyObject *
abcmeta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    abc *result = NULL;
    result = (abc *)PyType_Type.tp_new(type, args, kwds);
    if (!result) {
        return NULL;
    }
    result->abc_registry = PySet_New(NULL);
    return (PyObject *)result;
}

static PyObject *
abcmeta_register(abc *self, PyObject *args)
{
    PyObject *subclass = NULL;
    if (!PyArg_UnpackTuple(args, "register", 1, 1, &subclass)) {
        return NULL;
    }
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return NULL;
    }
    PySet_Add(self->abc_registry, subclass);
    Py_INCREF(subclass);
    return subclass;
}

static PyObject *
abcmeta_instancecheck(abc *self, PyObject *args)
{
    PyObject *instance = NULL;
    if (!PyArg_UnpackTuple(args, "__isinstance__", 1, 1, &instance)) {
        return NULL;
    }
    return Py_False;
}

static PyObject *
abcmeta_subclasscheck(abc *self, PyObject *args)
{
    PyObject *subclass = NULL;
    if (!PyArg_UnpackTuple(args, "__issubclass__", 1, 1, &subclass)) {
        return NULL;
    }
    return Py_False;
}

static PyObject *
abcmeta_dump(abc *self, PyObject *args)
{
    PyObject *file = NULL;
    if (!PyArg_UnpackTuple(args, "file", 0, 1, &file)) {
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef abcmeta_methods[] = {
    {"register", (PyCFunction)abcmeta_register, METH_VARARGS,
        PyDoc_STR("register subclass")},
    {"__instancecheck__", (PyCFunction)abcmeta_instancecheck, METH_VARARGS,
        PyDoc_STR("check if instance is an instance of the class")},
    {"__subclasscheck__", (PyCFunction)abcmeta_subclasscheck, METH_VARARGS,
        PyDoc_STR("check if subclass is a subclass of the class")},
    {"_dump_registry", (PyCFunction)abcmeta_dump, METH_VARARGS,
        PyDoc_STR("dump registry (private)")},
    {NULL,      NULL},
};

PyTypeObject ABCMeta = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "ABCMeta",                                  /* tp_name */
    sizeof(abc),                                /* tp_basicsize */
    0,                                          /* tp_itemsize */
    abcmeta_dealloc,                            /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_VERSION_TAG |
        Py_TPFLAGS_BASETYPE | Py_TPFLAGS_TYPE_SUBCLASS,         /* tp_flags */
    0,                                          /* tp_doc */
    abcmeta_traverse,                           /* tp_traverse */
    abcmeta_clear,                              /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    abcmeta_methods,                            /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    DEFERRED_ADDRESS(&PyType_Type),             /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    abcmeta_new,                                /* tp_new */
    0,                                          /* tp_free */
    0,                                          /* tp_is_gc */
};


static struct PyModuleDef _abcmodule = {
    PyModuleDef_HEAD_INIT,
    "_abc",
    _abc__doc__,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__abc(void)
{
    PyObject *m;

    m = PyModule_Create(&_abcmodule);
    if (m == NULL)
        return NULL;
    ABCMeta.tp_base = &PyType_Type;
    if (PyType_Ready(&ABCMeta) < 0) {
        return -1;
    }
    Py_INCREF(&ABCMeta);
    if (PyModule_AddObject(m, "ABCMeta",
                           (PyObject *) &ABCMeta) < 0) {
        return -1;
    }
    return m;
}
