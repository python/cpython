#include "parts.h"
#include "util.h"


/* Test _PyBytes_Resize() */
static PyObject *
bytes_resize(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *obj;
    Py_ssize_t newsize;
    int new;

    if (!PyArg_ParseTuple(args, "Onp", &obj, &newsize, &new))
        return NULL;

    NULLABLE(obj);
    if (new) {
        assert(obj != NULL);
        assert(PyBytes_CheckExact(obj));
        PyObject *newobj = PyBytes_FromStringAndSize(NULL, PyBytes_Size(obj));
        if (newobj == NULL) {
            return NULL;
        }
        memcpy(PyBytes_AsString(newobj), PyBytes_AsString(obj), PyBytes_Size(obj));
        obj = newobj;
    }
    else {
        Py_XINCREF(obj);
    }
    if (_PyBytes_Resize(&obj, newsize) < 0) {
        assert(obj == NULL);
    }
    else {
        assert(obj != NULL);
    }
    return obj;
}


/* Test PyBytes_Join() */
static PyObject *
bytes_join(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *sep, *iterable;
    if (!PyArg_ParseTuple(args, "OO", &sep, &iterable)) {
        return NULL;
    }
    NULLABLE(sep);
    NULLABLE(iterable);
    return PyBytes_Join(sep, iterable);
}


// --- PyBytesWriter type ---------------------------------------------------

typedef struct {
    PyObject_HEAD
    PyBytesWriter *writer;
    char *buf;
} WriterObject;


static PyObject *
writer_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)type->tp_alloc(type, 0);
    if (!self) {
        return NULL;
    }
    self->writer = NULL;
    self->buf = NULL;
    return (PyObject*)self;
}


static int
writer_init(PyObject *self_raw, PyObject *args, PyObject *kwargs)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (self->writer) {
        PyBytesWriter_Discard(self->writer);
    }

    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyBytesWriter() takes exactly no keyword arguments");
        return -1;
    }

    Py_ssize_t alloc;
    char *str;
    Py_ssize_t str_size;
    if (!PyArg_ParseTuple(args, "ny#", &alloc, &str, &str_size)) {
        return -1;
    }

    self->buf = PyBytesWriter_Create(&self->writer, alloc);
    if (self->buf == NULL) {
        return -1;
    }

    memcpy(self->buf, str, str_size);
    self->buf += str_size;

    return 0;
}


static void
writer_dealloc(PyObject *self_raw)
{
    WriterObject *self = (WriterObject *)self_raw;
    PyTypeObject *tp = Py_TYPE(self);
    if (self->writer) {
        PyBytesWriter_Discard(self->writer);
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
writer_write_bytes(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    char *str;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "yn", &str, &size)) {
        return NULL;
    }

    self->buf = PyBytesWriter_WriteBytes(self->writer, self->buf, str, size);
    if (self->buf == NULL) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_format_i(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    int number;
    if (!PyArg_ParseTuple(args, "i", &number)) {
        return NULL;
    }

    self->buf = PyBytesWriter_Format(self->writer, self->buf, "%i", number);
    if (self->buf == NULL) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_extend(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t extend;
    char *str;
    Py_ssize_t str_size;
    if (!PyArg_ParseTuple(args,
                          "ny#",
                          &extend, &str, &str_size)) {
        return NULL;
    }
    assert(extend >= str_size);

    self->buf = PyBytesWriter_Extend(self->writer, self->buf, extend);
    if (self->buf == NULL) {
        return NULL;
    }
    memcpy(self->buf, str, str_size);
    self->buf += str_size;

    Py_RETURN_NONE;
}


static PyObject*
writer_get_remaining(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t size = PyBytesWriter_GetRemaining(self->writer, self->buf);
    return PyLong_FromSsize_t(size);
}


static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str = PyBytesWriter_Finish(self->writer, self->buf);
    self->writer = NULL;
    return str;
}


static PyMethodDef writer_methods[] = {
    {"write_bytes", _PyCFunction_CAST(writer_write_bytes), METH_VARARGS},
    {"format_i", _PyCFunction_CAST(writer_format_i), METH_VARARGS},
    {"extend", _PyCFunction_CAST(writer_extend), METH_VARARGS},
    {"get_remaining", _PyCFunction_CAST(writer_get_remaining), METH_NOARGS},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
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
    .name = "_testcapi.PyBytesWriter",
    .basicsize = sizeof(WriterObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = Writer_Type_slots,
};


// Similar to bytes.center() with a different API: spaces are number of
// whitespaces added to the left and to the right.
static PyObject *
byteswriter_center_example(Py_ssize_t spaces, char *str, Py_ssize_t str_size)
{
    PyBytesWriter *writer;
    char *buf = PyBytesWriter_Create(&writer, spaces * 2);
    if (buf == NULL) {
        goto error;
    }
    assert(PyBytesWriter_GetRemaining(writer, buf) == spaces * 2);

    // Add left spaces
    memset(buf, ' ', spaces);
    buf += spaces;
    assert(PyBytesWriter_GetRemaining(writer, buf) == spaces);

    // Copy string
    buf = PyBytesWriter_Extend(writer, buf, str_size);
    if (buf == NULL) {
        goto error;
    }
    assert(PyBytesWriter_GetRemaining(writer, buf) == spaces + str_size);

    memcpy(buf, str, str_size);
    buf += str_size;
    assert(PyBytesWriter_GetRemaining(writer, buf) == spaces);

    // Add right spaces
    memset(buf, ' ', spaces);
    buf += spaces;
    assert(PyBytesWriter_GetRemaining(writer, buf) == 0);

    return PyBytesWriter_Finish(writer, buf);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}


static PyObject *
byteswriter_center(PyObject *Py_UNUSED(module), PyObject *args)
{
    Py_ssize_t spaces;
    char *str;
    Py_ssize_t str_size;
    if (!PyArg_ParseTuple(args, "ny#", &spaces, &str, &str_size)) {
        return NULL;
    }

    return byteswriter_center_example(spaces, str, str_size);
}


static PyMethodDef test_methods[] = {
    {"bytes_resize", bytes_resize, METH_VARARGS},
    {"bytes_join", bytes_join, METH_VARARGS},
    {"byteswriter_center", byteswriter_center, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Bytes(PyObject *m)
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
