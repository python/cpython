#include "Python.h"

typedef struct {
    PyObject_HEAD
    PyObject *container;
} MyObject;

static int
my_traverse(MyObject *self, visitproc visit, void *arg)
{
    if (self->container != NULL)
        return visit(self->container, arg);
    else
        return 0;
}

static int
my_clear(MyObject *self)
{
    Py_XDECREF(self->container);
    self->container = NULL;

    return 0;
}

static void
my_dealloc(MyObject *self)
{
    PyObject_GC_UnTrack((PyObject *) self);
    Py_XDECREF(self->container);
    PyObject_GC_Del(self);
}

static PyTypeObject
MyObject_Type = {
    PyObject_HEAD_INIT(NULL)
    0,
    "MyObject",
    sizeof(MyObject),
    0,
    (destructor)my_dealloc,     /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    0,                          /* tp_doc */
    (traverseproc)my_traverse,  /* tp_traverse */
    (inquiry)my_clear,          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
};

/* This constructor should be made accessible from Python. */
static PyObject *
new_object(PyObject *unused, PyObject *args)
{
    PyObject *container = NULL;
    MyObject *result = NULL;

    if (PyArg_ParseTuple(args, "|O:new_object", &container)) {
        result = PyObject_GC_New(MyObject, &MyObject_Type);
        if (result != NULL) {
            result->container = container;
            PyObject_GC_Track(result);
        }
    }
    return (PyObject *) result;
}
