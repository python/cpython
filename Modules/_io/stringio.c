#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "accu.h"
#include "_iomodule.h"

/* Implementation note: the buffer is always at least one character longer
   than the enclosed string, for proper functioning of _PyIO_find_line_ending.
*/

#define STATE_REALIZED 1
#define STATE_ACCUMULATING 2

/*[clinic input]
module _io
class _io.StringIO "stringio *" "&PyStringIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c17bc0f42165cd7d]*/

typedef struct {
    PyObject_HEAD
    Py_UCS4 *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    size_t buf_size;

    /* The stringio object can be in two states: accumulating or realized.
       In accumulating state, the internal buffer contains nothing and
       the contents are given by the embedded _PyAccu structure.
       In realized state, the internal buffer is meaningful and the
       _PyAccu is destroyed.
    */
    int state;
    _PyAccu accu;

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

static int _io_StringIO___init__(PyObject *self, PyObject *args, PyObject *kwargs);

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

#define ENSURE_REALIZED(self) \
    if (realize(self) < 0) { \
        return NULL; \
    }


/* Internal routine for changing the size, in terms of characters, of the
   buffer of StringIO objects.  The caller should ensure that the 'size'
   argument is non-negative.  Returns 0 on success, -1 otherwise. */
static int
resize_buffer(stringio *self, size_t size)
{
    /* Here, unsigned types are used to avoid dealing with signed integer
       overflow, which is undefined in C. */
    size_t alloc = self->buf_size;
    Py_UCS4 *new_buf = NULL;

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

    if (alloc > PY_SIZE_MAX / sizeof(Py_UCS4))
        goto overflow;
    new_buf = (Py_UCS4 *)PyMem_Realloc(self->buf, alloc * sizeof(Py_UCS4));
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

static PyObject *
make_intermediate(stringio *self)
{
    PyObject *intermediate = _PyAccu_Finish(&self->accu);
    self->state = STATE_REALIZED;
    if (intermediate == NULL)
        return NULL;
    if (_PyAccu_Init(&self->accu) ||
        _PyAccu_Accumulate(&self->accu, intermediate)) {
        Py_DECREF(intermediate);
        return NULL;
    }
    self->state = STATE_ACCUMULATING;
    return intermediate;
}

static int
realize(stringio *self)
{
    Py_ssize_t len;
    PyObject *intermediate;

    if (self->state == STATE_REALIZED)
        return 0;
    assert(self->state == STATE_ACCUMULATING);
    self->state = STATE_REALIZED;

    intermediate = _PyAccu_Finish(&self->accu);
    if (intermediate == NULL)
        return -1;

    /* Append the intermediate string to the internal buffer.
       The length should be equal to the current cursor position.
     */
    len = PyUnicode_GET_LENGTH(intermediate);
    if (resize_buffer(self, len) < 0) {
        Py_DECREF(intermediate);
        return -1;
    }
    if (!PyUnicode_AsUCS4(intermediate, self->buf, len, 0)) {
        Py_DECREF(intermediate);
        return -1;
    }

    Py_DECREF(intermediate);
    return 0;
}

/* Internal routine for writing a whole PyUnicode object to the buffer of a
   StringIO object. Returns 0 on success, or -1 on error. */
static Py_ssize_t
write_str(stringio *self, PyObject *obj)
{
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
    if (PyUnicode_READY(decoded)) {
        Py_DECREF(decoded);
        return -1;
    }
    len = PyUnicode_GET_LENGTH(decoded);
    assert(len >= 0);

    /* This overflow check is not strictly necessary. However, it avoids us to
       deal with funky things like comparing an unsigned and a signed
       integer. */
    if (self->pos > PY_SSIZE_T_MAX - len) {
        PyErr_SetString(PyExc_OverflowError,
                        "new position too large");
        goto fail;
    }

    if (self->state == STATE_ACCUMULATING) {
        if (self->string_size == self->pos) {
            if (_PyAccu_Accumulate(&self->accu, decoded))
                goto fail;
            goto success;
        }
        if (realize(self))
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
               (self->pos - self->string_size) * sizeof(Py_UCS4));
    }

    /* Copy the data to the internal buffer, overwriting some of the
       existing data if self->pos < self->string_size. */
    if (!PyUnicode_AsUCS4(decoded,
                          self->buf + self->pos,
                          self->buf_size - self->pos,
                          0))
        goto fail;

success:
    /* Set the new length of the internal string if it has changed. */
    self->pos += len;
    if (self->string_size < self->pos)
        self->string_size = self->pos;

    Py_DECREF(decoded);
    return 0;

fail:
    Py_XDECREF(decoded);
    return -1;
}

/*[clinic input]
_io.StringIO.getvalue

Retrieve the entire contents of the object.
[clinic start generated code]*/

static PyObject *
_io_StringIO_getvalue_impl(stringio *self)
/*[clinic end generated code: output=27b6a7bfeaebce01 input=d23cb81d6791cf88]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    if (self->state == STATE_ACCUMULATING)
        return make_intermediate(self);
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, self->buf,
                                     self->string_size);
}

/*[clinic input]
_io.StringIO.tell

Tell the current file position.
[clinic start generated code]*/

static PyObject *
_io_StringIO_tell_impl(stringio *self)
/*[clinic end generated code: output=2e87ac67b116c77b input=ec866ebaff02f405]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    return PyLong_FromSsize_t(self->pos);
}

/*[clinic input]
_io.StringIO.read
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read at most size characters, returned as a string.

If the argument is negative or omitted, read until EOF
is reached. Return an empty string at EOF.
[clinic start generated code]*/

static PyObject *
_io_StringIO_read_impl(stringio *self, Py_ssize_t size)
/*[clinic end generated code: output=ae8cf6002f71626c input=0921093383dfb92d]*/
{
    Py_ssize_t n;
    Py_UCS4 *output;

    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);

    /* adjust invalid sizes */
    n = self->string_size - self->pos;
    if (size < 0 || size > n) {
        size = n;
        if (size < 0)
            size = 0;
    }

    /* Optimization for seek(0); read() */
    if (self->state == STATE_ACCUMULATING && self->pos == 0 && size == n) {
        PyObject *result = make_intermediate(self);
        self->pos = self->string_size;
        return result;
    }

    ENSURE_REALIZED(self);
    output = self->buf + self->pos;
    self->pos += size;
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, output, size);
}

/* Internal helper, used by stringio_readline and stringio_iternext */
static PyObject *
_stringio_readline(stringio *self, Py_ssize_t limit)
{
    Py_UCS4 *start, *end, old_char;
    Py_ssize_t len, consumed;

    /* In case of overseek, return the empty string */
    if (self->pos >= self->string_size)
        return PyUnicode_New(0, 0);

    start = self->buf + self->pos;
    if (limit < 0 || limit > self->string_size - self->pos)
        limit = self->string_size - self->pos;

    end = start + limit;
    old_char = *end;
    *end = '\0';
    len = _PyIO_find_line_ending(
        self->readtranslate, self->readuniversal, self->readnl,
        PyUnicode_4BYTE_KIND, (char*)start, (char*)end, &consumed);
    *end = old_char;
    /* If we haven't found any line ending, we just return everything
       (`consumed` is ignored). */
    if (len < 0)
        len = limit;
    self->pos += len;
    return PyUnicode_FromKindAndData(PyUnicode_4BYTE_KIND, start, len);
}

/*[clinic input]
_io.StringIO.readline
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read until newline or EOF.

Returns an empty string if EOF is hit immediately.
[clinic start generated code]*/

static PyObject *
_io_StringIO_readline_impl(stringio *self, Py_ssize_t size)
/*[clinic end generated code: output=cabd6452f1b7e85d input=a5bd70bf682aa276]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    ENSURE_REALIZED(self);

    return _stringio_readline(self, size);
}

static PyObject *
stringio_iternext(stringio *self)
{
    PyObject *line;

    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    ENSURE_REALIZED(self);

    if (Py_TYPE(self) == &PyStringIO_Type) {
        /* Skip method call overhead for speed */
        line = _stringio_readline(self, -1);
    }
    else {
        /* XXX is subclassing StringIO really supported? */
        line = PyObject_CallMethodObjArgs((PyObject *)self,
                                           _PyIO_str_readline, NULL);
        if (line && !PyUnicode_Check(line)) {
            PyErr_Format(PyExc_OSError,
                         "readline() should have returned a str object, "
                         "not '%.200s'", Py_TYPE(line)->tp_name);
            Py_DECREF(line);
            return NULL;
        }
    }

    if (line == NULL)
        return NULL;

    if (PyUnicode_GET_LENGTH(line) == 0) {
        /* Reached EOF */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

/*[clinic input]
_io.StringIO.truncate
    pos as size: Py_ssize_t(accept={int, NoneType}, c_default="self->pos") = None
    /

Truncate size to pos.

The pos argument defaults to the current file position, as
returned by tell().  The current file position is unchanged.
Returns the new absolute position.
[clinic start generated code]*/

static PyObject *
_io_StringIO_truncate_impl(stringio *self, Py_ssize_t size)
/*[clinic end generated code: output=eb3aef8e06701365 input=5505cff90ca48b96]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);

    if (size < 0) {
        PyErr_Format(PyExc_ValueError,
                     "Negative size value %zd", size);
        return NULL;
    }

    if (size < self->string_size) {
        ENSURE_REALIZED(self);
        if (resize_buffer(self, size) < 0)
            return NULL;
        self->string_size = size;
    }

    return PyLong_FromSsize_t(size);
}

/*[clinic input]
_io.StringIO.seek
    pos: Py_ssize_t
    whence: int = 0
    /

Change stream position.

Seek to character offset pos relative to position indicated by whence:
    0  Start of stream (the default).  pos should be >= 0;
    1  Current position - pos must be 0;
    2  End of stream - pos must be 0.
Returns the new absolute position.
[clinic start generated code]*/

static PyObject *
_io_StringIO_seek_impl(stringio *self, Py_ssize_t pos, int whence)
/*[clinic end generated code: output=e9e0ac9a8ae71c25 input=e3855b24e7cae06a]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);

    if (whence != 0 && whence != 1 && whence != 2) {
        PyErr_Format(PyExc_ValueError,
                     "Invalid whence (%i, should be 0, 1 or 2)", whence);
        return NULL;
    }
    else if (pos < 0 && whence == 0) {
        PyErr_Format(PyExc_ValueError,
                     "Negative seek position %zd", pos);
        return NULL;
    }
    else if (whence != 0 && pos != 0) {
        PyErr_SetString(PyExc_OSError,
                        "Can't do nonzero cur-relative seeks");
        return NULL;
    }

    /* whence = 0: offset relative to beginning of the string.
       whence = 1: no change to current position.
       whence = 2: change position to end of file. */
    if (whence == 1) {
        pos = self->pos;
    }
    else if (whence == 2) {
        pos = self->string_size;
    }

    self->pos = pos;

    return PyLong_FromSsize_t(self->pos);
}

/*[clinic input]
_io.StringIO.write
    s as obj: object
    /

Write string to file.

Returns the number of characters written, which is always equal to
the length of the string.
[clinic start generated code]*/

static PyObject *
_io_StringIO_write(stringio *self, PyObject *obj)
/*[clinic end generated code: output=0deaba91a15b94da input=cf96f3b16586e669]*/
{
    Py_ssize_t size;

    CHECK_INITIALIZED(self);
    if (!PyUnicode_Check(obj)) {
        PyErr_Format(PyExc_TypeError, "string argument expected, got '%s'",
                     Py_TYPE(obj)->tp_name);
        return NULL;
    }
    if (PyUnicode_READY(obj))
        return NULL;
    CHECK_CLOSED(self);
    size = PyUnicode_GET_LENGTH(obj);

    if (size > 0 && write_str(self, obj) < 0)
        return NULL;

    return PyLong_FromSsize_t(size);
}

/*[clinic input]
_io.StringIO.close

Close the IO object.

Attempting any further operation after the object is closed
will raise a ValueError.

This method has no effect if the file is already closed.
[clinic start generated code]*/

static PyObject *
_io_StringIO_close_impl(stringio *self)
/*[clinic end generated code: output=04399355cbe518f1 input=cbc10b45f35d6d46]*/
{
    self->closed = 1;
    /* Free up some memory */
    if (resize_buffer(self, 0) < 0)
        return NULL;
    _PyAccu_Destroy(&self->accu);
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
    _PyAccu_Destroy(&self->accu);
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

    /* tp_alloc initializes all the fields to zero. So we don't have to
       initialize them here. */

    self->buf = (Py_UCS4 *)PyMem_Malloc(0);
    if (self->buf == NULL) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }

    return (PyObject *)self;
}

/*[clinic input]
_io.StringIO.__init__
    initial_value as value: object(c_default="NULL") = ''
    newline as newline_obj: object(c_default="NULL") = '\n'

Text I/O implementation using an in-memory buffer.

The initial_value argument sets the value of object.  The newline
argument is like the one of TextIOWrapper's constructor.
[clinic start generated code]*/

static int
_io_StringIO___init___impl(stringio *self, PyObject *value,
                           PyObject *newline_obj)
/*[clinic end generated code: output=a421ea023b22ef4e input=cee2d9181b2577a3]*/
{
    const char *newline = "\n";
    Py_ssize_t value_len;

    /* Parse the newline argument. We only want to allow unicode objects or
       None. */
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
        newline = PyUnicode_AsUTF8(newline_obj);
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

    _PyAccu_Destroy(&self->accu);
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
       (for newline == None, TextIOWrapper translates to os.linesep, but it
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
    if (value && value != Py_None)
        value_len = PyUnicode_GetLength(value);
    else
        value_len = 0;
    if (value_len > 0) {
        /* This is a heuristic, for newline translation might change
           the string length. */
        if (resize_buffer(self, 0) < 0)
            return -1;
        self->state = STATE_REALIZED;
        self->pos = 0;
        if (write_str(self, value) < 0)
            return -1;
    }
    else {
        /* Empty stringio object, we can start by accumulating */
        if (resize_buffer(self, 0) < 0)
            return -1;
        if (_PyAccu_Init(&self->accu))
            return -1;
        self->state = STATE_ACCUMULATING;
    }
    self->pos = 0;

    self->closed = 0;
    self->ok = 1;
    return 0;
}

/* Properties and pseudo-properties */

/*[clinic input]
_io.StringIO.readable

Returns True if the IO object can be read.
[clinic start generated code]*/

static PyObject *
_io_StringIO_readable_impl(stringio *self)
/*[clinic end generated code: output=b19d44dd8b1ceb99 input=39ce068b224c21ad]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/*[clinic input]
_io.StringIO.writable

Returns True if the IO object can be written.
[clinic start generated code]*/

static PyObject *
_io_StringIO_writable_impl(stringio *self)
/*[clinic end generated code: output=13e4dd77187074ca input=7a691353aac38835]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/*[clinic input]
_io.StringIO.seekable

Returns True if the IO object can be seeked.
[clinic start generated code]*/

static PyObject *
_io_StringIO_seekable_impl(stringio *self)
/*[clinic end generated code: output=4d20b4641c756879 input=4c606d05b32952e6]*/
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/* Pickling support.

   The implementation of __getstate__ is similar to the one for BytesIO,
   except that we also save the newline parameter. For __setstate__ and unlike
   BytesIO, we call __init__ to restore the object's state. Doing so allows us
   to avoid decoding the complex newline state while keeping the object
   representation compact.

   See comment in bytesio.c regarding why only pickle protocols and onward are
   supported.
*/

static PyObject *
stringio_getstate(stringio *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *initvalue = _io_StringIO_getvalue_impl(self);
    PyObject *dict;
    PyObject *state;

    if (initvalue == NULL)
        return NULL;
    if (self->dict == NULL) {
        Py_INCREF(Py_None);
        dict = Py_None;
    }
    else {
        dict = PyDict_Copy(self->dict);
        if (dict == NULL) {
            Py_DECREF(initvalue);
            return NULL;
        }
    }

    state = Py_BuildValue("(OOnN)", initvalue,
                          self->readnl ? self->readnl : Py_None,
                          self->pos, dict);
    Py_DECREF(initvalue);
    return state;
}

static PyObject *
stringio_setstate(stringio *self, PyObject *state)
{
    PyObject *initarg;
    PyObject *position_obj;
    PyObject *dict;
    Py_ssize_t pos;

    assert(state != NULL);
    CHECK_CLOSED(self);

    /* We allow the state tuple to be longer than 4, because we may need
       someday to extend the object's state without breaking
       backward-compatibility. */
    if (!PyTuple_Check(state) || PyTuple_GET_SIZE(state) < 4) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__setstate__ argument should be 4-tuple, got %.200s",
                     Py_TYPE(self)->tp_name, Py_TYPE(state)->tp_name);
        return NULL;
    }

    /* Initialize the object's state. */
    initarg = PyTuple_GetSlice(state, 0, 2);
    if (initarg == NULL)
        return NULL;
    if (_io_StringIO___init__((PyObject *)self, initarg, NULL) < 0) {
        Py_DECREF(initarg);
        return NULL;
    }
    Py_DECREF(initarg);

    /* Restore the buffer state. Even if __init__ did initialize the buffer,
       we have to initialize it again since __init__ may translate the
       newlines in the initial_value string. We clearly do not want that
       because the string value in the state tuple has already been translated
       once by __init__. So we do not take any chance and replace object's
       buffer completely. */
    {
        PyObject *item;
        Py_UCS4 *buf;
        Py_ssize_t bufsize;

        item = PyTuple_GET_ITEM(state, 0);
        buf = PyUnicode_AsUCS4Copy(item);
        if (buf == NULL)
            return NULL;
        bufsize = PyUnicode_GET_LENGTH(item);

        if (resize_buffer(self, bufsize) < 0) {
            PyMem_Free(buf);
            return NULL;
        }
        memcpy(self->buf, buf, bufsize * sizeof(Py_UCS4));
        PyMem_Free(buf);
        self->string_size = bufsize;
    }

    /* Set carefully the position value. Alternatively, we could use the seek
       method instead of modifying self->pos directly to better protect the
       object internal state against errneous (or malicious) inputs. */
    position_obj = PyTuple_GET_ITEM(state, 2);
    if (!PyLong_Check(position_obj)) {
        PyErr_Format(PyExc_TypeError,
                     "third item of state must be an integer, got %.200s",
                     Py_TYPE(position_obj)->tp_name);
        return NULL;
    }
    pos = PyLong_AsSsize_t(position_obj);
    if (pos == -1 && PyErr_Occurred())
        return NULL;
    if (pos < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "position value cannot be negative");
        return NULL;
    }
    self->pos = pos;

    /* Set the dictionary of the instance variables. */
    dict = PyTuple_GET_ITEM(state, 3);
    if (dict != Py_None) {
        if (!PyDict_Check(dict)) {
            PyErr_Format(PyExc_TypeError,
                         "fourth item of state should be a dict, got a %.200s",
                         Py_TYPE(dict)->tp_name);
            return NULL;
        }
        if (self->dict) {
            /* Alternatively, we could replace the internal dictionary
               completely. However, it seems more practical to just update it. */
            if (PyDict_Update(self->dict, dict) < 0)
                return NULL;
        }
        else {
            Py_INCREF(dict);
            self->dict = dict;
        }
    }

    Py_RETURN_NONE;
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

#include "clinic/stringio.c.h"

static struct PyMethodDef stringio_methods[] = {
    _IO_STRINGIO_CLOSE_METHODDEF
    _IO_STRINGIO_GETVALUE_METHODDEF
    _IO_STRINGIO_READ_METHODDEF
    _IO_STRINGIO_READLINE_METHODDEF
    _IO_STRINGIO_TELL_METHODDEF
    _IO_STRINGIO_TRUNCATE_METHODDEF
    _IO_STRINGIO_SEEK_METHODDEF
    _IO_STRINGIO_WRITE_METHODDEF

    _IO_STRINGIO_SEEKABLE_METHODDEF
    _IO_STRINGIO_READABLE_METHODDEF
    _IO_STRINGIO_WRITABLE_METHODDEF

    {"__getstate__", (PyCFunction)stringio_getstate, METH_NOARGS},
    {"__setstate__", (PyCFunction)stringio_setstate, METH_O},
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
    _io_StringIO___init____doc__,              /*tp_doc*/
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
    _io_StringIO___init__,                     /*tp_init*/
    0,                                         /*tp_alloc*/
    stringio_new,                              /*tp_new*/
};
