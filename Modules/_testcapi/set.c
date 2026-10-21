#include "parts.h"
#include "util.h"

static PyObject *
set_get_size(PyObject *self, PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PySet_GET_SIZE(obj));
}


static PyObject*
test_set_type_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }

    // Ensure that following tests don't modify the object,
    // to ensure that Py_DECREF() will not crash.
    assert(Py_TYPE(obj) == &PyList_Type);
    assert(Py_SIZE(obj) == 0);

    // bpo-39573: Test Py_SET_TYPE() and Py_SET_SIZE() functions.
    Py_SET_TYPE(obj, &PyList_Type);
    Py_SET_SIZE(obj, 0);

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


typedef struct {
    PyObject_HEAD
    PyFrozenSetWriter *writer;
} WriterObject;


static inline int
writer_check(WriterObject *self)
{
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "operation on finished/discarded writer");
        return -1;
    }
    return 0;
}


static PyObject *
writer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->writer = NULL;
    return (PyObject*)self;
}


// Test PyFrozenSetWriter_Create()
static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    if (PyTuple_GET_SIZE(args)) {
        PyErr_Format(PyExc_TypeError,
                     "PyFrozenSetWriter() takes no positional arguments");
        return -1;
    }
    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyFrozenSetWriter() takes no keyword arguments");
        return -1;
    }

    WriterObject *self = (WriterObject *)self_raw;
    PyFrozenSetWriter_Discard(self->writer);

    self->writer = PyFrozenSetWriter_Create();
    if (self->writer == NULL) {
        return -1;
    }

    return 0;
}


// Test PyFrozenSetWriter_Discard()
static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyFrozenSetWriter_Discard(self->writer);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


// Test PyFrozenSetWriter_Add()
static PyObject*
writer_add(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyFrozenSetWriter_Add(self->writer, item) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyFrozenSetWriter_Update()
static PyObject*
writer_update(PyObject *self_raw, PyObject *iterable)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyFrozenSetWriter_Update(self->writer, iterable) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyFrozenSetWriter_Finish()
static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *frozenset = PyFrozenSetWriter_Finish(self->writer);
    self->writer = NULL;
    return frozenset;
}


// Test PyFrozenSetWriter_Discard()
static PyObject*
writer_discard(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyFrozenSetWriter_Discard(self->writer);
    self->writer = NULL;
    Py_RETURN_NONE;
}


static PyMethodDef writer_methods[] = {
    {"add", _PyCFunction_CAST(writer_add), METH_O},
    {"update", _PyCFunction_CAST(writer_update), METH_O},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
    {"discard", _PyCFunction_CAST(writer_discard), METH_NOARGS},
    {NULL,              NULL}           /* sentinel */
};

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_new, writer_new},
    {Py_tp_init, writer_init},
    {Py_tp_dealloc, writer_dealloc},
    {Py_tp_methods, writer_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Writer_spec = {
    .name = "_testcapi.PyFrozenSetWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyMethodDef test_methods[] = {
    {"set_get_size", set_get_size, METH_O},
    {"test_set_type_size", test_set_type_size, METH_NOARGS},
    {NULL},
};


int
_PyTestCapi_Init_Set(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    PyTypeObject *writer_type = (PyTypeObject *)PyType_FromSpec(&Writer_spec);
    if (writer_type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, writer_type) < 0) {
        Py_DECREF(writer_type);
        return -1;
    }
    Py_DECREF(writer_type);

    return 0;
}
