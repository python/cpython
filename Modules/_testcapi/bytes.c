// Use pycore_bytes.h
#define PYTESTCAPI_NEED_INTERNAL_API

#include "parts.h"
#include "util.h"

#include "pycore_bytesobject.h"   // _PyBytesWriter_CreateByteArray()


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
                     "PyBytesWriter() takes exactly no keyword arguments");
        return -1;
    }

    Py_ssize_t alloc;
    char *str;
    Py_ssize_t str_size;
    int use_bytearray;
    if (!PyArg_ParseTuple(args, "ny#i",
                          &alloc, &str, &str_size, &use_bytearray)) {
        return -1;
    }

    WriterObject *self = (WriterObject *)self_raw;
    if (self->writer) {
        PyBytesWriter_Discard(self->writer);
    }
    if (use_bytearray) {
        self->writer = _PyBytesWriter_CreateByteArray(alloc);
    }
    else {
        self->writer = PyBytesWriter_Create(alloc);
    }
    if (self->writer == NULL) {
        return -1;
    }

    if (str_size) {
        char *buf = PyBytesWriter_GetData(self->writer);
        memcpy(buf, str, str_size);
    }

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

    char *bytes;
    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "yn", &bytes, &size)) {
        return NULL;
    }

    if (PyBytesWriter_WriteBytes(self->writer, bytes, size) < 0) {
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

    char *format;
    int value;
    if (!PyArg_ParseTuple(args, "yi", &format, &value)) {
        return NULL;
    }

    if (PyBytesWriter_Format(self->writer, format, value) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
writer_resize(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t size;
    char *str;
    Py_ssize_t str_size;
    if (!PyArg_ParseTuple(args,
                          "ny#",
                          &size, &str, &str_size)) {
        return NULL;
    }
    assert(size >= str_size);

    Py_ssize_t pos = PyBytesWriter_GetSize(self->writer);
    if (PyBytesWriter_Resize(self->writer, size) < 0) {
        return NULL;
    }

    char *buf = PyBytesWriter_GetData(self->writer);
    memcpy(buf + pos, str, str_size);

    Py_RETURN_NONE;
}


static PyObject*
writer_get_size(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t alloc = PyBytesWriter_GetSize(self->writer);
    return PyLong_FromSsize_t(alloc);
}


static PyObject*
writer_finish(PyObject *self_raw, PyObject *Py_UNUSED(args))
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    PyObject *str = PyBytesWriter_Finish(self->writer);
    self->writer = NULL;
    return str;
}


static PyObject*
writer_finish_with_size(PyObject *self_raw, PyObject *args)
{
    WriterObject *self = (WriterObject *)self_raw;
    if (writer_check(self) < 0) {
        return NULL;
    }

    Py_ssize_t size;
    if (!PyArg_ParseTuple(args, "n", &size)) {
        return NULL;
    }

    PyObject *str = PyBytesWriter_FinishWithSize(self->writer, size);
    self->writer = NULL;
    return str;
}


static PyMethodDef writer_methods[] = {
    {"write_bytes", _PyCFunction_CAST(writer_write_bytes), METH_VARARGS},
    {"format_i", _PyCFunction_CAST(writer_format_i), METH_VARARGS},
    {"resize", _PyCFunction_CAST(writer_resize), METH_VARARGS},
    {"get_size", _PyCFunction_CAST(writer_get_size), METH_NOARGS},
    {"finish", _PyCFunction_CAST(writer_finish), METH_NOARGS},
    {"finish_with_size", _PyCFunction_CAST(writer_finish_with_size), METH_VARARGS},
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


static PyObject *
byteswriter_abc(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyBytesWriter *writer = PyBytesWriter_Create(3);
    if (writer == NULL) {
        return NULL;
    }

    char *str = PyBytesWriter_GetData(writer);
    memcpy(str, "abc", 3);

    return PyBytesWriter_Finish(writer);
}


static PyObject *
byteswriter_resize(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Allocate 10 bytes
    PyBytesWriter *writer = PyBytesWriter_Create(10);
    if (writer == NULL) {
        return NULL;
    }
    char *buf = PyBytesWriter_GetData(writer);

    // Write some bytes
    memcpy(buf, "Hello ", strlen("Hello "));
    buf += strlen("Hello ");

    // Allocate 10 more bytes
    buf = PyBytesWriter_GrowAndUpdatePointer(writer, 10, buf);
    if (buf == NULL) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }

    // Write more bytes
    memcpy(buf, "World", strlen("World"));
    buf += strlen("World");

    // Truncate to the exact size and create a bytes object
    return PyBytesWriter_FinishWithPointer(writer, buf);
}


static PyObject *
byteswriter_highlevel(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyBytesWriter *writer = PyBytesWriter_Create(0);
    if (writer == NULL) {
        goto error;
    }
    if (PyBytesWriter_WriteBytes(writer, "Hello", -1) < 0) {
        goto error;
    }
    if (PyBytesWriter_Format(writer, " %s!", "World") < 0) {
        goto error;
    }
    return PyBytesWriter_Finish(writer);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}


static PyMethodDef test_methods[] = {
    {"bytes_resize", bytes_resize, METH_VARARGS},
    {"bytes_join", bytes_join, METH_VARARGS},
    {"byteswriter_abc", byteswriter_abc, METH_NOARGS},
    {"byteswriter_resize", byteswriter_resize, METH_NOARGS},
    {"byteswriter_highlevel", byteswriter_highlevel, METH_NOARGS},
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
