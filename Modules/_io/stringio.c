#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "_iomodule.h"

/* Implementation note: the buffer is always at least one character longer
   than the enclosed string, for proper functioning of _PyIO_find_line_ending.
*/

typedef struct {
    PyObject_HEAD
    Py_UNICODE *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;

    char ok; /* initialized? */
    char closed;
    char readuniversal;
    char readtranslate;
    PyObject *decoder;
    PyObject *readnl;
    PyObject *writenl;
    
    PyObject *dict;
    PyObject *weakreflist;
} stringio;

#define CHECK_INITIALIZED(self) \
    if (self->ok <= 0) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on uninitialized object"); \
        return NULL; \
    }

#define CHECK_CLOSED(self) \
    if (self->closed) { \
        PyErr_SetString(PyExc_ValueError, \
            "I/O operation on closed file"); \
        return NULL; \
    }

PyDoc_STRVAR(stringio_doc,
    "Text I/O implementation using an in-memory buffer.\n"
    "\n"
    "The initial_value argument sets the value of object.  The newline\n"
    "argument is like the one of TextIOWrapper's constructor.");


/* Internal routine for changing the size, in terms of characters, of the
   buffer of StringIO objects.  The caller should ensure that the 'size'
   argument is non-negative.  Returns 0 on success, -1 otherwise. */
static int
resize_buffer(stringio *self, size_t size)
{
    /* Here, unsigned types are used to avoid dealing with signed integer
       overflow, which is undefined in C. */
    size_t alloc = self->buf_size;
    Py_UNICODE *new_buf = NULL;

    assert(self->buf != NULL);

    /* Reserve one more char for line ending detection. */
    size = size + 1;
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

/* Internal routine for writing a whole PyUnicode object to the buffer of a
   StringIO object. Returns 0 on success, or -1 on error. */
static Py_ssize_t
write_str(stringio *self, PyObject *obj)
{
    Py_UNICODE *str;
    Py_ssize_t len;
    PyObject *decoded = NULL;
    assert(self->buf != NULL);
    assert(self->pos >= 0);

    if (self->decoder != NULL) {
        decoded = _PyIncrementalNewlineDecoder_decode(
            self->decoder, obj, 1 /* always final */);
    }
    else {
        decoded = obj;
        Py_INCREF(decoded);
    }
    if (self->writenl) {
        PyObject *translated = PyUnicode_Replace(
            decoded, _PyIO_str_nl, self->writenl, -1);
        Py_DECREF(decoded);
        decoded = translated;
    }
    if (decoded == NULL)
        return -1;

    assert(PyUnicode_Check(decoded));
    str = PyUnicode_AS_UNICODE(decoded);
    len = PyUnicode_GET_SIZE(decoded);

    assert(len >= 0);

    /* This overflow check is not strictly necessary. However, it avoids us to
       deal with funky things like comparing an unsigned and a signed
       integer. */
    if (self->pos > PY_SSIZE_T_MAX - len) {
        PyErr_SetString(PyExc_OverflowError,
                        "new position too large");
        goto fail;
    }
    if (self->pos + len > self->string_size) {
        if (resize_buffer(self, self->pos + len) < 0)
            goto fail;
    }

    if (self->pos > self->string_size) {
        /* In case of overseek, pad with null bytes the buffer region between
           the end of stream and the current position.

          0   lo      string_size                           hi
          |   |<---used--->|<----------available----------->|
          |   |            <--to pad-->|<---to write--->    |
          0   buf                   position

        */
        memset(self->buf + self->string_size, '\0',
               (self->pos - self->string_size) * sizeof(Py_UNICODE));
    }

    /* Copy the data to the internal buffer, overwriting some of the
       existing data if self->pos < self->string_size. */
    memcpy(self->buf + self->pos, str, len * sizeof(Py_UNICODE));
    self->pos += len;

    /* Set the new length of the internal string if it has changed. */
    if (self->string_size < self->pos) {
        self->string_size = self->pos;
    }

    Py_DECREF(decoded);
    return 0;

fail:
    Py_XDECREF(decoded);
    return -1;
}

PyDoc_STRVAR(stringio_getvalue_doc,
    "Retrieve the entire contents of the object.");

static PyObject *
stringio_getvalue(stringio *self)
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    return PyUnicode_FromUnicode(self->buf, self->string_size);
}

PyDoc_STRVAR(stringio_tell_doc,
    "Tell the current file position.");

static PyObject *
stringio_tell(stringio *self)
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    return PyLong_FromSsize_t(self->pos);
}

PyDoc_STRVAR(stringio_read_doc,
    "Read at most n characters, returned as a string.\n"
    "\n"
    "If the argument is negative or omitted, read until EOF\n"
    "is reached. Return an empty string at EOF.\n");

static PyObject *
stringio_read(stringio *self, PyObject *args)
{
    Py_ssize_t size, n;
    Py_UNICODE *output;
    PyObject *arg = Py_None;

    CHECK_INITIALIZED(self);
    if (!PyArg_ParseTuple(args, "|O:read", &arg))
        return NULL;
    CHECK_CLOSED(self);

    if (PyNumber_Check(arg)) {
        size = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
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

    output = self->buf + self->pos;
    self->pos += size;
    return PyUnicode_FromUnicode(output, size);
}

/* Internal helper, used by stringio_readline and stringio_iternext */
static PyObject *
_stringio_readline(stringio *self, Py_ssize_t limit)
{
    Py_UNICODE *start, *end, old_char;
    Py_ssize_t len, consumed;

    /* In case of overseek, return the empty string */
    if (self->pos >= self->string_size)
        return PyUnicode_FromString("");

    start = self->buf + self->pos;
    if (limit < 0 || limit > self->string_size - self->pos)
        limit = self->string_size - self->pos;

    end = start + limit;
    old_char = *end;
    *end = '\0';
    len = _PyIO_find_line_ending(
        self->readtranslate, self->readuniversal, self->readnl,
        start, end, &consumed);
    *end = old_char;
    /* If we haven't found any line ending, we just return everything
       (`consumed` is ignored). */
    if (len < 0)
        len = limit;
    self->pos += len;
    return PyUnicode_FromUnicode(start, len);
}

PyDoc_STRVAR(stringio_readline_doc,
    "Read until newline or EOF.\n"
    "\n"
    "Returns an empty string if EOF is hit immediately.\n");

static PyObject *
stringio_readline(stringio *self, PyObject *args)
{
    PyObject *arg = Py_None;
    Py_ssize_t limit = -1;

    CHECK_INITIALIZED(self);
    if (!PyArg_ParseTuple(args, "|O:readline", &arg))
        return NULL;
    CHECK_CLOSED(self);

    if (PyNumber_Check(arg)) {
        limit = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
        if (limit == -1 && PyErr_Occurred())
            return NULL;
    }
    else if (arg != Py_None) {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }
    return _stringio_readline(self, limit);
}

static PyObject *
stringio_iternext(stringio *self)
{
    PyObject *line;

    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);

    if (Py_TYPE(self) == &PyStringIO_Type) {
        /* Skip method call overhead for speed */
        line = _stringio_readline(self, -1);
    }
    else {
        /* XXX is subclassing StringIO really supported? */
        line = PyObject_CallMethodObjArgs((PyObject *)self,
                                           _PyIO_str_readline, NULL);
        if (line && !PyUnicode_Check(line)) {
            PyErr_Format(PyExc_IOError,
                         "readline() should have returned an str object, "
                         "not '%.200s'", Py_TYPE(line)->tp_name);
            Py_DECREF(line);
            return NULL;
        }
    }

    if (line == NULL)
        return NULL;

    if (PyUnicode_GET_SIZE(line) == 0) {
        /* Reached EOF */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

PyDoc_STRVAR(stringio_truncate_doc,
    "Truncate size to pos.\n"
    "\n"
    "The pos argument defaults to the current file position, as\n"
    "returned by tell().  The current file position is unchanged.\n"
    "Returns the new absolute position.\n");

static PyObject *
stringio_truncate(stringio *self, PyObject *args)
{
    Py_ssize_t size;
    PyObject *arg = Py_None;

    CHECK_INITIALIZED(self);
    if (!PyArg_ParseTuple(args, "|O:truncate", &arg))
        return NULL;
    CHECK_CLOSED(self);

    if (PyNumber_Check(arg)) {
        size = PyNumber_AsSsize_t(arg, PyExc_OverflowError);
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
        if (resize_buffer(self, size) < 0)
            return NULL;
        self->string_size = size;
    }

    return PyLong_FromSsize_t(size);
}

PyDoc_STRVAR(stringio_seek_doc,
    "Change stream position.\n"
    "\n"
    "Seek to character offset pos relative to position indicated by whence:\n"
    "    0  Start of stream (the default).  pos should be >= 0;\n"
    "    1  Current position - pos must be 0;\n"
    "    2  End of stream - pos must be 0.\n"
    "Returns the new absolute position.\n");

static PyObject *
stringio_seek(stringio *self, PyObject *args)
{
    Py_ssize_t pos;
    int mode = 0;

    CHECK_INITIALIZED(self);
    if (!PyArg_ParseTuple(args, "n|i:seek", &pos, &mode))
        return NULL;
    CHECK_CLOSED(self);

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

PyDoc_STRVAR(stringio_write_doc,
    "Write string to file.\n"
    "\n"
    "Returns the number of characters written, which is always equal to\n"
    "the length of the string.\n");

static PyObject *
stringio_write(stringio *self, PyObject *obj)
{
    Py_ssize_t size;

    CHECK_INITIALIZED(self);
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "string argument expected, got '%s'",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }
    CHECK_CLOSED(self);
    size = PyUnicode_GET_SIZE(obj);

    if (size > 0 && write_str(self, obj) < 0)
        return NULL;

    return PyLong_FromSsize_t(size);
}

PyDoc_STRVAR(stringio_close_doc,
    "Close the IO object. Attempting any further operation after the\n"
    "object is closed will raise a ValueError.\n"
    "\n"
    "This method has no effect if the file is already closed.\n");

static PyObject *
stringio_close(stringio *self)
{
    self->closed = 1;
    /* Free up some memory */
    if (resize_buffer(self, 0) < 0)
        return NULL;
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);
    Py_RETURN_NONE;
}

static int
stringio_traverse(stringio *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
stringio_clear(stringio *self)
{
    Py_CLEAR(self->dict);
    return 0;
}

static void
stringio_dealloc(stringio *self)
{
    _PyObject_GC_UNTRACK(self);
    self->ok = 0;
    if (self->buf) {
        PyMem_Free(self->buf);
        self->buf = NULL;
    }
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);
    Py_CLEAR(self->dict);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
stringio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    stringio *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (stringio *)type->tp_alloc(type, 0);
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

static int
stringio_init(stringio *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"initial_value", "newline", NULL};
    PyObject *value = NULL;
    PyObject *newline_obj = NULL;
    char *newline = "\n";

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO:__init__", kwlist,
                                     &value, &newline_obj))
        return -1;

    /* Parse the newline argument. This used to be done with the 'z'
       specifier, however this allowed any object with the buffer interface to
       be converted. Thus we have to parse it manually since we only want to
       allow unicode objects or None. */
    if (newline_obj == Py_None) {
        newline = NULL;
    }
    else if (newline_obj) {
        if (!PyUnicode_Check(newline_obj)) {
            PyErr_Format(PyExc_TypeError,
                         "newline must be str or None, not %.200s",
                         Py_TYPE(newline_obj)->tp_name);
            return -1;
        }
        newline = _PyUnicode_AsString(newline_obj);
        if (newline == NULL)
            return -1;
    }

    if (newline && newline[0] != '\0'
        && !(newline[0] == '\n' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\n' && newline[2] == '\0')) {
        PyErr_Format(PyExc_ValueError,
                     "illegal newline value: %R", newline_obj);
        return -1;
    }
    if (value && value != Py_None && !PyUnicode_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "initial_value must be str or None, not %.200s",
                     Py_TYPE(value)->tp_name);
        return -1;
    }

    self->ok = 0;

    Py_CLEAR(self->readnl);
    Py_CLEAR(self->writenl);
    Py_CLEAR(self->decoder);

    assert((newline != NULL && newline_obj != Py_None) ||
           (newline == NULL && newline_obj == Py_None));

    if (newline) {
        self->readnl = PyUnicode_FromString(newline);
        if (self->readnl == NULL)
            return -1;
    }
    self->readuniversal = (newline == NULL || newline[0] == '\0');
    self->readtranslate = (newline == NULL);
    /* If newline == "", we don't translate anything.
       If newline == "\n" or newline == None, we translate to "\n", which is
       a no-op.
       (for newline == None, TextIOWrapper translates to os.sepline, but it
       is pointless for StringIO)
    */
    if (newline != NULL && newline[0] == '\r') {
        self->writenl = self->readnl;
        Py_INCREF(self->writenl);
    }

    if (self->readuniversal) {
        self->decoder = PyObject_CallFunction(
            (PyObject *)&PyIncrementalNewlineDecoder_Type,
            "Oi", Py_None, (int) self->readtranslate);
        if (self->decoder == NULL)
            return -1;
    }

    /* Now everything is set up, resize buffer to size of initial value,
       and copy it */
    self->string_size = 0;
    if (value && value != Py_None) {
        Py_ssize_t len = PyUnicode_GetSize(value);
        /* This is a heuristic, for newline translation might change
           the string length. */
        if (resize_buffer(self, len) < 0)
            return -1;
        self->pos = 0;
        if (write_str(self, value) < 0)
            return -1;
    }
    else {
        if (resize_buffer(self, 0) < 0)
            return -1;
    }
    self->pos = 0;

    self->closed = 0;
    self->ok = 1;
    return 0;
}

/* Properties and pseudo-properties */
static PyObject *
stringio_seekable(stringio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    Py_RETURN_TRUE;
}

static PyObject *
stringio_readable(stringio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    Py_RETURN_TRUE;
}

static PyObject *
stringio_writable(stringio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    Py_RETURN_TRUE;
}

static PyObject *
stringio_closed(stringio *self, void *context)
{
    CHECK_INITIALIZED(self);
    return PyBool_FromLong(self->closed);
}

static PyObject *
stringio_line_buffering(stringio *self, void *context)
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    Py_RETURN_FALSE;
}

static PyObject *
stringio_newlines(stringio *self, void *context)
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    if (self->decoder == NULL)
        Py_RETURN_NONE;
    return PyObject_GetAttr(self->decoder, _PyIO_str_newlines);
}

static struct PyMethodDef stringio_methods[] = {
    {"close",    (PyCFunction)stringio_close,    METH_NOARGS,  stringio_close_doc},
    {"getvalue", (PyCFunction)stringio_getvalue, METH_NOARGS,  stringio_getvalue_doc},
    {"read",     (PyCFunction)stringio_read,     METH_VARARGS, stringio_read_doc},
    {"readline", (PyCFunction)stringio_readline, METH_VARARGS, stringio_readline_doc},
    {"tell",     (PyCFunction)stringio_tell,     METH_NOARGS,  stringio_tell_doc},
    {"truncate", (PyCFunction)stringio_truncate, METH_VARARGS, stringio_truncate_doc},
    {"seek",     (PyCFunction)stringio_seek,     METH_VARARGS, stringio_seek_doc},
    {"write",    (PyCFunction)stringio_write,    METH_O,       stringio_write_doc},
    
    {"seekable", (PyCFunction)stringio_seekable, METH_NOARGS},
    {"readable", (PyCFunction)stringio_readable, METH_NOARGS},
    {"writable", (PyCFunction)stringio_writable, METH_NOARGS},
    {NULL, NULL}        /* sentinel */
};

static PyGetSetDef stringio_getset[] = {
    {"closed",         (getter)stringio_closed,         NULL, NULL},
    {"newlines",       (getter)stringio_newlines,       NULL, NULL},
    /*  (following comments straight off of the original Python wrapper:)
        XXX Cruft to support the TextIOWrapper API. This would only
        be meaningful if StringIO supported the buffer attribute.
        Hopefully, a better solution, than adding these pseudo-attributes,
        will be found.
    */
    {"line_buffering", (getter)stringio_line_buffering, NULL, NULL},
    {NULL}
};

PyTypeObject PyStringIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.StringIO",                            /*tp_name*/
    sizeof(stringio),                    /*tp_basicsize*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
                       | Py_TPFLAGS_HAVE_GC,   /*tp_flags*/
    stringio_doc,                              /*tp_doc*/
    (traverseproc)stringio_traverse,           /*tp_traverse*/
    (inquiry)stringio_clear,                   /*tp_clear*/
    0,                                         /*tp_richcompare*/
    offsetof(stringio, weakreflist),            /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    (iternextfunc)stringio_iternext,           /*tp_iternext*/
    stringio_methods,                          /*tp_methods*/
    0,                                         /*tp_members*/
    stringio_getset,                           /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    offsetof(stringio, dict),                  /*tp_dictoffset*/
    (initproc)stringio_init,                   /*tp_init*/
    0,                                         /*tp_alloc*/
    stringio_new,                              /*tp_new*/
};
