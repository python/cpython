#include "parts.h"
#include "util.h"


static PyObject *
tuple_get_size(PyObject *Py_UNUSED(module), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_SIZE(PyTuple_GET_SIZE(obj));
}

static PyObject *
tuple_get_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    NULLABLE(obj);
    return Py_XNewRef(PyTuple_GET_ITEM(obj, i));
}

static PyObject *
tuple_copy(PyObject *tuple)
{
    Py_ssize_t size = PyTuple_GET_SIZE(tuple);
    PyObject *newtuple = PyTuple_New(size);
    if (!newtuple) {
        return NULL;
    }
    for (Py_ssize_t n = 0; n < size; n++) {
        PyTuple_SET_ITEM(newtuple, n, Py_XNewRef(PyTuple_GET_ITEM(tuple, n)));
    }
    return newtuple;
}

static PyObject *
tuple_set_item(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj, *value, *newtuple;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "OnO", &obj, &i, &value)) {
        return NULL;
    }
    NULLABLE(value);
    if (PyTuple_CheckExact(obj)) {
        newtuple = tuple_copy(obj);
        if (!newtuple) {
            return NULL;
        }

        PyObject *val = PyTuple_GET_ITEM(newtuple, i);
        PyTuple_SET_ITEM(newtuple, i, Py_XNewRef(value));
        Py_DECREF(val);
        return newtuple;
    }
    else {
        NULLABLE(obj);

        PyObject *val = PyTuple_GET_ITEM(obj, i);
        PyTuple_SET_ITEM(obj, i, Py_XNewRef(value));
        Py_DECREF(val);
        return Py_XNewRef(obj);
    }
}

static PyObject *
_tuple_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *tup;
    Py_ssize_t newsize;
    int new = 1;
    if (!PyArg_ParseTuple(args, "On|p", &tup, &newsize, &new)) {
        return NULL;
    }
    if (new) {
        tup = tuple_copy(tup);
        if (!tup) {
            return NULL;
        }
    }
    else {
        NULLABLE(tup);
        Py_XINCREF(tup);
    }
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    int r = _PyTuple_Resize(&tup, newsize);
    _Py_COMP_DIAG_POP
    if (r == -1) {
        assert(tup == NULL);
        return NULL;
    }
    return tup;
}

static PyObject *
_check_tuple_item_is_NULL(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t i;
    if (!PyArg_ParseTuple(args, "On", &obj, &i)) {
        return NULL;
    }
    return PyLong_FromLong(PyTuple_GET_ITEM(obj, i) == NULL);
}


static PyObject *
tuple_fromarray(PyObject* Py_UNUSED(module), PyObject *args)
{
    PyObject *src;
    Py_ssize_t size = UNINITIALIZED_SIZE;
    if (!PyArg_ParseTuple(args, "O|n", &src, &size)) {
        return NULL;
    }
    if (src != Py_None && !PyTuple_Check(src)) {
        PyErr_SetString(PyExc_TypeError, "expect a tuple");
        return NULL;
    }

    PyObject **items;
    if (src != Py_None) {
        items = &PyTuple_GET_ITEM(src, 0);
        if (size == UNINITIALIZED_SIZE) {
            size = PyTuple_GET_SIZE(src);
        }
    }
    else {
        items = NULL;
    }
    return PyTuple_FromArray(items, size);
}


// --- PyTupleWriter type ---------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyTupleWriter *writer;
} WriterObject;


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


static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyTupleWriter() takes exactly no keyword arguments");
        return -1;
    }

    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return -1;
    }

    WriterObject *self = (WriterObject *)self_raw;
    if (self->writer) {
        PyTupleWriter_Discard(self->writer);
    }
    self->writer = PyTupleWriter_Create(size);
    if (self->writer == NULL) {
        return -1;
    }
    return 0;
}


static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyTupleWriter_Discard(self->writer);
    }
    tp->tp_free(self);
    Py_DECREF(tp);
}


static inline int
writer_check(WriterObject *self)
{
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_ValueError, "operation on finished writer");
        return -1;
    }
    return 0;
}


static PyObject*
writer_add(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyTupleWriter_Add(self->writer, item) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_add_steal(PyObject *self_raw, PyObject *item)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    if (PyTupleWriter_AddSteal(self->writer, Py_NewRef(item)) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_add_array(PyObject *self_raw, PyObject *args)
{
    PyObject *tuple;
    if (!PyArg_ParseTuple(args, "O!", &PyTuple_Type, &tuple)) {
        return NULL;
    }

    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject **array = &PyTuple_GET_ITEM(tuple, 0);
    Py_ssize_t size = PyTuple_GET_SIZE(tuple);
    if (PyTupleWriter_AddArray(self->writer, array, size) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *tuple = PyTupleWriter_Finish(self->writer);
    self->writer = NULL;
    return tuple;
}


static PyMethodDef writer_methods[] = {
    {"add", _PyCFunction_CAST(writer_add), METH_O},
    {"add_steal", _PyCFunction_CAST(writer_add_steal), METH_O},
    {"add_array", _PyCFunction_CAST(writer_add_array), METH_VARARGS},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
    {NULL, NULL}  /* sentinel */
};

static PyType_Slot Writer_Type_slots[] = {
    {Py_tp_new, writer_new},
    {Py_tp_init, writer_init},
    {Py_tp_dealloc, writer_dealloc},
    {Py_tp_methods, writer_methods},
    {0, 0},  /* sentinel */
};

static PyType_Spec Writer_spec = {
    .name = "_testcapi.PyTupleWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


static PyMethodDef test_methods[] = {
    {"tuple_get_size", tuple_get_size, METH_O},
    {"tuple_get_item", tuple_get_item, METH_VARARGS},
    {"tuple_set_item", tuple_set_item, METH_VARARGS},
    {"_tuple_resize", _tuple_resize, METH_VARARGS},
    {"_check_tuple_item_is_NULL", _check_tuple_item_is_NULL, METH_VARARGS},
    {"tuple_fromarray", tuple_fromarray, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Tuple(PyObject *m)
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
