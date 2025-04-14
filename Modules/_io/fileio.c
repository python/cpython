/* Author: Daniel Stutzbach */

#include "Python.h"
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()

#include <stdbool.h>              // bool
#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // lseek()
#endif
#ifdef HAVE_SYS_TYPES_H
#  include <sys/types.h>
#endif
#ifdef HAVE_IO_H
#  include <io.h>
#endif
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>              // open()
#endif

#include "_iomodule.h"

/*
 * Known likely problems:
 *
 * - Files larger then 2**32-1
 * - Files with unicode filenames
 * - Passing numbers greater than 2**32-1 when an integer is expected
 * - Making it work on Windows and other oddball platforms
 *
 * To Do:
 *
 * - autoconfify header file inclusion
 */

#ifdef MS_WINDOWS
   // can simulate truncate with Win32 API functions; see file_truncate
#  define HAVE_FTRUNCATE
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

#if BUFSIZ < (8*1024)
#  define SMALLCHUNK (8*1024)
#elif (BUFSIZ >= (2 << 25))
#  error "unreasonable BUFSIZ > 64 MiB defined"
#else
#  define SMALLCHUNK BUFSIZ
#endif

/* Size at which a buffer is considered "large" and behavior should change to
   avoid excessive memory allocation */
#define LARGE_BUFFER_CUTOFF_SIZE 65536

/*[clinic input]
module _io
class _io.FileIO "fileio *" "clinic_state()->PyFileIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ac25ec278f4d6703]*/

typedef struct {
    PyObject_HEAD
    int fd;
    unsigned int created : 1;
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int appending : 1;
    signed int seekable : 2; /* -1 means unknown */
    unsigned int closefd : 1;
    char finalizing;
    /* Stat result which was grabbed at file open, useful for optimizing common
       File I/O patterns to be more efficient. This is only guidance / an
       estimate, as it is subject to Time-Of-Check to Time-Of-Use (TOCTOU)
       issues / bugs. Both the underlying file descriptor and file may be
       modified outside of the fileio object / Python (ex. gh-90102, GH-121941,
       gh-109523). */
    struct _Py_stat_struct *stat_atopen;
    PyObject *weakreflist;
    PyObject *dict;
} fileio;

#define PyFileIO_Check(state, op) (PyObject_TypeCheck((op), state->PyFileIO_Type))
#define PyFileIO_CAST(op) ((fileio *)(op))

/* Forward declarations */
static PyObject* portable_lseek(fileio *self, PyObject *posobj, int whence, bool suppress_pipe_error);

int
_PyFileIO_closed(PyObject *self)
{
    return (PyFileIO_CAST(self)->fd < 0);
}

/* Because this can call arbitrary code, it shouldn't be called when
   the refcount is 0 (that is, not directly from tp_dealloc unless
   the refcount has been temporarily re-incremented). */
static PyObject *
fileio_dealloc_warn(PyObject *op, PyObject *source)
{
    fileio *self = PyFileIO_CAST(op);
    if (self->fd >= 0 && self->closefd) {
        PyObject *exc = PyErr_GetRaisedException();
        if (PyErr_ResourceWarning(source, 1, "unclosed file %R", source)) {
            /* Spurious errors can appear at shutdown */
            if (PyErr_ExceptionMatches(PyExc_Warning)) {
                PyErr_FormatUnraisable("Exception ignored "
                                       "while finalizing file %R", self);
            }
        }
        PyErr_SetRaisedException(exc);
    }
    Py_RETURN_NONE;
}

/* Returns 0 on success, -1 with exception set on failure. */
static int
internal_close(fileio *self)
{
    int err = 0;
    int save_errno = 0;
    if (self->fd >= 0) {
        int fd = self->fd;
        self->fd = -1;
        /* fd is accessible and someone else may have closed it */
        Py_BEGIN_ALLOW_THREADS
        _Py_BEGIN_SUPPRESS_IPH
        err = close(fd);
        if (err < 0)
            save_errno = errno;
        _Py_END_SUPPRESS_IPH
        Py_END_ALLOW_THREADS
    }
    PyMem_Free(self->stat_atopen);
    self->stat_atopen = NULL;
    if (err < 0) {
        errno = save_errno;
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
}

/*[clinic input]
_io.FileIO.close

    cls: defining_class
    /

Close the file.

A closed file cannot be used for further I/O operations.  close() may be
called more than once without error.
[clinic start generated code]*/

static PyObject *
_io_FileIO_close_impl(fileio *self, PyTypeObject *cls)
/*[clinic end generated code: output=c30cbe9d1f23ca58 input=70da49e63db7c64d]*/
{
    PyObject *res;
    int rc;
    _PyIO_State *state = get_io_state_by_cls(cls);
    res = PyObject_CallMethodOneArg((PyObject*)state->PyRawIOBase_Type,
                                     &_Py_ID(close), (PyObject *)self);
    if (!self->closefd) {
        self->fd = -1;
        return res;
    }

    PyObject *exc = NULL;
    if (res == NULL) {
        exc = PyErr_GetRaisedException();
    }
    if (self->finalizing) {
        PyObject *r = fileio_dealloc_warn((PyObject*)self, (PyObject *) self);
        if (r) {
            Py_DECREF(r);
        }
        else {
            PyErr_Clear();
        }
    }
    rc = internal_close(self);
    if (res == NULL) {
        _PyErr_ChainExceptions1(exc);
    }
    if (rc < 0) {
        Py_CLEAR(res);
    }
    return res;
}

static PyObject *
fileio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    assert(type != NULL && type->tp_alloc != NULL);

    fileio *self = (fileio *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    self->fd = -1;
    self->created = 0;
    self->readable = 0;
    self->writable = 0;
    self->appending = 0;
    self->seekable = -1;
    self->stat_atopen = NULL;
    self->closefd = 1;
    self->weakreflist = NULL;
    return (PyObject *) self;
}

#ifdef O_CLOEXEC
extern int _Py_open_cloexec_works;
#endif

/*[clinic input]
_io.FileIO.__init__
    file as nameobj: object
    mode: str = "r"
    closefd: bool = True
    opener: object = None

Open a file.

The mode can be 'r' (default), 'w', 'x' or 'a' for reading,
writing, exclusive creation or appending.  The file will be created if it
doesn't exist when opened for writing or appending; it will be truncated
when opened for writing.  A FileExistsError will be raised if it already
exists when opened for creating. Opening a file for creating implies
writing so this mode behaves in a similar way to 'w'.Add a '+' to the mode
to allow simultaneous reading and writing. A custom opener can be used by
passing a callable as *opener*. The underlying file descriptor for the file
object is then obtained by calling opener with (*name*, *flags*).
*opener* must return an open file descriptor (passing os.open as *opener*
results in functionality similar to passing None).
[clinic start generated code]*/

static int
_io_FileIO___init___impl(fileio *self, PyObject *nameobj, const char *mode,
                         int closefd, PyObject *opener)
/*[clinic end generated code: output=23413f68e6484bbd input=588aac967e0ba74b]*/
{
#ifdef MS_WINDOWS
    wchar_t *widename = NULL;
#else
    const char *name = NULL;
#endif
    PyObject *stringobj = NULL;
    const char *s;
    int ret = 0;
    int rwa = 0, plus = 0;
    int flags = 0;
    int fd = -1;
    int fd_is_own = 0;
#ifdef O_CLOEXEC
    int *atomic_flag_works = &_Py_open_cloexec_works;
#elif !defined(MS_WINDOWS)
    int *atomic_flag_works = NULL;
#endif
    int fstat_result;
    int async_err = 0;

#ifdef Py_DEBUG
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    assert(PyFileIO_Check(state, self));
#endif
    if (self->fd >= 0) {
        if (self->closefd) {
            /* Have to close the existing file first. */
            if (internal_close(self) < 0) {
                return -1;
            }
        }
        else
            self->fd = -1;
    }

    if (PyBool_Check(nameobj)) {
        if (PyErr_WarnEx(PyExc_RuntimeWarning,
                "bool is used as a file descriptor", 1))
        {
            return -1;
        }
    }
    fd = PyLong_AsInt(nameobj);
    if (fd < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "negative file descriptor");
            return -1;
        }
        PyErr_Clear();
    }

    if (fd < 0) {
#ifdef MS_WINDOWS
        if (!PyUnicode_FSDecoder(nameobj, &stringobj)) {
            return -1;
        }
        widename = PyUnicode_AsWideCharString(stringobj, NULL);
        if (widename == NULL)
            return -1;
#else
        if (!PyUnicode_FSConverter(nameobj, &stringobj)) {
            return -1;
        }
        name = PyBytes_AS_STRING(stringobj);
#endif
    }

    s = mode;
    while (*s) {
        switch (*s++) {
        case 'x':
            if (rwa) {
            bad_mode:
                PyErr_SetString(PyExc_ValueError,
                                "Must have exactly one of create/read/write/append "
                                "mode and at most one plus");
                goto error;
            }
            rwa = 1;
            self->created = 1;
            self->writable = 1;
            flags |= O_EXCL | O_CREAT;
            break;
        case 'r':
            if (rwa)
                goto bad_mode;
            rwa = 1;
            self->readable = 1;
            break;
        case 'w':
            if (rwa)
                goto bad_mode;
            rwa = 1;
            self->writable = 1;
            flags |= O_CREAT | O_TRUNC;
            break;
        case 'a':
            if (rwa)
                goto bad_mode;
            rwa = 1;
            self->writable = 1;
            self->appending = 1;
            flags |= O_APPEND | O_CREAT;
            break;
        case 'b':
            break;
        case '+':
            if (plus)
                goto bad_mode;
            self->readable = self->writable = 1;
            plus = 1;
            break;
        default:
            PyErr_Format(PyExc_ValueError,
                         "invalid mode: %.200s", mode);
            goto error;
        }
    }

    if (!rwa)
        goto bad_mode;

    if (self->readable && self->writable)
        flags |= O_RDWR;
    else if (self->readable)
        flags |= O_RDONLY;
    else
        flags |= O_WRONLY;

#ifdef O_BINARY
    flags |= O_BINARY;
#endif

#ifdef MS_WINDOWS
    flags |= O_NOINHERIT;
#elif defined(O_CLOEXEC)
    flags |= O_CLOEXEC;
#endif

    if (PySys_Audit("open", "Osi", nameobj, mode, flags) < 0) {
        goto error;
    }

    if (fd >= 0) {
        self->fd = fd;
        self->closefd = closefd;
    }
    else {
        self->closefd = 1;
        if (!closefd) {
            PyErr_SetString(PyExc_ValueError,
                "Cannot use closefd=False with file name");
            goto error;
        }

        errno = 0;
        if (opener == Py_None) {
            do {
                Py_BEGIN_ALLOW_THREADS
#ifdef MS_WINDOWS
                self->fd = _wopen(widename, flags, 0666);
#else
                self->fd = open(name, flags, 0666);
#endif
                Py_END_ALLOW_THREADS
            } while (self->fd < 0 && errno == EINTR &&
                     !(async_err = PyErr_CheckSignals()));

            if (async_err)
                goto error;

            if (self->fd < 0) {
                PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, nameobj);
                goto error;
            }
        }
        else {
            PyObject *fdobj;

#ifndef MS_WINDOWS
            /* the opener may clear the atomic flag */
            atomic_flag_works = NULL;
#endif

            fdobj = PyObject_CallFunction(opener, "Oi", nameobj, flags);
            if (fdobj == NULL)
                goto error;
            if (!PyLong_Check(fdobj)) {
                Py_DECREF(fdobj);
                PyErr_SetString(PyExc_TypeError,
                        "expected integer from opener");
                goto error;
            }

            self->fd = PyLong_AsInt(fdobj);
            Py_DECREF(fdobj);
            if (self->fd < 0) {
                if (!PyErr_Occurred()) {
                    /* The opener returned a negative but didn't set an
                       exception.  See issue #27066 */
                    PyErr_Format(PyExc_ValueError,
                                 "opener returned %d", self->fd);
                }
                goto error;
            }
        }
        fd_is_own = 1;

#ifndef MS_WINDOWS
        if (_Py_set_inheritable(self->fd, 0, atomic_flag_works) < 0)
            goto error;
#endif
    }

    PyMem_Free(self->stat_atopen);
    self->stat_atopen = PyMem_New(struct _Py_stat_struct, 1);
    if (self->stat_atopen == NULL) {
        PyErr_NoMemory();
        goto error;
    }
    Py_BEGIN_ALLOW_THREADS
    fstat_result = _Py_fstat_noraise(self->fd, self->stat_atopen);
    Py_END_ALLOW_THREADS
    if (fstat_result < 0) {
        /* Tolerate fstat() errors other than EBADF.  See Issue #25717, where
        an anonymous file on a Virtual Box shared folder filesystem would
        raise ENOENT. */
#ifdef MS_WINDOWS
        if (GetLastError() == ERROR_INVALID_HANDLE) {
            PyErr_SetFromWindowsErr(0);
#else
        if (errno == EBADF) {
            PyErr_SetFromErrno(PyExc_OSError);
#endif
            goto error;
        }

        PyMem_Free(self->stat_atopen);
        self->stat_atopen = NULL;
    }
    else {
#if defined(S_ISDIR) && defined(EISDIR)
        /* On Unix, open will succeed for directories.
           In Python, there should be no file objects referring to
           directories, so we need a check.  */
        if (S_ISDIR(self->stat_atopen->st_mode)) {
            errno = EISDIR;
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, nameobj);
            goto error;
        }
#endif /* defined(S_ISDIR) */
    }

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
    /* don't translate newlines (\r\n <=> \n) */
    _setmode(self->fd, O_BINARY);
#endif

    if (PyObject_SetAttr((PyObject *)self, &_Py_ID(name), nameobj) < 0)
        goto error;

    if (self->appending) {
        /* For consistent behaviour, we explicitly seek to the
           end of file (otherwise, it might be done only on the
           first write()). */
        PyObject *pos = portable_lseek(self, NULL, 2, true);
        if (pos == NULL)
            goto error;
        Py_DECREF(pos);
    }

    goto done;

 error:
    ret = -1;
    if (!fd_is_own)
        self->fd = -1;
    if (self->fd >= 0) {
        PyObject *exc = PyErr_GetRaisedException();
        internal_close(self);
        _PyErr_ChainExceptions1(exc);
    }
    PyMem_Free(self->stat_atopen);
    self->stat_atopen = NULL;

 done:
#ifdef MS_WINDOWS
    PyMem_Free(widename);
#endif
    Py_CLEAR(stringobj);
    return ret;
}

static int
fileio_traverse(PyObject *op, visitproc visit, void *arg)
{
    fileio *self = PyFileIO_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->dict);
    return 0;
}

static int
fileio_clear(PyObject *op)
{
    fileio *self = PyFileIO_CAST(op);
    Py_CLEAR(self->dict);
    return 0;
}

static void
fileio_dealloc(PyObject *op)
{
    fileio *self = PyFileIO_CAST(op);
    self->finalizing = 1;
    if (_PyIOBase_finalize(op) < 0) {
        return;
    }

    _PyObject_GC_UNTRACK(self);
    if (self->stat_atopen != NULL) {
        PyMem_Free(self->stat_atopen);
        self->stat_atopen = NULL;
    }
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs(op);
    }
    (void)fileio_clear(op);

    PyTypeObject *tp = Py_TYPE(op);
    tp->tp_free(op);
    Py_DECREF(tp);
}

static PyObject *
err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
    return NULL;
}

static PyObject *
err_mode(_PyIO_State *state, const char *action)
{
    return PyErr_Format(state->unsupported_operation,
                        "File not open for %s", action);
}

/*[clinic input]
_io.FileIO.fileno

Return the underlying file descriptor (an integer).
[clinic start generated code]*/

static PyObject *
_io_FileIO_fileno_impl(fileio *self)
/*[clinic end generated code: output=a9626ce5398ece90 input=0b9b2de67335ada3]*/
{
    if (self->fd < 0)
        return err_closed();
    return PyLong_FromLong((long) self->fd);
}

/*[clinic input]
_io.FileIO.readable

True if file was opened in a read mode.
[clinic start generated code]*/

static PyObject *
_io_FileIO_readable_impl(fileio *self)
/*[clinic end generated code: output=640744a6150fe9ba input=a3fdfed6eea721c5]*/
{
    if (self->fd < 0)
        return err_closed();
    return PyBool_FromLong((long) self->readable);
}

/*[clinic input]
_io.FileIO.writable

True if file was opened in a write mode.
[clinic start generated code]*/

static PyObject *
_io_FileIO_writable_impl(fileio *self)
/*[clinic end generated code: output=96cefc5446e89977 input=c204a808ca2e1748]*/
{
    if (self->fd < 0)
        return err_closed();
    return PyBool_FromLong((long) self->writable);
}

/*[clinic input]
_io.FileIO.seekable

True if file supports random-access.
[clinic start generated code]*/

static PyObject *
_io_FileIO_seekable_impl(fileio *self)
/*[clinic end generated code: output=47909ca0a42e9287 input=c8e5554d2fd63c7f]*/
{
    if (self->fd < 0)
        return err_closed();
    if (self->seekable < 0) {
        /* portable_lseek() sets the seekable attribute */
        PyObject *pos = portable_lseek(self, NULL, SEEK_CUR, false);
        assert(self->seekable >= 0);
        if (pos == NULL) {
            PyErr_Clear();
        }
        else {
            Py_DECREF(pos);
        }
    }
    return PyBool_FromLong((long) self->seekable);
}

/*[clinic input]
_io.FileIO.readinto
    cls: defining_class
    buffer: Py_buffer(accept={rwbuffer})
    /

Same as RawIOBase.readinto().
[clinic start generated code]*/

static PyObject *
_io_FileIO_readinto_impl(fileio *self, PyTypeObject *cls, Py_buffer *buffer)
/*[clinic end generated code: output=97f0f3d69534db34 input=fd20323e18ce1ec8]*/
{
    Py_ssize_t n;
    int err;

    if (self->fd < 0)
        return err_closed();
    if (!self->readable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "reading");
    }

    n = _Py_read(self->fd, buffer->buf, buffer->len);
    /* copy errno because PyBuffer_Release() can indirectly modify it */
    err = errno;

    if (n == -1) {
        if (err == EAGAIN) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return NULL;
    }

    return PyLong_FromSsize_t(n);
}

static size_t
new_buffersize(fileio *self, size_t currentsize)
{
    size_t addend;

    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior.  For bigger sizes, use a
       less-than-double growth factor to avoid excessive allocation. */
    assert(currentsize <= PY_SSIZE_T_MAX);
    if (currentsize > LARGE_BUFFER_CUTOFF_SIZE)
        addend = currentsize >> 3;
    else
        addend = 256 + currentsize;
    if (addend < SMALLCHUNK)
        /* Avoid tiny read() calls. */
        addend = SMALLCHUNK;
    return addend + currentsize;
}

/*[clinic input]
_io.FileIO.readall

Read all data from the file, returned as bytes.

Reads until either there is an error or read() returns size 0 (indicates EOF).
If the file is already at EOF, returns an empty bytes object.

In non-blocking mode, returns as much data as could be read before EAGAIN. If no
data is available (EAGAIN is returned before bytes are read) returns None.
[clinic start generated code]*/

static PyObject *
_io_FileIO_readall_impl(fileio *self)
/*[clinic end generated code: output=faa0292b213b4022 input=1e19849857f5d0a1]*/
{
    Py_off_t pos, end;
    PyObject *result;
    Py_ssize_t bytes_read = 0;
    Py_ssize_t n;
    size_t bufsize;

    if (self->fd < 0) {
        return err_closed();
    }

    if (self->stat_atopen != NULL && self->stat_atopen->st_size < _PY_READ_MAX) {
        end = (Py_off_t)self->stat_atopen->st_size;
    }
    else {
        end = -1;
    }
    if (end <= 0) {
        /* Use a default size and resize as needed. */
        bufsize = SMALLCHUNK;
    }
    else {
        /* This is probably a real file. */
        if (end > _PY_READ_MAX - 1) {
            bufsize = _PY_READ_MAX;
        }
        else {
            /* In order to detect end of file, need a read() of at
               least 1 byte which returns size 0. Oversize the buffer
               by 1 byte so the I/O can be completed with two read()
               calls (one for all data, one for EOF) without needing
               to resize the buffer. */
            bufsize = (size_t)end + 1;
        }

        /* While a lot of code does open().read() to get the whole contents
           of a file it is possible a caller seeks/reads a ways into the file
           then calls readall() to get the rest, which would result in allocating
           more than required. Guard against that for larger files where we expect
           the I/O time to dominate anyways while keeping small files fast. */
        if (bufsize > LARGE_BUFFER_CUTOFF_SIZE) {
            Py_BEGIN_ALLOW_THREADS
            _Py_BEGIN_SUPPRESS_IPH
#ifdef MS_WINDOWS
            pos = _lseeki64(self->fd, 0L, SEEK_CUR);
#else
            pos = lseek(self->fd, 0L, SEEK_CUR);
#endif
            _Py_END_SUPPRESS_IPH
            Py_END_ALLOW_THREADS

            if (end >= pos && pos >= 0 && (end - pos) < (_PY_READ_MAX - 1)) {
                bufsize = (size_t)(end - pos) + 1;
            }
        }
    }


    result = PyBytes_FromStringAndSize(NULL, bufsize);
    if (result == NULL)
        return NULL;

    while (1) {
        if (bytes_read >= (Py_ssize_t)bufsize) {
            bufsize = new_buffersize(self, bytes_read);
            if (bufsize > PY_SSIZE_T_MAX || bufsize <= 0) {
                PyErr_SetString(PyExc_OverflowError,
                                "unbounded read returned more bytes "
                                "than a Python bytes object can hold");
                Py_DECREF(result);
                return NULL;
            }

            if (PyBytes_GET_SIZE(result) < (Py_ssize_t)bufsize) {
                if (_PyBytes_Resize(&result, bufsize) < 0)
                    return NULL;
            }
        }

        n = _Py_read(self->fd,
                     PyBytes_AS_STRING(result) + bytes_read,
                     bufsize - bytes_read);

        if (n == 0)
            break;
        if (n == -1) {
            if (errno == EAGAIN) {
                PyErr_Clear();
                if (bytes_read > 0)
                    break;
                Py_DECREF(result);
                Py_RETURN_NONE;
            }
            Py_DECREF(result);
            return NULL;
        }
        bytes_read += n;
    }

    if (PyBytes_GET_SIZE(result) > bytes_read) {
        if (_PyBytes_Resize(&result, bytes_read) < 0)
            return NULL;
    }
    return result;
}

/*[clinic input]
_io.FileIO.read
    cls: defining_class
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read at most size bytes, returned as bytes.

If size is less than 0, read all bytes in the file making multiple read calls.
See ``FileIO.readall``.

Attempts to make only one system call, retrying only per PEP 475 (EINTR). This
means less data may be returned than requested.

In non-blocking mode, returns None if no data is available. Return an empty
bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io_FileIO_read_impl(fileio *self, PyTypeObject *cls, Py_ssize_t size)
/*[clinic end generated code: output=bbd749c7c224143e input=cf21fddef7d38ab6]*/
{
    char *ptr;
    Py_ssize_t n;
    PyObject *bytes;

    if (self->fd < 0)
        return err_closed();
    if (!self->readable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "reading");
    }

    if (size < 0)
        return _io_FileIO_readall_impl(self);

    if (size > _PY_READ_MAX) {
        size = _PY_READ_MAX;
    }

    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return NULL;
    ptr = PyBytes_AS_STRING(bytes);

    n = _Py_read(self->fd, ptr, size);
    if (n == -1) {
        /* copy errno because Py_DECREF() can indirectly modify it */
        int err = errno;
        Py_DECREF(bytes);
        if (err == EAGAIN) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return NULL;
    }

    if (n != size) {
        if (_PyBytes_Resize(&bytes, n) < 0) {
            Py_CLEAR(bytes);
            return NULL;
        }
    }

    return (PyObject *) bytes;
}

/*[clinic input]
_io.FileIO.write
    cls: defining_class
    b: Py_buffer
    /

Write buffer b to file, return number of bytes written.

Only makes one system call, so not all of the data may be written.
The number of bytes actually written is returned.  In non-blocking mode,
returns None if the write would block.
[clinic start generated code]*/

static PyObject *
_io_FileIO_write_impl(fileio *self, PyTypeObject *cls, Py_buffer *b)
/*[clinic end generated code: output=927e25be80f3b77b input=2776314f043088f5]*/
{
    Py_ssize_t n;
    int err;

    if (self->fd < 0)
        return err_closed();
    if (!self->writable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "writing");
    }

    n = _Py_write(self->fd, b->buf, b->len);
    /* copy errno because PyBuffer_Release() can indirectly modify it */
    err = errno;

    if (n < 0) {
        if (err == EAGAIN) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        return NULL;
    }

    return PyLong_FromSsize_t(n);
}

/* XXX Windows support below is likely incomplete */

/* Cribbed from posix_lseek() */
static PyObject *
portable_lseek(fileio *self, PyObject *posobj, int whence, bool suppress_pipe_error)
{
    Py_off_t pos, res;
    int fd = self->fd;

#ifdef SEEK_SET
    /* Turn 0, 1, 2 into SEEK_{SET,CUR,END} */
    switch (whence) {
#if SEEK_SET != 0
    case 0: whence = SEEK_SET; break;
#endif
#if SEEK_CUR != 1
    case 1: whence = SEEK_CUR; break;
#endif
#if SEEK_END != 2
    case 2: whence = SEEK_END; break;
#endif
    }
#endif /* SEEK_SET */

    if (posobj == NULL) {
        pos = 0;
    }
    else {
#if defined(HAVE_LARGEFILE_SUPPORT)
        pos = PyLong_AsLongLong(posobj);
#else
        pos = PyLong_AsLong(posobj);
#endif
        if (PyErr_Occurred())
            return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
#ifdef MS_WINDOWS
    res = _lseeki64(fd, pos, whence);
#else
    res = lseek(fd, pos, whence);
#endif
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS

    if (self->seekable < 0) {
        self->seekable = (res >= 0);
    }

    if (res < 0) {
        if (suppress_pipe_error && errno == ESPIPE) {
            res = 0;
        } else {
            return PyErr_SetFromErrno(PyExc_OSError);
        }
    }

#if defined(HAVE_LARGEFILE_SUPPORT)
    return PyLong_FromLongLong(res);
#else
    return PyLong_FromLong(res);
#endif
}

/*[clinic input]
_io.FileIO.seek
    pos: object
    whence: int = 0
    /

Move to new file position and return the file position.

Argument offset is a byte count.  Optional argument whence defaults to
SEEK_SET or 0 (offset from start of file, offset should be >= 0); other values
are SEEK_CUR or 1 (move relative to current position, positive or negative),
and SEEK_END or 2 (move relative to end of file, usually negative, although
many platforms allow seeking beyond the end of a file).

Note that not all file objects are seekable.
[clinic start generated code]*/

static PyObject *
_io_FileIO_seek_impl(fileio *self, PyObject *pos, int whence)
/*[clinic end generated code: output=c976acdf054e6655 input=0439194b0774d454]*/
{
    if (self->fd < 0)
        return err_closed();

    return portable_lseek(self, pos, whence, false);
}

/*[clinic input]
_io.FileIO.tell

Current file position.

Can raise OSError for non seekable files.
[clinic start generated code]*/

static PyObject *
_io_FileIO_tell_impl(fileio *self)
/*[clinic end generated code: output=ffe2147058809d0b input=807e24ead4cec2f9]*/
{
    if (self->fd < 0)
        return err_closed();

    return portable_lseek(self, NULL, 1, false);
}

#ifdef HAVE_FTRUNCATE
/*[clinic input]
_io.FileIO.truncate
    cls: defining_class
    size as posobj: object = None
    /

Truncate the file to at most size bytes and return the truncated size.

Size defaults to the current file position, as returned by tell().
The current file position is changed to the value of size.
[clinic start generated code]*/

static PyObject *
_io_FileIO_truncate_impl(fileio *self, PyTypeObject *cls, PyObject *posobj)
/*[clinic end generated code: output=d936732a49e8d5a2 input=c367fb45d6bb2c18]*/
{
    Py_off_t pos;
    int ret;
    int fd;

    fd = self->fd;
    if (fd < 0)
        return err_closed();
    if (!self->writable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "writing");
    }

    if (posobj == Py_None) {
        /* Get the current position. */
        posobj = portable_lseek(self, NULL, 1, false);
        if (posobj == NULL)
            return NULL;
    }
    else {
        Py_INCREF(posobj);
    }

#if defined(HAVE_LARGEFILE_SUPPORT)
    pos = PyLong_AsLongLong(posobj);
#else
    pos = PyLong_AsLong(posobj);
#endif
    if (PyErr_Occurred()){
        Py_DECREF(posobj);
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    errno = 0;
#ifdef MS_WINDOWS
    ret = _chsize_s(fd, pos);
#else
    ret = ftruncate(fd, pos);
#endif
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS

    if (ret != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        Py_DECREF(posobj);
        return NULL;
    }

    /* Since the file was truncated, its size at open is no longer accurate
       as an estimate. Clear out the stat result, and rely on dynamic resize
       code if a readall is requested. */
    if (self->stat_atopen != NULL) {
        PyMem_Free(self->stat_atopen);
        self->stat_atopen = NULL;
    }

    return posobj;
}
#endif /* HAVE_FTRUNCATE */

static const char *
mode_string(fileio *self)
{
    if (self->created) {
        if (self->readable)
            return "xb+";
        else
            return "xb";
    }
    if (self->appending) {
        if (self->readable)
            return "ab+";
        else
            return "ab";
    }
    else if (self->readable) {
        if (self->writable)
            return "rb+";
        else
            return "rb";
    }
    else
        return "wb";
}

static PyObject *
fileio_repr(PyObject *op)
{
    fileio *self = PyFileIO_CAST(op);
    const char *type_name = Py_TYPE(self)->tp_name;

    if (self->fd < 0) {
        return PyUnicode_FromFormat("<%.100s [closed]>", type_name);
    }

    PyObject *nameobj;
    if (PyObject_GetOptionalAttr((PyObject *) self, &_Py_ID(name), &nameobj) < 0) {
        return NULL;
    }
    PyObject *res;
    if (nameobj == NULL) {
        res = PyUnicode_FromFormat(
            "<%.100s fd=%d mode='%s' closefd=%s>",
            type_name, self->fd, mode_string(self), self->closefd ? "True" : "False");
    }
    else {
        int status = Py_ReprEnter((PyObject *)self);
        res = NULL;
        if (status == 0) {
            res = PyUnicode_FromFormat(
                "<%.100s name=%R mode='%s' closefd=%s>",
                type_name, nameobj, mode_string(self), self->closefd ? "True" : "False");
            Py_ReprLeave((PyObject *)self);
        }
        else if (status > 0) {
            PyErr_Format(PyExc_RuntimeError,
                         "reentrant call inside %.100s.__repr__", type_name);
        }
        Py_DECREF(nameobj);
    }
    return res;
}

/*[clinic input]
_io.FileIO.isatty

True if the file is connected to a TTY device.
[clinic start generated code]*/

static PyObject *
_io_FileIO_isatty_impl(fileio *self)
/*[clinic end generated code: output=932c39924e9a8070 input=cd94ca1f5e95e843]*/
{
    long res;

    if (self->fd < 0)
        return err_closed();
    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    res = isatty(self->fd);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(res);
}

/* Checks whether the file is a TTY using an open-only optimization.

   TTYs are always character devices. If the interpreter knows a file is
   not a character device when it would call ``isatty``, can skip that
   call. Inside ``open()``  there is a fresh stat result that contains that
   information. Use the stat result to skip a system call. Outside of that
   context TOCTOU issues (the fd could be arbitrarily modified by
   surrounding code). */
static PyObject *
_io_FileIO_isatty_open_only(PyObject *op, PyObject *Py_UNUSED(dummy))
{
    fileio *self = PyFileIO_CAST(op);
    if (self->stat_atopen != NULL && !S_ISCHR(self->stat_atopen->st_mode)) {
        Py_RETURN_FALSE;
    }
    return _io_FileIO_isatty_impl(self);
}

#include "clinic/fileio.c.h"

static PyMethodDef fileio_methods[] = {
    _IO_FILEIO_READ_METHODDEF
    _IO_FILEIO_READALL_METHODDEF
    _IO_FILEIO_READINTO_METHODDEF
    _IO_FILEIO_WRITE_METHODDEF
    _IO_FILEIO_SEEK_METHODDEF
    _IO_FILEIO_TELL_METHODDEF
    _IO_FILEIO_TRUNCATE_METHODDEF
    _IO_FILEIO_CLOSE_METHODDEF
    _IO_FILEIO_SEEKABLE_METHODDEF
    _IO_FILEIO_READABLE_METHODDEF
    _IO_FILEIO_WRITABLE_METHODDEF
    _IO_FILEIO_FILENO_METHODDEF
    _IO_FILEIO_ISATTY_METHODDEF
    {"_isatty_open_only", _io_FileIO_isatty_open_only, METH_NOARGS},
    {"_dealloc_warn", fileio_dealloc_warn, METH_O, NULL},
    {"__reduce__", _PyIOBase_cannot_pickle, METH_NOARGS},
    {"__reduce_ex__", _PyIOBase_cannot_pickle, METH_O},
    {NULL,           NULL}             /* sentinel */
};

/* 'closed' and 'mode' are attributes for backwards compatibility reasons. */

static PyObject *
fileio_get_closed(PyObject *op, void *closure)
{
    fileio *self = PyFileIO_CAST(op);
    return PyBool_FromLong((long)(self->fd < 0));
}

static PyObject *
fileio_get_closefd(PyObject *op, void *closure)
{
    fileio *self = PyFileIO_CAST(op);
    return PyBool_FromLong((long)(self->closefd));
}

static PyObject *
fileio_get_mode(PyObject *op, void *closure)
{
    fileio *self = PyFileIO_CAST(op);
    return PyUnicode_FromString(mode_string(self));
}

static PyObject *
fileio_get_blksize(PyObject *op, void *closure)
{
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
    fileio *self = PyFileIO_CAST(op);
    if (self->stat_atopen != NULL && self->stat_atopen->st_blksize > 1) {
        return PyLong_FromLong(self->stat_atopen->st_blksize);
    }
#endif /* HAVE_STRUCT_STAT_ST_BLKSIZE */
    return PyLong_FromLong(DEFAULT_BUFFER_SIZE);
}

static PyGetSetDef fileio_getsetlist[] = {
    {"closed", fileio_get_closed, NULL, "True if the file is closed"},
    {"closefd", fileio_get_closefd, NULL,
        "True if the file descriptor will be closed by close()."},
    {"mode", fileio_get_mode, NULL, "String giving the file mode"},
    {"_blksize", fileio_get_blksize, NULL, "Stat st_blksize if available"},
    {NULL},
};

static PyMemberDef fileio_members[] = {
    {"_finalizing", Py_T_BOOL, offsetof(fileio, finalizing), 0},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(fileio, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(fileio, dict), Py_READONLY},
    {NULL}
};

static PyType_Slot fileio_slots[] = {
    {Py_tp_dealloc, fileio_dealloc},
    {Py_tp_repr, fileio_repr},
    {Py_tp_doc, (void *)_io_FileIO___init____doc__},
    {Py_tp_traverse, fileio_traverse},
    {Py_tp_clear, fileio_clear},
    {Py_tp_methods, fileio_methods},
    {Py_tp_members, fileio_members},
    {Py_tp_getset, fileio_getsetlist},
    {Py_tp_init, _io_FileIO___init__},
    {Py_tp_new, fileio_new},
    {0, NULL},
};

PyType_Spec fileio_spec = {
    .name = "_io.FileIO",
    .basicsize = sizeof(fileio),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = fileio_slots,
};
