// namespace object implementation

#include "Python.h"
#include "structmember.h"


typedef struct {
    PyObject_HEAD
    PyObject *ns_dict;
} _PyNamespaceObject;


static PyMemberDef namespace_members[] = {
    {"__dict__", T_OBJECT, offsetof(_PyNamespaceObject, ns_dict), READONLY},
    {NULL}
};


// Methods

static PyObject *
namespace_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = type->tp_alloc(type, 0);
    if (self != NULL) {
        _PyNamespaceObject *ns = (_PyNamespaceObject *)self;
        ns->ns_dict = PyDict_New();
        if (ns->ns_dict == NULL) {
            Py_DECREF(ns);
            return NULL;
        }
    }
    return self;
}


static int
namespace_init(_PyNamespaceObject *ns, PyObject *args, PyObject *kwds)
{
    if (PyTuple_GET_SIZE(args) != 0) {
        PyErr_Format(PyExc_TypeError, "no positional arguments expected");
        return -1;
    }
    if (kwds == NULL) {
        return 0;
    }
    if (!PyArg_ValidateKeywordArguments(kwds)) {
        return -1;
    }
    return PyDict_Update(ns->ns_dict, kwds);
}


static void
namespace_dealloc(_PyNamespaceObject *ns)
{
    PyObject_GC_UnTrack(ns);
    Py_CLEAR(ns->ns_dict);
    Py_TYPE(ns)->tp_free((PyObject *)ns);
}


static PyObject *
namespace_repr(PyObject *ns)
{
    int i, loop_error = 0;
    PyObject *pairs = NULL, *d = NULL, *keys = NULL, *keys_iter = NULL;
    PyObject *key;
    PyObject *separator, *pairsrepr, *repr = NULL;
    const char * name;

    name = Py_IS_TYPE(ns, &_PyNamespace_Type) ? "namespace"
                                               : Py_TYPE(ns)->tp_name;

    i = Py_ReprEnter(ns);
    if (i != 0) {
        return i > 0 ? PyUnicode_FromFormat("%s(...)", name) : NULL;
    }

    pairs = PyList_New(0);
    if (pairs == NULL)
        goto error;

    d = ((_PyNamespaceObject *)ns)->ns_dict;
    assert(d != NULL);
    Py_INCREF(d);

    keys = PyDict_Keys(d);
    if (keys == NULL)
        goto error;
    if (PyList_Sort(keys) != 0)
        goto error;

    keys_iter = PyObject_GetIter(keys);
    if (keys_iter == NULL)
        goto error;

    while ((key = PyIter_Next(keys_iter)) != NULL) {
        if (PyUnicode_Check(key) && PyUnicode_GET_LENGTH(key) > 0) {
            PyObject *value, *item;

            value = PyDict_GetItemWithError(d, key);
            if (value != NULL) {
                item = PyUnicode_FromFormat("%U=%R", key, value);
                if (item == NULL) {
                    loop_error = 1;
                }
                else {
                    loop_error = PyList_Append(pairs, item);
                    Py_DECREF(item);
                }
            }
            else if (PyErr_Occurred()) {
                loop_error = 1;
            }
        }

        Py_DECREF(key);
        if (loop_error)
            goto error;
    }

    separator = PyUnicode_FromString(", ");
    if (separator == NULL)
        goto error;

    pairsrepr = PyUnicode_Join(separator, pairs);
    Py_DECREF(separator);
    if (pairsrepr == NULL)
        goto error;

    repr = PyUnicode_FromFormat("%s(%S)", name, pairsrepr);
    Py_DECREF(pairsrepr);

error:
    Py_XDECREF(pairs);
    Py_XDECREF(d);
    Py_XDECREF(keys);
    Py_XDECREF(keys_iter);
    Py_ReprLeave(ns);

    return repr;
}


static int
namespace_traverse(_PyNamespaceObject *ns, visitproc visit, void *arg)
{
    Py_VISIT(ns->ns_dict);
    return 0;
}


static int
namespace_clear(_PyNamespaceObject *ns)
{
    Py_CLEAR(ns->ns_dict);
    return 0;
}


static PyObject *
namespace_richcompare(PyObject *self, PyObject *other, int op)
{
    if (PyObject_TypeCheck(self, &_PyNamespace_Type) &&
        PyObject_TypeCheck(other, &_PyNamespace_Type))
        return PyObject_RichCompare(((_PyNamespaceObject *)self)->ns_dict,
                                   ((_PyNamespaceObject *)other)->ns_dict, op);
    Py_RETURN_NOTIMPLEMENTED;
}


PyDoc_STRVAR(namespace_reduce__doc__, "Return state information for pickling");

static PyObject *
namespace_reduce(_PyNamespaceObject *ns, PyObject *Py_UNUSED(ignored))
{
    PyObject *result, *args = PyTuple_New(0);

    if (!args)
        return NULL;

    result = PyTuple_Pack(3, (PyObject *)Py_TYPE(ns), args, ns->ns_dict);
    Py_DECREF(args);
    return result;
}


static PyMethodDef namespace_methods[] = {
    {"__reduce__", (PyCFunction)namespace_reduce, METH_NOARGS,
     namespace_reduce__doc__},
    {NULL,         NULL}  // sentinel
};


PyDoc_STRVAR(namespace_doc,
"A simple attribute-based namespace.\n\
\n\
SimpleNamespace(**kwargs)");

PyTypeObject _PyNamespace_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "types.SimpleNamespace",                    /* tp_name */
    sizeof(_PyNamespaceObject),                 /* tp_basicsize */
    0,                                          /* tp_itemsize */
    (destructor)namespace_dealloc,              /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)namespace_repr,                   /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE,                    /* tp_flags */
    namespace_doc,                              /* tp_doc */
    (traverseproc)namespace_traverse,           /* tp_traverse */
    (inquiry)namespace_clear,                   /* tp_clear */
    namespace_richcompare,                      /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    namespace_methods,                          /* tp_methods */
    namespace_members,                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(_PyNamespaceObject, ns_dict),      /* tp_dictoffset */
    (initproc)namespace_init,                   /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    (newfunc)namespace_new,                     /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};


PyObject *
_PyNamespace_New(PyObject *kwds)
{
    PyObject *ns = namespace_new(&_PyNamespace_Type, NULL, NULL);
    if (ns == NULL)
        return NULL;

    if (kwds == NULL)
        return ns;
    if (PyDict_Update(((_PyNamespaceObject *)ns)->ns_dict, kwds) != 0) {
        Py_DECREF(ns);
        return NULL;
    }

    return (PyObject *)ns;
}
