#include "Python.h"

/* This module is a stripped down version of _bytesio.c with a Py_UNICODE
   buffer. Most of the functionality is provided by subclassing _StringIO. */


typedef struct {
    PyObject_HEAD
    Py_UNICODE *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;
} StringIOObject;


/* Internal routine for changing the size, in terms of characters, of the
   buffer of StringIO objects.  The caller should ensure that the 'size'
   argument is non-negative.  Returns 0 on success, -1 otherwise. */
static int
resize_buffer(StringIOObject *self, size_t size)
{
    /* Here, unsigned types are used to avoid dealing with signed integer
       overflow, which is undefined in C. */
    size_t alloc = self->buf_size;
    Py_UNICODE *new_buf = NULL;

    assert(self->buf != NULL);

    /* For simplicity, stay in the range of the signed type. Anyway, Python
       doesn't allow strings to be longer than this. */
    if (size > PY_SSIZE_T_MAX)
        goto overflow;

    if (size < alloc / 2) {
        /* Major downsize; resize down to exact size. */
        alloc = size + 1;
    }
    else if (size < alloc) {
        /* Within allocated size; quick exit */
        return 0;
    }
    else if (size <= alloc * 1.125) {
        /* Moderate upsize; overallocate similar to list_resize() */
        alloc = size + (size >> 3) + (size < 9 ? 3 : 6);
    }
    else {
        /* Major upsize; resize up to exact size */
        alloc = size + 1;
    }

    if (alloc > ((size_t)-1) / sizeof(Py_UNICODE))
        goto overflow;
    new_buf = (Py_UNICODE *)PyMem_Realloc(self->buf,
                                          alloc * sizeof(Py_UNICODE));
    if (new_buf == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    self->buf_size = alloc;
    self->buf = new_buf;

    return 0;

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "new buffer size too large");
    return -1;
}

/* Internal routine for writing a string of characters to the buffer of a
   StringIO object. Returns the number of bytes wrote, or -1 on error. */
static Py_ssize_t
write_str(StringIOObject *self, const Py_UNICODE *str, Py_ssize_t len)
{
    assert(self->buf != NULL);
    assert(self->pos >= 0);
    assert(len >= 0);

    /* This overflow check is not strictly necessary. However, it avoids us to
       deal with funky things like comparing an unsigned and a signed
       integer. */
    if (self->pos > PY_SSIZE_T_MAX - len) {
        PyErr_SetString(PyExc_OverflowError,
                        "new position too large");
        return -1;
    }
    if (self->pos + len > self->string_size) {
        if (resize_buffer(self, self->pos + len) < 0)
            return -1;
    }

    if (self->pos > self->string_size) {
        /* In case of overseek, pad with null bytes the buffer region between
           the end of stream and the current position.

          0   lo      string_size                           hi
          |   |<---used--->|<----------available----------->|
          |   |            <--to pad-->|<---to write--->    |
          0   buf                   positon

        */
        memset(self->buf + self->string_size, '\0',
               (self->pos - self->string_size) * sizeof(Py_UNICODE));
    }

    /* Copy the data to the internal buffer, overwriting some of the
       existing data if self->pos < self->string_size. */
    memcpy(self->buf + self->pos, str, len * sizeof(Py_UNICODE));
    self->pos += len;

    /* Set the new length of the internal string if it has changed */
    if (self->string_size < self->pos) {
        self->string_size = self->pos;
    }

    return len;
}

static PyObject *
stringio_getvalue(StringIOObject *self)
{
    return PyUnicode_FromUnicode(self->buf, self->string_size);
}

static PyObject *
stringio_tell(StringIOObject *self)
{
    return PyLong_FromSsize_t(self->pos);
}

static PyObject *
stringio_read(StringIOObject *self, PyObject *args)
{
    Py_ssize_t size, n;
    Py_UNICODE *output;
    PyObject *arg = Py_None;

    if (!PyArg_ParseTuple(args, "|O:read", &arg))
        return NULL;

    if (PyLong_Check(arg)) {
        size = PyLong_AsSsize_t(arg);
        if (size == -1 && PyErr_Occurred())
            return NULL;
    }
    else if (arg == Py_None) {
        /* Read until EOF is reached, by default. */
        size = -1;
    }
    else {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }

    /* adjust invalid sizes */
    n = self->string_size - self->pos;
    if (size < 0 || size > n) {
        size = n;
        if (size < 0)
            size = 0;
    }

    assert(self->buf != NULL);
    output = self->buf + self->pos;
    self->pos += size;

    return PyUnicode_FromUnicode(output, size);
}

static PyObject *
stringio_truncate(StringIOObject *self, PyObject *args)
{
    Py_ssize_t size;
    PyObject *arg = Py_None;

    if (!PyArg_ParseTuple(args, "|O:truncate", &arg))
        return NULL;

    if (PyLong_Check(arg)) {
        size = PyLong_AsSsize_t(arg);
        if (size == -1 && PyErr_Occurred())
            return NULL;
    }
    else if (arg == Py_None) {
        /* Truncate to current position if no argument is passed. */
        size = self->pos;
    }
    else {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }

    if (size < 0) {
        PyErr_Format(PyExc_ValueError,
                     "Negative size value %zd", size);
        return NULL;
    }

    if (size < self->string_size) {
        self->string_size = size;
        if (resize_buffer(self, size) < 0)
            return NULL;
    }
    self->pos = size;

    return PyLong_FromSsize_t(size);
}

static PyObject *
stringio_seek(StringIOObject *self, PyObject *args)
{
    Py_ssize_t pos;
    int mode = 0;

    if (!PyArg_ParseTuple(args, "n|i:seek", &pos, &mode))
        return NULL;

    if (mode != 0 && mode != 1 && mode != 2) {
        PyErr_Format(PyExc_ValueError,
                     "Invalid whence (%i, should be 0, 1 or 2)", mode);
        return NULL;
    }
    else if (pos < 0 && mode == 0) {
        PyErr_Format(PyExc_ValueError,
                     "Negative seek position %zd", pos);
        return NULL;
    }
    else if (mode != 0 && pos != 0) {
        PyErr_SetString(PyExc_IOError,
                        "Can't do nonzero cur-relative seeks");
        return NULL;
    }

    /* mode 0: offset relative to beginning of the string.
       mode 1: no change to current position.
       mode 2: change position to end of file. */
    if (mode == 1) {
        pos = self->pos;
    }
    else if (mode == 2) {
        pos = self->string_size;
    }

    self->pos = pos;

    return PyLong_FromSsize_t(self->pos);
}

static PyObject *
stringio_write(StringIOObject *self, PyObject *obj)
{
    const Py_UNICODE *str;
    Py_ssize_t size;
    Py_ssize_t n = 0;

    if (PyUnicode_Check(obj)) {
        str = PyUnicode_AsUnicode(obj);
        size = PyUnicode_GetSize(obj);
    }
    else {
        PyErr_Format(PyExc_TypeError, "string argument expected, got '%s'",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }

    if (size != 0) {
        n = write_str(self, str, size);
        if (n < 0)
            return NULL;
    }

    return PyLong_FromSsize_t(n);
}

static void
stringio_dealloc(StringIOObject *self)
{
    PyMem_Free(self->buf);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
stringio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    StringIOObject *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (StringIOObject *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    self->string_size = 0;
    self->pos = 0;
    self->buf_size = 0;
    self->buf = (Py_UNICODE *)PyMem_Malloc(0);
    if (self->buf == NULL) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }

    return (PyObject *)self;
}

static struct PyMethodDef stringio_methods[] = {
    {"getvalue",   (PyCFunction)stringio_getvalue, METH_VARARGS, NULL},
    {"read",       (PyCFunction)stringio_read,     METH_VARARGS, NULL},
    {"tell",       (PyCFunction)stringio_tell,     METH_NOARGS,  NULL},
    {"truncate",   (PyCFunction)stringio_truncate, METH_VARARGS, NULL},
    {"seek",       (PyCFunction)stringio_seek,     METH_VARARGS, NULL},
    {"write",      (PyCFunction)stringio_write,    METH_O,       NULL},
    {NULL, NULL}        /* sentinel */
};

static PyTypeObject StringIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_stringio._StringIO",                     /*tp_name*/
    sizeof(StringIOObject),                    /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)stringio_dealloc,              /*tp_dealloc*/
    0,                                         /*tp_print*/
    0,                                         /*tp_getattr*/
    0,                                         /*tp_setattr*/
    0,                                         /*tp_reserved*/
    0,                                         /*tp_repr*/
    0,                                         /*tp_as_number*/
    0,                                         /*tp_as_sequence*/
    0,                                         /*tp_as_mapping*/
    0,                                         /*tp_hash*/
    0,                                         /*tp_call*/
    0,                                         /*tp_str*/
    0,                                         /*tp_getattro*/
    0,                                         /*tp_setattro*/
    0,                                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    0,                                         /*tp_doc*/
    0,                                         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    stringio_methods,                          /*tp_methods*/
    0,                                         /*tp_members*/
    0,                                         /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    0,                                         /*tp_init*/
    0,                                         /*tp_alloc*/
    stringio_new,                              /*tp_new*/
};

static struct PyModuleDef _stringiomodule = {
	PyModuleDef_HEAD_INIT,
	"_stringio",
	NULL,
	-1,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

PyMODINIT_FUNC
PyInit__stringio(void)
{
    PyObject *m;

    if (PyType_Ready(&StringIO_Type) < 0)
        return NULL;
    m = PyModule_Create(&_stringiomodule);
    if (m == NULL)
        return NULL;
    Py_INCREF(&StringIO_Type);
    if (PyModule_AddObject(m, "_StringIO", (PyObject *)&StringIO_Type) < 0)
        return NULL;
    return m;
}
