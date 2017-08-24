#include "Python.h"
#include "structmember.h"       /* for offsetof() */
#include "_iomodule.h"

/*[clinic input]
module _io
class _io.BytesIO "bytesio *" "&PyBytesIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=7f50ec034f5c0b26]*/

typedef struct {
    PyObject_HEAD
    PyObject *buf;
    Py_ssize_t pos;
    Py_ssize_t string_size;
    PyObject *dict;
    PyObject *weakreflist;
    Py_ssize_t exports;
} bytesio;

typedef struct {
    PyObject_HEAD
    bytesio *source;
} bytesiobuf;

/* The bytesio object can be in three states:
  * Py_REFCNT(buf) == 1, exports == 0.
  * Py_REFCNT(buf) > 1.  exports == 0,
    first modification or export causes the internal buffer copying.
  * exports > 0.  Py_REFCNT(buf) == 1, any modifications are forbidden.
*/

#define CHECK_CLOSED(self)                                  \
    if ((self)->buf == NULL) {                              \
        PyErr_SetString(PyExc_ValueError,                   \
                        "I/O operation on closed file.");   \
        return NULL;                                        \
    }

#define CHECK_EXPORTS(self) \
    if ((self)->exports > 0) { \
        PyErr_SetString(PyExc_BufferError, \
                        "Existing exports of data: object cannot be re-sized"); \
        return NULL; \
    }

#define SHARED_BUF(self) (Py_REFCNT((self)->buf) > 1)


/* Internal routine to get a line from the buffer of a BytesIO
   object. Returns the length between the current position to the
   next newline character. */
static Py_ssize_t
scan_eol(bytesio *self, Py_ssize_t len)
{
    const char *start, *n;
    Py_ssize_t maxlen;

    assert(self->buf != NULL);
    assert(self->pos >= 0);

    if (self->pos >= self->string_size)
        return 0;

    /* Move to the end of the line, up to the end of the string, s. */
    maxlen = self->string_size - self->pos;
    if (len < 0 || len > maxlen)
        len = maxlen;

    if (len) {
        start = PyBytes_AS_STRING(self->buf) + self->pos;
        n = memchr(start, '\n', len);
        if (n)
            /* Get the length from the current position to the end of
               the line. */
            len = n - start + 1;
    }
    assert(len >= 0);
    assert(self->pos < PY_SSIZE_T_MAX - len);

    return len;
}

/* Internal routine for detaching the shared buffer of BytesIO objects.
   The caller should ensure that the 'size' argument is non-negative and
   not lesser than self->string_size.  Returns 0 on success, -1 otherwise. */
static int
unshare_buffer(bytesio *self, size_t size)
{
    PyObject *new_buf;
    assert(SHARED_BUF(self));
    assert(self->exports == 0);
    assert(size >= (size_t)self->string_size);
    new_buf = PyBytes_FromStringAndSize(NULL, size);
    if (new_buf == NULL)
        return -1;
    memcpy(PyBytes_AS_STRING(new_buf), PyBytes_AS_STRING(self->buf),
           self->string_size);
    Py_SETREF(self->buf, new_buf);
    return 0;
}

/* Internal routine for changing the size of the buffer of BytesIO objects.
   The caller should ensure that the 'size' argument is non-negative.  Returns
   0 on success, -1 otherwise. */
static int
resize_buffer(bytesio *self, size_t size)
{
    /* Here, unsigned types are used to avoid dealing with signed integer
       overflow, which is undefined in C. */
    size_t alloc = PyBytes_GET_SIZE(self->buf);

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

    if (alloc > ((size_t)-1) / sizeof(char))
        goto overflow;

    if (SHARED_BUF(self)) {
        if (unshare_buffer(self, alloc) < 0)
            return -1;
    }
    else {
        if (_PyBytes_Resize(&self->buf, alloc) < 0)
            return -1;
    }

    return 0;

  overflow:
    PyErr_SetString(PyExc_OverflowError,
                    "new buffer size too large");
    return -1;
}

/* Internal routine for writing a string of bytes to the buffer of a BytesIO
   object. Returns the number of bytes written, or -1 on error. */
static Py_ssize_t
write_bytes(bytesio *self, const char *bytes, Py_ssize_t len)
{
    size_t endpos;
    assert(self->buf != NULL);
    assert(self->pos >= 0);
    assert(len >= 0);

    endpos = (size_t)self->pos + len;
    if (endpos > (size_t)PyBytes_GET_SIZE(self->buf)) {
        if (resize_buffer(self, endpos) < 0)
            return -1;
    }
    else if (SHARED_BUF(self)) {
        if (unshare_buffer(self, Py_MAX(endpos, (size_t)self->string_size)) < 0)
            return -1;
    }

    if (self->pos > self->string_size) {
        /* In case of overseek, pad with null bytes the buffer region between
           the end of stream and the current position.

          0   lo      string_size                           hi
          |   |<---used--->|<----------available----------->|
          |   |            <--to pad-->|<---to write--->    |
          0   buf                   position
        */
        memset(PyBytes_AS_STRING(self->buf) + self->string_size, '\0',
               (self->pos - self->string_size) * sizeof(char));
    }

    /* Copy the data to the internal buffer, overwriting some of the existing
       data if self->pos < self->string_size. */
    memcpy(PyBytes_AS_STRING(self->buf) + self->pos, bytes, len);
    self->pos = endpos;

    /* Set the new length of the internal string if it has changed. */
    if ((size_t)self->string_size < endpos) {
        self->string_size = endpos;
    }

    return len;
}

static PyObject *
bytesio_get_closed(bytesio *self)
{
    if (self->buf == NULL) {
        Py_RETURN_TRUE;
    }
    else {
        Py_RETURN_FALSE;
    }
}

/*[clinic input]
_io.BytesIO.readable

Returns True if the IO object can be read.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_readable_impl(bytesio *self)
/*[clinic end generated code: output=4e93822ad5b62263 input=96c5d0cccfb29f5c]*/
{
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/*[clinic input]
_io.BytesIO.writable

Returns True if the IO object can be written.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_writable_impl(bytesio *self)
/*[clinic end generated code: output=64ff6a254b1150b8 input=700eed808277560a]*/
{
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/*[clinic input]
_io.BytesIO.seekable

Returns True if the IO object can be seeked.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_seekable_impl(bytesio *self)
/*[clinic end generated code: output=6b417f46dcc09b56 input=9421f65627a344dd]*/
{
    CHECK_CLOSED(self);
    Py_RETURN_TRUE;
}

/*[clinic input]
_io.BytesIO.flush

Does nothing.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_flush_impl(bytesio *self)
/*[clinic end generated code: output=187e3d781ca134a0 input=561ea490be4581a7]*/
{
    CHECK_CLOSED(self);
    Py_RETURN_NONE;
}

/*[clinic input]
_io.BytesIO.getbuffer

Get a read-write view over the contents of the BytesIO object.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_getbuffer_impl(bytesio *self)
/*[clinic end generated code: output=72cd7c6e13aa09ed input=8f738ef615865176]*/
{
    PyTypeObject *type = &_PyBytesIOBuffer_Type;
    bytesiobuf *buf;
    PyObject *view;

    CHECK_CLOSED(self);

    buf = (bytesiobuf *) type->tp_alloc(type, 0);
    if (buf == NULL)
        return NULL;
    Py_INCREF(self);
    buf->source = self;
    view = PyMemoryView_FromObject((PyObject *) buf);
    Py_DECREF(buf);
    return view;
}

/*[clinic input]
_io.BytesIO.getvalue

Retrieve the entire contents of the BytesIO object.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_getvalue_impl(bytesio *self)
/*[clinic end generated code: output=b3f6a3233c8fd628 input=4b403ac0af3973ed]*/
{
    CHECK_CLOSED(self);
    if (self->string_size <= 1 || self->exports > 0)
        return PyBytes_FromStringAndSize(PyBytes_AS_STRING(self->buf),
                                         self->string_size);

    if (self->string_size != PyBytes_GET_SIZE(self->buf)) {
        if (SHARED_BUF(self)) {
            if (unshare_buffer(self, self->string_size) < 0)
                return NULL;
        }
        else {
            if (_PyBytes_Resize(&self->buf, self->string_size) < 0)
                return NULL;
        }
    }
    Py_INCREF(self->buf);
    return self->buf;
}

/*[clinic input]
_io.BytesIO.isatty

Always returns False.

BytesIO objects are not connected to a TTY-like device.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_isatty_impl(bytesio *self)
/*[clinic end generated code: output=df67712e669f6c8f input=6f97f0985d13f827]*/
{
    CHECK_CLOSED(self);
    Py_RETURN_FALSE;
}

/*[clinic input]
_io.BytesIO.tell

Current file position, an integer.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_tell_impl(bytesio *self)
/*[clinic end generated code: output=b54b0f93cd0e5e1d input=b106adf099cb3657]*/
{
    CHECK_CLOSED(self);
    return PyLong_FromSsize_t(self->pos);
}

static PyObject *
read_bytes(bytesio *self, Py_ssize_t size)
{
    char *output;

    assert(self->buf != NULL);
    assert(size <= self->string_size);
    if (size > 1 &&
        self->pos == 0 && size == PyBytes_GET_SIZE(self->buf) &&
        self->exports == 0) {
        self->pos += size;
        Py_INCREF(self->buf);
        return self->buf;
    }

    output = PyBytes_AS_STRING(self->buf) + self->pos;
    self->pos += size;
    return PyBytes_FromStringAndSize(output, size);
}

/*[clinic input]
_io.BytesIO.read
    size as arg: object = None
    /

Read at most size bytes, returned as a bytes object.

If the size argument is negative, read until EOF is reached.
Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_read_impl(bytesio *self, PyObject *arg)
/*[clinic end generated code: output=85dacb535c1e1781 input=cc7ba4a797bb1555]*/
{
    Py_ssize_t size, n;

    CHECK_CLOSED(self);

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

    return read_bytes(self, size);
}


/*[clinic input]
_io.BytesIO.read1
    size: object
    /

Read at most size bytes, returned as a bytes object.

If the size argument is negative or omitted, read until EOF is reached.
Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_read1(bytesio *self, PyObject *size)
/*[clinic end generated code: output=16021f5d0ac3d4e2 input=d4f40bb8f2f99418]*/
{
    return _io_BytesIO_read_impl(self, size);
}

/*[clinic input]
_io.BytesIO.readline
    size as arg: object = None
    /

Next line from the file, as a bytes object.

Retain newline.  A non-negative size argument limits the maximum
number of bytes to return (an incomplete line may be returned then).
Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_readline_impl(bytesio *self, PyObject *arg)
/*[clinic end generated code: output=1c2115534a4f9276 input=ca31f06de6eab257]*/
{
    Py_ssize_t size, n;

    CHECK_CLOSED(self);

    if (PyLong_Check(arg)) {
        size = PyLong_AsSsize_t(arg);
        if (size == -1 && PyErr_Occurred())
            return NULL;
    }
    else if (arg == Py_None) {
        /* No size limit, by default. */
        size = -1;
    }
    else {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }

    n = scan_eol(self, size);

    return read_bytes(self, n);
}

/*[clinic input]
_io.BytesIO.readlines
    size as arg: object = None
    /

List of bytes objects, each a line from the file.

Call readline() repeatedly and return a list of the lines so read.
The optional size argument, if given, is an approximate bound on the
total number of bytes in the lines returned.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_readlines_impl(bytesio *self, PyObject *arg)
/*[clinic end generated code: output=09b8e34c880808ff input=691aa1314f2c2a87]*/
{
    Py_ssize_t maxsize, size, n;
    PyObject *result, *line;
    char *output;

    CHECK_CLOSED(self);

    if (PyLong_Check(arg)) {
        maxsize = PyLong_AsSsize_t(arg);
        if (maxsize == -1 && PyErr_Occurred())
            return NULL;
    }
    else if (arg == Py_None) {
        /* No size limit, by default. */
        maxsize = -1;
    }
    else {
        PyErr_Format(PyExc_TypeError, "integer argument expected, got '%s'",
                     Py_TYPE(arg)->tp_name);
        return NULL;
    }

    size = 0;
    result = PyList_New(0);
    if (!result)
        return NULL;

    output = PyBytes_AS_STRING(self->buf) + self->pos;
    while ((n = scan_eol(self, -1)) != 0) {
        self->pos += n;
        line = PyBytes_FromStringAndSize(output, n);
        if (!line)
            goto on_error;
        if (PyList_Append(result, line) == -1) {
            Py_DECREF(line);
            goto on_error;
        }
        Py_DECREF(line);
        size += n;
        if (maxsize > 0 && size >= maxsize)
            break;
        output += n;
    }
    return result;

  on_error:
    Py_DECREF(result);
    return NULL;
}

/*[clinic input]
_io.BytesIO.readinto
    buffer: Py_buffer(accept={rwbuffer})
    /

Read bytes into buffer.

Returns number of bytes read (0 for EOF), or None if the object
is set not to block and has no data to read.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_readinto_impl(bytesio *self, Py_buffer *buffer)
/*[clinic end generated code: output=a5d407217dcf0639 input=1424d0fdce857919]*/
{
    Py_ssize_t len, n;

    CHECK_CLOSED(self);

    /* adjust invalid sizes */
    len = buffer->len;
    n = self->string_size - self->pos;
    if (len > n) {
        len = n;
        if (len < 0)
            len = 0;
    }

    memcpy(buffer->buf, PyBytes_AS_STRING(self->buf) + self->pos, len);
    assert(self->pos + len < PY_SSIZE_T_MAX);
    assert(len >= 0);
    self->pos += len;

    return PyLong_FromSsize_t(len);
}

/*[clinic input]
_io.BytesIO.truncate
    size as arg: object = None
    /

Truncate the file to at most size bytes.

Size defaults to the current file position, as returned by tell().
The current file position is unchanged.  Returns the new size.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_truncate_impl(bytesio *self, PyObject *arg)
/*[clinic end generated code: output=81e6be60e67ddd66 input=11ed1966835462ba]*/
{
    Py_ssize_t size;

    CHECK_CLOSED(self);
    CHECK_EXPORTS(self);

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
                     "negative size value %zd", size);
        return NULL;
    }

    if (size < self->string_size) {
        self->string_size = size;
        if (resize_buffer(self, size) < 0)
            return NULL;
    }

    return PyLong_FromSsize_t(size);
}

static PyObject *
bytesio_iternext(bytesio *self)
{
    Py_ssize_t n;

    CHECK_CLOSED(self);

    n = scan_eol(self, -1);

    if (n == 0)
        return NULL;

    return read_bytes(self, n);
}

/*[clinic input]
_io.BytesIO.seek
    pos: Py_ssize_t
    whence: int = 0
    /

Change stream position.

Seek to byte offset pos relative to position indicated by whence:
     0  Start of stream (the default).  pos should be >= 0;
     1  Current position - pos may be negative;
     2  End of stream - pos usually negative.
Returns the new absolute position.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_seek_impl(bytesio *self, Py_ssize_t pos, int whence)
/*[clinic end generated code: output=c26204a68e9190e4 input=1e875e6ebc652948]*/
{
    CHECK_CLOSED(self);

    if (pos < 0 && whence == 0) {
        PyErr_Format(PyExc_ValueError,
                     "negative seek value %zd", pos);
        return NULL;
    }

    /* whence = 0: offset relative to beginning of the string.
       whence = 1: offset relative to current position.
       whence = 2: offset relative the end of the string. */
    if (whence == 1) {
        if (pos > PY_SSIZE_T_MAX - self->pos) {
            PyErr_SetString(PyExc_OverflowError,
                            "new position too large");
            return NULL;
        }
        pos += self->pos;
    }
    else if (whence == 2) {
        if (pos > PY_SSIZE_T_MAX - self->string_size) {
            PyErr_SetString(PyExc_OverflowError,
                            "new position too large");
            return NULL;
        }
        pos += self->string_size;
    }
    else if (whence != 0) {
        PyErr_Format(PyExc_ValueError,
                     "invalid whence (%i, should be 0, 1 or 2)", whence);
        return NULL;
    }

    if (pos < 0)
        pos = 0;
    self->pos = pos;

    return PyLong_FromSsize_t(self->pos);
}

/*[clinic input]
_io.BytesIO.write
    b: object
    /

Write bytes to file.

Return the number of bytes written.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_write(bytesio *self, PyObject *b)
/*[clinic end generated code: output=53316d99800a0b95 input=f5ec7c8c64ed720a]*/
{
    Py_ssize_t n = 0;
    Py_buffer buf;

    CHECK_CLOSED(self);
    CHECK_EXPORTS(self);

    if (PyObject_GetBuffer(b, &buf, PyBUF_CONTIG_RO) < 0)
        return NULL;

    if (buf.len != 0)
        n = write_bytes(self, buf.buf, buf.len);

    PyBuffer_Release(&buf);
    return n >= 0 ? PyLong_FromSsize_t(n) : NULL;
}

/*[clinic input]
_io.BytesIO.writelines
    lines: object
    /

Write lines to the file.

Note that newlines are not added.  lines can be any iterable object
producing bytes-like objects. This is equivalent to calling write() for
each element.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_writelines(bytesio *self, PyObject *lines)
/*[clinic end generated code: output=7f33aa3271c91752 input=e972539176fc8fc1]*/
{
    PyObject *it, *item;
    PyObject *ret;

    CHECK_CLOSED(self);

    it = PyObject_GetIter(lines);
    if (it == NULL)
        return NULL;

    while ((item = PyIter_Next(it)) != NULL) {
        ret = _io_BytesIO_write(self, item);
        Py_DECREF(item);
        if (ret == NULL) {
            Py_DECREF(it);
            return NULL;
        }
        Py_DECREF(ret);
    }
    Py_DECREF(it);

    /* See if PyIter_Next failed */
    if (PyErr_Occurred())
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]
_io.BytesIO.close

Disable all I/O operations.
[clinic start generated code]*/

static PyObject *
_io_BytesIO_close_impl(bytesio *self)
/*[clinic end generated code: output=1471bb9411af84a0 input=37e1f55556e61f60]*/
{
    CHECK_EXPORTS(self);
    Py_CLEAR(self->buf);
    Py_RETURN_NONE;
}

/* Pickling support.

   Note that only pickle protocol 2 and onward are supported since we use
   extended __reduce__ API of PEP 307 to make BytesIO instances picklable.

   Providing support for protocol < 2 would require the __reduce_ex__ method
   which is notably long-winded when defined properly.

   For BytesIO, the implementation would similar to one coded for
   object.__reduce_ex__, but slightly less general. To be more specific, we
   could call bytesio_getstate directly and avoid checking for the presence of
   a fallback __reduce__ method. However, we would still need a __newobj__
   function to use the efficient instance representation of PEP 307.
 */

static PyObject *
bytesio_getstate(bytesio *self)
{
    PyObject *initvalue = _io_BytesIO_getvalue_impl(self);
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

    state = Py_BuildValue("(OnN)", initvalue, self->pos, dict);
    Py_DECREF(initvalue);
    return state;
}

static PyObject *
bytesio_setstate(bytesio *self, PyObject *state)
{
    PyObject *result;
    PyObject *position_obj;
    PyObject *dict;
    Py_ssize_t pos;

    assert(state != NULL);

    /* We allow the state tuple to be longer than 3, because we may need
       someday to extend the object's state without breaking
       backward-compatibility. */
    if (!PyTuple_Check(state) || Py_SIZE(state) < 3) {
        PyErr_Format(PyExc_TypeError,
                     "%.200s.__setstate__ argument should be 3-tuple, got %.200s",
                     Py_TYPE(self)->tp_name, Py_TYPE(state)->tp_name);
        return NULL;
    }
    CHECK_EXPORTS(self);
    /* Reset the object to its default state. This is only needed to handle
       the case of repeated calls to __setstate__. */
    self->string_size = 0;
    self->pos = 0;

    /* Set the value of the internal buffer. If state[0] does not support the
       buffer protocol, bytesio_write will raise the appropriate TypeError. */
    result = _io_BytesIO_write(self, PyTuple_GET_ITEM(state, 0));
    if (result == NULL)
        return NULL;
    Py_DECREF(result);

    /* Set carefully the position value. Alternatively, we could use the seek
       method instead of modifying self->pos directly to better protect the
       object internal state against errneous (or malicious) inputs. */
    position_obj = PyTuple_GET_ITEM(state, 1);
    if (!PyLong_Check(position_obj)) {
        PyErr_Format(PyExc_TypeError,
                     "second item of state must be an integer, not %.200s",
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
    dict = PyTuple_GET_ITEM(state, 2);
    if (dict != Py_None) {
        if (!PyDict_Check(dict)) {
            PyErr_Format(PyExc_TypeError,
                         "third item of state should be a dict, got a %.200s",
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

static void
bytesio_dealloc(bytesio *self)
{
    _PyObject_GC_UNTRACK(self);
    if (self->exports > 0) {
        PyErr_SetString(PyExc_SystemError,
                        "deallocated BytesIO object has exported buffers");
        PyErr_Print();
    }
    Py_CLEAR(self->buf);
    Py_CLEAR(self->dict);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
bytesio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    bytesio *self;

    assert(type != NULL && type->tp_alloc != NULL);
    self = (bytesio *)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    /* tp_alloc initializes all the fields to zero. So we don't have to
       initialize them here. */

    self->buf = PyBytes_FromStringAndSize(NULL, 0);
    if (self->buf == NULL) {
        Py_DECREF(self);
        return PyErr_NoMemory();
    }

    return (PyObject *)self;
}

/*[clinic input]
_io.BytesIO.__init__
    initial_bytes as initvalue: object(c_default="NULL") = b''

Buffered I/O implementation using an in-memory bytes buffer.
[clinic start generated code]*/

static int
_io_BytesIO___init___impl(bytesio *self, PyObject *initvalue)
/*[clinic end generated code: output=65c0c51e24c5b621 input=aac7f31b67bf0fb6]*/
{
    /* In case, __init__ is called multiple times. */
    self->string_size = 0;
    self->pos = 0;

    if (self->exports > 0) {
        PyErr_SetString(PyExc_BufferError,
                        "Existing exports of data: object cannot be re-sized");
        return -1;
    }
    if (initvalue && initvalue != Py_None) {
        if (PyBytes_CheckExact(initvalue)) {
            Py_INCREF(initvalue);
            Py_XSETREF(self->buf, initvalue);
            self->string_size = PyBytes_GET_SIZE(initvalue);
        }
        else {
            PyObject *res;
            res = _io_BytesIO_write(self, initvalue);
            if (res == NULL)
                return -1;
            Py_DECREF(res);
            self->pos = 0;
        }
    }

    return 0;
}

static PyObject *
bytesio_sizeof(bytesio *self, void *unused)
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self));
    if (self->buf && !SHARED_BUF(self))
        res += _PySys_GetSizeOf(self->buf);
    return PyLong_FromSsize_t(res);
}

static int
bytesio_traverse(bytesio *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
bytesio_clear(bytesio *self)
{
    Py_CLEAR(self->dict);
    return 0;
}


#include "clinic/bytesio.c.h"

static PyGetSetDef bytesio_getsetlist[] = {
    {"closed",  (getter)bytesio_get_closed, NULL,
     "True if the file is closed."},
    {NULL},            /* sentinel */
};

static struct PyMethodDef bytesio_methods[] = {
    _IO_BYTESIO_READABLE_METHODDEF
    _IO_BYTESIO_SEEKABLE_METHODDEF
    _IO_BYTESIO_WRITABLE_METHODDEF
    _IO_BYTESIO_CLOSE_METHODDEF
    _IO_BYTESIO_FLUSH_METHODDEF
    _IO_BYTESIO_ISATTY_METHODDEF
    _IO_BYTESIO_TELL_METHODDEF
    _IO_BYTESIO_WRITE_METHODDEF
    _IO_BYTESIO_WRITELINES_METHODDEF
    _IO_BYTESIO_READ1_METHODDEF
    _IO_BYTESIO_READINTO_METHODDEF
    _IO_BYTESIO_READLINE_METHODDEF
    _IO_BYTESIO_READLINES_METHODDEF
    _IO_BYTESIO_READ_METHODDEF
    _IO_BYTESIO_GETBUFFER_METHODDEF
    _IO_BYTESIO_GETVALUE_METHODDEF
    _IO_BYTESIO_SEEK_METHODDEF
    _IO_BYTESIO_TRUNCATE_METHODDEF
    {"__getstate__",  (PyCFunction)bytesio_getstate,  METH_NOARGS, NULL},
    {"__setstate__",  (PyCFunction)bytesio_setstate,  METH_O, NULL},
    {"__sizeof__", (PyCFunction)bytesio_sizeof,     METH_NOARGS, NULL},
    {NULL, NULL}        /* sentinel */
};

PyTypeObject PyBytesIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BytesIO",                             /*tp_name*/
    sizeof(bytesio),                     /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)bytesio_dealloc,               /*tp_dealloc*/
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
    Py_TPFLAGS_HAVE_GC,                        /*tp_flags*/
    _io_BytesIO___init____doc__,               /*tp_doc*/
    (traverseproc)bytesio_traverse,            /*tp_traverse*/
    (inquiry)bytesio_clear,                    /*tp_clear*/
    0,                                         /*tp_richcompare*/
    offsetof(bytesio, weakreflist),      /*tp_weaklistoffset*/
    PyObject_SelfIter,                         /*tp_iter*/
    (iternextfunc)bytesio_iternext,            /*tp_iternext*/
    bytesio_methods,                           /*tp_methods*/
    0,                                         /*tp_members*/
    bytesio_getsetlist,                        /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    offsetof(bytesio, dict),             /*tp_dictoffset*/
    _io_BytesIO___init__,                      /*tp_init*/
    0,                                         /*tp_alloc*/
    bytesio_new,                               /*tp_new*/
};


/*
 * Implementation of the small intermediate object used by getbuffer().
 * getbuffer() returns a memoryview over this object, which should make it
 * invisible from Python code.
 */

static int
bytesiobuf_getbuffer(bytesiobuf *obj, Py_buffer *view, int flags)
{
    bytesio *b = (bytesio *) obj->source;

    if (view == NULL) {
        PyErr_SetString(PyExc_BufferError,
            "bytesiobuf_getbuffer: view==NULL argument is obsolete");
        return -1;
    }
    if (SHARED_BUF(b)) {
        if (unshare_buffer(b, b->string_size) < 0)
            return -1;
    }

    /* cannot fail if view != NULL and readonly == 0 */
    (void)PyBuffer_FillInfo(view, (PyObject*)obj,
                            PyBytes_AS_STRING(b->buf), b->string_size,
                            0, flags);
    b->exports++;
    return 0;
}

static void
bytesiobuf_releasebuffer(bytesiobuf *obj, Py_buffer *view)
{
    bytesio *b = (bytesio *) obj->source;
    b->exports--;
}

static int
bytesiobuf_traverse(bytesiobuf *self, visitproc visit, void *arg)
{
    Py_VISIT(self->source);
    return 0;
}

static void
bytesiobuf_dealloc(bytesiobuf *self)
{
    /* bpo-31095: UnTrack is needed before calling any callbacks */
    PyObject_GC_UnTrack(self);
    Py_CLEAR(self->source);
    Py_TYPE(self)->tp_free(self);
}

static PyBufferProcs bytesiobuf_as_buffer = {
    (getbufferproc) bytesiobuf_getbuffer,
    (releasebufferproc) bytesiobuf_releasebuffer,
};

PyTypeObject _PyBytesIOBuffer_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io._BytesIOBuffer",                      /*tp_name*/
    sizeof(bytesiobuf),                        /*tp_basicsize*/
    0,                                         /*tp_itemsize*/
    (destructor)bytesiobuf_dealloc,            /*tp_dealloc*/
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
    &bytesiobuf_as_buffer,                     /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,   /*tp_flags*/
    0,                                         /*tp_doc*/
    (traverseproc)bytesiobuf_traverse,         /*tp_traverse*/
    0,                                         /*tp_clear*/
    0,                                         /*tp_richcompare*/
    0,                                         /*tp_weaklistoffset*/
    0,                                         /*tp_iter*/
    0,                                         /*tp_iternext*/
    0,                                         /*tp_methods*/
    0,                                         /*tp_members*/
    0,                                         /*tp_getset*/
    0,                                         /*tp_base*/
    0,                                         /*tp_dict*/
    0,                                         /*tp_descr_get*/
    0,                                         /*tp_descr_set*/
    0,                                         /*tp_dictoffset*/
    0,                                         /*tp_init*/
    0,                                         /*tp_alloc*/
    0,                                         /*tp_new*/
};
