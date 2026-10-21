#include "parts.h"
#include "util.h"


static PyObject *
list_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyList_GET_SIZE(obj));
}


static PyObject *
list_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyList_GET_ITEM(obj, i));
}


static PyObject *
list_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(value);
    PyList_SET_ITEM(obj, i, Py_XNewRef(value));
    Py_RETURN_NONE;

}


static PyObject *
list_clear(PyObject* Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyList_Clear(obj));
}


static PyObject *
list_extend(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *arg;
    if (!PyArg_ParseTuple(args, "OO", &obj, &arg)) {
        return NULL;
    }
    NULLABLE(obj);
    NULLABLE(arg);
    RETURN_INT(PyList_Extend(obj, arg));
}


static PyObject*
test_list_api(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject* list;
    int i;

    /* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
    list = PyList_New(NLIST);
    if (list == (PyObject*)NULL)
        return (PyObject*)NULL;
    /* list = range(NLIST) */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyLong_FromLong(i);
        if (anint == (PyObject*)NULL) {
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
        PyList_SET_ITEM(list, i, anint);
    }
    /* list.reverse(), via PyList_Reverse() */
    i = PyList_Reverse(list);   /* should not blow up! */
    if (i != 0) {
        Py_DECREF(list);
        return (PyObject*)NULL;
    }
    /* Check that list == range(29, -1, -1) now */
    for (i = 0; i < NLIST; ++i) {
        PyObject* anint = PyList_GET_ITEM(list, i);
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            PyErr_SetString(PyExc_AssertionError,
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
    }
    Py_DECREF(list);
#undef NLIST

    Py_RETURN_NONE;
}


typedef struct {
    PyObject_HEAD
    PyListWriter *writer;
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


// Test PyListWriter_Create()
static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyListWriter() takes no keyword arguments");
        return -1;
    }

    Py_ssize_t size = 0;
    if (!PyArg_ParseTuple(args, "|n", &size)) {
        return -1;
    }

    WriterObject *self = (WriterObject *)self_raw;
    PyListWriter_Discard(self->writer);

    self->writer = PyListWriter_Create(size);
    if (self->writer == NULL) {
        return -1;
    }

    return 0;
}


// Test PyListWriter_Discard()
static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyListWriter_Discard(self->writer);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


// Test PyListWriter_Append()
static PyObject*
writer_append(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyListWriter_Append(self->writer, item) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListWriter_AppendSteal()
static PyObject*
writer_append_steal(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyListWriter_AppendSteal(self->writer, Py_NewRef(item)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListWriter_Extend()
static PyObject*
writer_extend(PyObject *self_raw, PyObject *iterable)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyListWriter_Extend(self->writer, iterable) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListWriter_ExtendArray()
static PyObject*
writer_extend_array(PyObject *self_raw, PyObject *tuple)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (!PyTuple_Check(tuple)) {
        PyErr_Format(PyExc_TypeError,
                     "expect tuple, got %T", tuple);
        return NULL;
    }
    PyObject **items = &PyTuple_GET_ITEM(tuple, 0);
    Py_ssize_t len = PyTuple_GET_SIZE(tuple);

    if (PyListWriter_ExtendArray(self->writer, items, len) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


// Test PyListWriter_Finish()
static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *list = PyListWriter_Finish(self->writer);
    self->writer = NULL;
    return list;
}


// Test PyListWriter_Discard()
static PyObject*
writer_discard(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyListWriter_Discard(self->writer);
    self->writer = NULL;
    Py_RETURN_NONE;
}


static PyMethodDef writer_methods[] = {
    {"append", _PyCFunction_CAST(writer_append), METH_O},
    {"append_steal", _PyCFunction_CAST(writer_append_steal), METH_O},
    {"extend", _PyCFunction_CAST(writer_extend), METH_O},
    {"extend_array", _PyCFunction_CAST(writer_extend_array), METH_O},
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
    .name = "_testcapi.PyListWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyMethodDef test_methods[] = {
    {"list_get_size", list_get_size, METH_O},
    {"list_get_item", list_get_item, METH_VARARGS},
    {"list_set_item", list_set_item, METH_VARARGS},
    {"list_clear", list_clear, METH_O},
    {"list_extend", list_extend, METH_VARARGS},
    {"test_list_api", test_list_api, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_List(PyObject *m)
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
