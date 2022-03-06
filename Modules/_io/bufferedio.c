/*
    An implementation of Buffered I/O as defined by PEP 3116 - "New I/O"

    Classes defined here: BufferedIOBase, BufferedReader, BufferedWriter,
    BufferedRandom.

    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "pycore_object.h"
#include "structmember.h"         // PyMemberDef
#include "_iomodule.h"

/*[clinic input]
module _io
class _io._BufferedIOBase "PyObject *" "&PyBufferedIOBase_Type"
class _io._Buffered "buffered *" "&PyBufferedIOBase_Type"
class _io.BufferedReader "buffered *" "&PyBufferedReader_Type"
class _io.BufferedWriter "buffered *" "&PyBufferedWriter_Type"
class _io.BufferedRWPair "rwpair *" "&PyBufferedRWPair_Type"
class _io.BufferedRandom "buffered *" "&PyBufferedRandom_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=59460b9c5639984d]*/

_Py_IDENTIFIER(close);
_Py_IDENTIFIER(_dealloc_warn);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(isatty);
_Py_IDENTIFIER(mode);
_Py_IDENTIFIER(name);
_Py_IDENTIFIER(peek);
_Py_IDENTIFIER(read);
_Py_IDENTIFIER(read1);
_Py_IDENTIFIER(readable);
_Py_IDENTIFIER(readinto);
_Py_IDENTIFIER(readinto1);
_Py_IDENTIFIER(writable);
_Py_IDENTIFIER(write);

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
_bufferediobase_readinto_generic(PyObject *self, Py_buffer *buffer, char readinto1)
{
    Py_ssize_t len;
    PyObject *data;

    data = _PyObject_CallMethodId(self,
                                  readinto1 ? &PyId_read1 : &PyId_read,
                                  "n", buffer->len);
    if (data == NULL)
        return NULL;

    if (!PyBytes_Check(data)) {
        Py_DECREF(data);
        PyErr_SetString(PyExc_TypeError, "read() should return bytes");
        return NULL;
    }

    len = PyBytes_GET_SIZE(data);
    if (len > buffer->len) {
        PyErr_Format(PyExc_ValueError,
                     "read() returned too much data: "
                     "%zd bytes requested, %zd returned",
                     buffer->len, len);
        Py_DECREF(data);
        return NULL;
    }
    memcpy(buffer->buf, PyBytes_AS_STRING(data), len);

    Py_DECREF(data);

    return PyLong_FromSsize_t(len);
}

/*[clinic input]
_io._BufferedIOBase.readinto
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_readinto_impl(PyObject *self, Py_buffer *buffer)
/*[clinic end generated code: output=8c8cda6684af8038 input=00a6b9a38f29830a]*/
{
    return _bufferediobase_readinto_generic(self, buffer, 0);
}

/*[clinic input]
_io._BufferedIOBase.readinto1
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_readinto1_impl(PyObject *self, Py_buffer *buffer)
/*[clinic end generated code: output=358623e4fd2b69d3 input=ebad75b4aadfb9be]*/
{
    return _bufferediobase_readinto_generic(self, buffer, 1);
}

static PyObject *
bufferediobase_unsupported(const char *message)
{
    _PyIO_State *state = IO_STATE();
    if (state != NULL)
        PyErr_SetString(state->unsupported_operation, message);
    return NULL;
}

/*[clinic input]
_io._BufferedIOBase.detach

Disconnect this buffer from its underlying raw stream and return it.

After the raw stream has been detached, the buffer is in an unusable
state.
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_detach_impl(PyObject *self)
/*[clinic end generated code: output=754977c8d10ed88c input=822427fb58fe4169]*/
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
    "Returns the number of bytes written, which is always the length of b\n"
    "in bytes.\n"
    "\n"
    "Raises BlockingIOError if the buffer is full and the\n"
    "underlying raw stream cannot accept more data at the moment.\n");

static PyObject *
bufferediobase_write(PyObject *self, PyObject *args)
{
    return bufferediobase_unsupported("write");
}


typedef struct {
    PyObject_HEAD

    PyObject *raw;
    int ok;    /* Initialized? */
    int detached;
    int readable;
    int writable;
    char finalizing;

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

    PyThread_type_lock lock;
    volatile unsigned long owner;

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

static int
_enter_buffered_busy(buffered *self)
{
    int relax_locking;
    PyLockStatus st;
    if (self->owner == PyThread_get_thread_ident()) {
        PyErr_Format(PyExc_RuntimeError,
                     "reentrant call inside %R", self);
        return 0;
    }
    relax_locking = _Py_IsFinalizing();
    Py_BEGIN_ALLOW_THREADS
    if (!relax_locking)
        st = PyThread_acquire_lock(self->lock, 1);
    else {
        /* When finalizing, we don't want a deadlock to happen with daemon
         * threads abruptly shut down while they owned the lock.
         * Therefore, only wait for a grace period (1 s.).
         * Note that non-daemon threads have already exited here, so this
         * shouldn't affect carefully written threaded I/O code.
         */
        st = PyThread_acquire_lock_timed(self->lock, (PY_TIMEOUT_T)1e6, 0);
    }
    Py_END_ALLOW_THREADS
    if (relax_locking && st != PY_LOCK_ACQUIRED) {
        PyObject *ascii = PyObject_ASCII((PyObject*)self);
        _Py_FatalErrorFormat(__func__,
            "could not acquire lock for %s at interpreter "
            "shutdown, possibly due to daemon threads",
            ascii ? PyUnicode_AsUTF8(ascii) : "<ascii(self) failed>");
    }
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
    (!self->buffer || \
    (self->fast_closed_checks \
     ? _PyFileIO_closed(self->raw) \
     : buffered_closed(self)))

#define CHECK_CLOSED(self, error_msg) \
    if (IS_CLOSED(self) & (Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t) == 0)) { \
        PyErr_SetString(PyExc_ValueError, error_msg); \
        return NULL; \
    } \

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
    self->finalizing = 1;
    if (_PyIOBase_finalize((PyObject *) self) < 0)
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
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
buffered_sizeof(buffered *self, PyObject *Py_UNUSED(ignored))
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self));
    if (self->buffer)
        res += self->buffer_size;
    return PyLong_FromSsize_t(res);
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
        r = _PyObject_CallMethodIdOneArg(self->raw, &PyId__dealloc_warn,
                                         source);
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
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_flush);
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
    PyObject *res = NULL, *exc = NULL, *val, *tb;
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

    if (self->finalizing) {
        PyObject *r = buffered_dealloc_warn(self, (PyObject *) self);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    /* flush() will most probably re-take the lock, so drop it first */
    LEAVE_BUFFERED(self)
    res = PyObject_CallMethodNoArgs((PyObject *)self, _PyIO_str_flush);
    if (!ENTER_BUFFERED(self))
        return NULL;
    if (res == NULL)
        PyErr_Fetch(&exc, &val, &tb);
    else
        Py_DECREF(res);

    res = PyObject_CallMethodNoArgs(self->raw, _PyIO_str_close);

    if (self->buffer) {
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }

    if (exc != NULL) {
        _PyErr_ChainExceptions(exc, val, tb);
        Py_CLEAR(res);
    }

    self->read_end = 0;
    self->pos = 0;

end:
    LEAVE_BUFFERED(self)
    return res;
}

/* detach */

static PyObject *
buffered_detach(buffered *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *raw, *res;
    CHECK_INITIALIZED(self)
    res = PyObject_CallMethodNoArgs((PyObject *)self, _PyIO_str_flush);
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
buffered_seekable(buffered *self, PyObject *Py_UNUSED(ignored))
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_seekable);
}

static PyObject *
buffered_readable(buffered *self, PyObject *Py_UNUSED(ignored))
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_readable);
}

static PyObject *
buffered_writable(buffered *self, PyObject *Py_UNUSED(ignored))
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_writable);
}

static PyObject *
buffered_name_get(buffered *self, void *context)
{
    CHECK_INITIALIZED(self)
    return _PyObject_GetAttrId(self->raw, &PyId_name);
}

static PyObject *
buffered_mode_get(buffered *self, void *context)
{
    CHECK_INITIALIZED(self)
    return _PyObject_GetAttrId(self->raw, &PyId_mode);
}

/* Lower-level APIs */

static PyObject *
buffered_fileno(buffered *self, PyObject *Py_UNUSED(ignored))
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_fileno);
}

static PyObject *
buffered_isatty(buffered *self, PyObject *Py_UNUSED(ignored))
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, _PyIO_str_isatty);
}

/* Forward decls */
static PyObject *
_bufferedwriter_flush_unlocked(buffered *);
static Py_ssize_t
_bufferedreader_fill_buffer(buffered *self);
static void
_bufferedreader_reset_buf(buffered *self);
static void
_bufferedwriter_reset_buf(buffered *self);
static PyObject *
_bufferedreader_peek_unlocked(buffered *self);
static PyObject *
_bufferedreader_read_all(buffered *self);
static PyObject *
_bufferedreader_read_fast(buffered *self, Py_ssize_t);
static PyObject *
_bufferedreader_read_generic(buffered *self, Py_ssize_t);
static Py_ssize_t
_bufferedreader_raw_read(buffered *self, char *start, Py_ssize_t len);

/*
 * Helpers
 */

/* Sets the current error to BlockingIOError */
static void
_set_BlockingIOError(const char *msg, Py_ssize_t written)
{
    PyObject *err;
    PyErr_Clear();
    err = PyObject_CallFunction(PyExc_BlockingIOError, "isn",
                                errno, msg, written);
    if (err)
        PyErr_SetObject(PyExc_BlockingIOError, err);
    Py_XDECREF(err);
}

/* Returns the address of the `written` member if a BlockingIOError was
   raised, NULL otherwise. The error is always re-raised. */
static Py_ssize_t *
_buffered_check_blocking_error(void)
{
    PyObject *t, *v, *tb;
    PyOSErrorObject *err;

    PyErr_Fetch(&t, &v, &tb);
    if (v == NULL || !PyErr_GivenExceptionMatches(v, PyExc_BlockingIOError)) {
        PyErr_Restore(t, v, tb);
        return NULL;
    }
    err = (PyOSErrorObject *) v;
    /* TODO: sanity check (err->written >= 0) */
    PyErr_Restore(t, v, tb);
    return &err->written;
}

static Py_off_t
_buffered_raw_tell(buffered *self)
{
    Py_off_t n;
    PyObject *res;
    res = PyObject_CallMethodNoArgs(self->raw, _PyIO_str_tell);
    if (res == NULL)
        return -1;
    n = PyNumber_AsOff_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0) {
        if (!PyErr_Occurred())
            PyErr_Format(PyExc_OSError,
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
            PyErr_Format(PyExc_OSError,
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
    if (self->lock)
        PyThread_free_lock(self->lock);
    self->lock = PyThread_allocate_lock();
    if (self->lock == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "can't allocate read lock");
        return -1;
    }
    self->owner = 0;
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

/* Return 1 if an OSError with errno == EINTR is set (and then
   clears the error indicator), 0 otherwise.
   Should only be called when PyErr_Occurred() is true.
*/
int
_PyIO_trap_eintr(void)
{
    static PyObject *eintr_int = NULL;
    PyObject *typ, *val, *tb;
    PyOSErrorObject *env_err;

    if (eintr_int == NULL) {
        eintr_int = PyLong_FromLong(EINTR);
        assert(eintr_int != NULL);
    }
    if (!PyErr_ExceptionMatches(PyExc_OSError))
        return 0;
    PyErr_Fetch(&typ, &val, &tb);
    PyErr_NormalizeException(&typ, &val, &tb);
    env_err = (PyOSErrorObject *) val;
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
buffered_flush_and_rewind_unlocked(buffered *self)
{
    PyObject *res;

    res = _bufferedwriter_flush_unlocked(self);
    if (res == NULL)
        return NULL;
    Py_DECREF(res);

    if (self->readable) {
        /* Rewind the raw stream so that its position corresponds to
           the current logical position. */
        Py_off_t n;
        n = _buffered_raw_seek(self, -RAW_OFFSET(self), 1);
        _bufferedreader_reset_buf(self);
        if (n == -1)
            return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
buffered_flush(buffered *self, PyObject *args)
{
    PyObject *res;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "flush of closed file")

    if (!ENTER_BUFFERED(self))
        return NULL;
    res = buffered_flush_and_rewind_unlocked(self);
    LEAVE_BUFFERED(self)

    return res;
}

/*[clinic input]
_io._Buffered.peek
    size: Py_ssize_t = 0
    /

[clinic start generated code]*/

static PyObject *
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size)
/*[clinic end generated code: output=ba7a097ca230102b input=37ffb97d06ff4adb]*/
{
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "peek of closed file")

    if (!ENTER_BUFFERED(self))
        return NULL;

    if (self->writable) {
        res = buffered_flush_and_rewind_unlocked(self);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }
    res = _bufferedreader_peek_unlocked(self);

end:
    LEAVE_BUFFERED(self)
    return res;
}

/*[clinic input]
_io._Buffered.read
    size as n: Py_ssize_t(accept={int, NoneType}) = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_read_impl(buffered *self, Py_ssize_t n)
/*[clinic end generated code: output=f41c78bb15b9bbe9 input=7df81e82e08a68a2]*/
{
    PyObject *res;

    CHECK_INITIALIZED(self)
    if (n < -1) {
        PyErr_SetString(PyExc_ValueError,
                        "read length must be non-negative or -1");
        return NULL;
    }

    CHECK_CLOSED(self, "read of closed file")

    if (n == -1) {
        /* The number of bytes is unspecified, read until the end of stream */
        if (!ENTER_BUFFERED(self))
            return NULL;
        res = _bufferedreader_read_all(self);
    }
    else {
        res = _bufferedreader_read_fast(self, n);
        if (res != Py_None)
            return res;
        Py_DECREF(res);
        if (!ENTER_BUFFERED(self))
            return NULL;
        res = _bufferedreader_read_generic(self, n);
    }

    LEAVE_BUFFERED(self)
    return res;
}

/*[clinic input]
_io._Buffered.read1
    size as n: Py_ssize_t = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n)
/*[clinic end generated code: output=bcc4fb4e54d103a3 input=7d22de9630b61774]*/
{
    Py_ssize_t have, r;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    if (n < 0) {
        n = self->buffer_size;
    }

    CHECK_CLOSED(self, "read of closed file")

    if (n == 0)
        return PyBytes_FromStringAndSize(NULL, 0);

    /* Return up to n bytes.  If at least one byte is buffered, we
       only return buffered bytes.  Otherwise, we do one raw read. */

    have = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (have > 0) {
        n = Py_MIN(have, n);
        res = _bufferedreader_read_fast(self, n);
        assert(res != Py_None);
        return res;
    }
    res = PyBytes_FromStringAndSize(NULL, n);
    if (res == NULL)
        return NULL;
    if (!ENTER_BUFFERED(self)) {
        Py_DECREF(res);
        return NULL;
    }
    _bufferedreader_reset_buf(self);
    r = _bufferedreader_raw_read(self, PyBytes_AS_STRING(res), n);
    LEAVE_BUFFERED(self)
    if (r == -1) {
        Py_DECREF(res);
        return NULL;
    }
    if (r == -2)
        r = 0;
    if (n > r)
        _PyBytes_Resize(&res, r);
    return res;
}

static PyObject *
_buffered_readinto_generic(buffered *self, Py_buffer *buffer, char readinto1)
{
    Py_ssize_t n, written = 0, remaining;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "readinto of closed file")

    n = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (n > 0) {
        if (n >= buffer->len) {
            memcpy(buffer->buf, self->buffer + self->pos, buffer->len);
            self->pos += buffer->len;
            return PyLong_FromSsize_t(buffer->len);
        }
        memcpy(buffer->buf, self->buffer + self->pos, n);
        self->pos += n;
        written = n;
    }

    if (!ENTER_BUFFERED(self))
        return NULL;

    if (self->writable) {
        res = buffered_flush_and_rewind_unlocked(self);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
    }

    _bufferedreader_reset_buf(self);
    self->pos = 0;

    for (remaining = buffer->len - written;
         remaining > 0;
         written += n, remaining -= n) {
        /* If remaining bytes is larger than internal buffer size, copy
         * directly into caller's buffer. */
        if (remaining > self->buffer_size) {
            n = _bufferedreader_raw_read(self, (char *) buffer->buf + written,
                                         remaining);
        }

        /* In readinto1 mode, we do not want to fill the internal
           buffer if we already have some data to return */
        else if (!(readinto1 && written)) {
            n = _bufferedreader_fill_buffer(self);
            if (n > 0) {
                if (n > remaining)
                    n = remaining;
                memcpy((char *) buffer->buf + written,
                       self->buffer + self->pos, n);
                self->pos += n;
                continue; /* short circuit */
            }
        }
        else
            n = 0;

        if (n == 0 || (n == -2 && written > 0))
            break;
        if (n < 0) {
            if (n == -2) {
                Py_INCREF(Py_None);
                res = Py_None;
            }
            goto end;
        }

        /* At most one read in readinto1 mode */
        if (readinto1) {
            written += n;
            break;
        }
    }
    res = PyLong_FromSsize_t(written);

end:
    LEAVE_BUFFERED(self);
    return res;
}

/*[clinic input]
_io._Buffered.readinto
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readinto_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=bcb376580b1d8170 input=ed6b98b7a20a3008]*/
{
    return _buffered_readinto_generic(self, buffer, 0);
}

/*[clinic input]
_io._Buffered.readinto1
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readinto1_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=6e5c6ac5868205d6 input=4455c5d55fdf1687]*/
{
    return _buffered_readinto_generic(self, buffer, 1);
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
        self->pos += n;
        if (limit >= 0)
            limit -= n;
    }
    if (self->writable) {
        PyObject *r = buffered_flush_and_rewind_unlocked(self);
        if (r == NULL)
            goto end;
        Py_DECREF(r);
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
    Py_XSETREF(res, _PyBytes_Join(_PyIO_empty_bytes, chunks));

end:
    LEAVE_BUFFERED(self)
end_unlocked:
    Py_XDECREF(chunks);
    return res;
}

/*[clinic input]
_io._Buffered.readline
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size)
/*[clinic end generated code: output=24dd2aa6e33be83c input=673b6240e315ef8a]*/
{
    CHECK_INITIALIZED(self)
    return _buffered_readline(self, size);
}


static PyObject *
buffered_tell(buffered *self, PyObject *Py_UNUSED(ignored))
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

/*[clinic input]
_io._Buffered.seek
    target as targetobj: object
    whence: int = 0
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_seek_impl(buffered *self, PyObject *targetobj, int whence)
/*[clinic end generated code: output=7ae0e8dc46efdefb input=a9c4920bfcba6163]*/
{
    Py_off_t target, n;
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)

    /* Do some error checking instead of trusting OS 'seek()'
    ** error detection, just in case.
    */
    if ((whence < 0 || whence >2)
#ifdef SEEK_HOLE
        && (whence != SEEK_HOLE)
#endif
#ifdef SEEK_DATA
        && (whence != SEEK_DATA)
#endif
        ) {
        PyErr_Format(PyExc_ValueError,
                     "whence value %d unsupported", whence);
        return NULL;
    }

    CHECK_CLOSED(self, "seek of closed file")

    if (_PyIOBase_check_seekable(self->raw, Py_True) == NULL)
        return NULL;

    target = PyNumber_AsOff_t(targetobj, PyExc_ValueError);
    if (target == -1 && PyErr_Occurred())
        return NULL;

    /* SEEK_SET and SEEK_CUR are special because we could seek inside the
       buffer. Other whence values must be managed without this optimization.
       Some Operating Systems can provide additional values, like
       SEEK_HOLE/SEEK_DATA. */
    if (((whence == 0) || (whence == 1)) && self->readable) {
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
        res = _bufferedwriter_flush_unlocked(self);
        if (res == NULL)
            goto end;
        Py_CLEAR(res);
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

/*[clinic input]
_io._Buffered.truncate
    pos: object = None
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyObject *pos)
/*[clinic end generated code: output=667ca03c60c270de input=8a1be34d57cca2d3]*/
{
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "truncate of closed file")
    if (!self->writable) {
        return bufferediobase_unsupported("truncate");
    }
    if (!ENTER_BUFFERED(self))
        return NULL;

    res = buffered_flush_and_rewind_unlocked(self);
    if (res == NULL) {
        goto end;
    }
    Py_CLEAR(res);

    res = PyObject_CallMethodOneArg(self->raw, _PyIO_str_truncate, pos);
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
        line = PyObject_CallMethodNoArgs((PyObject *)self,
                                             _PyIO_str_readline);
        if (line && !PyBytes_Check(line)) {
            PyErr_Format(PyExc_OSError,
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

    if (_PyObject_LookupAttrId((PyObject *) self, &PyId_name, &nameobj) < 0) {
        if (!PyErr_ExceptionMatches(PyExc_ValueError)) {
            return NULL;
        }
        /* Ignore ValueError raised if the underlying stream was detached */
        PyErr_Clear();
    }
    if (nameobj == NULL) {
        res = PyUnicode_FromFormat("<%s>", Py_TYPE(self)->tp_name);
    }
    else {
        int status = Py_ReprEnter((PyObject *)self);
        res = NULL;
        if (status == 0) {
            res = PyUnicode_FromFormat("<%s name=%R>",
                                       Py_TYPE(self)->tp_name, nameobj);
            Py_ReprLeave((PyObject *)self);
        }
        else if (status > 0) {
            PyErr_Format(PyExc_RuntimeError,
                         "reentrant call inside %s.__repr__",
                         Py_TYPE(self)->tp_name);
        }
        Py_DECREF(nameobj);
    }
    return res;
}

/*
 * class BufferedReader
 */

static void _bufferedreader_reset_buf(buffered *self)
{
    self->read_end = -1;
}

/*[clinic input]
_io.BufferedReader.__init__
    raw: object
    buffer_size: Py_ssize_t(c_default="DEFAULT_BUFFER_SIZE") = DEFAULT_BUFFER_SIZE

Create a new buffered reader using the given readable raw IO object.
[clinic start generated code]*/

static int
_io_BufferedReader___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size)
/*[clinic end generated code: output=cddcfefa0ed294c4 input=fb887e06f11b4e48]*/
{
    self->ok = 0;
    self->detached = 0;

    if (_PyIOBase_check_readable(raw, Py_True) == NULL)
        return -1;

    Py_INCREF(raw);
    Py_XSETREF(self->raw, raw);
    self->buffer_size = buffer_size;
    self->readable = 1;
    self->writable = 0;

    if (_buffered_init(self) < 0)
        return -1;
    _bufferedreader_reset_buf(self);

    self->fast_closed_checks = (Py_IS_TYPE(self, &PyBufferedReader_Type) &&
                                Py_IS_TYPE(raw, &PyFileIO_Type));

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
        res = PyObject_CallMethodOneArg(self->raw, _PyIO_str_readinto, memobj);
    } while (res == NULL && _PyIO_trap_eintr());
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

    if (n == -1 && PyErr_Occurred()) {
        _PyErr_FormatFromCause(
            PyExc_OSError,
            "raw readinto() failed"
        );
        return -1;
    }

    if (n < 0 || n > len) {
        PyErr_Format(PyExc_OSError,
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
    PyObject *res = NULL, *data = NULL, *tmp = NULL, *chunks = NULL, *readall;

    /* First copy what we have in the current buffer. */
    current_size = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (current_size) {
        data = PyBytes_FromStringAndSize(
            self->buffer + self->pos, current_size);
        if (data == NULL)
            return NULL;
        self->pos += current_size;
    }
    /* We're going past the buffer's bounds, flush it */
    if (self->writable) {
        tmp = buffered_flush_and_rewind_unlocked(self);
        if (tmp == NULL)
            goto cleanup;
        Py_CLEAR(tmp);
    }
    _bufferedreader_reset_buf(self);

    if (_PyObject_LookupAttr(self->raw, _PyIO_str_readall, &readall) < 0) {
        goto cleanup;
    }
    if (readall) {
        tmp = _PyObject_CallNoArg(readall);
        Py_DECREF(readall);
        if (tmp == NULL)
            goto cleanup;
        if (tmp != Py_None && !PyBytes_Check(tmp)) {
            PyErr_SetString(PyExc_TypeError, "readall() should return bytes");
            goto cleanup;
        }
        if (current_size == 0) {
            res = tmp;
        } else {
            if (tmp != Py_None) {
                PyBytes_Concat(&data, tmp);
            }
            res = data;
        }
        goto cleanup;
    }

    chunks = PyList_New(0);
    if (chunks == NULL)
        goto cleanup;

    while (1) {
        if (data) {
            if (PyList_Append(chunks, data) < 0)
                goto cleanup;
            Py_CLEAR(data);
        }

        /* Read until EOF or until read() would block. */
        data = PyObject_CallMethodNoArgs(self->raw, _PyIO_str_read);
        if (data == NULL)
            goto cleanup;
        if (data != Py_None && !PyBytes_Check(data)) {
            PyErr_SetString(PyExc_TypeError, "read() should return bytes");
            goto cleanup;
        }
        if (data == Py_None || PyBytes_GET_SIZE(data) == 0) {
            if (current_size == 0) {
                res = data;
                goto cleanup;
            }
            else {
                tmp = _PyBytes_Join(_PyIO_empty_bytes, chunks);
                res = tmp;
                goto cleanup;
            }
        }
        current_size += PyBytes_GET_SIZE(data);
        if (self->abs_pos != -1)
            self->abs_pos += PyBytes_GET_SIZE(data);
    }
cleanup:
    /* res is either NULL or a borrowed ref */
    Py_XINCREF(res);
    Py_XDECREF(data);
    Py_XDECREF(tmp);
    Py_XDECREF(chunks);
    return res;
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
        self->pos += current_size;
    }
    /* Flush the write buffer if necessary */
    if (self->writable) {
        PyObject *r = buffered_flush_and_rewind_unlocked(self);
        if (r == NULL)
            goto error;
        Py_DECREF(r);
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
            Py_RETURN_NONE;
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
            Py_RETURN_NONE;
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
_bufferedreader_peek_unlocked(buffered *self)
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



/*
 * class BufferedWriter
 */
static void
_bufferedwriter_reset_buf(buffered *self)
{
    self->write_pos = 0;
    self->write_end = -1;
}

/*[clinic input]
_io.BufferedWriter.__init__
    raw: object
    buffer_size: Py_ssize_t(c_default="DEFAULT_BUFFER_SIZE") = DEFAULT_BUFFER_SIZE

A buffer for a writeable sequential RawIO object.

The constructor creates a BufferedWriter for the given writeable raw
stream. If the buffer_size is not given, it defaults to
DEFAULT_BUFFER_SIZE.
[clinic start generated code]*/

static int
_io_BufferedWriter___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size)
/*[clinic end generated code: output=c8942a020c0dee64 input=914be9b95e16007b]*/
{
    self->ok = 0;
    self->detached = 0;

    if (_PyIOBase_check_writable(raw, Py_True) == NULL)
        return -1;

    Py_INCREF(raw);
    Py_XSETREF(self->raw, raw);
    self->readable = 0;
    self->writable = 1;

    self->buffer_size = buffer_size;
    if (_buffered_init(self) < 0)
        return -1;
    _bufferedwriter_reset_buf(self);
    self->pos = 0;

    self->fast_closed_checks = (Py_IS_TYPE(self, &PyBufferedWriter_Type) &&
                                Py_IS_TYPE(raw, &PyFileIO_Type));

    self->ok = 1;
    return 0;
}

static Py_ssize_t
_bufferedwriter_raw_write(buffered *self, char *start, Py_ssize_t len)
{
    Py_buffer buf;
    PyObject *memobj, *res;
    Py_ssize_t n;
    int errnum;
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
        errno = 0;
        res = PyObject_CallMethodOneArg(self->raw, _PyIO_str_write, memobj);
        errnum = errno;
    } while (res == NULL && _PyIO_trap_eintr());
    Py_DECREF(memobj);
    if (res == NULL)
        return -1;
    if (res == Py_None) {
        /* Non-blocking stream would have blocked. Special return code!
           Being paranoid we reset errno in case it is changed by code
           triggered by a decref.  errno is used by _set_BlockingIOError(). */
        Py_DECREF(res);
        errno = errnum;
        return -2;
    }
    n = PyNumber_AsSsize_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n < 0 || n > len) {
        PyErr_Format(PyExc_OSError,
                     "raw write() returned invalid length %zd "
                     "(should have been between 0 and %zd)", n, len);
        return -1;
    }
    if (n > 0 && self->abs_pos != -1)
        self->abs_pos += n;
    return n;
}

static PyObject *
_bufferedwriter_flush_unlocked(buffered *self)
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
            goto error;
        }
        else if (n == -2) {
            _set_BlockingIOError("write could not complete without blocking",
                                 0);
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


end:
    /* This ensures that after return from this function,
       VALID_WRITE_BUFFER(self) returns false.

       This is a required condition because when a tell() is called
       after flushing and if VALID_READ_BUFFER(self) is false, we need
       VALID_WRITE_BUFFER(self) to be false to have
       RAW_OFFSET(self) == 0.

       Issue: https://bugs.python.org/issue32228 */
    _bufferedwriter_reset_buf(self);
    Py_RETURN_NONE;

error:
    return NULL;
}

/*[clinic input]
_io.BufferedWriter.write
    buffer: Py_buffer
    /
[clinic start generated code]*/

static PyObject *
_io_BufferedWriter_write_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=7f8d1365759bfc6b input=dd87dd85fc7f8850]*/
{
    PyObject *res = NULL;
    Py_ssize_t written, avail, remaining;
    Py_off_t offset;

    CHECK_INITIALIZED(self)

    if (!ENTER_BUFFERED(self))
        return NULL;

    /* Issue #31976: Check for closed file after acquiring the lock. Another
       thread could be holding the lock while closing the file. */
    if (IS_CLOSED(self)) {
        PyErr_SetString(PyExc_ValueError, "write to closed file");
        goto error;
    }

    /* Fast path: the data to write can be fully buffered. */
    if (!VALID_READ_BUFFER(self) && !VALID_WRITE_BUFFER(self)) {
        self->pos = 0;
        self->raw_pos = 0;
    }
    avail = Py_SAFE_DOWNCAST(self->buffer_size - self->pos, Py_off_t, Py_ssize_t);
    if (buffer->len <= avail) {
        memcpy(self->buffer + self->pos, buffer->buf, buffer->len);
        if (!VALID_WRITE_BUFFER(self) || self->write_pos > self->pos) {
            self->write_pos = self->pos;
        }
        ADJUST_POSITION(self, self->pos + buffer->len);
        if (self->pos > self->write_end)
            self->write_end = self->pos;
        written = buffer->len;
        goto end;
    }

    /* First write the current buffer */
    res = _bufferedwriter_flush_unlocked(self);
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
        if (buffer->len <= avail) {
            /* Everything can be buffered */
            PyErr_Clear();
            memcpy(self->buffer + self->write_end, buffer->buf, buffer->len);
            self->write_end += buffer->len;
            self->pos += buffer->len;
            written = buffer->len;
            goto end;
        }
        /* Buffer as much as possible. */
        memcpy(self->buffer + self->write_end, buffer->buf, avail);
        self->write_end += avail;
        self->pos += avail;
        /* XXX Modifying the existing exception e using the pointer w
           will change e.characters_written but not e.args[2].
           Therefore we just replace with a new error. */
        _set_BlockingIOError("write could not complete without blocking",
                             avail);
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
    remaining = buffer->len;
    written = 0;
    while (remaining > self->buffer_size) {
        Py_ssize_t n = _bufferedwriter_raw_write(
            self, (char *) buffer->buf + written, buffer->len - written);
        if (n == -1) {
            goto error;
        } else if (n == -2) {
            /* Write failed because raw file is non-blocking */
            if (remaining > self->buffer_size) {
                /* Can't buffer everything, still buffer as much as possible */
                memcpy(self->buffer,
                       (char *) buffer->buf + written, self->buffer_size);
                self->raw_pos = 0;
                ADJUST_POSITION(self, self->buffer_size);
                self->write_end = self->buffer_size;
                written += self->buffer_size;
                _set_BlockingIOError("write could not complete without "
                                     "blocking", written);
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
        memcpy(self->buffer, (char *) buffer->buf + written, remaining);
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
    return res;
}



/*
 * BufferedRWPair
 */

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

/*[clinic input]
_io.BufferedRWPair.__init__
    reader: object
    writer: object
    buffer_size: Py_ssize_t(c_default="DEFAULT_BUFFER_SIZE") = DEFAULT_BUFFER_SIZE
    /

A buffered reader and writer object together.

A buffered reader object and buffered writer object put together to
form a sequential IO object that can read and write. This is typically
used with a socket or two-way pipe.

reader and writer are RawIOBase objects that are readable and
writeable respectively. If the buffer_size is omitted it defaults to
DEFAULT_BUFFER_SIZE.
[clinic start generated code]*/

static int
_io_BufferedRWPair___init___impl(rwpair *self, PyObject *reader,
                                 PyObject *writer, Py_ssize_t buffer_size)
/*[clinic end generated code: output=327e73d1aee8f984 input=620d42d71f33a031]*/
{
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
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);
    Py_CLEAR(self->reader);
    Py_CLEAR(self->writer);
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
_forward_call(buffered *self, _Py_Identifier *name, PyObject *args)
{
    PyObject *func, *ret;
    if (self == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "I/O operation on uninitialized object");
        return NULL;
    }

    func = _PyObject_GetAttrId((PyObject *)self, name);
    if (func == NULL) {
        PyErr_SetString(PyExc_AttributeError, name->string);
        return NULL;
    }

    ret = PyObject_CallObject(func, args);
    Py_DECREF(func);
    return ret;
}

static PyObject *
bufferedrwpair_read(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, &PyId_read, args);
}

static PyObject *
bufferedrwpair_peek(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, &PyId_peek, args);
}

static PyObject *
bufferedrwpair_read1(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, &PyId_read1, args);
}

static PyObject *
bufferedrwpair_readinto(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, &PyId_readinto, args);
}

static PyObject *
bufferedrwpair_readinto1(rwpair *self, PyObject *args)
{
    return _forward_call(self->reader, &PyId_readinto1, args);
}

static PyObject *
bufferedrwpair_write(rwpair *self, PyObject *args)
{
    return _forward_call(self->writer, &PyId_write, args);
}

static PyObject *
bufferedrwpair_flush(rwpair *self, PyObject *Py_UNUSED(ignored))
{
    return _forward_call(self->writer, &PyId_flush, NULL);
}

static PyObject *
bufferedrwpair_readable(rwpair *self, PyObject *Py_UNUSED(ignored))
{
    return _forward_call(self->reader, &PyId_readable, NULL);
}

static PyObject *
bufferedrwpair_writable(rwpair *self, PyObject *Py_UNUSED(ignored))
{
    return _forward_call(self->writer, &PyId_writable, NULL);
}

static PyObject *
bufferedrwpair_close(rwpair *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *exc = NULL, *val, *tb;
    PyObject *ret = _forward_call(self->writer, &PyId_close, NULL);
    if (ret == NULL)
        PyErr_Fetch(&exc, &val, &tb);
    else
        Py_DECREF(ret);
    ret = _forward_call(self->reader, &PyId_close, NULL);
    if (exc != NULL) {
        _PyErr_ChainExceptions(exc, val, tb);
        Py_CLEAR(ret);
    }
    return ret;
}

static PyObject *
bufferedrwpair_isatty(rwpair *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *ret = _forward_call(self->writer, &PyId_isatty, NULL);

    if (ret != Py_False) {
        /* either True or exception */
        return ret;
    }
    Py_DECREF(ret);

    return _forward_call(self->reader, &PyId_isatty, NULL);
}

static PyObject *
bufferedrwpair_closed_get(rwpair *self, void *context)
{
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                "the BufferedRWPair object is being garbage-collected");
        return NULL;
    }
    return PyObject_GetAttr((PyObject *) self->writer, _PyIO_str_closed);
}



/*
 * BufferedRandom
 */

/*[clinic input]
_io.BufferedRandom.__init__
    raw: object
    buffer_size: Py_ssize_t(c_default="DEFAULT_BUFFER_SIZE") = DEFAULT_BUFFER_SIZE

A buffered interface to random access streams.

The constructor creates a reader and writer for a seekable stream,
raw, given in the first argument. If the buffer_size is omitted it
defaults to DEFAULT_BUFFER_SIZE.
[clinic start generated code]*/

static int
_io_BufferedRandom___init___impl(buffered *self, PyObject *raw,
                                 Py_ssize_t buffer_size)
/*[clinic end generated code: output=d3d64eb0f64e64a3 input=a4e818fb86d0e50c]*/
{
    self->ok = 0;
    self->detached = 0;

    if (_PyIOBase_check_seekable(raw, Py_True) == NULL)
        return -1;
    if (_PyIOBase_check_readable(raw, Py_True) == NULL)
        return -1;
    if (_PyIOBase_check_writable(raw, Py_True) == NULL)
        return -1;

    Py_INCREF(raw);
    Py_XSETREF(self->raw, raw);
    self->buffer_size = buffer_size;
    self->readable = 1;
    self->writable = 1;

    if (_buffered_init(self) < 0)
        return -1;
    _bufferedreader_reset_buf(self);
    _bufferedwriter_reset_buf(self);
    self->pos = 0;

    self->fast_closed_checks = (Py_IS_TYPE(self, &PyBufferedRandom_Type) &&
                                Py_IS_TYPE(raw, &PyFileIO_Type));

    self->ok = 1;
    return 0;
}

#include "clinic/bufferedio.c.h"


static PyMethodDef bufferediobase_methods[] = {
    _IO__BUFFEREDIOBASE_DETACH_METHODDEF
    {"read", bufferediobase_read, METH_VARARGS, bufferediobase_read_doc},
    {"read1", bufferediobase_read1, METH_VARARGS, bufferediobase_read1_doc},
    _IO__BUFFEREDIOBASE_READINTO_METHODDEF
    _IO__BUFFEREDIOBASE_READINTO1_METHODDEF
    {"write", bufferediobase_write, METH_VARARGS, bufferediobase_write_doc},
    {NULL, NULL}
};

PyTypeObject PyBufferedIOBase_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io._BufferedIOBase",      /*tp_name*/
    0,                          /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    0,                          /*tp_dealloc*/
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
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
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
    0,                          /* tp_version_tag */
    0,                          /* tp_finalize */
};


static PyMethodDef bufferedreader_methods[] = {
    /* BufferedIOMixin methods */
    {"detach", (PyCFunction)buffered_detach, METH_NOARGS},
    {"flush", (PyCFunction)buffered_simple_flush, METH_NOARGS},
    {"close", (PyCFunction)buffered_close, METH_NOARGS},
    {"seekable", (PyCFunction)buffered_seekable, METH_NOARGS},
    {"readable", (PyCFunction)buffered_readable, METH_NOARGS},
    {"fileno", (PyCFunction)buffered_fileno, METH_NOARGS},
    {"isatty", (PyCFunction)buffered_isatty, METH_NOARGS},
    {"_dealloc_warn", (PyCFunction)buffered_dealloc_warn, METH_O},

    _IO__BUFFERED_READ_METHODDEF
    _IO__BUFFERED_PEEK_METHODDEF
    _IO__BUFFERED_READ1_METHODDEF
    _IO__BUFFERED_READINTO_METHODDEF
    _IO__BUFFERED_READINTO1_METHODDEF
    _IO__BUFFERED_READLINE_METHODDEF
    _IO__BUFFERED_SEEK_METHODDEF
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    _IO__BUFFERED_TRUNCATE_METHODDEF
    {"__sizeof__", (PyCFunction)buffered_sizeof, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedreader_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {"_finalizing", T_BOOL, offsetof(buffered, finalizing), 0},
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
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
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
    _io_BufferedReader___init____doc__, /* tp_doc */
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
    _io_BufferedReader___init__, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
    0,                          /* tp_version_tag */
    0,                          /* tp_finalize */
};


static PyMethodDef bufferedwriter_methods[] = {
    /* BufferedIOMixin methods */
    {"close", (PyCFunction)buffered_close, METH_NOARGS},
    {"detach", (PyCFunction)buffered_detach, METH_NOARGS},
    {"seekable", (PyCFunction)buffered_seekable, METH_NOARGS},
    {"writable", (PyCFunction)buffered_writable, METH_NOARGS},
    {"fileno", (PyCFunction)buffered_fileno, METH_NOARGS},
    {"isatty", (PyCFunction)buffered_isatty, METH_NOARGS},
    {"_dealloc_warn", (PyCFunction)buffered_dealloc_warn, METH_O},

    _IO_BUFFEREDWRITER_WRITE_METHODDEF
    _IO__BUFFERED_TRUNCATE_METHODDEF
    {"flush", (PyCFunction)buffered_flush, METH_NOARGS},
    _IO__BUFFERED_SEEK_METHODDEF
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    {"__sizeof__", (PyCFunction)buffered_sizeof, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedwriter_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {"_finalizing", T_BOOL, offsetof(buffered, finalizing), 0},
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
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
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
    _io_BufferedWriter___init____doc__, /* tp_doc */
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
    _io_BufferedWriter___init__, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
    0,                          /* tp_version_tag */
    0,                          /* tp_finalize */
};


static PyMethodDef bufferedrwpair_methods[] = {
    {"read", (PyCFunction)bufferedrwpair_read, METH_VARARGS},
    {"peek", (PyCFunction)bufferedrwpair_peek, METH_VARARGS},
    {"read1", (PyCFunction)bufferedrwpair_read1, METH_VARARGS},
    {"readinto", (PyCFunction)bufferedrwpair_readinto, METH_VARARGS},
    {"readinto1", (PyCFunction)bufferedrwpair_readinto1, METH_VARARGS},

    {"write", (PyCFunction)bufferedrwpair_write, METH_VARARGS},
    {"flush", (PyCFunction)bufferedrwpair_flush, METH_NOARGS},

    {"readable", (PyCFunction)bufferedrwpair_readable, METH_NOARGS},
    {"writable", (PyCFunction)bufferedrwpair_writable, METH_NOARGS},

    {"close", (PyCFunction)bufferedrwpair_close, METH_NOARGS},
    {"isatty", (PyCFunction)bufferedrwpair_isatty, METH_NOARGS},

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
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
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
    _io_BufferedRWPair___init____doc__, /* tp_doc */
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
    _io_BufferedRWPair___init__, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
    0,                          /* tp_version_tag */
    0,                          /* tp_finalize */
};


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

    {"flush", (PyCFunction)buffered_flush, METH_NOARGS},

    _IO__BUFFERED_SEEK_METHODDEF
    {"tell", (PyCFunction)buffered_tell, METH_NOARGS},
    _IO__BUFFERED_TRUNCATE_METHODDEF
    _IO__BUFFERED_READ_METHODDEF
    _IO__BUFFERED_READ1_METHODDEF
    _IO__BUFFERED_READINTO_METHODDEF
    _IO__BUFFERED_READINTO1_METHODDEF
    _IO__BUFFERED_READLINE_METHODDEF
    _IO__BUFFERED_PEEK_METHODDEF
    _IO_BUFFEREDWRITER_WRITE_METHODDEF
    {"__sizeof__", (PyCFunction)buffered_sizeof, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedrandom_members[] = {
    {"raw", T_OBJECT, offsetof(buffered, raw), READONLY},
    {"_finalizing", T_BOOL, offsetof(buffered, finalizing), 0},
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
    0,                          /*tp_vectorcall_offset*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_as_async*/
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
    _io_BufferedRandom___init____doc__, /* tp_doc */
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
    _io_BufferedRandom___init__, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
    0,                          /* tp_free */
    0,                          /* tp_is_gc */
    0,                          /* tp_bases */
    0,                          /* tp_mro */
    0,                          /* tp_cache */
    0,                          /* tp_subclasses */
    0,                          /* tp_weaklist */
    0,                          /* tp_del */
    0,                          /* tp_version_tag */
    0,                          /* tp_finalize */
};
