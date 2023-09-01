#include "Python.h"
#include "structmember.h"         // PyMemberDef

PyDoc_STRVAR(xxsubtype__doc__,
"xxsubtype is an example module showing how to subtype builtin types from C.\n"
"test_descr.py in the standard test suite requires it in order to complete.\n"
"If you don't care about the examples, and don't intend to run the Python\n"
"test suite, you can recompile Python without Modules/xxsubtype.c.");

/* We link this module statically for convenience.  If compiled as a shared
   library instead, some compilers don't allow addresses of Python objects
   defined in other libraries to be used in static initializers here.  The
   DEFERRED_ADDRESS macro is used to tag the slots where such addresses
   appear; the module init function must fill in the tagged slots at runtime.
   The argument is for documentation -- the macro ignores it.
*/
#define DEFERRED_ADDRESS(ADDR) 0

/* spamlist -- a list subtype */

typedef struct {
    PyListObject list;
    int state;
} spamlistobject;

static PyObject *
spamlist_getstate(spamlistobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":getstate"))
        return NULL;
    return PyLong_FromLong(self->state);
}

static PyObject *
spamlist_setstate(spamlistobject *self, PyObject *args)
{
    int state;

    if (!PyArg_ParseTuple(args, "i:setstate", &state))
        return NULL;
    self->state = state;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
spamlist_specialmeth(PyObject *self, PyObject *args, PyObject *kw)
{
    PyObject *result = PyTuple_New(3);

    if (result != NULL) {
        if (self == NULL)
            self = Py_None;
        if (kw == NULL)
            kw = Py_None;
        Py_INCREF(self);
        PyTuple_SET_ITEM(result, 0, self);
        Py_INCREF(args);
        PyTuple_SET_ITEM(result, 1, args);
        Py_INCREF(kw);
        PyTuple_SET_ITEM(result, 2, kw);
    }
    return result;
}

static PyMethodDef spamlist_methods[] = {
    {"getstate", (PyCFunction)spamlist_getstate, METH_VARARGS,
        PyDoc_STR("getstate() -> state")},
    {"setstate", (PyCFunction)spamlist_setstate, METH_VARARGS,
        PyDoc_STR("setstate(state)")},
    /* These entries differ only in the flags; they are used by the tests
       in test.test_descr. */
    {"classmeth", _PyCFunction_CAST(spamlist_specialmeth),
        METH_VARARGS | METH_KEYWORDS | METH_CLASS,
        PyDoc_STR("classmeth(*args, **kw)")},
    {"staticmeth", _PyCFunction_CAST(spamlist_specialmeth),
        METH_VARARGS | METH_KEYWORDS | METH_STATIC,
        PyDoc_STR("staticmeth(*args, **kw)")},
    {NULL,      NULL},
};

static int
spamlist_init(spamlistobject *self, PyObject *args, PyObject *kwds)
{
    if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->state = 0;
    return 0;
}

static PyObject *
spamlist_state_get(spamlistobject *self, void *Py_UNUSED(ignored))
{
    return PyLong_FromLong(self->state);
}

static PyGetSetDef spamlist_getsets[] = {
    {"state", (getter)spamlist_state_get, NULL,
     PyDoc_STR("an int variable for demonstration purposes")},
    {0}
};

static PyTypeObject spamlist_type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "xxsubtype.spamlist",
    sizeof(spamlistobject),
    0,
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    spamlist_methods,                           /* tp_methods */
    0,                                          /* tp_members */
    spamlist_getsets,                           /* tp_getset */
    DEFERRED_ADDRESS(&PyList_Type),             /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)spamlist_init,                    /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};

/* spamdict -- a dict subtype */

typedef struct {
    PyDictObject dict;
    int state;
} spamdictobject;

static PyObject *
spamdict_getstate(spamdictobject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":getstate"))
        return NULL;
    return PyLong_FromLong(self->state);
}

static PyObject *
spamdict_setstate(spamdictobject *self, PyObject *args)
{
    int state;

    if (!PyArg_ParseTuple(args, "i:setstate", &state))
        return NULL;
    self->state = state;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef spamdict_methods[] = {
    {"getstate", (PyCFunction)spamdict_getstate, METH_VARARGS,
        PyDoc_STR("getstate() -> state")},
    {"setstate", (PyCFunction)spamdict_setstate, METH_VARARGS,
        PyDoc_STR("setstate(state)")},
    {NULL,      NULL},
};

static int
spamdict_init(spamdictobject *self, PyObject *args, PyObject *kwds)
{
    if (PyDict_Type.tp_init((PyObject *)self, args, kwds) < 0)
        return -1;
    self->state = 0;
    return 0;
}

static PyMemberDef spamdict_members[] = {
    {"state", T_INT, offsetof(spamdictobject, state), READONLY,
     PyDoc_STR("an int variable for demonstration purposes")},
    {0}
};

static PyTypeObject spamdict_type = {
    PyVarObject_HEAD_INIT(DEFERRED_ADDRESS(&PyType_Type), 0)
    "xxsubtype.spamdict",
    sizeof(spamdictobject),
    0,
    0,                                          /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    spamdict_methods,                           /* tp_methods */
    spamdict_members,                           /* tp_members */
    0,                                          /* tp_getset */
    DEFERRED_ADDRESS(&PyDict_Type),             /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)spamdict_init,                    /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};

static PyObject *
spam_bench(PyObject *self, PyObject *args)
{
    PyObject *obj, *name, *res;
    int n = 1000;
    time_t t0 = 0, t1 = 0;

    if (!PyArg_ParseTuple(args, "OU|i", &obj, &name, &n))
        return NULL;
#ifdef HAVE_CLOCK
    t0 = clock();
    while (--n >= 0) {
        res = PyObject_GetAttr(obj, name);
        if (res == NULL)
            return NULL;
        Py_DECREF(res);
    }
    t1 = clock();
#endif
    return PyFloat_FromDouble((double)(t1-t0) / CLOCKS_PER_SEC);
}

static PyMethodDef xxsubtype_functions[] = {
    {"bench",           spam_bench,     METH_VARARGS},
    {NULL,              NULL}           /* sentinel */
};

static int
xxsubtype_exec(PyObject* m)
{
    /* Fill in deferred data addresses.  This must be done before
       PyType_Ready() is called.  Note that PyType_Ready() automatically
       initializes the ob.ob_type field to &PyType_Type if it's NULL,
       so it's not necessary to fill in ob_type first. */
    spamdict_type.tp_base = &PyDict_Type;
    if (PyType_Ready(&spamdict_type) < 0)
        return -1;

    spamlist_type.tp_base = &PyList_Type;
    if (PyType_Ready(&spamlist_type) < 0)
        return -1;

    if (PyType_Ready(&spamlist_type) < 0)
        return -1;
    if (PyType_Ready(&spamdict_type) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "spamlist",
                              (PyObject *) &spamlist_type) < 0)
        return -1;

    if (PyModule_AddObjectRef(m, "spamdict",
                              (PyObject *) &spamdict_type) < 0)
        return -1;
    return 0;
}

static struct PyModuleDef_Slot xxsubtype_slots[] = {
    {Py_mod_exec, xxsubtype_exec},
    {0, NULL},
};

static struct PyModuleDef xxsubtypemodule = {
    PyModuleDef_HEAD_INIT,
    "xxsubtype",
    xxsubtype__doc__,
    0,
    xxsubtype_functions,
    xxsubtype_slots,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit_xxsubtype(void)
{
    return PyModuleDef_Init(&xxsubtypemodule);
}
