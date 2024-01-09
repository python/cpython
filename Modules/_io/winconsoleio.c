/*
    An implementation of Windows console I/O

    Classes defined here: _WindowsConsoleIO

    Written by Steve Dower
*/

#include "Python.h"
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
#include "pycore_object.h"        // _PyObject_GC_UNTRACK()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()

#ifdef HAVE_WINDOWS_CONSOLE_IO


#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stddef.h> /* For offsetof */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <fcntl.h>

#include "_iomodule.h"

/* BUFSIZ determines how many characters can be typed at the console
   before it starts blocking. */
#if BUFSIZ < (16*1024)
#define SMALLCHUNK (2*1024)
#elif (BUFSIZ >= (2 << 25))
#error "unreasonable BUFSIZ > 64 MiB defined"
#else
#define SMALLCHUNK BUFSIZ
#endif

/* BUFMAX determines how many bytes can be read in one go. */
#define BUFMAX (32*1024*1024)

/* SMALLBUF determines how many utf-8 characters will be
   buffered within the stream, in order to support reads
   of less than one character */
#define SMALLBUF 4

char _get_console_type(HANDLE handle) {
    DWORD mode, peek_count;

    if (handle == INVALID_HANDLE_VALUE)
        return '\0';

    if (!GetConsoleMode(handle, &mode))
        return '\0';

    /* Peek at the handle to see whether it is an input or output handle */
    if (GetNumberOfConsoleInputEvents(handle, &peek_count))
        return 'r';
    return 'w';
}

char _PyIO_get_console_type(PyObject *path_or_fd) {
    int fd = PyLong_AsLong(path_or_fd);
    PyErr_Clear();
    if (fd >= 0) {
        HANDLE handle = _Py_get_osfhandle_noraise(fd);
        if (handle == INVALID_HANDLE_VALUE)
            return '\0';
        return _get_console_type(handle);
    }

    PyObject *decoded;
    wchar_t *decoded_wstr;

    if (!PyUnicode_FSDecoder(path_or_fd, &decoded)) {
        PyErr_Clear();
        return '\0';
    }
    decoded_wstr = PyUnicode_AsWideCharString(decoded, NULL);
    Py_CLEAR(decoded);
    if (!decoded_wstr) {
        PyErr_Clear();
        return '\0';
    }

    char m = '\0';
    if (!_wcsicmp(decoded_wstr, L"CONIN$")) {
        m = 'r';
    } else if (!_wcsicmp(decoded_wstr, L"CONOUT$")) {
        m = 'w';
    } else if (!_wcsicmp(decoded_wstr, L"CON")) {
        m = 'x';
    }
    if (m) {
        PyMem_Free(decoded_wstr);
        return m;
    }

    DWORD length;
    wchar_t name_buf[MAX_PATH], *pname_buf = name_buf;

    length = GetFullPathNameW(decoded_wstr, MAX_PATH, pname_buf, NULL);
    if (length > MAX_PATH) {
        pname_buf = PyMem_New(wchar_t, length);
        if (pname_buf)
            length = GetFullPathNameW(decoded_wstr, length, pname_buf, NULL);
        else
            length = 0;
    }
    PyMem_Free(decoded_wstr);

    if (length) {
        wchar_t *name = pname_buf;
        if (length >= 4 && name[3] == L'\\' &&
            (name[2] == L'.' || name[2] == L'?') &&
            name[1] == L'\\' && name[0] == L'\\') {
            name += 4;
        }
        if (!_wcsicmp(name, L"CONIN$")) {
            m = 'r';
        } else if (!_wcsicmp(name, L"CONOUT$")) {
            m = 'w';
        } else if (!_wcsicmp(name, L"CON")) {
            m = 'x';
        }
    }

    if (pname_buf != name_buf)
        PyMem_Free(pname_buf);
    return m;
}

static DWORD
_find_last_utf8_boundary(const char *buf, DWORD len)
{
    /* This function never returns 0, returns the original len instead */
    DWORD count = 1;
    if (len == 0 || (buf[len - 1] & 0x80) == 0) {
        return len;
    }
    for (;; count++) {
        if (count > 3 || count >= len) {
            return len;
        }
        if ((buf[len - count] & 0xc0) != 0x80) {
            return len - count;
        }
    }
}

/*[clinic input]
module _io
class _io._WindowsConsoleIO "winconsoleio *" "clinic_state()->PyWindowsConsoleIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=05526e723011ab36]*/

typedef struct {
    PyObject_HEAD
    int fd;
    unsigned int created : 1;
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int closefd : 1;
    char finalizing;
    unsigned int blksize;
    PyObject *weakreflist;
    PyObject *dict;
    char buf[SMALLBUF];
    wchar_t wbuf;
} winconsoleio;

int
_PyWindowsConsoleIO_closed(PyObject *self)
{
    return ((winconsoleio *)self)->fd == -1;
}


/* Returns 0 on success, -1 with exception set on failure. */
static int
internal_close(winconsoleio *self)
{
    if (self->fd != -1) {
        if (self->closefd) {
            _Py_BEGIN_SUPPRESS_IPH
            close(self->fd);
            _Py_END_SUPPRESS_IPH
        }
        self->fd = -1;
    }
    return 0;
}

/*[clinic input]
_io._WindowsConsoleIO.close
    cls: defining_class
    /

Close the console object.

A closed console object cannot be used for further I/O operations.
close() may be called more than once without error.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_close_impl(winconsoleio *self, PyTypeObject *cls)
/*[clinic end generated code: output=e50c1808c063e1e2 input=161001bd2a649a4b]*/
{
    PyObject *res;
    PyObject *exc;
    int rc;

    _PyIO_State *state = get_io_state_by_cls(cls);
    res = PyObject_CallMethodOneArg((PyObject*)state->PyRawIOBase_Type,
                                    &_Py_ID(close), (PyObject*)self);
    if (!self->closefd) {
        self->fd = -1;
        return res;
    }
    if (res == NULL) {
        exc = PyErr_GetRaisedException();
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
winconsoleio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    winconsoleio *self;

    assert(type != NULL && type->tp_alloc != NULL);

    self = (winconsoleio *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->fd = -1;
        self->created = 0;
        self->readable = 0;
        self->writable = 0;
        self->closefd = 0;
        self->blksize = 0;
        self->weakreflist = NULL;
    }

    return (PyObject *) self;
}

/*[clinic input]
_io._WindowsConsoleIO.__init__
    file as nameobj: object
    mode: str = "r"
    closefd: bool = True
    opener: object = None

Open a console buffer by file descriptor.

The mode can be 'rb' (default), or 'wb' for reading or writing bytes. All
other mode characters will be ignored. Mode 'b' will be assumed if it is
omitted. The *opener* parameter is always ignored.
[clinic start generated code]*/

static int
_io__WindowsConsoleIO___init___impl(winconsoleio *self, PyObject *nameobj,
                                    const char *mode, int closefd,
                                    PyObject *opener)
/*[clinic end generated code: output=3fd9cbcdd8d95429 input=7a3eed6bbe998fd9]*/
{
    const char *s;
    wchar_t *name = NULL;
    char console_type = '\0';
    int ret = 0;
    int rwa = 0;
    int fd = -1;
    int fd_is_own = 0;
    HANDLE handle = NULL;

#ifndef NDEBUG
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    assert(PyObject_TypeCheck(self, state->PyWindowsConsoleIO_Type));
#endif
    if (self->fd >= 0) {
        if (self->closefd) {
            /* Have to close the existing file first. */
            if (internal_close(self) < 0)
                return -1;
        }
        else
            self->fd = -1;
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
    self->fd = fd;

    if (fd < 0) {
        PyObject *decodedname;

        int d = PyUnicode_FSDecoder(nameobj, (void*)&decodedname);
        if (!d)
            return -1;

        name = PyUnicode_AsWideCharString(decodedname, NULL);
        console_type = _PyIO_get_console_type(decodedname);
        Py_CLEAR(decodedname);
        if (name == NULL)
            return -1;
    }

    s = mode;
    while (*s) {
        switch (*s++) {
        case '+':
        case 'a':
        case 'b':
        case 'x':
            break;
        case 'r':
            if (rwa)
                goto bad_mode;
            rwa = 1;
            self->readable = 1;
            if (console_type == 'x')
                console_type = 'r';
            break;
        case 'w':
            if (rwa)
                goto bad_mode;
            rwa = 1;
            self->writable = 1;
            if (console_type == 'x')
                console_type = 'w';
            break;
        default:
            PyErr_Format(PyExc_ValueError,
                         "invalid mode: %.200s", mode);
            goto error;
        }
    }

    if (!rwa)
        goto bad_mode;

    if (fd >= 0) {
        handle = _Py_get_osfhandle_noraise(fd);
        self->closefd = 0;
    } else {
        DWORD access = GENERIC_READ;

        self->closefd = 1;
        if (!closefd) {
            PyErr_SetString(PyExc_ValueError,
                "Cannot use closefd=False with file name");
            goto error;
        }

        if (self->writable)
            access = GENERIC_WRITE;

        Py_BEGIN_ALLOW_THREADS
        /* Attempt to open for read/write initially, then fall back
           on the specific access. This is required for modern names
           CONIN$ and CONOUT$, which allow reading/writing state as
           well as reading/writing content. */
        handle = CreateFileW(name, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE)
            handle = CreateFileW(name, access,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        Py_END_ALLOW_THREADS

        if (handle == INVALID_HANDLE_VALUE) {
            PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError, GetLastError(), nameobj);
            goto error;
        }

        if (self->writable)
            self->fd = _Py_open_osfhandle_noraise(handle, _O_WRONLY | _O_BINARY);
        else
            self->fd = _Py_open_osfhandle_noraise(handle, _O_RDONLY | _O_BINARY);
        if (self->fd < 0) {
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, nameobj);
            CloseHandle(handle);
            goto error;
        }
    }

    if (console_type == '\0')
        console_type = _get_console_type(handle);

    if (self->writable && console_type != 'w') {
        PyErr_SetString(PyExc_ValueError,
            "Cannot open console input buffer for writing");
        goto error;
    }
    if (self->readable && console_type != 'r') {
        PyErr_SetString(PyExc_ValueError,
            "Cannot open console output buffer for reading");
        goto error;
    }

    self->blksize = DEFAULT_BUFFER_SIZE;
    memset(self->buf, 0, 4);

    if (PyObject_SetAttr((PyObject *)self, &_Py_ID(name), nameobj) < 0)
        goto error;

    goto done;

bad_mode:
    PyErr_SetString(PyExc_ValueError,
                    "Must have exactly one of read or write mode");
error:
    ret = -1;
    internal_close(self);

done:
    if (name)
        PyMem_Free(name);
    return ret;
}

static int
winconsoleio_traverse(winconsoleio *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->dict);
    return 0;
}

static int
winconsoleio_clear(winconsoleio *self)
{
    Py_CLEAR(self->dict);
    return 0;
}

static void
winconsoleio_dealloc(winconsoleio *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    self->finalizing = 1;
    if (_PyIOBase_finalize((PyObject *) self) < 0)
        return;
    _PyObject_GC_UNTRACK(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_CLEAR(self->dict);
    tp->tp_free((PyObject *)self);
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
                        "Console buffer does not support %s", action);
}

/*[clinic input]
_io._WindowsConsoleIO.fileno

Return the underlying file descriptor (an integer).

[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_fileno_impl(winconsoleio *self)
/*[clinic end generated code: output=006fa74ce3b5cfbf input=845c47ebbc3a2f67]*/
{
    if (self->fd < 0)
        return err_closed();
    return PyLong_FromLong(self->fd);
}

/*[clinic input]
_io._WindowsConsoleIO.readable

True if console is an input buffer.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_readable_impl(winconsoleio *self)
/*[clinic end generated code: output=daf9cef2743becf0 input=6be9defb5302daae]*/
{
    if (self->fd == -1)
        return err_closed();
    return PyBool_FromLong((long) self->readable);
}

/*[clinic input]
_io._WindowsConsoleIO.writable

True if console is an output buffer.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_writable_impl(winconsoleio *self)
/*[clinic end generated code: output=e0a2ad7eae5abf67 input=cefbd8abc24df6a0]*/
{
    if (self->fd == -1)
        return err_closed();
    return PyBool_FromLong((long) self->writable);
}

static DWORD
_buflen(winconsoleio *self)
{
    for (DWORD i = 0; i < SMALLBUF; ++i) {
        if (!self->buf[i])
            return i;
    }
    return SMALLBUF;
}

static DWORD
_copyfrombuf(winconsoleio *self, char *buf, DWORD len)
{
    DWORD n = 0;

    while (self->buf[0] && len--) {
        buf[n++] = self->buf[0];
        for (int i = 1; i < SMALLBUF; ++i)
            self->buf[i - 1] = self->buf[i];
        self->buf[SMALLBUF - 1] = 0;
    }

    return n;
}

static wchar_t *
read_console_w(HANDLE handle, DWORD maxlen, DWORD *readlen) {
    int err = 0, sig = 0;

    wchar_t *buf = (wchar_t*)PyMem_Malloc(maxlen * sizeof(wchar_t));
    if (!buf)
        goto error;

    *readlen = 0;

    //DebugBreak();
    Py_BEGIN_ALLOW_THREADS
    DWORD off = 0;
    while (off < maxlen) {
        DWORD n = (DWORD)-1;
        DWORD len = min(maxlen - off, BUFSIZ);
        SetLastError(0);
        BOOL res = ReadConsoleW(handle, &buf[off], len, &n, NULL);

        if (!res) {
            err = GetLastError();
            break;
        }
        if (n == (DWORD)-1 && (err = GetLastError()) == ERROR_OPERATION_ABORTED) {
            break;
        }
        if (n == 0) {
            err = GetLastError();
            if (err != ERROR_OPERATION_ABORTED)
                break;
            err = 0;
            HANDLE hInterruptEvent = _PyOS_SigintEvent();
            if (WaitForSingleObjectEx(hInterruptEvent, 100, FALSE)
                    == WAIT_OBJECT_0) {
                ResetEvent(hInterruptEvent);
                Py_BLOCK_THREADS
                sig = PyErr_CheckSignals();
                Py_UNBLOCK_THREADS
                if (sig < 0)
                    break;
            }
        }
        *readlen += n;

        /* If we didn't read a full buffer that time, don't try
           again or we will block a second time. */
        if (n < len)
            break;
        /* If the buffer ended with a newline, break out */
        if (buf[*readlen - 1] == '\n')
            break;
        /* If the buffer ends with a high surrogate, expand the
           buffer and read an extra character. */
        WORD char_type;
        if (off + BUFSIZ >= maxlen &&
            GetStringTypeW(CT_CTYPE3, &buf[*readlen - 1], 1, &char_type) &&
            char_type == C3_HIGHSURROGATE) {
            wchar_t *newbuf;
            maxlen += 1;
            Py_BLOCK_THREADS
            newbuf = (wchar_t*)PyMem_Realloc(buf, maxlen * sizeof(wchar_t));
            Py_UNBLOCK_THREADS
            if (!newbuf) {
                sig = -1;
                break;
            }
            buf = newbuf;
            /* Only advance by n and not BUFSIZ in this case */
            off += n;
            continue;
        }

        off += BUFSIZ;
    }

    Py_END_ALLOW_THREADS

    if (sig)
        goto error;
    if (err) {
        PyErr_SetFromWindowsErr(err);
        goto error;
    }

    if (*readlen > 0 && buf[0] == L'\x1a') {
        PyMem_Free(buf);
        buf = (wchar_t *)PyMem_Malloc(sizeof(wchar_t));
        if (!buf)
            goto error;
        buf[0] = L'\0';
        *readlen = 0;
    }

    return buf;

error:
    if (buf)
        PyMem_Free(buf);
    return NULL;
}


static Py_ssize_t
readinto(_PyIO_State *state, winconsoleio *self, char *buf, Py_ssize_t len)
{
    if (self->fd == -1) {
        err_closed();
        return -1;
    }
    if (!self->readable) {
        err_mode(state, "reading");
        return -1;
    }
    if (len == 0)
        return 0;
    if (len > BUFMAX) {
        PyErr_Format(PyExc_ValueError, "cannot read more than %d bytes", BUFMAX);
        return -1;
    }

    HANDLE handle = _Py_get_osfhandle(self->fd);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;

    /* Each character may take up to 4 bytes in the final buffer.
       This is highly conservative, but necessary to avoid
       failure for any given Unicode input (e.g. \U0010ffff).
       If the caller requests fewer than 4 bytes, we buffer one
       character.
    */
    DWORD wlen = (DWORD)(len / 4);
    if (wlen == 0) {
        wlen = 1;
    }

    DWORD read_len = _copyfrombuf(self, buf, (DWORD)len);
    if (read_len) {
        buf = &buf[read_len];
        len -= read_len;
        wlen -= 1;
    }
    if (len == read_len || wlen == 0)
        return read_len;

    DWORD n;
    wchar_t *wbuf = read_console_w(handle, wlen, &n);
    if (wbuf == NULL)
        return -1;
    if (n == 0) {
        PyMem_Free(wbuf);
        return read_len;
    }

    int err = 0;
    DWORD u8n = 0;

    Py_BEGIN_ALLOW_THREADS
    if (len < 4) {
        if (WideCharToMultiByte(CP_UTF8, 0, wbuf, n,
                self->buf, sizeof(self->buf) / sizeof(self->buf[0]),
                NULL, NULL))
            u8n = _copyfrombuf(self, buf, (DWORD)len);
    } else {
        u8n = WideCharToMultiByte(CP_UTF8, 0, wbuf, n,
            buf, (DWORD)len, NULL, NULL);
    }

    if (u8n) {
        read_len += u8n;
        u8n = 0;
    } else {
        err = GetLastError();
        if (err == ERROR_INSUFFICIENT_BUFFER) {
            /* Calculate the needed buffer for a more useful error, as this
                means our "/ 4" logic above is insufficient for some input.
            */
            u8n = WideCharToMultiByte(CP_UTF8, 0, wbuf, n,
                NULL, 0, NULL, NULL);
        }
    }
    Py_END_ALLOW_THREADS

    PyMem_Free(wbuf);

    if (u8n) {
        PyErr_Format(PyExc_SystemError,
            "Buffer had room for %zd bytes but %u bytes required",
            len, u8n);
        return -1;
    }
    if (err) {
        PyErr_SetFromWindowsErr(err);
        return -1;
    }

    return read_len;
}

/*[clinic input]
_io._WindowsConsoleIO.readinto
    cls: defining_class
    buffer: Py_buffer(accept={rwbuffer})
    /

Same as RawIOBase.readinto().
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_readinto_impl(winconsoleio *self, PyTypeObject *cls,
                                    Py_buffer *buffer)
/*[clinic end generated code: output=96717c74f6204b79 input=4b0627c3b1645f78]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    Py_ssize_t len = readinto(state, self, buffer->buf, buffer->len);
    if (len < 0)
        return NULL;

    return PyLong_FromSsize_t(len);
}

static DWORD
new_buffersize(winconsoleio *self, DWORD currentsize)
{
    DWORD addend;

    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior.  For bigger sizes, use a
       less-than-double growth factor to avoid excessive allocation. */
    if (currentsize > 65536)
        addend = currentsize >> 3;
    else
        addend = 256 + currentsize;
    if (addend < SMALLCHUNK)
        /* Avoid tiny read() calls. */
        addend = SMALLCHUNK;
    return addend + currentsize;
}

/*[clinic input]
_io._WindowsConsoleIO.readall

Read all data from the console, returned as bytes.

Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_readall_impl(winconsoleio *self)
/*[clinic end generated code: output=e6d312c684f6e23b input=4024d649a1006e69]*/
{
    wchar_t *buf;
    DWORD bufsize, n, len = 0;
    PyObject *bytes;
    DWORD bytes_size, rn;
    HANDLE handle;

    if (self->fd == -1)
        return err_closed();

    handle = _Py_get_osfhandle(self->fd);
    if (handle == INVALID_HANDLE_VALUE)
        return NULL;

    bufsize = BUFSIZ;

    buf = (wchar_t*)PyMem_Malloc((bufsize + 1) * sizeof(wchar_t));
    if (buf == NULL)
        return NULL;

    while (1) {
        wchar_t *subbuf;

        if (len >= (Py_ssize_t)bufsize) {
            DWORD newsize = new_buffersize(self, len);
            if (newsize > BUFMAX)
                break;
            if (newsize < bufsize) {
                PyErr_SetString(PyExc_OverflowError,
                                "unbounded read returned more bytes "
                                "than a Python bytes object can hold");
                PyMem_Free(buf);
                return NULL;
            }
            bufsize = newsize;

            wchar_t *tmp = PyMem_Realloc(buf,
                                         (bufsize + 1) * sizeof(wchar_t));
            if (tmp == NULL) {
                PyMem_Free(buf);
                return NULL;
            }
            buf = tmp;
        }

        subbuf = read_console_w(handle, bufsize - len, &n);

        if (subbuf == NULL) {
            PyMem_Free(buf);
            return NULL;
        }

        if (n > 0)
            wcsncpy_s(&buf[len], bufsize - len + 1, subbuf, n);

        PyMem_Free(subbuf);

        /* when the read is empty we break */
        if (n == 0)
            break;

        len += n;
    }

    if (len == 0 && _buflen(self) == 0) {
        /* when the result starts with ^Z we return an empty buffer */
        PyMem_Free(buf);
        return PyBytes_FromStringAndSize(NULL, 0);
    }

    if (len) {
        Py_BEGIN_ALLOW_THREADS
        bytes_size = WideCharToMultiByte(CP_UTF8, 0, buf, len,
            NULL, 0, NULL, NULL);
        Py_END_ALLOW_THREADS

        if (!bytes_size) {
            DWORD err = GetLastError();
            PyMem_Free(buf);
            return PyErr_SetFromWindowsErr(err);
        }
    } else {
        bytes_size = 0;
    }

    bytes_size += _buflen(self);
    bytes = PyBytes_FromStringAndSize(NULL, bytes_size);
    rn = _copyfrombuf(self, PyBytes_AS_STRING(bytes), bytes_size);

    if (len) {
        Py_BEGIN_ALLOW_THREADS
        bytes_size = WideCharToMultiByte(CP_UTF8, 0, buf, len,
            &PyBytes_AS_STRING(bytes)[rn], bytes_size - rn, NULL, NULL);
        Py_END_ALLOW_THREADS

        if (!bytes_size) {
            DWORD err = GetLastError();
            PyMem_Free(buf);
            Py_CLEAR(bytes);
            return PyErr_SetFromWindowsErr(err);
        }

        /* add back the number of preserved bytes */
        bytes_size += rn;
    }

    PyMem_Free(buf);
    if (bytes_size < (size_t)PyBytes_GET_SIZE(bytes)) {
        if (_PyBytes_Resize(&bytes, n * sizeof(wchar_t)) < 0) {
            Py_CLEAR(bytes);
            return NULL;
        }
    }
    return bytes;
}

/*[clinic input]
_io._WindowsConsoleIO.read
    cls: defining_class
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read at most size bytes, returned as bytes.

Only makes one system call when size is a positive integer,
so less data may be returned than requested.
Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_read_impl(winconsoleio *self, PyTypeObject *cls,
                                Py_ssize_t size)
/*[clinic end generated code: output=7e569a586537c0ae input=a14570a5da273365]*/
{
    PyObject *bytes;
    Py_ssize_t bytes_size;

    if (self->fd == -1)
        return err_closed();
    if (!self->readable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "reading");
    }

    if (size < 0)
        return _io__WindowsConsoleIO_readall_impl(self);
    if (size > BUFMAX) {
        PyErr_Format(PyExc_ValueError, "cannot read more than %d bytes", BUFMAX);
        return NULL;
    }

    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return NULL;

    _PyIO_State *state = get_io_state_by_cls(cls);
    bytes_size = readinto(state, self, PyBytes_AS_STRING(bytes),
                          PyBytes_GET_SIZE(bytes));
    if (bytes_size < 0) {
        Py_CLEAR(bytes);
        return NULL;
    }

    if (bytes_size < PyBytes_GET_SIZE(bytes)) {
        if (_PyBytes_Resize(&bytes, bytes_size) < 0) {
            Py_CLEAR(bytes);
            return NULL;
        }
    }

    return bytes;
}

/*[clinic input]
_io._WindowsConsoleIO.write
    cls: defining_class
    b: Py_buffer
    /

Write buffer b to file, return number of bytes written.

Only makes one system call, so not all of the data may be written.
The number of bytes actually written is returned.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_write_impl(winconsoleio *self, PyTypeObject *cls,
                                 Py_buffer *b)
/*[clinic end generated code: output=e8019f480243cb29 input=10ac37c19339dfbe]*/
{
    BOOL res = TRUE;
    wchar_t *wbuf;
    DWORD len, wlen, n = 0;
    HANDLE handle;

    if (self->fd == -1)
        return err_closed();
    if (!self->writable) {
        _PyIO_State *state = get_io_state_by_cls(cls);
        return err_mode(state, "writing");
    }

    handle = _Py_get_osfhandle(self->fd);
    if (handle == INVALID_HANDLE_VALUE)
        return NULL;

    if (!b->len) {
        return PyLong_FromLong(0);
    }
    if (b->len > BUFMAX)
        len = BUFMAX;
    else
        len = (DWORD)b->len;

    Py_BEGIN_ALLOW_THREADS
    wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len, NULL, 0);

    /* issue11395 there is an unspecified upper bound on how many bytes
       can be written at once. We cap at 32k - the caller will have to
       handle partial writes.
       Since we don't know how many input bytes are being ignored, we
       have to reduce and recalculate. */
    while (wlen > 32766 / sizeof(wchar_t)) {
        len /= 2;
        /* Fix for github issues gh-110913 and gh-82052. */
        len = _find_last_utf8_boundary(b->buf, len);
        wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len, NULL, 0);
    }
    Py_END_ALLOW_THREADS

    if (!wlen)
        return PyErr_SetFromWindowsErr(0);

    wbuf = (wchar_t*)PyMem_Malloc(wlen * sizeof(wchar_t));

    Py_BEGIN_ALLOW_THREADS
    wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len, wbuf, wlen);
    if (wlen) {
        res = WriteConsoleW(handle, wbuf, wlen, &n, NULL);
        if (res && n < wlen) {
            /* Wrote fewer characters than expected, which means our
             * len value may be wrong. So recalculate it from the
             * characters that were written. As this could potentially
             * result in a different value, we also validate that value.
             */
            len = WideCharToMultiByte(CP_UTF8, 0, wbuf, n,
                NULL, 0, NULL, NULL);
            if (len) {
                wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len,
                    NULL, 0);
                assert(wlen == len);
            }
        }
    } else
        res = 0;
    Py_END_ALLOW_THREADS

    if (!res) {
        DWORD err = GetLastError();
        PyMem_Free(wbuf);
        return PyErr_SetFromWindowsErr(err);
    }

    PyMem_Free(wbuf);
    return PyLong_FromSsize_t(len);
}

static PyObject *
winconsoleio_repr(winconsoleio *self)
{
    const char *type_name = (Py_TYPE((PyObject *)self)->tp_name);

    if (self->fd == -1) {
        return PyUnicode_FromFormat("<%.100s [closed]>", type_name);
    }

    if (self->readable) {
        return PyUnicode_FromFormat("<%.100s mode='rb' closefd=%s>",
                                    type_name,
                                    self->closefd ? "True" : "False");
    }
    if (self->writable) {
        return PyUnicode_FromFormat("<%.100s mode='wb' closefd=%s>",
                                    type_name,
                                    self->closefd ? "True" : "False");
    }

    PyErr_SetString(PyExc_SystemError, "_WindowsConsoleIO has invalid mode");
    return NULL;
}

/*[clinic input]
_io._WindowsConsoleIO.isatty

Always True.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_isatty_impl(winconsoleio *self)
/*[clinic end generated code: output=9eac09d287c11bd7 input=9b91591dbe356f86]*/
{
    if (self->fd == -1)
        return err_closed();

    Py_RETURN_TRUE;
}

#define clinic_state() (find_io_state_by_def(Py_TYPE(self)))
#include "clinic/winconsoleio.c.h"
#undef clinic_state

static PyMethodDef winconsoleio_methods[] = {
    _IO__WINDOWSCONSOLEIO_READ_METHODDEF
    _IO__WINDOWSCONSOLEIO_READALL_METHODDEF
    _IO__WINDOWSCONSOLEIO_READINTO_METHODDEF
    _IO__WINDOWSCONSOLEIO_WRITE_METHODDEF
    _IO__WINDOWSCONSOLEIO_CLOSE_METHODDEF
    _IO__WINDOWSCONSOLEIO_READABLE_METHODDEF
    _IO__WINDOWSCONSOLEIO_WRITABLE_METHODDEF
    _IO__WINDOWSCONSOLEIO_FILENO_METHODDEF
    _IO__WINDOWSCONSOLEIO_ISATTY_METHODDEF
    {NULL,           NULL}             /* sentinel */
};

/* 'closed' and 'mode' are attributes for compatibility with FileIO. */

static PyObject *
get_closed(winconsoleio *self, void *closure)
{
    return PyBool_FromLong((long)(self->fd == -1));
}

static PyObject *
get_closefd(winconsoleio *self, void *closure)
{
    return PyBool_FromLong((long)(self->closefd));
}

static PyObject *
get_mode(winconsoleio *self, void *closure)
{
    return PyUnicode_FromString(self->readable ? "rb" : "wb");
}

static PyGetSetDef winconsoleio_getsetlist[] = {
    {"closed", (getter)get_closed, NULL, "True if the file is closed"},
    {"closefd", (getter)get_closefd, NULL,
        "True if the file descriptor will be closed by close()."},
    {"mode", (getter)get_mode, NULL, "String giving the file mode"},
    {NULL},
};

static PyMemberDef winconsoleio_members[] = {
    {"_blksize", Py_T_UINT, offsetof(winconsoleio, blksize), 0},
    {"_finalizing", Py_T_BOOL, offsetof(winconsoleio, finalizing), 0},
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(winconsoleio, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(winconsoleio, dict), Py_READONLY},
    {NULL}
};

static PyType_Slot winconsoleio_slots[] = {
    {Py_tp_dealloc, winconsoleio_dealloc},
    {Py_tp_repr, winconsoleio_repr},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_doc, (void *)_io__WindowsConsoleIO___init____doc__},
    {Py_tp_traverse, winconsoleio_traverse},
    {Py_tp_clear, winconsoleio_clear},
    {Py_tp_methods, winconsoleio_methods},
    {Py_tp_members, winconsoleio_members},
    {Py_tp_getset, winconsoleio_getsetlist},
    {Py_tp_init, _io__WindowsConsoleIO___init__},
    {Py_tp_new, winconsoleio_new},
    {0, NULL},
};

PyType_Spec winconsoleio_spec = {
    .name = "_io._WindowsConsoleIO",
    .basicsize = sizeof(winconsoleio),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = winconsoleio_slots,
};

#endif /* HAVE_WINDOWS_CONSOLE_IO */
