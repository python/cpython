/*
    An implementation of Buffered I/O as defined by PEP 3116 - "New I/O"

    Classes defined here: BufferedIOBase, BufferedReader, BufferedWriter,
    BufferedRandom.

    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/

#include "Python.h"
#include "pycore_call.h"                // _PyObject_CallNoArgs()
#include "pycore_fileutils.h"           // _PyFile_Flush
#include "pycore_object.h"              // _PyObject_GC_UNTRACK()
#include "pycore_pyerrors.h"            // _Py_FatalErrorFormat()
#include "pycore_pylifecycle.h"         // _Py_IsInterpreterFinalizing()
#include "pycore_weakref.h"             // FT_CLEAR_WEAKREFS()

#include "_iomodule.h"

/*[clinic input]
module _io
class _io._BufferedIOBase "PyObject *" "clinic_state()->PyBufferedIOBase_Type"
class _io._Buffered "buffered *" "clinic_state()->PyBufferedIOBase_Type"
class _io.BufferedReader "buffered *" "clinic_state()->PyBufferedReader_Type"
class _io.BufferedWriter "buffered *" "clinic_state()->PyBufferedWriter_Type"
class _io.BufferedRWPair "rwpair *" "clinic_state()->PyBufferedRWPair_Type"
class _io.BufferedRandom "buffered *" "clinic_state()->PyBufferedRandom_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3b3ef9cbbbad4590]*/

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

    PyObject *attr = readinto1
        ? &_Py_ID(read1)
        : &_Py_ID(read);
    data = _PyObject_CallMethod(self, attr, "n", buffer->len);
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
@critical_section
_io._BufferedIOBase.readinto
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_readinto_impl(PyObject *self, Py_buffer *buffer)
/*[clinic end generated code: output=8c8cda6684af8038 input=5273d20db7f56e1a]*/
{
    return _bufferediobase_readinto_generic(self, buffer, 0);
}

/*[clinic input]
@critical_section
_io._BufferedIOBase.readinto1
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_readinto1_impl(PyObject *self, Py_buffer *buffer)
/*[clinic end generated code: output=358623e4fd2b69d3 input=d6eb723dedcee654]*/
{
    return _bufferediobase_readinto_generic(self, buffer, 1);
}

static PyObject *
bufferediobase_unsupported(_PyIO_State *state, const char *message)
{
    PyErr_SetString(state->unsupported_operation, message);
    return NULL;
}

/*[clinic input]
_io._BufferedIOBase.detach

    cls: defining_class
    /

Disconnect this buffer from its underlying raw stream and return it.

After the raw stream has been detached, the buffer is in an unusable
state.
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_detach_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=b87b135d67cd4448 input=0b61a7b4357c1ea7]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return bufferediobase_unsupported(state, "detach");
}

/*[clinic input]
_io._BufferedIOBase.read

    cls: defining_class
    size: int(unused=True) = -1
    /

Read and return up to n bytes.

If the size argument is omitted, None, or negative, read and
return all data until EOF.

If the size argument is positive, and the underlying raw stream is
not 'interactive', multiple raw reads may be issued to satisfy
the byte count (unless EOF is reached first).
However, for interactive raw streams (as well as sockets and pipes),
at most one raw read will be issued, and a short result does not
imply that EOF is imminent.

Return an empty bytes object on EOF.

Return None if the underlying raw stream was open in non-blocking
mode and no data is available at the moment.
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_read_impl(PyObject *self, PyTypeObject *cls,
                              int Py_UNUSED(size))
/*[clinic end generated code: output=aceb2765587b0a29 input=824f6f910465e61a]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return bufferediobase_unsupported(state, "read");
}

/*[clinic input]
@permit_long_summary
_io._BufferedIOBase.read1

    cls: defining_class
    size: int(unused=True) = -1
    /

Read and return up to size bytes, with at most one read() call to the underlying raw stream.

Return an empty bytes object on EOF.
A short result does not imply that EOF is imminent.
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_read1_impl(PyObject *self, PyTypeObject *cls,
                               int Py_UNUSED(size))
/*[clinic end generated code: output=2e7fc62972487eaa input=1e76df255063afd6]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return bufferediobase_unsupported(state, "read1");
}

/*[clinic input]
_io._BufferedIOBase.write

    cls: defining_class
    b: object(unused=True)
    /

Write buffer b to the IO stream.

Return the number of bytes written, which is always
the length of b in bytes.

Raise BlockingIOError if the buffer is full and the
underlying raw stream cannot accept more data at the moment.
[clinic start generated code]*/

static PyObject *
_io__BufferedIOBase_write_impl(PyObject *self, PyTypeObject *cls,
                               PyObject *Py_UNUSED(b))
/*[clinic end generated code: output=712c635246bf2306 input=9793f5c8f71029ad]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return bufferediobase_unsupported(state, "write");
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

#define buffered_CAST(op)   ((buffered *)(op))

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
    PyInterpreterState *interp = _PyInterpreterState_GET();
    relax_locking = _Py_IsInterpreterFinalizing(interp);
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
    (!self->buffer ? 1 : \
    (self->fast_closed_checks \
     ? _PyFileIO_closed(self->raw) \
     : buffered_closed(self)))

#define CHECK_CLOSED(self, error_msg) \
    do { \
        int _closed = IS_CLOSED(self); \
        if (_closed < 0) { \
            return NULL; \
        } \
        if (_closed && \
            (Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t) == 0)) \
        { \
            PyErr_SetString(PyExc_ValueError, error_msg); \
            return NULL; \
        } \
    } while (0);

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


static int
buffered_clear(PyObject *op)
{
    buffered *self = buffered_CAST(op);
    self->ok = 0;
    Py_CLEAR(self->raw);
    Py_CLEAR(self->dict);
    return 0;
}

static void
buffered_dealloc(PyObject *op)
{
    buffered *self = buffered_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    self->finalizing = 1;
    if (_PyIOBase_finalize(op) < 0)
        return;
    _PyObject_GC_UNTRACK(self);
    self->ok = 0;
    FT_CLEAR_WEAKREFS(op, self->weakreflist);
    if (self->buffer) {
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }
    if (self->lock) {
        PyThread_free_lock(self->lock);
        self->lock = NULL;
    }
    (void)buffered_clear(op);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/*[clinic input]
@critical_section
_io._Buffered.__sizeof__
[clinic start generated code]*/

static PyObject *
_io__Buffered___sizeof___impl(buffered *self)
/*[clinic end generated code: output=0231ef7f5053134e input=07a32d578073ea64]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    if (self->buffer) {
        res += (size_t)self->buffer_size;
    }
    return PyLong_FromSize_t(res);
}

static int
buffered_traverse(PyObject *op, visitproc visit, void *arg)
{
    buffered *self = buffered_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->raw);
    Py_VISIT(self->dict);
    return 0;
}

/* Because this can call arbitrary code, it shouldn't be called when
   the refcount is 0 (that is, not directly from tp_dealloc unless
   the refcount has been temporarily re-incremented). */
/*[clinic input]
_io._Buffered._dealloc_warn

    source: object
    /

[clinic start generated code]*/

static PyObject *
_io__Buffered__dealloc_warn_impl(buffered *self, PyObject *source)
/*[clinic end generated code: output=d8db21c6dec0e614 input=8f845f2a4786391c]*/
{
    if (self->ok && self->raw) {
        PyObject *r;
        r = PyObject_CallMethodOneArg(self->raw, &_Py_ID(_dealloc_warn), source);
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
/*[clinic input]
@critical_section
_io._Buffered.flush as _io__Buffered_simple_flush
[clinic start generated code]*/

static PyObject *
_io__Buffered_simple_flush_impl(buffered *self)
/*[clinic end generated code: output=29ebb3820db1bdfd input=5248cb84a65f80bd]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(flush));
}

static int
buffered_closed(buffered *self)
{
    int closed;
    PyObject *res;
    CHECK_INITIALIZED_INT(self)
    res = PyObject_GetAttr(self->raw, &_Py_ID(closed));
    if (res == NULL)
        return -1;
    closed = PyObject_IsTrue(res);
    Py_DECREF(res);
    return closed;
}

/*[clinic input]
@critical_section
@getter
_io._Buffered.closed
[clinic start generated code]*/

static PyObject *
_io__Buffered_closed_get_impl(buffered *self)
/*[clinic end generated code: output=f08ce57290703a1a input=18eddefdfe4a3d2f]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttr(self->raw, &_Py_ID(closed));
}

/*[clinic input]
@critical_section
_io._Buffered.close
[clinic start generated code]*/

static PyObject *
_io__Buffered_close_impl(buffered *self)
/*[clinic end generated code: output=7280b7b42033be0c input=56d95935b03fd326]*/
{
    PyObject *res = NULL;
    int r;

    CHECK_INITIALIZED(self)
    if (!ENTER_BUFFERED(self)) {
        return NULL;
    }
    /* gh-138720: Use IS_CLOSED to match flush CHECK_CLOSED. */
    r = IS_CLOSED(self);
    if (r < 0)
        goto end;
    if (r > 0) {
        res = Py_NewRef(Py_None);
        goto end;
    }

    if (self->finalizing) {
        PyObject *r = _io__Buffered__dealloc_warn_impl(self, (PyObject *)self);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    /* flush() will most probably re-take the lock, so drop it first */
    LEAVE_BUFFERED(self)
    r = _PyFile_Flush((PyObject *)self);
    if (!ENTER_BUFFERED(self)) {
        return NULL;
    }
    PyObject *exc = NULL;
    if (r < 0) {
        exc = PyErr_GetRaisedException();
    }

    res = PyObject_CallMethodNoArgs(self->raw, &_Py_ID(close));

    if (self->buffer) {
        PyMem_Free(self->buffer);
        self->buffer = NULL;
    }

    if (exc != NULL) {
        _PyErr_ChainExceptions1(exc);
        Py_CLEAR(res);
    }

    self->read_end = 0;
    self->pos = 0;

end:
    LEAVE_BUFFERED(self)
    return res;
}

/*[clinic input]
@critical_section
_io._Buffered.detach
[clinic start generated code]*/

static PyObject *
_io__Buffered_detach_impl(buffered *self)
/*[clinic end generated code: output=dd0fc057b8b779f7 input=d4ef1828a678be37]*/
{
    PyObject *raw;
    CHECK_INITIALIZED(self)
    if (_PyFile_Flush((PyObject *)self) < 0) {
        return NULL;
    }
    raw = self->raw;
    self->raw = NULL;
    self->detached = 1;
    self->ok = 0;
    return raw;
}

/* Inquiries */

/*[clinic input]
@critical_section
_io._Buffered.seekable
[clinic start generated code]*/

static PyObject *
_io__Buffered_seekable_impl(buffered *self)
/*[clinic end generated code: output=90172abb5ceb6e8f input=e3a4fc1d297b2fd3]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(seekable));
}

/*[clinic input]
@critical_section
_io._Buffered.readable
[clinic start generated code]*/

static PyObject *
_io__Buffered_readable_impl(buffered *self)
/*[clinic end generated code: output=92afa07661ecb698 input=abe54107d59bca9a]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(readable));
}

/*[clinic input]
@critical_section
_io._Buffered.writable
[clinic start generated code]*/

static PyObject *
_io__Buffered_writable_impl(buffered *self)
/*[clinic end generated code: output=4e3eee8d6f9d8552 input=45eb76bf6a10e6f7]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(writable));
}


/*[clinic input]
@critical_section
@getter
_io._Buffered.name
[clinic start generated code]*/

static PyObject *
_io__Buffered_name_get_impl(buffered *self)
/*[clinic end generated code: output=d2adf384051d3d10 input=6b84a0e6126f545e]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttr(self->raw, &_Py_ID(name));
}

/*[clinic input]
@critical_section
@getter
_io._Buffered.mode
[clinic start generated code]*/

static PyObject *
_io__Buffered_mode_get_impl(buffered *self)
/*[clinic end generated code: output=0feb205748892fa4 input=0762d5e28542fd8c]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_GetAttr(self->raw, &_Py_ID(mode));
}

/* Lower-level APIs */

/*[clinic input]
@critical_section
_io._Buffered.fileno
[clinic start generated code]*/

static PyObject *
_io__Buffered_fileno_impl(buffered *self)
/*[clinic end generated code: output=b717648d58a95ee3 input=1c4fead777bae20a]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(fileno));
}

/*[clinic input]
@critical_section
_io._Buffered.isatty
[clinic start generated code]*/

static PyObject *
_io__Buffered_isatty_impl(buffered *self)
/*[clinic end generated code: output=c20e55caae67baea input=e53d182d7e490e3a]*/
{
    CHECK_INITIALIZED(self)
    return PyObject_CallMethodNoArgs(self->raw, &_Py_ID(isatty));
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
    PyObject *exc = PyErr_GetRaisedException();
    if (exc == NULL || !PyErr_GivenExceptionMatches(exc, PyExc_BlockingIOError)) {
        PyErr_SetRaisedException(exc);
        return NULL;
    }
    PyOSErrorObject *err = (PyOSErrorObject *)exc;
    /* TODO: sanity check (err->written >= 0) */
    PyErr_SetRaisedException(exc);
    return &err->written;
}

static Py_off_t
_buffered_raw_tell(buffered *self)
{
    Py_off_t n;
    PyObject *res;
    res = PyObject_CallMethodNoArgs(self->raw, &_Py_ID(tell));
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
    res = PyObject_CallMethodObjArgs(self->raw, &_Py_ID(seek),
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
    if (!PyErr_ExceptionMatches(PyExc_OSError)) {
        return 0;
    }
    PyObject *exc = PyErr_GetRaisedException();
    PyOSErrorObject *env_err = (PyOSErrorObject *)exc;
    assert(env_err != NULL);
    if (env_err->myerrno != NULL) {
        assert(EINTR > 0 && EINTR < INT_MAX);
        assert(PyLong_CheckExact(env_err->myerrno));
        int overflow;
        int myerrno = PyLong_AsLongAndOverflow(env_err->myerrno, &overflow);
        PyErr_Clear();
        if (myerrno == EINTR) {
            Py_DECREF(exc);
            return 1;
        }
    }
    /* This silences any error set by PyObject_RichCompareBool() */
    PyErr_SetRaisedException(exc);
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

/*[clinic input]
@critical_section
_io._Buffered.flush
[clinic start generated code]*/

static PyObject *
_io__Buffered_flush_impl(buffered *self)
/*[clinic end generated code: output=da2674ef1ce71f3a input=6b30de9f083419c2]*/
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
@critical_section
_io._Buffered.peek
    size: Py_ssize_t = 0
    /

[clinic start generated code]*/

static PyObject *
_io__Buffered_peek_impl(buffered *self, Py_ssize_t size)
/*[clinic end generated code: output=ba7a097ca230102b input=56733376f926d982]*/
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
@critical_section
_io._Buffered.read
    size as n: Py_ssize_t(accept={int, NoneType}) = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_read_impl(buffered *self, Py_ssize_t n)
/*[clinic end generated code: output=f41c78bb15b9bbe9 input=bdb4b0425b295472]*/
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
@critical_section
_io._Buffered.read1
    size as n: Py_ssize_t = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_read1_impl(buffered *self, Py_ssize_t n)
/*[clinic end generated code: output=bcc4fb4e54d103a3 input=3d0ad241aa52b36c]*/
{
    CHECK_INITIALIZED(self)
    if (n < 0) {
        n = self->buffer_size;
    }

    CHECK_CLOSED(self, "read of closed file")

    if (n == 0) {
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    }

    /* Return up to n bytes.  If at least one byte is buffered, we
       only return buffered bytes.  Otherwise, we do one raw read. */

    Py_ssize_t have = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (have > 0) {
        n = Py_MIN(have, n);
        PyObject *res = _bufferedreader_read_fast(self, n);
        assert(res != Py_None);
        return res;
    }

    if (!ENTER_BUFFERED(self)) {
        return NULL;
    }

    /* Flush the write buffer if necessary */
    if (self->writable) {
        PyObject *res = buffered_flush_and_rewind_unlocked(self);
        if (res == NULL) {
            LEAVE_BUFFERED(self)
            return NULL;
        }
        Py_DECREF(res);
    }
    _bufferedreader_reset_buf(self);

    PyBytesWriter *writer = PyBytesWriter_Create(n);
    if (writer == NULL) {
        LEAVE_BUFFERED(self)
        return NULL;
    }

    Py_ssize_t r = _bufferedreader_raw_read(self,
                                            PyBytesWriter_GetData(writer), n);
    LEAVE_BUFFERED(self)
    if (r == -1) {
        PyBytesWriter_Discard(writer);
        return NULL;
    }
    if (r == -2) {
        r = 0;
    }

    return PyBytesWriter_FinishWithSize(writer, r);
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
                res = Py_NewRef(Py_None);
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
@critical_section
_io._Buffered.readinto
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readinto_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=bcb376580b1d8170 input=777c33e7adaa2bcd]*/
{
    return _buffered_readinto_generic(self, buffer, 0);
}

/*[clinic input]
@critical_section
_io._Buffered.readinto1
    buffer: Py_buffer(accept={rwbuffer})
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readinto1_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=6e5c6ac5868205d6 input=ef03cc5fc92a6895]*/
{
    return _buffered_readinto_generic(self, buffer, 1);
}


static PyObject *
_buffered_readline(buffered *self, Py_ssize_t limit)
{
    PyObject *res = NULL;
    PyObject *chunks = NULL;
    Py_ssize_t n;
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
        if (limit >= 0)
            limit -= n;
    }
found:
    if (res != NULL && PyList_Append(chunks, res) < 0) {
        Py_CLEAR(res);
        goto end;
    }
    Py_XSETREF(res, PyBytes_Join((PyObject *)&_Py_SINGLETON(bytes_empty), chunks));

end:
    LEAVE_BUFFERED(self)
end_unlocked:
    Py_XDECREF(chunks);
    return res;
}

/*[clinic input]
@critical_section
_io._Buffered.readline
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_readline_impl(buffered *self, Py_ssize_t size)
/*[clinic end generated code: output=24dd2aa6e33be83c input=e81ca5abd4280776]*/
{
    CHECK_INITIALIZED(self)
    return _buffered_readline(self, size);
}


/*[clinic input]
@critical_section
_io._Buffered.tell
[clinic start generated code]*/

static PyObject *
_io__Buffered_tell_impl(buffered *self)
/*[clinic end generated code: output=386972ae84716c1e input=ab12e67d8abcb42f]*/
{
    Py_off_t pos;

    CHECK_INITIALIZED(self)
    pos = _buffered_raw_tell(self);
    if (pos == -1)
        return NULL;
    pos -= RAW_OFFSET(self);

    // GH-95782
    if (pos < 0)
        pos = 0;

    return PyLong_FromOff_t(pos);
}

/*[clinic input]
@critical_section
_io._Buffered.seek
    target as targetobj: object
    whence: int = 0
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_seek_impl(buffered *self, PyObject *targetobj, int whence)
/*[clinic end generated code: output=7ae0e8dc46efdefb input=b5a12be70e0ad07b]*/
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

    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    if (_PyIOBase_check_seekable(state, self->raw, Py_True) == NULL) {
        return NULL;
    }

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

                // GH-95782
                if (current - avail + offset < 0)
                    return PyLong_FromOff_t(0);

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
@critical_section
_io._Buffered.truncate
    cls: defining_class
    pos: object = None
    /
[clinic start generated code]*/

static PyObject *
_io__Buffered_truncate_impl(buffered *self, PyTypeObject *cls, PyObject *pos)
/*[clinic end generated code: output=fe3882fbffe79f1a input=e3cbf794575bd794]*/
{
    PyObject *res = NULL;

    CHECK_INITIALIZED(self)
    CHECK_CLOSED(self, "truncate of closed file")
    if (!self->writable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return bufferediobase_unsupported(state, "truncate");
    }
    if (!ENTER_BUFFERED(self))
        return NULL;

    res = buffered_flush_and_rewind_unlocked(self);
    if (res == NULL) {
        goto end;
    }
    Py_CLEAR(res);

    res = PyObject_CallMethodOneArg(self->raw, &_Py_ID(truncate), pos);
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
buffered_iternext(PyObject *op)
{
    buffered *self = buffered_CAST(op);
    PyObject *line;
    PyTypeObject *tp;

    CHECK_INITIALIZED(self);

    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    tp = Py_TYPE(self);
    if (tp == state->PyBufferedReader_Type ||
        tp == state->PyBufferedRandom_Type)
    {
        /* Skip method call overhead for speed */
        line = _buffered_readline(self, -1);
    }
    else {
        line = PyObject_CallMethodNoArgs((PyObject *)self,
                                             &_Py_ID(readline));
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
buffered_repr(PyObject *op)
{
    buffered *self = buffered_CAST(op);
    PyObject *nameobj, *res;

    if (PyObject_GetOptionalAttr((PyObject *) self, &_Py_ID(name), &nameobj) < 0) {
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

    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    if (_PyIOBase_check_readable(state, raw, Py_True) == NULL) {
        return -1;
    }

    Py_XSETREF(self->raw, Py_NewRef(raw));
    self->buffer_size = buffer_size;
    self->readable = 1;
    self->writable = 0;

    if (_buffered_init(self) < 0)
        return -1;
    _bufferedreader_reset_buf(self);

    self->fast_closed_checks = (
        Py_IS_TYPE(self, state->PyBufferedReader_Type) &&
        Py_IS_TYPE(raw, state->PyFileIO_Type)
    );

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
        res = PyObject_CallMethodOneArg(self->raw, &_Py_ID(readinto), memobj);
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

    if (PyObject_GetOptionalAttr(self->raw, &_Py_ID(readall), &readall) < 0) {
        goto cleanup;
    }
    if (readall) {
        tmp = _PyObject_CallNoArgs(readall);
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
        data = PyObject_CallMethodNoArgs(self->raw, &_Py_ID(read));
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
                tmp = PyBytes_Join((PyObject *)&_Py_SINGLETON(bytes_empty), chunks);
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
    Py_ssize_t current_size, remaining, written;

    current_size = Py_SAFE_DOWNCAST(READAHEAD(self), Py_off_t, Py_ssize_t);
    if (n <= current_size)
        return _bufferedreader_read_fast(self, n);

    PyBytesWriter *writer = PyBytesWriter_Create(n);
    if (writer == NULL) {
        goto error;
    }
    char *out = PyBytesWriter_GetData(writer);

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
                return PyBytesWriter_FinishWithSize(writer, written);
            }
            PyBytesWriter_Discard(writer);
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
                return PyBytesWriter_FinishWithSize(writer, written);
            }
            PyBytesWriter_Discard(writer);
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

    return PyBytesWriter_Finish(writer);

error:
    PyBytesWriter_Discard(writer);
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

    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    if (_PyIOBase_check_writable(state, raw, Py_True) == NULL) {
        return -1;
    }

    Py_INCREF(raw);
    Py_XSETREF(self->raw, raw);
    self->readable = 0;
    self->writable = 1;

    self->buffer_size = buffer_size;
    if (_buffered_init(self) < 0)
        return -1;
    _bufferedwriter_reset_buf(self);
    self->pos = 0;

    self->fast_closed_checks = (
        Py_IS_TYPE(self, state->PyBufferedWriter_Type) &&
        Py_IS_TYPE(raw, state->PyFileIO_Type)
    );

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
        res = PyObject_CallMethodOneArg(self->raw, &_Py_ID(write), memobj);
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
@critical_section
_io.BufferedWriter.write
    buffer: Py_buffer
    /
[clinic start generated code]*/

static PyObject *
_io_BufferedWriter_write_impl(buffered *self, Py_buffer *buffer)
/*[clinic end generated code: output=7f8d1365759bfc6b input=6a9c041de0c337be]*/
{
    PyObject *res = NULL;
    Py_ssize_t written, avail, remaining;
    Py_off_t offset;
    int r;

    CHECK_INITIALIZED(self)

    if (!ENTER_BUFFERED(self))
        return NULL;

    /* Issue #31976: Check for closed file after acquiring the lock. Another
       thread could be holding the lock while closing the file. */
    r = IS_CLOSED(self);
    if (r < 0) {
        goto error;
    }
    if (r > 0) {
        PyErr_SetString(PyExc_ValueError, "write to closed file");
        goto error;
    }

    /* Fast path: the data to write can be fully buffered. */
    if (!VALID_READ_BUFFER(self) && !VALID_WRITE_BUFFER(self)) {
        self->pos = 0;
        self->raw_pos = 0;
    }
    avail = Py_SAFE_DOWNCAST(self->buffer_size - self->pos, Py_off_t, Py_ssize_t);
    if (buffer->len <= avail && buffer->len < self->buffer_size) {
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
    while (remaining >= self->buffer_size) {
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

#define rwpair_CAST(op) ((rwpair *)(op))

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
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    if (_PyIOBase_check_readable(state, reader, Py_True) == NULL) {
        return -1;
    }
    if (_PyIOBase_check_writable(state, writer, Py_True) == NULL) {
        return -1;
    }

    self->reader = (buffered *) PyObject_CallFunction(
            (PyObject *)state->PyBufferedReader_Type,
            "On", reader, buffer_size);
    if (self->reader == NULL)
        return -1;

    self->writer = (buffered *) PyObject_CallFunction(
            (PyObject *)state->PyBufferedWriter_Type,
            "On", writer, buffer_size);
    if (self->writer == NULL) {
        Py_CLEAR(self->reader);
        return -1;
    }

    return 0;
}

static int
bufferedrwpair_traverse(PyObject *op, visitproc visit, void *arg)
{
    rwpair *self = rwpair_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->dict);
    Py_VISIT(self->reader);
    Py_VISIT(self->writer);
    return 0;
}

static int
bufferedrwpair_clear(PyObject *op)
{
    rwpair *self = rwpair_CAST(op);
    Py_CLEAR(self->reader);
    Py_CLEAR(self->writer);
    Py_CLEAR(self->dict);
    return 0;
}

static void
bufferedrwpair_dealloc(PyObject *op)
{
    rwpair *self = rwpair_CAST(op);
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    FT_CLEAR_WEAKREFS(op, self->weakreflist);
    (void)bufferedrwpair_clear(op);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
_forward_call(buffered *self, PyObject *name, PyObject *args)
{
    PyObject *func, *ret;
    if (self == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "I/O operation on uninitialized object");
        return NULL;
    }

    func = PyObject_GetAttr((PyObject *)self, name);
    if (func == NULL) {
        PyErr_SetObject(PyExc_AttributeError, name);
        return NULL;
    }

    ret = PyObject_CallObject(func, args);
    Py_DECREF(func);
    return ret;
}

static PyObject *
bufferedrwpair_read(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(read), args);
}

static PyObject *
bufferedrwpair_peek(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(peek), args);
}

static PyObject *
bufferedrwpair_read1(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(read1), args);
}

static PyObject *
bufferedrwpair_readinto(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(readinto), args);
}

static PyObject *
bufferedrwpair_readinto1(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(readinto1), args);
}

static PyObject *
bufferedrwpair_write(PyObject *op, PyObject *args)
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->writer, &_Py_ID(write), args);
}

static PyObject *
bufferedrwpair_flush(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->writer, &_Py_ID(flush), NULL);
}

static PyObject *
bufferedrwpair_readable(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->reader, &_Py_ID(readable), NULL);
}

static PyObject *
bufferedrwpair_writable(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    return _forward_call(self->writer, &_Py_ID(writable), NULL);
}

static PyObject *
bufferedrwpair_close(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    PyObject *exc = NULL;
    PyObject *ret = _forward_call(self->writer, &_Py_ID(close), NULL);
    if (ret == NULL) {
        exc = PyErr_GetRaisedException();
    }
    else {
        Py_DECREF(ret);
    }
    ret = _forward_call(self->reader, &_Py_ID(close), NULL);
    if (exc != NULL) {
        _PyErr_ChainExceptions1(exc);
        Py_CLEAR(ret);
    }
    return ret;
}

static PyObject *
bufferedrwpair_isatty(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    PyObject *ret = _forward_call(self->writer, &_Py_ID(isatty), NULL);

    if (ret != Py_False) {
        /* either True or exception */
        return ret;
    }
    Py_DECREF(ret);

    return _forward_call(self->reader, &_Py_ID(isatty), NULL);
}

static PyObject *
bufferedrwpair_closed_get(PyObject *op, void *Py_UNUSED(dummy))
{
    rwpair *self = rwpair_CAST(op);
    if (self->writer == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                "the BufferedRWPair object is being garbage-collected");
        return NULL;
    }
    return PyObject_GetAttr((PyObject *) self->writer, &_Py_ID(closed));
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

    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    if (_PyIOBase_check_seekable(state, raw, Py_True) == NULL) {
        return -1;
    }
    if (_PyIOBase_check_readable(state, raw, Py_True) == NULL) {
        return -1;
    }
    if (_PyIOBase_check_writable(state, raw, Py_True) == NULL) {
        return -1;
    }

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

    self->fast_closed_checks = (Py_IS_TYPE(self, state->PyBufferedRandom_Type) &&
                                Py_IS_TYPE(raw, state->PyFileIO_Type));

    self->ok = 1;
    return 0;
}

#define clinic_state() (find_io_state_by_def(Py_TYPE(self)))
#include "clinic/bufferedio.c.h"
#undef clinic_state

static PyMethodDef bufferediobase_methods[] = {
    _IO__BUFFEREDIOBASE_DETACH_METHODDEF
    _IO__BUFFEREDIOBASE_READ_METHODDEF
    _IO__BUFFEREDIOBASE_READ1_METHODDEF
    _IO__BUFFEREDIOBASE_READINTO_METHODDEF
    _IO__BUFFEREDIOBASE_READINTO1_METHODDEF
    _IO__BUFFEREDIOBASE_WRITE_METHODDEF
    {NULL, NULL}
};

static PyType_Slot bufferediobase_slots[] = {
    {Py_tp_doc, (void *)bufferediobase_doc},
    {Py_tp_methods, bufferediobase_methods},
    {0, NULL},
};

/* Do not set Py_TPFLAGS_HAVE_GC so that tp_traverse and tp_clear are inherited */
PyType_Spec _Py_bufferediobase_spec = {
    .name = "_io._BufferedIOBase",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bufferediobase_slots,
};

static PyMethodDef bufferedreader_methods[] = {
    /* BufferedIOMixin methods */
    _IO__BUFFERED_DETACH_METHODDEF
    _IO__BUFFERED_SIMPLE_FLUSH_METHODDEF
    _IO__BUFFERED_CLOSE_METHODDEF
    _IO__BUFFERED_SEEKABLE_METHODDEF
    _IO__BUFFERED_READABLE_METHODDEF
    _IO__BUFFERED_FILENO_METHODDEF
    _IO__BUFFERED_ISATTY_METHODDEF
    _IO__BUFFERED__DEALLOC_WARN_METHODDEF

    _IO__BUFFERED_READ_METHODDEF
    _IO__BUFFERED_PEEK_METHODDEF
    _IO__BUFFERED_READ1_METHODDEF
    _IO__BUFFERED_READINTO_METHODDEF
    _IO__BUFFERED_READINTO1_METHODDEF
    _IO__BUFFERED_READLINE_METHODDEF
    _IO__BUFFERED_SEEK_METHODDEF
    _IO__BUFFERED_TELL_METHODDEF
    _IO__BUFFERED_TRUNCATE_METHODDEF
    _IO__BUFFERED___SIZEOF___METHODDEF

    {"__getstate__", _PyIOBase_cannot_pickle, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedreader_members[] = {
    {"raw", _Py_T_OBJECT, offsetof(buffered, raw), Py_READONLY},
    {"_finalizing", Py_T_BOOL, offsetof(buffered, finalizing), 0},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(buffered, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(buffered, dict), Py_READONLY},
    {NULL}
};

static PyGetSetDef bufferedreader_getset[] = {
    _IO__BUFFERED_CLOSED_GETSETDEF
    _IO__BUFFERED_NAME_GETSETDEF
    _IO__BUFFERED_MODE_GETSETDEF
    {NULL}
};


static PyType_Slot bufferedreader_slots[] = {
    {Py_tp_dealloc, buffered_dealloc},
    {Py_tp_repr, buffered_repr},
    {Py_tp_doc, (void *)_io_BufferedReader___init____doc__},
    {Py_tp_traverse, buffered_traverse},
    {Py_tp_clear, buffered_clear},
    {Py_tp_iternext, buffered_iternext},
    {Py_tp_methods, bufferedreader_methods},
    {Py_tp_members, bufferedreader_members},
    {Py_tp_getset, bufferedreader_getset},
    {Py_tp_init, _io_BufferedReader___init__},
    {0, NULL},
};

PyType_Spec _Py_bufferedreader_spec = {
    .name = "_io.BufferedReader",
    .basicsize = sizeof(buffered),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bufferedreader_slots,
};

static PyMethodDef bufferedwriter_methods[] = {
    /* BufferedIOMixin methods */
    _IO__BUFFERED_CLOSE_METHODDEF
    _IO__BUFFERED_DETACH_METHODDEF
    _IO__BUFFERED_SEEKABLE_METHODDEF
    _IO__BUFFERED_WRITABLE_METHODDEF
    _IO__BUFFERED_FILENO_METHODDEF
    _IO__BUFFERED_ISATTY_METHODDEF
    _IO__BUFFERED__DEALLOC_WARN_METHODDEF

    _IO_BUFFEREDWRITER_WRITE_METHODDEF
    _IO__BUFFERED_TRUNCATE_METHODDEF
    _IO__BUFFERED_FLUSH_METHODDEF
    _IO__BUFFERED_SEEK_METHODDEF
    _IO__BUFFERED_TELL_METHODDEF
    _IO__BUFFERED___SIZEOF___METHODDEF

    {"__getstate__", _PyIOBase_cannot_pickle, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedwriter_members[] = {
    {"raw", _Py_T_OBJECT, offsetof(buffered, raw), Py_READONLY},
    {"_finalizing", Py_T_BOOL, offsetof(buffered, finalizing), 0},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(buffered, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(buffered, dict), Py_READONLY},
    {NULL}
};

static PyGetSetDef bufferedwriter_getset[] = {
    _IO__BUFFERED_CLOSED_GETSETDEF
    _IO__BUFFERED_NAME_GETSETDEF
    _IO__BUFFERED_MODE_GETSETDEF
    {NULL}
};


static PyType_Slot bufferedwriter_slots[] = {
    {Py_tp_dealloc, buffered_dealloc},
    {Py_tp_repr, buffered_repr},
    {Py_tp_doc, (void *)_io_BufferedWriter___init____doc__},
    {Py_tp_traverse, buffered_traverse},
    {Py_tp_clear, buffered_clear},
    {Py_tp_methods, bufferedwriter_methods},
    {Py_tp_members, bufferedwriter_members},
    {Py_tp_getset, bufferedwriter_getset},
    {Py_tp_init, _io_BufferedWriter___init__},
    {0, NULL},
};

PyType_Spec _Py_bufferedwriter_spec = {
    .name = "_io.BufferedWriter",
    .basicsize = sizeof(buffered),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bufferedwriter_slots,
};

static PyMethodDef bufferedrwpair_methods[] = {
    {"read", bufferedrwpair_read, METH_VARARGS},
    {"peek", bufferedrwpair_peek, METH_VARARGS},
    {"read1", bufferedrwpair_read1, METH_VARARGS},
    {"readinto", bufferedrwpair_readinto, METH_VARARGS},
    {"readinto1", bufferedrwpair_readinto1, METH_VARARGS},

    {"write", bufferedrwpair_write, METH_VARARGS},
    {"flush", bufferedrwpair_flush, METH_NOARGS},

    {"readable", bufferedrwpair_readable, METH_NOARGS},
    {"writable", bufferedrwpair_writable, METH_NOARGS},

    {"close", bufferedrwpair_close, METH_NOARGS},
    {"isatty", bufferedrwpair_isatty, METH_NOARGS},

    {NULL, NULL}
};

static PyMemberDef bufferedrwpair_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(rwpair, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(rwpair, dict), Py_READONLY},
    {NULL}
};

static PyGetSetDef bufferedrwpair_getset[] = {
    {"closed", bufferedrwpair_closed_get, NULL, NULL},
    {NULL}
};

static PyType_Slot bufferedrwpair_slots[] = {
    {Py_tp_dealloc, bufferedrwpair_dealloc},
    {Py_tp_doc, (void *)_io_BufferedRWPair___init____doc__},
    {Py_tp_traverse, bufferedrwpair_traverse},
    {Py_tp_clear, bufferedrwpair_clear},
    {Py_tp_methods, bufferedrwpair_methods},
    {Py_tp_members, bufferedrwpair_members},
    {Py_tp_getset, bufferedrwpair_getset},
    {Py_tp_init, _io_BufferedRWPair___init__},
    {0, NULL},
};

PyType_Spec _Py_bufferedrwpair_spec = {
    .name = "_io.BufferedRWPair",
    .basicsize = sizeof(rwpair),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bufferedrwpair_slots,
};


static PyMethodDef bufferedrandom_methods[] = {
    /* BufferedIOMixin methods */
    _IO__BUFFERED_CLOSE_METHODDEF
    _IO__BUFFERED_DETACH_METHODDEF
    _IO__BUFFERED_SEEKABLE_METHODDEF
    _IO__BUFFERED_READABLE_METHODDEF
    _IO__BUFFERED_WRITABLE_METHODDEF
    _IO__BUFFERED_FILENO_METHODDEF
    _IO__BUFFERED_ISATTY_METHODDEF
    _IO__BUFFERED__DEALLOC_WARN_METHODDEF

    _IO__BUFFERED_FLUSH_METHODDEF

    _IO__BUFFERED_SEEK_METHODDEF
    _IO__BUFFERED_TELL_METHODDEF
    _IO__BUFFERED_TRUNCATE_METHODDEF
    _IO__BUFFERED_READ_METHODDEF
    _IO__BUFFERED_READ1_METHODDEF
    _IO__BUFFERED_READINTO_METHODDEF
    _IO__BUFFERED_READINTO1_METHODDEF
    _IO__BUFFERED_READLINE_METHODDEF
    _IO__BUFFERED_PEEK_METHODDEF
    _IO_BUFFEREDWRITER_WRITE_METHODDEF
    _IO__BUFFERED___SIZEOF___METHODDEF

    {"__getstate__", _PyIOBase_cannot_pickle, METH_NOARGS},
    {NULL, NULL}
};

static PyMemberDef bufferedrandom_members[] = {
    {"raw", _Py_T_OBJECT, offsetof(buffered, raw), Py_READONLY},
    {"_finalizing", Py_T_BOOL, offsetof(buffered, finalizing), 0},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(buffered, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(buffered, dict), Py_READONLY},
    {NULL}
};

static PyGetSetDef bufferedrandom_getset[] = {
    _IO__BUFFERED_CLOSED_GETSETDEF
    _IO__BUFFERED_NAME_GETSETDEF
    _IO__BUFFERED_MODE_GETSETDEF
    {NULL}
};


static PyType_Slot bufferedrandom_slots[] = {
    {Py_tp_dealloc, buffered_dealloc},
    {Py_tp_repr, buffered_repr},
    {Py_tp_doc, (void *)_io_BufferedRandom___init____doc__},
    {Py_tp_traverse, buffered_traverse},
    {Py_tp_clear, buffered_clear},
    {Py_tp_iternext, buffered_iternext},
    {Py_tp_methods, bufferedrandom_methods},
    {Py_tp_members, bufferedrandom_members},
    {Py_tp_getset, bufferedrandom_getset},
    {Py_tp_init, _io_BufferedRandom___init__},
    {0, NULL},
};

PyType_Spec _Py_bufferedrandom_spec = {
    .name = "_io.BufferedRandom",
    .basicsize = sizeof(buffered),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = bufferedrandom_slots,
};
