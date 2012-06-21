/* Author: Daniel Stutzbach */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <stddef.h> /* For offsetof */
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
/* can simulate truncate with Win32 API functions; see file_truncate */
#define HAVE_FTRUNCATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if BUFSIZ < (8*1024)
#define SMALLCHUNK (8*1024)
#elif (BUFSIZ >= (2 << 25))
#error "unreasonable BUFSIZ > 64MB defined"
#else
#define SMALLCHUNK BUFSIZ
#endif

typedef struct {
    PyObject_HEAD
    int fd;
    unsigned int readable : 1;
    unsigned int writable : 1;
    signed int seekable : 2; /* -1 means unknown */
    unsigned int closefd : 1;
    unsigned int deallocating: 1;
    PyObject *weakreflist;
    PyObject *dict;
} fileio;

PyTypeObject PyFileIO_Type;

#define PyFileIO_Check(op) (PyObject_TypeCheck((op), &PyFileIO_Type))

int
_PyFileIO_closed(PyObject *self)
{
    return ((fileio *)self)->fd < 0;
}

/* Because this can call arbitrary code, it shouldn't be called when
   the refcount is 0 (that is, not directly from tp_dealloc unless
   the refcount has been temporarily re-incremented). */
static PyObject *
fileio_dealloc_warn(fileio *self, PyObject *source)
{
    if (self->fd >= 0 && self->closefd) {
        PyObject *exc, *val, *tb;
        PyErr_Fetch(&exc, &val, &tb);
        if (PyErr_WarnFormat(PyExc_ResourceWarning, 1,
                             "unclosed file %R", source)) {
            /* Spurious errors can appear at shutdown */
            if (PyErr_ExceptionMatches(PyExc_Warning))
                PyErr_WriteUnraisable((PyObject *) self);
        }
        PyErr_Restore(exc, val, tb);
    }
    Py_RETURN_NONE;
}

static PyObject *
portable_lseek(int fd, PyObject *posobj, int whence);

static PyObject *portable_lseek(int fd, PyObject *posobj, int whence);

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
        if (_PyVerify_fd(fd)) {
            Py_BEGIN_ALLOW_THREADS
            err = close(fd);
            if (err < 0)
                save_errno = errno;
            Py_END_ALLOW_THREADS
        } else {
            save_errno = errno;
            err = -1;
        }
    }
    if (err < 0) {
        errno = save_errno;
        PyErr_SetFromErrno(PyExc_IOError);
        return -1;
    }
    return 0;
}

static PyObject *
fileio_close(fileio *self)
{
    if (!self->closefd) {
        self->fd = -1;
        Py_RETURN_NONE;
    }
    if (self->deallocating) {
        PyObject *r = fileio_dealloc_warn(self, (PyObject *) self);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    errno = internal_close(self);
    if (errno < 0)
        return NULL;

    return PyObject_CallMethod((PyObject*)&PyRawIOBase_Type,
                               "close", "O", self);
}

static PyObject *
fileio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    fileio *self;

    assert(type != NULL && type->tp_alloc != NULL);

    self = (fileio *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->fd = -1;
        self->readable = 0;
        self->writable = 0;
        self->seekable = -1;
        self->closefd = 1;
        self->weakreflist = NULL;
    }

    return (PyObject *) self;
}

/* On Unix, open will succeed for directories.
   In Python, there should be no file objects referring to
   directories, so we need a check.  */

static int
dircheck(fileio* self, const char *name)
{
#if defined(HAVE_FSTAT) && defined(S_IFDIR) && defined(EISDIR)
    struct stat buf;
    if (self->fd < 0)
        return 0;
    if (fstat(self->fd, &buf) == 0 && S_ISDIR(buf.st_mode)) {
        char *msg = strerror(EISDIR);
        PyObject *exc;
        if (internal_close(self))
            return -1;

        exc = PyObject_CallFunction(PyExc_IOError, "(iss)",
                                    EISDIR, msg, name);
        PyErr_SetObject(PyExc_IOError, exc);
        Py_XDECREF(exc);
        return -1;
    }
#endif
    return 0;
}

static int
check_fd(int fd)
{
#if defined(HAVE_FSTAT)
    struct stat buf;
    if (!_PyVerify_fd(fd) || (fstat(fd, &buf) < 0 && errno == EBADF)) {
        PyObject *exc;
        char *msg = strerror(EBADF);
        exc = PyObject_CallFunction(PyExc_OSError, "(is)",
                                    EBADF, msg);
        PyErr_SetObject(PyExc_OSError, exc);
        Py_XDECREF(exc);
        return -1;
    }
#endif
    return 0;
}


static int
fileio_init(PyObject *oself, PyObject *args, PyObject *kwds)
{
    fileio *self = (fileio *) oself;
    static char *kwlist[] = {"file", "mode", "closefd", NULL};
    const char *name = NULL;
    PyObject *nameobj, *stringobj = NULL;
    char *mode = "r";
    char *s;
#ifdef MS_WINDOWS
    Py_UNICODE *widename = NULL;
#endif
    int ret = 0;
    int rwa = 0, plus = 0, append = 0;
    int flags = 0;
    int fd = -1;
    int closefd = 1;
    int fd_is_own = 0;

    assert(PyFileIO_Check(oself));
    if (self->fd >= 0) {
        if (self->closefd) {
            /* Have to close the existing file first. */
            if (internal_close(self) < 0)
                return -1;
        }
        else
            self->fd = -1;
    }

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|si:fileio",
                                     kwlist, &nameobj, &mode, &closefd))
        return -1;

    if (PyFloat_Check(nameobj)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float");
        return -1;
    }

    fd = PyLong_AsLong(nameobj);
    if (fd < 0) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_ValueError,
                            "Negative filedescriptor");
            return -1;
        }
        PyErr_Clear();
    }

#ifdef MS_WINDOWS
    if (PyUnicode_Check(nameobj)) {
        int rv = _PyUnicode_HasNULChars(nameobj);
        if (rv) {
            if (rv != -1)
                PyErr_SetString(PyExc_TypeError, "embedded NUL character");
            return -1;
        }
        widename = PyUnicode_AS_UNICODE(nameobj);
    }
    if (widename == NULL)
#endif
    if (fd < 0)
    {
        if (!PyUnicode_FSConverter(nameobj, &stringobj)) {
            return -1;
        }
        name = PyBytes_AS_STRING(stringobj);
    }

    s = mode;
    while (*s) {
        switch (*s++) {
        case 'r':
            if (rwa) {
            bad_mode:
                PyErr_SetString(PyExc_ValueError,
                                "Must have exactly one of read/write/append "
                                "mode and at most one plus");
                goto error;
            }
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
            flags |= O_CREAT;
            append = 1;
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

#ifdef O_APPEND
    if (append)
        flags |= O_APPEND;
#endif

    if (fd >= 0) {
        if (check_fd(fd))
            goto error;
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

        Py_BEGIN_ALLOW_THREADS
        errno = 0;
#ifdef MS_WINDOWS
        if (widename != NULL)
            self->fd = _wopen(widename, flags, 0666);
        else
#endif
            self->fd = open(name, flags, 0666);
        Py_END_ALLOW_THREADS
        fd_is_own = 1;
        if (self->fd < 0) {
#ifdef MS_WINDOWS
            if (widename != NULL)
                PyErr_SetFromErrnoWithUnicodeFilename(PyExc_IOError, widename);
            else
#endif
                PyErr_SetFromErrnoWithFilename(PyExc_IOError, name);
            goto error;
        }
        if (dircheck(self, name) < 0)
            goto error;
    }

#if defined(MS_WINDOWS) || defined(__CYGWIN__)
    /* don't translate newlines (\r\n <=> \n) */
    _setmode(self->fd, O_BINARY);
#endif

    if (PyObject_SetAttrString((PyObject *)self, "name", nameobj) < 0)
        goto error;

    if (append) {
        /* For consistent behaviour, we explicitly seek to the
           end of file (otherwise, it might be done only on the
           first write()). */
        PyObject *pos = portable_lseek(self->fd, NULL, 2);
        if (pos == NULL)
            goto error;
        Py_DECREF(pos);
    }

    goto done;

 error:
    ret = -1;
    if (!fd_is_own)
        self->fd = -1;
    if (self->fd >= 0)
        internal_close(self);

 done:
    Py_CLEAR(stringobj);
    return ret;
}

static int
fileio_traverse(fileio *self, visitproc visit, void *arg)
{
    Py_VISIT(self->dict);
    return 0;
}

static int
fileio_clear(fileio *self)
{
    Py_CLEAR(self->dict);
    return 0;
}

static void
fileio_dealloc(fileio *self)
{
    self->deallocating = 1;
    if (_PyIOBase_finalize((PyObject *) self) < 0)
        return;
    _PyObject_GC_UNTRACK(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed file");
    return NULL;
}

static PyObject *
err_mode(char *action)
{
    PyErr_Format(IO_STATE->unsupported_operation,
                 "File not open for %s", action);
    return NULL;
}

static PyObject *
fileio_fileno(fileio *self)
{
    if (self->fd < 0)
        return err_closed();
    return PyLong_FromLong((long) self->fd);
}

static PyObject *
fileio_readable(fileio *self)
{
    if (self->fd < 0)
        return err_closed();
    return PyBool_FromLong((long) self->readable);
}

static PyObject *
fileio_writable(fileio *self)
{
    if (self->fd < 0)
        return err_closed();
    return PyBool_FromLong((long) self->writable);
}

static PyObject *
fileio_seekable(fileio *self)
{
    if (self->fd < 0)
        return err_closed();
    if (self->seekable < 0) {
        PyObject *pos = portable_lseek(self->fd, NULL, SEEK_CUR);
        if (pos == NULL) {
            PyErr_Clear();
            self->seekable = 0;
        } else {
            Py_DECREF(pos);
            self->seekable = 1;
        }
    }
    return PyBool_FromLong((long) self->seekable);
}

static PyObject *
fileio_readinto(fileio *self, PyObject *args)
{
    Py_buffer pbuf;
    Py_ssize_t n, len;
    int err;

    if (self->fd < 0)
        return err_closed();
    if (!self->readable)
        return err_mode("reading");

    if (!PyArg_ParseTuple(args, "w*", &pbuf))
        return NULL;

    if (_PyVerify_fd(self->fd)) {
        len = pbuf.len;
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
#if defined(MS_WIN64) || defined(MS_WINDOWS)
        if (len > INT_MAX)
            len = INT_MAX;
        n = read(self->fd, pbuf.buf, (int)len);
#else
        n = read(self->fd, pbuf.buf, len);
#endif
        Py_END_ALLOW_THREADS
    } else
        n = -1;
    err = errno;
    PyBuffer_Release(&pbuf);
    if (n < 0) {
        if (err == EAGAIN)
            Py_RETURN_NONE;
        errno = err;
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    return PyLong_FromSsize_t(n);
}

static size_t
new_buffersize(fileio *self, size_t currentsize)
{
#ifdef HAVE_FSTAT
    off_t pos, end;
    struct stat st;
    if (fstat(self->fd, &st) == 0) {
        end = st.st_size;
        pos = lseek(self->fd, 0L, SEEK_CUR);
        /* Files claiming a size smaller than SMALLCHUNK may
           actually be streaming pseudo-files. In this case, we
           apply the more aggressive algorithm below.
        */
        if (end >= SMALLCHUNK && end >= pos && pos >= 0) {
            /* Add 1 so if the file were to grow we'd notice. */
            return currentsize + end - pos + 1;
        }
    }
#endif
    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior. Use a less-than-double
       growth factor to avoid excessive allocation. */
    return currentsize + (currentsize >> 3) + 6;
}

static PyObject *
fileio_readall(fileio *self)
{
    PyObject *result;
    Py_ssize_t total = 0;
    int n;

    if (self->fd < 0)
        return err_closed();
    if (!_PyVerify_fd(self->fd))
        return PyErr_SetFromErrno(PyExc_IOError);

    result = PyBytes_FromStringAndSize(NULL, SMALLCHUNK);
    if (result == NULL)
        return NULL;

    while (1) {
        size_t newsize = new_buffersize(self, total);
        if (newsize > PY_SSIZE_T_MAX || newsize <= 0) {
            PyErr_SetString(PyExc_OverflowError,
                "unbounded read returned more bytes "
                "than a Python string can hold ");
            Py_DECREF(result);
            return NULL;
        }

        if (PyBytes_GET_SIZE(result) < (Py_ssize_t)newsize) {
            if (_PyBytes_Resize(&result, newsize) < 0) {
                if (total == 0) {
                    Py_DECREF(result);
                    return NULL;
                }
                PyErr_Clear();
                break;
            }
        }
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        n = read(self->fd,
                 PyBytes_AS_STRING(result) + total,
                 newsize - total);
        Py_END_ALLOW_THREADS
        if (n == 0)
            break;
        if (n < 0) {
            if (total > 0)
                break;
            if (errno == EAGAIN) {
                Py_DECREF(result);
                Py_RETURN_NONE;
            }
            Py_DECREF(result);
            PyErr_SetFromErrno(PyExc_IOError);
            return NULL;
        }
        total += n;
    }

    if (PyBytes_GET_SIZE(result) > total) {
        if (_PyBytes_Resize(&result, total) < 0) {
            /* This should never happen, but just in case */
            Py_DECREF(result);
            return NULL;
        }
    }
    return result;
}

static PyObject *
fileio_read(fileio *self, PyObject *args)
{
    char *ptr;
    Py_ssize_t n;
    Py_ssize_t size = -1;
    PyObject *bytes;

    if (self->fd < 0)
        return err_closed();
    if (!self->readable)
        return err_mode("reading");

    if (!PyArg_ParseTuple(args, "|O&", &_PyIO_ConvertSsize_t, &size))
        return NULL;

    if (size < 0) {
        return fileio_readall(self);
    }

#if defined(MS_WIN64) || defined(MS_WINDOWS)
    if (size > INT_MAX)
        size = INT_MAX;
#endif
    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return NULL;
    ptr = PyBytes_AS_STRING(bytes);

    if (_PyVerify_fd(self->fd)) {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
#if defined(MS_WIN64) || defined(MS_WINDOWS)
        n = read(self->fd, ptr, (int)size);
#else
        n = read(self->fd, ptr, size);
#endif
        Py_END_ALLOW_THREADS
    } else
        n = -1;

    if (n < 0) {
        int err = errno;
        Py_DECREF(bytes);
        if (err == EAGAIN)
            Py_RETURN_NONE;
        errno = err;
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    if (n != size) {
        if (_PyBytes_Resize(&bytes, n) < 0) {
            Py_DECREF(bytes);
            return NULL;
        }
    }

    return (PyObject *) bytes;
}

static PyObject *
fileio_write(fileio *self, PyObject *args)
{
    Py_buffer pbuf;
    Py_ssize_t n, len;
    int err;

    if (self->fd < 0)
        return err_closed();
    if (!self->writable)
        return err_mode("writing");

    if (!PyArg_ParseTuple(args, "y*", &pbuf))
        return NULL;

    if (_PyVerify_fd(self->fd)) {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        len = pbuf.len;
#if defined(MS_WIN64) || defined(MS_WINDOWS)
        if (len > 32767 && isatty(self->fd)) {
            /* Issue #11395: the Windows console returns an error (12: not
               enough space error) on writing into stdout if stdout mode is
               binary and the length is greater than 66,000 bytes (or less,
               depending on heap usage). */
            len = 32767;
        }
        else if (len > INT_MAX)
            len = INT_MAX;
        n = write(self->fd, pbuf.buf, (int)len);
#else
        n = write(self->fd, pbuf.buf, len);
#endif
        Py_END_ALLOW_THREADS
    } else
        n = -1;
    err = errno;

    PyBuffer_Release(&pbuf);

    if (n < 0) {
        if (err == EAGAIN)
            Py_RETURN_NONE;
        errno = err;
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    return PyLong_FromSsize_t(n);
}

/* XXX Windows support below is likely incomplete */

/* Cribbed from posix_lseek() */
static PyObject *
portable_lseek(int fd, PyObject *posobj, int whence)
{
    Py_off_t pos, res;

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

    if (posobj == NULL)
        pos = 0;
    else {
        if(PyFloat_Check(posobj)) {
            PyErr_SetString(PyExc_TypeError, "an integer is required");
            return NULL;
        }
#if defined(HAVE_LARGEFILE_SUPPORT)
        pos = PyLong_AsLongLong(posobj);
#else
        pos = PyLong_AsLong(posobj);
#endif
        if (PyErr_Occurred())
            return NULL;
    }

    if (_PyVerify_fd(fd)) {
        Py_BEGIN_ALLOW_THREADS
#if defined(MS_WIN64) || defined(MS_WINDOWS)
        res = _lseeki64(fd, pos, whence);
#else
        res = lseek(fd, pos, whence);
#endif
        Py_END_ALLOW_THREADS
    } else
        res = -1;
    if (res < 0)
        return PyErr_SetFromErrno(PyExc_IOError);

#if defined(HAVE_LARGEFILE_SUPPORT)
    return PyLong_FromLongLong(res);
#else
    return PyLong_FromLong(res);
#endif
}

static PyObject *
fileio_seek(fileio *self, PyObject *args)
{
    PyObject *posobj;
    int whence = 0;

    if (self->fd < 0)
        return err_closed();

    if (!PyArg_ParseTuple(args, "O|i", &posobj, &whence))
        return NULL;

    return portable_lseek(self->fd, posobj, whence);
}

static PyObject *
fileio_tell(fileio *self, PyObject *args)
{
    if (self->fd < 0)
        return err_closed();

    return portable_lseek(self->fd, NULL, 1);
}

#ifdef HAVE_FTRUNCATE
static PyObject *
fileio_truncate(fileio *self, PyObject *args)
{
    PyObject *posobj = NULL; /* the new size wanted by the user */
#ifndef MS_WINDOWS
    Py_off_t pos;
#endif
    int ret;
    int fd;

    fd = self->fd;
    if (fd < 0)
        return err_closed();
    if (!self->writable)
        return err_mode("writing");

    if (!PyArg_ParseTuple(args, "|O", &posobj))
        return NULL;

    if (posobj == Py_None || posobj == NULL) {
        /* Get the current position. */
        posobj = portable_lseek(fd, NULL, 1);
        if (posobj == NULL)
            return NULL;
    }
    else {
        Py_INCREF(posobj);
    }

#ifdef MS_WINDOWS
    /* MS _chsize doesn't work if newsize doesn't fit in 32 bits,
       so don't even try using it. */
    {
        PyObject *oldposobj, *tempposobj;
        HANDLE hFile;

        /* we save the file pointer position */
        oldposobj = portable_lseek(fd, NULL, 1);
        if (oldposobj == NULL) {
            Py_DECREF(posobj);
            return NULL;
        }

        /* we then move to the truncation position */
        tempposobj = portable_lseek(fd, posobj, 0);
        if (tempposobj == NULL) {
            Py_DECREF(oldposobj);
            Py_DECREF(posobj);
            return NULL;
        }
        Py_DECREF(tempposobj);

        /* Truncate.  Note that this may grow the file! */
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        hFile = (HANDLE)_get_osfhandle(fd);
        ret = hFile == (HANDLE)-1; /* testing for INVALID_HANDLE value */
        if (ret == 0) {
            ret = SetEndOfFile(hFile) == 0;
            if (ret)
                errno = EACCES;
        }
        Py_END_ALLOW_THREADS

        /* we restore the file pointer position in any case */
        tempposobj = portable_lseek(fd, oldposobj, 0);
        Py_DECREF(oldposobj);
        if (tempposobj == NULL) {
            Py_DECREF(posobj);
            return NULL;
        }
        Py_DECREF(tempposobj);
    }
#else

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
    errno = 0;
    ret = ftruncate(fd, pos);
    Py_END_ALLOW_THREADS

#endif /* !MS_WINDOWS */

    if (ret != 0) {
        Py_DECREF(posobj);
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }

    return posobj;
}
#endif /* HAVE_FTRUNCATE */

static char *
mode_string(fileio *self)
{
    if (self->readable) {
        if (self->writable)
            return "rb+";
        else
            return "rb";
    }
    else
        return "wb";
}

static PyObject *
fileio_repr(fileio *self)
{
    PyObject *nameobj, *res;

    if (self->fd < 0)
        return PyUnicode_FromFormat("<_io.FileIO [closed]>");

    nameobj = PyObject_GetAttrString((PyObject *) self, "name");
    if (nameobj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_Clear();
        else
            return NULL;
        res = PyUnicode_FromFormat("<_io.FileIO fd=%d mode='%s'>",
                                   self->fd, mode_string(self));
    }
    else {
        res = PyUnicode_FromFormat("<_io.FileIO name=%R mode='%s'>",
                                   nameobj, mode_string(self));
        Py_DECREF(nameobj);
    }
    return res;
}

static PyObject *
fileio_isatty(fileio *self)
{
    long res;

    if (self->fd < 0)
        return err_closed();
    Py_BEGIN_ALLOW_THREADS
    res = isatty(self->fd);
    Py_END_ALLOW_THREADS
    return PyBool_FromLong(res);
}

static PyObject *
fileio_getstate(fileio *self)
{
    PyErr_Format(PyExc_TypeError,
                 "cannot serialize '%s' object", Py_TYPE(self)->tp_name);
    return NULL;
}


PyDoc_STRVAR(fileio_doc,
"file(name: str[, mode: str]) -> file IO object\n"
"\n"
"Open a file.  The mode can be 'r', 'w' or 'a' for reading (default),\n"
"writing or appending.  The file will be created if it doesn't exist\n"
"when opened for writing or appending; it will be truncated when\n"
"opened for writing.  Add a '+' to the mode to allow simultaneous\n"
"reading and writing.");

PyDoc_STRVAR(read_doc,
"read(size: int) -> bytes.  read at most size bytes, returned as bytes.\n"
"\n"
"Only makes one system call, so less data may be returned than requested\n"
"In non-blocking mode, returns None if no data is available.\n"
"On end-of-file, returns ''.");

PyDoc_STRVAR(readall_doc,
"readall() -> bytes.  read all data from the file, returned as bytes.\n"
"\n"
"In non-blocking mode, returns as much as is immediately available,\n"
"or None if no data is available.  On end-of-file, returns ''.");

PyDoc_STRVAR(write_doc,
"write(b: bytes) -> int.  Write bytes b to file, return number written.\n"
"\n"
"Only makes one system call, so not all of the data may be written.\n"
"The number of bytes actually written is returned.");

PyDoc_STRVAR(fileno_doc,
"fileno() -> int. \"file descriptor\".\n"
"\n"
"This is needed for lower-level file interfaces, such the fcntl module.");

PyDoc_STRVAR(seek_doc,
"seek(offset: int[, whence: int]) -> None.  Move to new file position.\n"
"\n"
"Argument offset is a byte count.  Optional argument whence defaults to\n"
"0 (offset from start of file, offset should be >= 0); other values are 1\n"
"(move relative to current position, positive or negative), and 2 (move\n"
"relative to end of file, usually negative, although many platforms allow\n"
"seeking beyond the end of a file)."
"\n"
"Note that not all file objects are seekable.");

#ifdef HAVE_FTRUNCATE
PyDoc_STRVAR(truncate_doc,
"truncate([size: int]) -> None.  Truncate the file to at most size bytes.\n"
"\n"
"Size defaults to the current file position, as returned by tell()."
"The current file position is changed to the value of size.");
#endif

PyDoc_STRVAR(tell_doc,
"tell() -> int.  Current file position");

PyDoc_STRVAR(readinto_doc,
"readinto() -> Same as RawIOBase.readinto().");

PyDoc_STRVAR(close_doc,
"close() -> None.  Close the file.\n"
"\n"
"A closed file cannot be used for further I/O operations.  close() may be\n"
"called more than once without error.  Changes the fileno to -1.");

PyDoc_STRVAR(isatty_doc,
"isatty() -> bool.  True if the file is connected to a tty device.");

PyDoc_STRVAR(seekable_doc,
"seekable() -> bool.  True if file supports random-access.");

PyDoc_STRVAR(readable_doc,
"readable() -> bool.  True if file was opened in a read mode.");

PyDoc_STRVAR(writable_doc,
"writable() -> bool.  True if file was opened in a write mode.");

static PyMethodDef fileio_methods[] = {
    {"read",     (PyCFunction)fileio_read,         METH_VARARGS, read_doc},
    {"readall",  (PyCFunction)fileio_readall,  METH_NOARGS,  readall_doc},
    {"readinto", (PyCFunction)fileio_readinto, METH_VARARGS, readinto_doc},
    {"write",    (PyCFunction)fileio_write,        METH_VARARGS, write_doc},
    {"seek",     (PyCFunction)fileio_seek,         METH_VARARGS, seek_doc},
    {"tell",     (PyCFunction)fileio_tell,         METH_VARARGS, tell_doc},
#ifdef HAVE_FTRUNCATE
    {"truncate", (PyCFunction)fileio_truncate, METH_VARARGS, truncate_doc},
#endif
    {"close",    (PyCFunction)fileio_close,        METH_NOARGS,  close_doc},
    {"seekable", (PyCFunction)fileio_seekable, METH_NOARGS,      seekable_doc},
    {"readable", (PyCFunction)fileio_readable, METH_NOARGS,      readable_doc},
    {"writable", (PyCFunction)fileio_writable, METH_NOARGS,      writable_doc},
    {"fileno",   (PyCFunction)fileio_fileno,   METH_NOARGS,      fileno_doc},
    {"isatty",   (PyCFunction)fileio_isatty,   METH_NOARGS,      isatty_doc},
    {"_dealloc_warn", (PyCFunction)fileio_dealloc_warn, METH_O, NULL},
    {"__getstate__", (PyCFunction)fileio_getstate, METH_NOARGS, NULL},
    {NULL,           NULL}             /* sentinel */
};

/* 'closed' and 'mode' are attributes for backwards compatibility reasons. */

static PyObject *
get_closed(fileio *self, void *closure)
{
    return PyBool_FromLong((long)(self->fd < 0));
}

static PyObject *
get_closefd(fileio *self, void *closure)
{
    return PyBool_FromLong((long)(self->closefd));
}

static PyObject *
get_mode(fileio *self, void *closure)
{
    return PyUnicode_FromString(mode_string(self));
}

static PyGetSetDef fileio_getsetlist[] = {
    {"closed", (getter)get_closed, NULL, "True if the file is closed"},
    {"closefd", (getter)get_closefd, NULL,
        "True if the file descriptor will be closed"},
    {"mode", (getter)get_mode, NULL, "String giving the file mode"},
    {NULL},
};

PyTypeObject PyFileIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.FileIO",
    sizeof(fileio),
    0,
    (destructor)fileio_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)fileio_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
                    | Py_TPFLAGS_HAVE_GC,       /* tp_flags */
    fileio_doc,                                 /* tp_doc */
    (traverseproc)fileio_traverse,              /* tp_traverse */
    (inquiry)fileio_clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(fileio, weakreflist),      /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    fileio_methods,                             /* tp_methods */
    0,                                          /* tp_members */
    fileio_getsetlist,                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(fileio, dict),         /* tp_dictoffset */
    fileio_init,                                /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    fileio_new,                                 /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
