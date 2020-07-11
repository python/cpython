#include<Python.h>

typedef struct {
    PyObject_HEAD
    PyObject *test;
}testobject;

static void
test_dealloc(testobject *obj)
{
    Py_XDECREF(obj->test);
}

static PyObject*
test_repr(testobject * obj)
{
    return NULL;
}

static int
test_traverse(testobject *obj, visitproc visit, void *arg)
{
    Py_VISIT(obj->test);
}

static PyObject *
test_new(PyTypeObject *type, PyObject *args, PyObject *kw)
{
    return NULL;
}

PyDoc_STRVAR(test_doc, "test get typeslots");

static PyTypeObject test_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "test",                             /* tp_name */
    sizeof(testobject),                 /* tp_basicsize */
    0,                                  /* tp_itemsize */
    /* methods */
    (destructor)test_dealloc,           /* tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    (reprfunc)test_repr,                /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    test_doc,                           /* tp_doc */
    (traverseproc)test_traverse,        /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    0,                                  /* tp_alloc */
    test_new,                           /* tp_new */
    PyObject_GC_Del,                    /* tp_free */
};

static struct PyModuleDef _gettypeslots = {
    PyModuleDef_HEAD_INIT,
    "_testgettypeslots",
    "_testgettypeslots doc",
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__testgettypeslots(void)
{
    PyObject *module = PyModule_Create(&_gettypeslots);
    if (module == NULL) {
        return NULL;
    }
    if (PyModule_AddType(module, &test_type) < 0) {
        Py_DECREF(module);
        return NULL;
    }
    char *tp_name = PyType_GetSlot(&test_type, Py_tp_name);
    unsigned long tp_flags = (unsigned long)PyType_GetSlot(
            &test_type, Py_tp_flags);
    newfunc tp_new = PyType_GetSlot(&test_type, Py_tp_new);
    freefunc tp_free = PyType_GetSlot(&test_type, Py_tp_free);
    return module;
}
