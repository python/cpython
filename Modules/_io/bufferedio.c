/*
    An implementation of Buffered I/O as defined by PEP 3116 - "New I/O"
    
    Classes defined here: BufferedIOBase, BufferedReader, BufferedWriter,
    BufferedRandom.
    
    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "pythread.h"
#include "_iomodule.h"

/*
 * BufferedIOBase class, inherits from IOBase.
 */
PyDoc_STRVAR(bufferediobase_doc,
    "Base class for buffered IO objects.\n"
    "\n"
    "The main difference with RawIOBase is that the read() method\n"
    "supports omitting the size argument, and does not have a default\n"
    "implementation that defers to readinto().\n"
    "\n"
    "In addition, read(), readinto() and write() may raise\n"
    "BlockingIOError if the underlying raw stream is in non-blocking\n"
    "mode and not ready; unlike their raw counterparts, they will never\n"
    "return None.\n"
    "\n"
    "A typical implementation should not inherit from a RawIOBase\n"
    "implementation, but wrap one.\n"
    );

static PyObject *
bufferediobase_readinto(PyObject *self, PyObject *args)
{
    Py_buffer buf;
    Py_ssize_t len;
    PyObject *data;

    if (!PyArg_ParseTuple(args, "w*:readinto", &buf)) {
        return NULL;
    }

    data = PyObject_CallMethod(self, "read", "n", buf.len);
    if (data == NULL)
        goto error;

    if (!PyBytes_Check(data)) {
        Py_DECREF(data);
        PyErr_SetString(PyExc_TypeError, "read() should return bytes");
        goto error;
    }

    len = Py_SIZE(data);
    memcpy(buf.buf, PyBytes_AS_STRING(data), len);

    PyBuffer_Release(&buf);
    Py_DECREF(data);

    return PyLong_FromSsize_t(len);

  error:
    PyBuffer_Release(&buf);
    return NULL;
}

static PyObject *
bufferediobase_unsupported(const char *message)
{
    PyErr_SetString(IO_STATE->unsupported_operation, message);
    return NULL;
}

PyDoc_STRVAR(bufferediobase_detach_doc,
    "Disconnect this buffer from its underlying raw stream and return it.\n"
    "\n"
    "After the raw stream has been detached, the buffer is in an unusable\n"
    "state.\n");

static PyObject *
bufferediobase_detach(PyObject *self)
{
    return bufferediobase_unsupported("detach");
}

PyDoc_STRVAR(bufferediobase_read_doc,
    "Read and return up to n bytes.\n"
    "\n"
    "If the argument is omitted, None, or negative, reads and\n"
    "returns all data until EOF.\n"
    "\n"
    "If the argument is positive, and the underlying raw stream is\n"
    "not 'interactive', multiple raw reads may be issued to satisfy\n"
    "the byte count (unless EOF is reached first).  But for\n"
    "interactive raw streams (as well as sockets and pipes), at most\n"
    "one raw read will be issued, and a short result does not imply\n"
    "that EOF is imminent.\n"
    "\n"
    "Returns an empty bytes object on EOF.\n"
    "\n"
    "Returns None if the underlying raw stream was open in non-blocking\n"
    "mode and no data is available at the moment.\n");

static PyObject *
bufferediobase_read(PyObject *self, PyObject *args)
{
    return bufferediobase_unsupported("read");
}

PyDoc_STRVAR(bufferediobase_read1_doc,
    "Read and return up to n bytes, with at most one read() call\n"
    "to the underlying raw stream. A short result does not imply\n"
    "that EOF is imminent.\n"
    "\n"
    "Returns an empty bytes object on EOF.\n");

static PyObject *
bufferediobase_read1(PyObject *self, PyObject *args)
{
    return bufferediobase_unsupported("read1");
}

PyDoc_STRVAR(bufferediobase_write_doc,
    "Write the given buffer to the IO stream.\n"
    "\n"
    "Returns the number of bytes written, which is never less than\n"
    "len(b).\n"
    "\n"
    "Raises BlockingIOError if the buffer is full and the\n"
    "underlying raw stream cannot accept more data at the moment.\n");

static PyObject *
bufferediobase_write(PyObject *self, PyObject *args)
{
    return bufferediobase_unsupported("write");
}


static PyMethodDef bufferediobase_methods[] = {
    {"detach", (PyCFunction)bufferediobase_detach, METH_NOARGS, bufferediobase_detach_doc},
    {"read", bufferediobase_read, METH_VARARGS, bufferediobase_read_doc},
    {"read1", bufferediobase_read1, METH_VARARGS, bufferediobase_read1_doc},
    {"readinto", bufferediobase_readinto, METH_VARARGS, NULL},
    {"write", bufferediobase_write, METH_VARARGS, bufferediobase_write_doc},
    {NULL, NULL}
};

PyTypeObject PyBufferedIOBase_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io._BufferedIOBase",      /*tp_name*/
    0,                          /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    0,                          /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    bufferediobase_doc,         /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    bufferediobase_methods,     /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    &PyIOBase_Type,             /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    0,                          /* tp_new */
};


typedef struct {
    PyObject_HEAD

    PyObject *raw;
    int ok;    /* Initialized? */
    int detached;
    int readable;
    int writable;
    int deallocating;
    
    /* True if this is a vanilla Buffered object (rather than a user derived
       class) *and* the raw stream is a vanilla FileIO object. */
    int fast_closed_checks;

    /* Absolute position inside the raw stream (-1 if unknown). */
    Py_off_t abs_pos;

    /* A static buffer of size `buffer_size` */
    char *buffer;
    /* Current logical position in the buffer. */
    Py_off_t pos;
    /* Position of the raw stream in the buffer. */
    Py_off_t raw_pos;

    /* Just after the last buffered byte in the buffer, or -1 if the buffer
       isn't ready for reading. */
    Py_off_t read_end;

    /* Just after the last byte actually written */
    Py_off_t write_pos;
    /* Just after the last byte waiting to be written, or -1 if the buffer
       isn't ready for writing. */
    Py_off_t write_end;

#ifdef WITH_THREAD
    PyThread_type_lock lock;
    volatile long owner;
#endif

    Py_ssize_t buffer_size;
    Py_ssize_t buffer_mask;

    PyObject *dict;
    PyObject *weakreflist;
} buffered;

/*
    Implementation notes:
    
    * BufferedReader, BufferedWriter and BufferedRandom try to share most
      methods (this is helped by the members `readable` and `writable`, which
      are initialized in the respective constructors)
    * They also share a single buffer for reading and writing. This enables
      interleaved reads and writes without flushing. It also makes the logic
      a bit trickier to get right.
    * The absolute position of the raw stream is cached, if possible, in the
      `abs_pos` member. It must be updated every time an operation is done
      on the raw stream. If not sure, it can be reinitialized by calling
      _buffered_raw_tell(), which queries the raw stream (_buffered_raw_seek()
      also does it). To read it, use RAW_TELL().
    * Three helpers, _bufferedreader_raw_read, _bufferedwriter_raw_write and
      _bufferedwriter_flush_unlocked do a lot of useful housekeeping.

    NOTE: we should try to maintain block alignment of reads and writes to the
    raw stream (according to the buffer size), but for now it is only done
    in read() and friends.
    
*/

/* These macros protect the buffered object against concurrent operations. */

#ifdef WITH_THREAD

static int
_enter_buffered_busy(buffered *self)
{
    if (self->owner == PyThread_get_thread_ident()) {
        PyErr_Format(PyExc_RuntimeError,
                     "reentrant call inside %R", self);
        return 0;
    }
    Py_BEGIN_ALLOW_THREADS
    PyThread_acquire_lock(self->lock, 1);
    Py_END_ALLOW_THREADS
    return 1;
}

#define ENTER_BUFFERED(self) \
    ( (PyThread_acquire_lock(self->lock, 0) ? \
       1 : _enter_buffered_busy(self)) \
     && (self->owner = PyThread_get_thread_ident(), 1) )

#define LEAVE_BUFFERED(self) \
    do { \
        self->owner = 0; \
        PyThread_release_lock(self->lock); \
    } while(0);

#else
#define ENTER_BUFFERED(self) 1
#define LEAVE_BUFFERED(self)
#endif

#define CHECK_INITIALIZED(self) \
    if (self->ok <= 0) { \
        if (self->detached) { \
            PyErr_SetString(PyExc_ValueError, \
                 "raw stream has been detached"); \
        } else { \
            PyErr_SetString(PyExc_ValueError, \
                "I/O operation on uninitialized object"); \
        } \
        return NULL; \
    }

#define CHECK_INITIALIZED_INT(self) \
    if (self->ok <= 0) { \
        if (self->detached) { \
            PyErr_SetString(PyExc_ValueError, \
                 "raw stream has been detached"); \
        } else { \
            PyErr_SetString(PyExc_ValueError, \
                "I/O operation on uninitialized object"); \
        } \
        return -1; \
    }

#define IS_CLOSED(self) \
    (self->fast_closed_checks \
     ? _PyFileIO_closed(self->raw) \
     : buffered_closed(self))

#define CHECK_CLOSED(self, error_msg) \
    if (IS_CLOSED(self)) { \
        PyErr_SetString(PyExc_ValueError, error_msg); \
        return NULL; \
    }


#define VALID_READ_BUFFER(self) \
    (self->readable && self->read_end != -1)

#define VALID_WRITE_BUFFER(self) \
    (self->writable && self->write_end != -1)

#define ADJUST_POSITION(self, _new_pos) \
    do { \
        self->pos = _new_pos; \
        if (VALID_READ_BUFFER(self) && self->read_end < self->pos) \
            self->read_end = self->pos; \
    } while(0)

#define READAHEAD(self) \
    ((self->readable && VALID_READ_BUFFER(self)) \
        ? (self->read_end - self->pos) : 0)

#define RAW_OFFSET(self) \
    (((VALID_READ_BUFFER(self) || VALID_WRITE_BUFFER(self)) \
        && self->raw_pos >= 0) ? self->raw_pos - self->pos : 0)

#define RAW_TELL(self) \
    (self->abs_pos != -1 ? self->abs_pos : _buffered_raw_tell(self))

#define MINUS_LAST_BLOCK(self, size) \
    (self->buffer_mask ? \
        (size & ~self->buffer_mask) : \
        (self->buffer_size * (size / self->buffer_size)))


static void
buffered_dealloc(buffered *self)
{
    self->deallocating = 1;
    if (self->ok && _PyIOBase_finalize((PyObject *) self) < 0)
        return;
    _PyObject_GC_UNTRACK(self);
    self->ok = 0;
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);
    Py_CLEAR(self->raw);
    if (self->buffer) {
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }
#ifdef WITH_THREAD
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }
#endif
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
buffered_traverse(buffered *self, visitproc visit, void *arg)
{
    Py_VISIT(self->raw);
    Py_VISIT(self->dict);
    return 0;
}

static int
buffered_clear(buffered *self)
{
    if (self->ok && _PyIOBase_finalize((PyObject *) self) < 0)
        return -1;
    self->ok = 0;
    Py_CLEAR(self->raw);
    Py_CLEAR(self->dict);
    return 0;
}

/* Because this can call arbitrary code, it shouldn't be called when
   the refcount is 0 (that is, not directly from tp_dealloc unless
   the refcount has been temporarily re-incremented). */
static PyObject *
buffered_dealloc_warn(buffered *self, PyObject *source)
{
    if (self->ok && self->raw) {
        PyObject *r;
        r = PyObject_CallMethod(self->raw, "_dealloc_warn", "O", source);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    Py_RETURN_NONE;
}

/*
 * _BufferedIOMixin methods
 * This is not a class, just a collection of methods that will be reused
 * by BufferedReader and BufferedWriter
 */

/* Flush and close */

static PyObject *
buffered_simple_flush(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_flush, NULL);
}

static int
buffered_closed(buffered *self)
{
    int closed;
    PyObject *res;
    CHECK_INITIALIZED_INT(self)
    res = PyObject_GetAttr(self->raw, _PyIO_str_closed);
    if (res == NULL)
        return -1;
    closed = PyObject_IsTrue(res);
    Py_DECREF(res);
    return closed;
}

static PyObject *
buffered_closed_get(buffered *self, void *context)
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttr(self->raw, _PyIO_str_closed);
}

static PyObject *
buffered_close(buffered *self, PyObject *args)
{
    PyObject *res = NULL;
    int r;

    CHECK_INITIALIZED(self)
    if (!ENTER_BUFFERED(self))
        return NULL;

    r = buffered_closed(self);
    if (r < 0)
        goto end;
    if (r > 0) {
        res = Py_None;
        Py_INCREF(res);
        goto end;
    }

    if (self->deallocating) {
        PyObject *r = buffered_dealloc_warn(self, (PyObject *) self);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    /* flush() will most probably re-take the lock, so drop it first */
    LEAVE_BUFFERED(self)
    res = PyObject_CallMethodObjArgs((PyObject *)self, _PyIO_str_flush, NULL);
    if (!ENTER_BUFFERED(self))
        return NULL;
    if (res == NULL) {
        goto end;
    }
    Py_XDECREF(res);

    res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_close, NULL);

end:
    LEAVE_BUFFERED(self)
    return res;
}

/* detach */

static PyObject *
buffered_detach(buffered *self, PyObject *args)
{
    PyObject *raw, *res;
    CHECK_INITIALIZED(self)
    res = PyObject_CallMethodObjArgs((PyObject *)self, _PyIO_str_flush, NULL);
    if (res == NULL)
        return NULL;
    Py_DECREF(res);
    raw = self->raw;
    self->raw = NULL;
    self->detached = 1;
    self->ok = 0;
    return raw;
}

/* Inquiries */

static PyObject *
buffered_seekable(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_seekable, NULL);
}

static PyObject *
buffered_readable(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_readable, NULL);
}

static PyObject *
buffered_writable(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_writable, NULL);
}

static PyObject *
buffered_name_get(buffered *self, void *context)
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttrString(self->raw, "name");
}

static PyObject *
buffered_mode_get(buffered *self, void *context)
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttrString(self->raw, "mode");
}

/* Lower-level APIs */

static PyObject *
buffered_fileno(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_fileno, NULL);
}

static PyObject *
buffered_isatty(buffered *self, PyObject *args)
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodObjArgs(self->raw, _PyIO_str_isatty, NULL);
}

/* Serialization */

static PyObject *
buffered_getstate(buffered *self, PyObject *args)
{
    PyErr_Format(PyExc_TypeError,
                 "cannot serialize '%s' object", Py_TYPE(self)->tp_name);
    return NULL;
}

/* Forward decls */
static PyObject *
_bufferedwriter_flush_unlocked(buffered *, int);
static Py_ssize_t
_bufferedreader_fill_buffer(buffered *self);
static void
_bufferedreader_reset_buf(buffered *self);
static void
_bufferedwriter_reset_buf(buffered *self);
static PyObject *
_bufferedreader_peek_unlocked(buffered *self, Py_ssize_t);
static PyObject *
_bufferedreader_read_all(buffered *self);
static PyObject *
_bufferedreader_read_fast(buffered *self, Py_ssize_t);
static PyObject *
_bufferedreader_read_generic(buffered *self, Py_ssize_t);


/*
 * Helpers
 */

/* Returns the address of the `written` member if a BlockingIOError was
   raised, NULL otherwise. The error is always re-raised. */
static Py_ssize_t *
_buffered_check_blocking_error(void)
{
    PyObject *t, *v, *tb;
    PyBlockingIOErrorObject *err;

    PyErr_Fetch(&t, &v, &tb);
    if (v == NULL || !PyErr_GivenExceptionMatches(v, PyExc_BlockingIOError)) {
        PyErr_Restore(t, v, tb);
        return NULL;
    }
    err = (PyBlockingIOErrorObject *) v;
    /* TODO: sanity check (err->written >= 0) */
    PyErr_Restore(t, v, tb);
    return &err->written;
}

static Py_off_t
_buffered_raw_tell(buffered *self)
{
    Py_off_t n;
    PyObject *res;
    res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_tell, NULL);
    if (res == NULL)
        return -1;
    n = PyNumber_AsOff_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_IOError,
                         "Raw stream returned invalid position %" PY_PRIdOFF,
			 (PY_OFF_T_COMPAT)n);
        return -1;
    }
    self->abs_pos = n;
    return n;
}

static Py_off_t
_buffered_raw_seek(buffered *self, Py_off_t target, int whence)
{
    PyObject *res, *posobj, *whenceobj;
    Py_off_t n;

    posobj = PyLong_FromOff_t(target);
    if (posobj == NULL)
        return -1;
    whenceobj = PyLong_FromLong(whence);
    if (whenceobj == NULL) {
        Py_DECREF(posobj);
        return -1;
    }
    res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_seek,
                                     posobj, whenceobj, NULL);
    Py_DECREF(posobj);
    Py_DECREF(whenceobj);
    if (res == NULL)
        return -1;
    n = PyNumber_AsOff_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_IOError,
                         "Raw stream returned invalid position %" PY_PRIdOFF,
			 (PY_OFF_T_COMPAT)n);
        return -1;
    }
    self->abs_pos = n;
    return n;
}

static int
_buffered_init(buffered *self)
{
    Py_ssize_t n;
    if (self->buffer_size <= 0) {
        PyErr_SetString(PyExc_ValueError,
            "buffer size must be strictly positive");
        return -1;
    }
    if (self->buffer)
        PyMem_Free(self->buffer);
    self->buffer = PyMem_Malloc(self->buffer_size);
    if (self->buffer == NULL) {
        PyErr_NoMemory();
        return -1;
    }
#ifdef WITH_THREAD
    if (self->lock)
        PyThread_free_lock(self->lock);
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "can't allocate read lock");
        return -1;
    }
    self->owner = 0;
#endif
    /* Find out whether buffer_size is a power of 2 */
    /* XXX is this optimization useful? */
    for (n = self->buffer_size - 1; n & 1; n >>= 1)
        ;
    if (n == 0)
        self->buffer_mask = self->buffer_size - 1;
    else
        self->buffer_mask = 0;
    if (_buffered_raw_tell(self) == -1)
        PyErr_Clear();
    return 0;
}

/* Return 1 if an EnvironmentError with errno == EINTR is set (and then
   clears the error indicator), 0 otherwise.
   Should only be called when PyErr_Occurred() is true.
*/
static int
_trap_eintr(void)
{
    static PyObject *eintr_int = NULL;
    PyObject *typ, *val, *tb;
    PyEnvironmentErrorObject *env_err;

    if (eintr_int == NULL) {
        eintr_int = PyLong_FromLong(EINTR);
        assert(eintr_int != NULL);
    }
    if (!PyErr_ExceptionMatches(PyExc_EnvironmentError))
        return 0;
    PyErr_Fetch(&typ, &val, &tb);
    PyErr_NormalizeException(&typ, &val, &tb);
    env_err = (PyEnvironmentErrorObject *) val;
    assert(env_err != NULL);
    if (env_err->myerrno != NULL &&
        PyObject_RichCompareBool(env_err->myerrno, eintr_int, Py_EQ) > 0) {
        Py_DECREF(typ);
        Py_DECREF(val);
        Py_XDECREF(tb);
        return 1;
    }
    /* This silences any error set by PyObject_RichCompareBool() */
    PyErr_Restore(typ, val, tb);
    return 0;
}

/*
 * Shared methods and wrappers
 */

static PyObject *
buffered_flush(buffered *self, PyObject *args)
{
    PyObject *res;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "flush of closed file")

    if (!ENTER_BUFFERED(self))
        return NULL;
    res = _bufferedwriter_flush_unlocked(self, 0);
    if (res != NULL && self->readable) {
        /* Rewind the raw stream so that its position corresponds to
           the current logical position. */
        Py_off_t n;
        n = _buffered_raw_seek(self, -RAW_OFFSET(self), 1);
        if (n == -1)
            Py_CLEAR(res);
        _bufferedreader_reset_buf(self);
    }
    LEAVE_BUFFERED(self)

    return res;
}

static PyObject *
buffered_peek(buffered *self, PyObject *args)
{
    Py_ssize_t n = 0;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "|n:peek", &n)) {
        return NULL;
    }

    if (!ENTER_BUFFERED(self))
        return NULL;

    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 1);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }
    res = _bufferedreader_peek_unlocked(self, n);

end:
    LEAVE_BUFFERED(self)
    return res;
}

static PyObject *
buffered_read(buffered *self, PyObject *args)
{
    Py_ssize_t n = -1;
    PyObject *res;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "|O&:read", &_PyIO_ConvertSsize_t, &n)) {
        return NULL;
    }
    if (n < -1) {
        PyErr_SetString(PyExc_ValueError,
                        "read length must be positive or -1");
        return NULL;
    }

    CHECK_CLOSED(self, "read of closed file")

    if (n == -1) {
        /* The number of bytes is unspecified, read until the end of stream */
        if (!ENTER_BUFFERED(self))
            return NULL;
        res = _bufferedreader_read_all(self);
        LEAVE_BUFFERED(self)
    }
    else {
        res = _bufferedreader_read_fast(self, n);
        if (res == Py_None) {
            Py_DECREF(res);
            if (!ENTER_BUFFERED(self))
                return NULL;
            res = _bufferedreader_read_generic(self, n);
            LEAVE_BUFFERED(self)
        }
    }

    return res;
}

static PyObject *
buffered_read1(buffered *self, PyObject *args)
{
    Py_ssize_t n, have, r;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "n:read1", &n)) {
        return NULL;
    }

    if (n < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "read length must be positive");
        return NULL;
    }
    if (n == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    if (!ENTER_BUFFERED(self))
        return NULL;
    
    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 1);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }

    /* Return up to n bytes.  If at least one byte is buffered, we
       only return buffered bytes.  Otherwise, we do one raw read. */

    /* XXX: this mimicks the io.py implementation but is probably wrong.
       If we need to read from the raw stream, then we could actually read
       all `n` bytes asked by the caller (and possibly more, so as to fill
       our buffer for the next reads). */

    have = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (have > 0) {
        if (n > have)
            n = have;
        res = PyBytes_FromStringAndSize(self->buffer + self->pos, n);
        if (res == NULL)
            goto end;
        self->pos += n;
        goto end;
    }

    /* Fill the buffer from the raw stream, and copy it to the result. */
    _bufferedreader_reset_buf(self);
    r = _bufferedreader_fill_buffer(self);
    if (r == -1)
        goto end;
    if (r == -2)
        r = 0;
    if (n > r)
        n = r;
    res = PyBytes_FromStringAndSize(self->buffer, n);
    if (res == NULL)
        goto end;
    self->pos = n;

end:
    LEAVE_BUFFERED(self)
    return res;
}

static PyObject *
buffered_readinto(buffered *self, PyObject *args)
{
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    
    /* TODO: use raw.readinto() instead! */
    if (self->writable) {
        if (!ENTER_BUFFERED(self))
            return NULL;
        res = _bufferedwriter_flush_unlocked(self, 0);
        LEAVE_BUFFERED(self)
        if (res == NULL)
            goto end;
        Py_DECREF(res);
    }
    res = bufferediobase_readinto((PyObject *)self, args);

end:
    return res;
}

static PyObject *
_buffered_readline(buffered *self, Py_ssize_t limit)
{
    PyObject *res = NULL;
    PyObject *chunks = NULL;
    Py_ssize_t n, written = 0;
    const char *start, *s, *end;

    CHECK_CLOSED(self, "readline of closed file")

    /* First, try to find a line in the buffer. This can run unlocked because
       the calls to the C API are simple enough that they can't trigger
       any thread switch. */
    n = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (limit >= 0 && n > limit)
        n = limit;
    start = self->buffer + self->pos;
    s = memchr(start, '\n', n);
    if (s != NULL) {
        res = PyBytes_FromStringAndSize(start, s - start + 1);
        if (res != NULL)
            self->pos += s - start + 1;
        goto end_unlocked;
    }
    if (n == limit) {
        res = PyBytes_FromStringAndSize(start, n);
        if (res != NULL)
            self->pos += n;
        goto end_unlocked;
    }

    if (!ENTER_BUFFERED(self))
        goto end_unlocked;

    /* Now we try to get some more from the raw stream */
    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 1);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }
    chunks = PyList_New(0);
    if (chunks == NULL)
        goto end;
    if (n > 0) {
        res = PyBytes_FromStringAndSize(start, n);
        if (res == NULL)
            goto end;
        if (PyList_Append(chunks, res) < 0) {
            Py_CLEAR(res);
            goto end;
        }
        Py_CLEAR(res);
        written += n;
        if (limit >= 0)
            limit -= n;
    }

    for (;;) {
        _bufferedreader_reset_buf(self);
        n = _bufferedreader_fill_buffer(self);
        if (n == -1)
            goto end;
        if (n <= 0)
            break;
        if (limit >= 0 && n > limit)
            n = limit;
        start = self->buffer;
        end = start + n;
        s = start;
        while (s < end) {
            if (*s++ == '\n') {
                res = PyBytes_FromStringAndSize(start, s - start);
                if (res == NULL)
                    goto end;
                self->pos = s - start;
                goto found;
            }
        }
        res = PyBytes_FromStringAndSize(start, n);
        if (res == NULL)
            goto end;
        if (n == limit) {
            self->pos = n;
            break;
        }
        if (PyList_Append(chunks, res) < 0) {
            Py_CLEAR(res);
            goto end;
        }
        Py_CLEAR(res);
        written += n;
        if (limit >= 0)
            limit -= n;
    }
found:
    if (res != NULL && PyList_Append(chunks, res) < 0) {
        Py_CLEAR(res);
        goto end;
    }
    Py_CLEAR(res);
    res = _PyBytes_Join(_PyIO_empty_bytes, chunks);

end:
    LEAVE_BUFFERED(self)
end_unlocked:
    Py_XDECREF(chunks);
    return res;
}

static PyObject *
buffered_readline(buffered *self, PyObject *args)
{
    Py_ssize_t limit = -1;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "|O&:readline", &_PyIO_ConvertSsize_t, &limit))
        return NULL;
    return _buffered_readline(self, limit);
}


static PyObject *
buffered_tell(buffered *self, PyObject *args)
{
    Py_off_t pos;

    CHECK_INITIALIZED(self)
    pos = _buffered_raw_tell(self);
    if (pos == -1)
        return NULL;
    pos -= RAW_OFFSET(self);
    /* TODO: sanity check (pos >= 0) */
    return PyLong_FromOff_t(pos);
}

static PyObject *
buffered_seek(buffered *self, PyObject *args)
{
    Py_off_t target, n;
    int whence = 0;
    PyObject *targetobj, *res = NULL;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "O|i:seek", &targetobj, &whence)) {
        return NULL;
    }
    if (whence < 0 || whence > 2) {
        PyErr_Format(PyExc_ValueError,
                     "whence must be between 0 and 2, not %d", whence);
        return NULL;
    }

    CHECK_CLOSED(self, "seek of closed file")

    target = PyNumber_AsOff_t(targetobj, PyExc_ValueError);
    if (target == -1 && PyErr_Occurred())
        return NULL;

    if (whence != 2 && self->readable) {
        Py_off_t current, avail;
        /* Check if seeking leaves us inside the current buffer,
           so as to return quickly if possible. Also, we needn't take the
           lock in this fast path.
           Don't know how to do that when whence == 2, though. */
        /* NOTE: RAW_TELL() can release the GIL but the object is in a stable
           state at this point. */
        current = RAW_TELL(self);
        avail = READAHEAD(self);
        if (avail > 0) {
            Py_off_t offset;
            if (whence == 0)
                offset = target - (current - RAW_OFFSET(self));
            else
                offset = target;
            if (offset >= -self->pos && offset <= avail) {
                self->pos += offset;
                return PyLong_FromOff_t(current - avail + offset);
            }
        }
    }

    if (!ENTER_BUFFERED(self))
        return NULL;

    /* Fallback: invoke raw seek() method and clear buffer */
    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 0);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
        _bufferedwriter_reset_buf(self);
    }

    /* TODO: align on block boundary and read buffer if needed? */
    if (whence == 1)
        target -= RAW_OFFSET(self);
    n = _buffered_raw_seek(self, target, whence);
    if (n == -1)
        goto end;
    self->raw_pos = -1;
    res = PyLong_FromOff_t(n);
    if (res != NULL && self->readable)
        _bufferedreader_reset_buf(self);

end:
    LEAVE_BUFFERED(self)
    return res;
}

static PyObject *
buffered_truncate(buffered *self, PyObject *args)
{
    PyObject *pos = Py_None;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "|O:truncate", &pos)) {
        return NULL;
    }

    if (!ENTER_BUFFERED(self))
        return NULL;

    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 0);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }
    if (self->readable) {
        if (pos == Py_None) {
            /* Rewind the raw stream so that its position corresponds to
               the current logical position. */
            if (_buffered_raw_seek(self, -RAW_OFFSET(self), 1) == -1)
                goto end;
        }
        _bufferedreader_reset_buf(self);
    }
    res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_truncate, pos, NULL);
    if (res == NULL)
        goto end;
    /* Reset cached position */
    if (_buffered_raw_tell(self) == -1)
        PyErr_Clear();

end:
    LEAVE_BUFFERED(self)
    return res;
}

static PyObject *
buffered_iternext(buffered *self)
{
    PyObject *line;
    PyTypeObject *tp;

    CHECK_INITIALIZED(self);

    tp = Py_TYPE(self);
    if (tp == &PyBufferedReader_Type ||
        tp == &PyBufferedRandom_Type) {
        /* Skip method call overhead for speed */
        line = _buffered_readline(self, -1);
    }
    else {
        line = PyObject_CallMethodObjArgs((PyObject *)self,
                                           _PyIO_str_readline, NULL);
        if (line && !PyBytes_Check(line)) {
            PyErr_Format(PyExc_IOError,
                         "readline() should have returned a bytes object, "
                         "not '%.200s'", Py_TYPE(line)->tp_name);
            Py_DECREF(line);
            return NULL;
        }
    }

    if (line == NULL)
        return NULL;

    if (PyBytes_GET_SIZE(line) == 0) {
        /* Reached EOF or would have blocked */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

static PyObject *
buffered_repr(buffered *self)
{
    PyObject *nameobj, *res;

    nameobj = PyObject_GetAttrString((PyObject *) self, "name");
    if (nameobj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_Clear();
        else
            return NULL;
        res = PyUnicode_FromFormat("<%s>", Py_TYPE(self)->tp_name);
    }
    else {
        res = PyUnicode_FromFormat("<%s name=%R>",
                                   Py_TYPE(self)->tp_name, nameobj);
        Py_DECREF(nameobj);
    }
    return res;
}

/*
 * class BufferedReader
 */

PyDoc_STRVAR(bufferedreader_doc,
             "Create a new buffered reader using the given readable raw IO object.");

static void _bufferedreader_reset_buf(buffered *self)
{
    self->read_end = -1;
}

static int
bufferedreader_init(buffered *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"raw", "buffer_size", NULL};
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;
    PyObject *raw;

    self->ok = 0;
    self->detached = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|n:BufferedReader", kwlist,
                                     &raw, &buffer_size)) {
        return -1;
    }

    if (_PyIOBase_check_readable(raw, Py_True) == NULL)
        return -1;

    Py_CLEAR(self->raw);
    Py_INCREF(raw);
    self->raw = raw;
    self->buffer_size = buffer_size;
    self->readable = 1;
    self->writable = 0;

    if (_buffered_init(self) < 0)
        return -1;
    _bufferedreader_reset_buf(self);

    self->fast_closed_checks = (Py_TYPE(self) == &PyBufferedReader_Type &&
                                Py_TYPE(raw) == &PyFileIO_Type);

    self->ok = 1;
    return 0;
}

static Py_ssize_t
_bufferedreader_raw_read(buffered *self, char *start, Py_ssize_t len)
{
    Py_buffer buf;
    PyObject *memobj, *res;
    Py_ssize_t n;
    /* NOTE: the buffer needn't be released as its object is NULL. */
    if (PyBuffer_FillInfo(&buf, NULL, start, len, 0, PyBUF_CONTIG) == -1)
        return -1;
    memobj = PyMemoryView_FromBuffer(&buf);
    if (memobj == NULL)
        return -1;
    /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals() when EINTR
       occurs so we needn't do it ourselves.
       We then retry reading, ignoring the signal if no handler has
       raised (see issue #10956).
    */
    do {
        res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_readinto, memobj, NULL);
    } while (res == NULL && _trap_eintr());
    Py_DECREF(memobj);
    if (res == NULL)
        return -1;
    if (res == Py_None) {
        /* Non-blocking stream would have blocked. Special return code! */
        Py_DECREF(res);
        return -2;
    }
    n = PyNumber_AsSsize_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0 || n > len) {
        PyErr_Format(PyExc_IOError,
                     "raw readinto() returned invalid length %zd "
                     "(should have been between 0 and %zd)", n, len);
        return -1;
    }
    if (n > 0 && self->abs_pos != -1)
        self->abs_pos += n;
    return n;
}

static Py_ssize_t
_bufferedreader_fill_buffer(buffered *self)
{
    Py_ssize_t start, len, n;
    if (VALID_READ_BUFFER(self))
        start = Py_SAFE_DOWNCAST(self->read_end, Py_off_t, Py_ssize_t);
    else
        start = 0;
    len = self->buffer_size - start;
    n = _bufferedreader_raw_read(self, self->buffer + start, len);
    if (n <= 0)
        return n;
    self->read_end = start + n;
    self->raw_pos = start + n;
    return n;
}

static PyObject *
_bufferedreader_read_all(buffered *self)
{
    Py_ssize_t current_size;
    PyObject *res, *data = NULL;
    PyObject *chunks = PyList_New(0);

    if (chunks == NULL)
        return NULL;

    /* First copy what we have in the current buffer. */
    current_size = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (current_size) {
        data = PyBytes_FromStringAndSize(
            self->buffer + self->pos, current_size);
        if (data == NULL) {
            Py_DECREF(chunks);
            return NULL;
        }
    }
    _bufferedreader_reset_buf(self);
    /* We're going past the buffer's bounds, flush it */
    if (self->writable) {
        res = _bufferedwriter_flush_unlocked(self, 1);
        if (res == NULL) {
            Py_DECREF(chunks);
            return NULL;
        }
        Py_CLEAR(res);
    }
    while (1) {
        if (data) {
            if (PyList_Append(chunks, data) < 0) {
                Py_DECREF(data);
                Py_DECREF(chunks);
                return NULL;
            }
            Py_DECREF(data);
        }

        /* Read until EOF or until read() would block. */
        data = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_read, NULL);
        if (data == NULL) {
            Py_DECREF(chunks);
            return NULL;
        }
        if (data != Py_None && !PyBytes_Check(data)) {
            Py_DECREF(data);
            Py_DECREF(chunks);
            PyErr_SetString(PyExc_TypeError, "read() should return bytes");
            return NULL;
        }
        if (data == Py_None || PyBytes_GET_SIZE(data) == 0) {
            if (current_size == 0) {
                Py_DECREF(chunks);
                return data;
            }
            else {
                res = _PyBytes_Join(_PyIO_empty_bytes, chunks);
                Py_DECREF(data);
                Py_DECREF(chunks);
                return res;
            }
        }
        current_size += PyBytes_GET_SIZE(data);
        if (self->abs_pos != -1)
            self->abs_pos += PyBytes_GET_SIZE(data);
    }
}

/* Read n bytes from the buffer if it can, otherwise return None.
   This function is simple enough that it can run unlocked. */
static PyObject *
_bufferedreader_read_fast(buffered *self, Py_ssize_t n)
{
    Py_ssize_t current_size;

    current_size = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (n <= current_size) {
        /* Fast path: the data to read is fully buffered. */
        PyObject *res = PyBytes_FromStringAndSize(self->buffer + self->pos, n);
        if (res != NULL)
            self->pos += n;
        return res;
    }
    Py_RETURN_NONE;
}

/* Generic read function: read from the stream until enough bytes are read,
 * or until an EOF occurs or until read() would block.
 */
static PyObject *
_bufferedreader_read_generic(buffered *self, Py_ssize_t n)
{
    PyObject *res = NULL;
    Py_ssize_t current_size, remaining, written;
    char *out;

    current_size = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (n <= current_size)
        return _bufferedreader_read_fast(self, n);

    res = PyBytes_FromStringAndSize(NULL, n);
    if (res == NULL)
        goto error;
    out = PyBytes_AS_STRING(res);
    remaining = n;
    written = 0;
    if (current_size > 0) {
        memcpy(out, self->buffer + self->pos, current_size);
        remaining -= current_size;
        written += current_size;
    }
    _bufferedreader_reset_buf(self);
    while (remaining > 0) {
        /* We want to read a whole block at the end into buffer.
           If we had readv() we could do this in one pass. */
        Py_ssize_t r = MINUS_LAST_BLOCK(self, remaining);
        if (r == 0)
            break;
        r = _bufferedreader_raw_read(self, out + written, r);
        if (r == -1)
            goto error;
        if (r == 0 || r == -2) {
            /* EOF occurred or read() would block. */
            if (r == 0 || written > 0) {
                if (_PyBytes_Resize(&res, written))
                    goto error;
                return res;
            }
            Py_DECREF(res);
            Py_INCREF(Py_None);
            return Py_None;
        }
        remaining -= r;
        written += r;
    }
    assert(remaining <= self->buffer_size);
    self->pos = 0;
    self->raw_pos = 0;
    self->read_end = 0;
    /* NOTE: when the read is satisfied, we avoid issuing any additional
       reads, which could block indefinitely (e.g. on a socket).
       See issue #9550. */
    while (remaining > 0 && self->read_end < self->buffer_size) {
        Py_ssize_t r = _bufferedreader_fill_buffer(self);
        if (r == -1)
            goto error;
        if (r == 0 || r == -2) {
            /* EOF occurred or read() would block. */
            if (r == 0 || written > 0) {
                if (_PyBytes_Resize(&res, written))
                    goto error;
                return res;
            }
            Py_DECREF(res);
            Py_INCREF(Py_None);
            return Py_None;
        }
        if (remaining > r) {
            memcpy(out + written, self->buffer + self->pos, r);
            written += r;
            self->pos += r;
            remaining -= r;
        }
        else if (remaining > 0) {
            memcpy(out + written, self->buffer + self->pos, remaining);
            written += remaining;
            self->pos += remaining;
            remaining = 0;
        }
        if (remaining == 0)
            break;
    }

    return res;

error:
    Py_XDECREF(res);
    return NULL;
}

static PyObject *
_bufferedreader_peek_unlocked(buffered *self, Py_ssize_t n)
{
    Py_ssize_t have, r;

    have = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    /* Constraints:
       1. we don't want to advance the file position.
       2. we don't want to lose block alignment, so we can't shift the buffer
          to make some place.
       Therefore, we either return `have` bytes (if > 0), or a full buffer.
    */
    if (have > 0) {
        return PyBytes_FromStringAndSize(self->buffer + self->pos, have);
    }

    /* Fill the buffer from the raw stream, and copy it to the result. */
    _bufferedreader_reset_buf(self);
    r = _bufferedreader_fill_buffer(self);
    if (r == -1)
        return NULL;
    if (r == -2)
        r = 0;
    self->pos = 0;
    return PyBytes_FromStringAndSize(self->buffer, r);
}

static PyMethodDef bufferedreader_methods[] = {
    /* BufferedIOMixin methods */
    {"detach", (PyCFunction)buffered_detach, METH_NOARGS},
    {"flush", (PyCFunction)buffered_simple_flush, METH_NOARGS},
    {"close", (PyCFunction)buffered_close, METH_NOARGS},
    {"seekable", (PyCFunction)buffered_seekable, METH_NOARGS},
    {"readable", (PyCFunction)buffered_readable, METH_NOARGS},
    {"writable", (PyCFunction)buffered_writable, METH_NOARGS},
    {"fileno", (PyCFunction)buffered_fileno, METH_NOARGS},
    {"isatty", (PyCFunction)buffered_isatty, METH_NOARGS},
    {"_dealloc_warn", (PyCFunction)buffered_dealloc_warn, METH_O},
    {"__getstate__", (PyCFunction)buffered_getstate, METH_NOARGS},

    {"read", (PyCFunction)buffered_read, METH_VARARGS},
    {"peek", (PyCFunction)buffered_peek, METH_VARARGS},
    {"read1", (PyCFunction)buffered_read1, METH_VARARGS},
    {"readline", (PyCFunction)buffered_readline, METH_VARARGS},
    {"seek", (PyCFunction)buffered_seek, METH_VARARGS},
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    {"truncate", (PyCFunction)buffered_truncate, METH_VARARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedreader_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {NULL}
};

static PyGetSetDef bufferedreader_getset[] = {
    {"closed", (getter)buffered_closed_get, NULL, NULL},
    {"name", (getter)buffered_name_get, NULL, NULL},
    {"mode", (getter)buffered_mode_get, NULL, NULL},
    {NULL}
};


PyTypeObject PyBufferedReader_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BufferedReader",       /*tp_name*/
    sizeof(buffered),           /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)buffered_dealloc,     /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    (reprfunc)buffered_repr,    /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
            | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    bufferedreader_doc,         /* tp_doc */
    (traverseproc)buffered_traverse, /* tp_traverse */
    (inquiry)buffered_clear,    /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof(buffered, weakreflist), /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    (iternextfunc)buffered_iternext, /* tp_iternext */
    bufferedreader_methods,     /* tp_methods */
    bufferedreader_members,     /* tp_members */
    bufferedreader_getset,      /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(buffered, dict), /* tp_dictoffset */
    (initproc)bufferedreader_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};



static int
complain_about_max_buffer_size(void)
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "max_buffer_size is deprecated", 1) < 0)
        return 0;
    return 1;
}

/*
 * class BufferedWriter
 */
PyDoc_STRVAR(bufferedwriter_doc,
    "A buffer for a writeable sequential RawIO object.\n"
    "\n"
    "The constructor creates a BufferedWriter for the given writeable raw\n"
    "stream. If the buffer_size is not given, it defaults to\n"
    "DEFAULT_BUFFER_SIZE. max_buffer_size isn't used anymore.\n"
    );

static void
_bufferedwriter_reset_buf(buffered *self)
{
    self->write_pos = 0;
    self->write_end = -1;
}

static int
bufferedwriter_init(buffered *self, PyObject *args, PyObject *kwds)
{
    /* TODO: properly deprecate max_buffer_size */
    char *kwlist[] = {"raw", "buffer_size", "max_buffer_size", NULL};
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;
    Py_ssize_t max_buffer_size = -234;
    PyObject *raw;

    self->ok = 0;
    self->detached = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|nn:BufferedReader", kwlist,
                                     &raw, &buffer_size, &max_buffer_size)) {
        return -1;
    }

    if (max_buffer_size != -234 && !complain_about_max_buffer_size())
        return -1;

    if (_PyIOBase_check_writable(raw, Py_True) == NULL)
        return -1;

    Py_CLEAR(self->raw);
    Py_INCREF(raw);
    self->raw = raw;
    self->readable = 0;
    self->writable = 1;

    self->buffer_size = buffer_size;
    if (_buffered_init(self) < 0)
        return -1;
    _bufferedwriter_reset_buf(self);
    self->pos = 0;

    self->fast_closed_checks = (Py_TYPE(self) == &PyBufferedWriter_Type &&
                                Py_TYPE(raw) == &PyFileIO_Type);

    self->ok = 1;
    return 0;
}

static Py_ssize_t
_bufferedwriter_raw_write(buffered *self, char *start, Py_ssize_t len)
{
    Py_buffer buf;
    PyObject *memobj, *res;
    Py_ssize_t n;
    /* NOTE: the buffer needn't be released as its object is NULL. */
    if (PyBuffer_FillInfo(&buf, NULL, start, len, 1, PyBUF_CONTIG_RO) == -1)
        return -1;
    memobj = PyMemoryView_FromBuffer(&buf);
    if (memobj == NULL)
        return -1;
    /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals() when EINTR
       occurs so we needn't do it ourselves.
       We then retry writing, ignoring the signal if no handler has
       raised (see issue #10956).
    */
    do {
        res = PyObject_CallMethodObjArgs(self->raw, _PyIO_str_write, memobj, NULL);
    } while (res == NULL && _trap_eintr());
    Py_DECREF(memobj);
    if (res == NULL)
        return -1;
    n = PyNumber_AsSsize_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0 || n > len) {
        PyErr_Format(PyExc_IOError,
                     "raw write() returned invalid length %zd "
                     "(should have been between 0 and %zd)", n, len);
        return -1;
    }
    if (n > 0 && self->abs_pos != -1)
        self->abs_pos += n;
    return n;
}

/* `restore_pos` is 1 if we need to restore the raw stream position at
   the end, 0 otherwise. */
static PyObject *
_bufferedwriter_flush_unlocked(buffered *self, int restore_pos)
{
    Py_ssize_t written = 0;
    Py_off_t n, rewind;

    if (!VALID_WRITE_BUFFER(self) || self->write_pos == self->write_end)
        goto end;
    /* First, rewind */
    rewind = RAW_OFFSET(self) + (self->pos - self->write_pos);
    if (rewind != 0) {
        n = _buffered_raw_seek(self, -rewind, 1);
        if (n < 0) {
            goto error;
        }
        self->raw_pos -= rewind;
    }
    while (self->write_pos < self->write_end) {
        n = _bufferedwriter_raw_write(self,
            self->buffer + self->write_pos,
            Py_SAFE_DOWNCAST(self->write_end - self->write_pos,
                             Py_off_t, Py_ssize_t));
        if (n == -1) {
            Py_ssize_t *w = _buffered_check_blocking_error();
            if (w == NULL)
                goto error;
            self->write_pos += *w;
            self->raw_pos = self->write_pos;
            written += *w;
            *w = written;
            /* Already re-raised */
            goto error;
        }
        self->write_pos += n;
        self->raw_pos = self->write_pos;
        written += Py_SAFE_DOWNCAST(n, Py_off_t, Py_ssize_t);
        /* Partial writes can return successfully when interrupted by a
           signal (see write(2)).  We must run signal handlers before
           blocking another time, possibly indefinitely. */
        if (PyErr_CheckSignals() < 0)
            goto error;
    }

    if (restore_pos) {
        Py_off_t forward = rewind - written;
        if (forward != 0) {
            n = _buffered_raw_seek(self, forward, 1);
            if (n < 0) {
                goto error;
            }
            self->raw_pos += forward;
        }
    }
    _bufferedwriter_reset_buf(self);

end:
    Py_RETURN_NONE;

error:
    return NULL;
}

static PyObject *
bufferedwriter_write(buffered *self, PyObject *args)
{
    PyObject *res = NULL;
    Py_buffer buf;
    Py_ssize_t written, avail, remaining;
    Py_off_t offset;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "y*:write", &buf)) {
        return NULL;
    }

    if (IS_CLOSED(self)) {
        PyErr_SetString(PyExc_ValueError, "write to closed file");
        PyBuffer_Release(&buf);
        return NULL;
    }

    if (!ENTER_BUFFERED(self)) {
        PyBuffer_Release(&buf);
        return NULL;
    }

    /* Fast path: the data to write can be fully buffered. */
    if (!VALID_READ_BUFFER(self) && !VALID_WRITE_BUFFER(self)) {
        self->pos = 0;
        self->raw_pos = 0;
    }
    avail = Py_SAFE_DOWNCAST(self->buffer_size - self->pos, Py_off_t, Py_ssize_t);
    if (buf.len <= avail) {
        memcpy(self->buffer + self->pos, buf.buf, buf.len);
        if (!VALID_WRITE_BUFFER(self) || self->write_pos > self->pos) {
            self->write_pos = self->pos;
        }
        ADJUST_POSITION(self, self->pos + buf.len);
        if (self->pos > self->write_end)
            self->write_end = self->pos;
        written = buf.len;
        goto end;
    }

    /* First write the current buffer */
    res = _bufferedwriter_flush_unlocked(self, 0);
    if (res == NULL) {
        Py_ssize_t *w = _buffered_check_blocking_error();
        if (w == NULL)
            goto error;
        if (self->readable)
            _bufferedreader_reset_buf(self);
        /* Make some place by shifting the buffer. */
        assert(VALID_WRITE_BUFFER(self));
        memmove(self->buffer, self->buffer + self->write_pos,
                Py_SAFE_DOWNCAST(self->write_end - self->write_pos,
                                 Py_off_t, Py_ssize_t));
        self->write_end -= self->write_pos;
        self->raw_pos -= self->write_pos;
        self->pos -= self->write_pos;
        self->write_pos = 0;
        avail = Py_SAFE_DOWNCAST(self->buffer_size - self->write_end,
                                 Py_off_t, Py_ssize_t);
        if (buf.len <= avail) {
            /* Everything can be buffered */
            PyErr_Clear();
            memcpy(self->buffer + self->write_end, buf.buf, buf.len);
            self->write_end += buf.len;
            written = buf.len;
            goto end;
        }
        /* Buffer as much as possible. */
        memcpy(self->buffer + self->write_end, buf.buf, avail);
        self->write_end += avail;
        /* Already re-raised */
        *w = avail;
        goto error;
    }
    Py_CLEAR(res);

    /* Adjust the raw stream position if it is away from the logical stream
       position. This happens if the read buffer has been filled but not
       modified (and therefore _bufferedwriter_flush_unlocked() didn't rewind
       the raw stream by itself).
       Fixes issue #6629.
    */
    offset = RAW_OFFSET(self);
    if (offset != 0) {
        if (_buffered_raw_seek(self, -offset, 1) < 0)
            goto error;
        self->raw_pos -= offset;
    }

    /* Then write buf itself. At this point the buffer has been emptied. */
    remaining = buf.len;
    written = 0;
    while (remaining > self->buffer_size) {
        Py_ssize_t n = _bufferedwriter_raw_write(
            self, (char *) buf.buf + written, buf.len - written);
        if (n == -1) {
            Py_ssize_t *w = _buffered_check_blocking_error();
            if (w == NULL)
                goto error;
            written += *w;
            remaining -= *w;
            if (remaining > self->buffer_size) {
                /* Can't buffer everything, still buffer as much as possible */
                memcpy(self->buffer,
                       (char *) buf.buf + written, self->buffer_size);
                self->raw_pos = 0;
                ADJUST_POSITION(self, self->buffer_size);
                self->write_end = self->buffer_size;
                *w = written + self->buffer_size;
                /* Already re-raised */
                goto error;
            }
            PyErr_Clear();
            break;
        }
        written += n;
        remaining -= n;
        /* Partial writes can return successfully when interrupted by a
           signal (see write(2)).  We must run signal handlers before
           blocking another time, possibly indefinitely. */
        if (PyErr_CheckSignals() < 0)
            goto error;
    }
    if (self->readable)
        _bufferedreader_reset_buf(self);
    if (remaining > 0) {
        memcpy(self->buffer, (char *) buf.buf + written, remaining);
        written += remaining;
    }
    self->write_pos = 0;
    /* TODO: sanity check (remaining >= 0) */
    self->write_end = remaining;
    ADJUST_POSITION(self, remaining);
    self->raw_pos = 0;

end:
    res = PyLong_FromSsize_t(written);

error:
    LEAVE_BUFFERED(self)
    PyBuffer_Release(&buf);
    return res;
}

static PyMethodDef bufferedwriter_methods[] = {
    /* BufferedIOMixin methods */
    {"close", (PyCFunction)buffered_close, METH_NOARGS},
    {"detach", (PyCFunction)buffered_detach, METH_NOARGS},
    {"seekable", (PyCFunction)buffered_seekable, METH_NOARGS},
    {"readable", (PyCFunction)buffered_readable, METH_NOARGS},
    {"writable", (PyCFunction)buffered_writable, METH_NOARGS},
    {"fileno", (PyCFunction)buffered_fileno, METH_NOARGS},
    {"isatty", (PyCFunction)buffered_isatty, METH_NOARGS},
    {"_dealloc_warn", (PyCFunction)buffered_dealloc_warn, METH_O},
    {"__getstate__", (PyCFunction)buffered_getstate, METH_NOARGS},

    {"write", (PyCFunction)bufferedwriter_write, METH_VARARGS},
    {"truncate", (PyCFunction)buffered_truncate, METH_VARARGS},
    {"flush", (PyCFunction)buffered_flush, METH_NOARGS},
    {"seek", (PyCFunction)buffered_seek, METH_VARARGS},
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedwriter_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {NULL}
};

static PyGetSetDef bufferedwriter_getset[] = {
    {"closed", (getter)buffered_closed_get, NULL, NULL},
    {"name", (getter)buffered_name_get, NULL, NULL},
    {"mode", (getter)buffered_mode_get, NULL, NULL},
    {NULL}
};


PyTypeObject PyBufferedWriter_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BufferedWriter",       /*tp_name*/
    sizeof(buffered),           /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)buffered_dealloc,     /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    (reprfunc)buffered_repr,    /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC,   /*tp_flags*/
    bufferedwriter_doc,         /* tp_doc */
    (traverseproc)buffered_traverse, /* tp_traverse */
    (inquiry)buffered_clear,    /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof(buffered, weakreflist), /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    bufferedwriter_methods,     /* tp_methods */
    bufferedwriter_members,     /* tp_members */
    bufferedwriter_getset,      /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(buffered, dict),   /* tp_dictoffset */
    (initproc)bufferedwriter_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};



/*
 * BufferedRWPair
 */

PyDoc_STRVAR(bufferedrwpair_doc,
    "A buffered reader and writer object together.\n"
    "\n"
    "A buffered reader object and buffered writer object put together to\n"
    "form a sequential IO object that can read and write. This is typically\n"
    "used with a socket or two-way pipe.\n"
    "\n"
    "reader and writer are RawIOBase objects that are readable and\n"
    "writeable respectively. If the buffer_size is omitted it defaults to\n"
    "DEFAULT_BUFFER_SIZE.\n"
    );

/* XXX The usefulness of this (compared to having two separate IO objects) is
 * questionable.
 */

typedef struct {
    PyObject_HEAD
    buffered *reader;
    buffered *writer;
    PyObject *dict;
    PyObject *weakreflist;
} rwpair;

static int
bufferedrwpair_init(rwpair *self, PyObject *args, PyObject *kwds)
{
    PyObject *reader, *writer;
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;
    Py_ssize_t max_buffer_size = -234;

    if (!PyArg_ParseTuple(args, "OO|nn:BufferedRWPair", &reader, &writer,
                          &buffer_size, &max_buffer_size)) {
        return -1;
    }

    if (max_buffer_size != -234 && !complain_about_max_buffer_size())
        return -1;

    if (_PyIOBase_check_readable(reader, Py_True) == NULL)
        return -1;
    if (_PyIOBase_check_writable(writer, Py_True) == NULL)
        return -1;

    self->reader = (buffered *) PyObject_CallFunction(
            (PyObject *) &PyBufferedReader_Type, "On", reader, buffer_size);
    if (self->reader == NULL)
        return -1;

    self->writer = (buffered *) PyObject_CallFunction(
            (PyObject *) &PyBufferedWriter_Type, "On", writer, buffer_size);
    if (self->writer == NULL) {
        Py_CLEAR(self->reader);
        return -1;
    }

    return 0;
}

static int
bufferedrwpair_traverse(rwpair *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
bufferedrwpair_clear(rwpair *self)
{
    Py_CLEAR(self->reader);
    Py_CLEAR(self->writer);
    Py_CLEAR(self->dict);
    return 0;
}

static void
bufferedrwpair_dealloc(rwpair *self)
{
    _PyObject_GC_UNTRACK(self);
    Py_CLEAR(self->reader);
    Py_CLEAR(self->writer);
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
_forward_call(buffered *self, const char *name, PyObject *args)
{
    PyObject *func = PyObject_GetAttrString((PyObject *)self, name);
    PyObject *ret;

    if (func == NULL) {
        PyErr_SetString(PyExc_AttributeError, name);
        return NULL;
    }

    ret = PyObject_CallObject(func, args);
    Py_DECREF(func);
    return ret;
}

static PyObject *
bufferedrwpair_read(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, "read", args);
}

static PyObject *
bufferedrwpair_peek(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, "peek", args);
}

static PyObject *
bufferedrwpair_read1(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, "read1", args);
}

static PyObject *
bufferedrwpair_readinto(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, "readinto", args);
}

static PyObject *
bufferedrwpair_write(rwpair *self, PyObject *args)
{
    return _forward_call(self->writer, "write", args);
}

static PyObject *
bufferedrwpair_flush(rwpair *self, PyObject *args)
{
    return _forward_call(self->writer, "flush", args);
}

static PyObject *
bufferedrwpair_readable(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, "readable", args);
}

static PyObject *
bufferedrwpair_writable(rwpair *self, PyObject *args)
{
    return _forward_call(self->writer, "writable", args);
}

static PyObject *
bufferedrwpair_close(rwpair *self, PyObject *args)
{
    PyObject *ret = _forward_call(self->writer, "close", args);
    if (ret == NULL)
        return NULL;
    Py_DECREF(ret);

    return _forward_call(self->reader, "close", args);
}

static PyObject *
bufferedrwpair_isatty(rwpair *self, PyObject *args)
{
    PyObject *ret = _forward_call(self->writer, "isatty", args);

    if (ret != Py_False) {
        /* either True or exception */
        return ret;
    }
    Py_DECREF(ret);

    return _forward_call(self->reader, "isatty", args);
}

static PyObject *
bufferedrwpair_closed_get(rwpair *self, void *context)
{
    return PyObject_GetAttr((PyObject *) self->writer, _PyIO_str_closed);
}

static PyMethodDef bufferedrwpair_methods[] = {
    {"read", (PyCFunction)bufferedrwpair_read, METH_VARARGS},
    {"peek", (PyCFunction)bufferedrwpair_peek, METH_VARARGS},
    {"read1", (PyCFunction)bufferedrwpair_read1, METH_VARARGS},
    {"readinto", (PyCFunction)bufferedrwpair_readinto, METH_VARARGS},

    {"write", (PyCFunction)bufferedrwpair_write, METH_VARARGS},
    {"flush", (PyCFunction)bufferedrwpair_flush, METH_NOARGS},

    {"readable", (PyCFunction)bufferedrwpair_readable, METH_NOARGS},
    {"writable", (PyCFunction)bufferedrwpair_writable, METH_NOARGS},

    {"close", (PyCFunction)bufferedrwpair_close, METH_NOARGS},
    {"isatty", (PyCFunction)bufferedrwpair_isatty, METH_NOARGS},

    {"__getstate__", (PyCFunction)buffered_getstate, METH_NOARGS},

    {NULL, NULL}
};

static PyGetSetDef bufferedrwpair_getset[] = {
    {"closed", (getter)bufferedrwpair_closed_get, NULL, NULL},
    {NULL}
};

PyTypeObject PyBufferedRWPair_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BufferedRWPair",       /*tp_name*/
    sizeof(rwpair),            /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)bufferedrwpair_dealloc,     /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC,   /* tp_flags */
    bufferedrwpair_doc,         /* tp_doc */
    (traverseproc)bufferedrwpair_traverse, /* tp_traverse */
    (inquiry)bufferedrwpair_clear, /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof(rwpair, weakreflist), /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    bufferedrwpair_methods,     /* tp_methods */
    0,                          /* tp_members */
    bufferedrwpair_getset,      /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(rwpair, dict),     /* tp_dictoffset */
    (initproc)bufferedrwpair_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};



/*
 * BufferedRandom
 */

PyDoc_STRVAR(bufferedrandom_doc,
    "A buffered interface to random access streams.\n"
    "\n"
    "The constructor creates a reader and writer for a seekable stream,\n"
    "raw, given in the first argument. If the buffer_size is omitted it\n"
    "defaults to DEFAULT_BUFFER_SIZE. max_buffer_size isn't used anymore.\n"
    );

static int
bufferedrandom_init(buffered *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"raw", "buffer_size", "max_buffer_size", NULL};
    Py_ssize_t buffer_size = DEFAULT_BUFFER_SIZE;
    Py_ssize_t max_buffer_size = -234;
    PyObject *raw;

    self->ok = 0;
    self->detached = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|nn:BufferedReader", kwlist,
                                     &raw, &buffer_size, &max_buffer_size)) {
        return -1;
    }

    if (max_buffer_size != -234 && !complain_about_max_buffer_size())
        return -1;

    if (_PyIOBase_check_seekable(raw, Py_True) == NULL)
        return -1;
    if (_PyIOBase_check_readable(raw, Py_True) == NULL)
        return -1;
    if (_PyIOBase_check_writable(raw, Py_True) == NULL)
        return -1;

    Py_CLEAR(self->raw);
    Py_INCREF(raw);
    self->raw = raw;
    self->buffer_size = buffer_size;
    self->readable = 1;
    self->writable = 1;

    if (_buffered_init(self) < 0)
        return -1;
    _bufferedreader_reset_buf(self);
    _bufferedwriter_reset_buf(self);
    self->pos = 0;

    self->fast_closed_checks = (Py_TYPE(self) == &PyBufferedRandom_Type &&
                                Py_TYPE(raw) == &PyFileIO_Type);

    self->ok = 1;
    return 0;
}

static PyMethodDef bufferedrandom_methods[] = {
    /* BufferedIOMixin methods */
    {"close", (PyCFunction)buffered_close, METH_NOARGS},
    {"detach", (PyCFunction)buffered_detach, METH_NOARGS},
    {"seekable", (PyCFunction)buffered_seekable, METH_NOARGS},
    {"readable", (PyCFunction)buffered_readable, METH_NOARGS},
    {"writable", (PyCFunction)buffered_writable, METH_NOARGS},
    {"fileno", (PyCFunction)buffered_fileno, METH_NOARGS},
    {"isatty", (PyCFunction)buffered_isatty, METH_NOARGS},
    {"_dealloc_warn", (PyCFunction)buffered_dealloc_warn, METH_O},
    {"__getstate__", (PyCFunction)buffered_getstate, METH_NOARGS},

    {"flush", (PyCFunction)buffered_flush, METH_NOARGS},

    {"seek", (PyCFunction)buffered_seek, METH_VARARGS},
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    {"truncate", (PyCFunction)buffered_truncate, METH_VARARGS},
    {"read", (PyCFunction)buffered_read, METH_VARARGS},
    {"read1", (PyCFunction)buffered_read1, METH_VARARGS},
    {"readinto", (PyCFunction)buffered_readinto, METH_VARARGS},
    {"readline", (PyCFunction)buffered_readline, METH_VARARGS},
    {"peek", (PyCFunction)buffered_peek, METH_VARARGS},
    {"write", (PyCFunction)bufferedwriter_write, METH_VARARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedrandom_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {NULL}
};

static PyGetSetDef bufferedrandom_getset[] = {
    {"closed", (getter)buffered_closed_get, NULL, NULL},
    {"name", (getter)buffered_name_get, NULL, NULL},
    {"mode", (getter)buffered_mode_get, NULL, NULL},
    {NULL}
};


PyTypeObject PyBufferedRandom_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.BufferedRandom",       /*tp_name*/
    sizeof(buffered),           /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)buffered_dealloc,     /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    (reprfunc)buffered_repr,    /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
        | Py_TPFLAGS_HAVE_GC,   /*tp_flags*/
    bufferedrandom_doc,         /* tp_doc */
    (traverseproc)buffered_traverse, /* tp_traverse */
    (inquiry)buffered_clear,    /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof(buffered, weakreflist), /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    (iternextfunc)buffered_iternext, /* tp_iternext */
    bufferedrandom_methods,     /* tp_methods */
    bufferedrandom_members,     /* tp_members */
    bufferedrandom_getset,      /* tp_getset */
    0,                          /* tp_base */
    0,                          /*tp_dict*/
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(buffered, dict), /*tp_dictoffset*/
    (initproc)bufferedrandom_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};
