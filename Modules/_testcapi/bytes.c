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
    WriterObject *self = (WriterObject *)self_raw;
    if (self->writer) {
        PyBytesWriter_Discard(self->writer);
    }

    if (kwargs && PyDict_GET_SIZE(kwargs)) {
        PyErr_Format(PyExc_TypeError,
                     "PyBytesWriter() takes exactly no keyword arguments");
        return -1;
    }

    Py_ssize_t prealloc;
    if (!PyArg_ParseTuple(args, "n", &prealloc)) {
        return -1;
    }

    self->writer = PyBytesWriter_Create(prealloc);
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

    if (PyBytesWriter_WriteBytes(self->writer, str, size) < 0) {
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

    void *buf = PyBytesWriter_Alloc(self->writer, extend);
    if (buf == NULL) {
        return NULL;
    }
    memcpy(buf, str, str_size);
    buf += str_size;

    if (PyBytesWriter_Truncate(self->writer, buf) < 0) {
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

    PyObject *str = PyBytesWriter_Finish(self->writer);
    self->writer = NULL;
    return str;
}


static PyMethodDef writer_methods[] = {
    {"write_bytes", writer_write_bytes, METH_VARARGS},
    {"extend", writer_extend, METH_VARARGS},
    {"finish", writer_finish, METH_NOARGS},
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
byteswriter_hello_world(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyBytesWriter *writer = PyBytesWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }

    char *buf = PyBytesWriter_Alloc(writer, 20);
    if (buf == NULL) {
        goto error;
    }
    memcpy(buf, "Hello World", strlen("Hello World"));
    buf += strlen("Hello World");
    if (PyBytesWriter_Truncate(writer, buf) < 0) {
        goto error;
    }

    return PyBytesWriter_Finish(writer);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}


static PyObject *
byteswriter_alloc(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyBytesWriter *writer = PyBytesWriter_Create(3);
    if (writer == NULL) {
        return NULL;
    }

    // Allocate 10 bytes
    char *buf = PyBytesWriter_Alloc(writer, 3);
    if (buf == NULL) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }
    memcpy(buf, "abc", 3);
    return PyBytesWriter_Finish(writer);
}


static PyObject *
byteswriter_extend(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    PyBytesWriter *writer = PyBytesWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }

    // Allocate 10 bytes
    char *buf = PyBytesWriter_Alloc(writer, 10);
    if (buf == NULL) {
        goto error;
    }

    // Write some bytes
    memcpy(buf, "Hello ", strlen("Hello "));
    buf += strlen("Hello ");

    // Allocate 10 more bytes
    buf = PyBytesWriter_Extend(writer, buf, 10);
    if (buf == NULL) {
        goto error;
    }

    // Write more bytes
    memcpy(buf, "World", strlen("World"));
    buf += strlen("World");

    // Truncate to len("Hello World") bytes
    if (PyBytesWriter_Truncate(writer, buf) < 0) {
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
    {"byteswriter_hello_world", byteswriter_hello_world, METH_NOARGS},
    {"byteswriter_alloc", byteswriter_alloc, METH_NOARGS},
    {"byteswriter_extend", byteswriter_extend, METH_NOARGS},
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
