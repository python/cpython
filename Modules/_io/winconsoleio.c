/*
    An implementation of Windows console I/O

    Classes defined here: _WindowsConsoleIO

    Written by Steve Dower
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#ifdef MS_WINDOWS

#include "structmember.h"
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <stddef.h> /* For offsetof */

#define WIN32_LEAN_AND_MEAN
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
        HANDLE handle;
        _Py_BEGIN_SUPPRESS_IPH
        handle = (HANDLE)_get_osfhandle(fd);
        _Py_END_SUPPRESS_IPH
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


/*[clinic input]
module _io
class _io._WindowsConsoleIO "winconsoleio *" "&PyWindowsConsoleIO_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e897fdc1fba4e131]*/

typedef struct {
    PyObject_HEAD
    HANDLE handle;
    int fd;
    unsigned int created : 1;
    unsigned int readable : 1;
    unsigned int writable : 1;
    unsigned int closehandle : 1;
    char finalizing;
    unsigned int blksize;
    PyObject *weakreflist;
    PyObject *dict;
    char buf[SMALLBUF];
    wchar_t wbuf;
} winconsoleio;

PyTypeObject PyWindowsConsoleIO_Type;

_Py_IDENTIFIER(name);

int
_PyWindowsConsoleIO_closed(PyObject *self)
{
    return ((winconsoleio *)self)->handle == INVALID_HANDLE_VALUE;
}


/* Returns 0 on success, -1 with exception set on failure. */
static int
internal_close(winconsoleio *self)
{
    if (self->handle != INVALID_HANDLE_VALUE) {
        if (self->closehandle) {
            if (self->fd >= 0) {
                _Py_BEGIN_SUPPRESS_IPH
                close(self->fd);
                _Py_END_SUPPRESS_IPH
            }
            CloseHandle(self->handle);
        }
        self->handle = INVALID_HANDLE_VALUE;
        self->fd = -1;
    }
    return 0;
}

/*[clinic input]
_io._WindowsConsoleIO.close

Close the handle.

A closed handle cannot be used for further I/O operations.  close() may be
called more than once without error.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_close_impl(winconsoleio *self)
/*[clinic end generated code: output=27ef95b66c29057b input=185617e349ae4c7b]*/
{
    PyObject *res;
    PyObject *exc, *val, *tb;
    int rc;
    _Py_IDENTIFIER(close);
    res = _PyObject_CallMethodIdObjArgs((PyObject*)&PyRawIOBase_Type,
                                        &PyId_close, self, NULL);
    if (!self->closehandle) {
        self->handle = INVALID_HANDLE_VALUE;
        return res;
    }
    if (res == NULL)
        PyErr_Fetch(&exc, &val, &tb);
    rc = internal_close(self);
    if (res == NULL)
        _PyErr_ChainExceptions(exc, val, tb);
    if (rc < 0)
        Py_CLEAR(res);
    return res;
}

static PyObject *
winconsoleio_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    winconsoleio *self;

    assert(type != NULL && type->tp_alloc != NULL);

    self = (winconsoleio *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->handle = INVALID_HANDLE_VALUE;
        self->fd = -1;
        self->created = 0;
        self->readable = 0;
        self->writable = 0;
        self->closehandle = 0;
        self->blksize = 0;
        self->weakreflist = NULL;
    }

    return (PyObject *) self;
}

/*[clinic input]
_io._WindowsConsoleIO.__init__
    file as nameobj: object
    mode: str = "r"
    closefd: bool(accept={int}) = True
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
/*[clinic end generated code: output=3fd9cbcdd8d95429 input=06ae4b863c63244b]*/
{
    const char *s;
    wchar_t *name = NULL;
    char console_type = '\0';
    int ret = 0;
    int rwa = 0;
    int fd = -1;
    int fd_is_own = 0;

    assert(PyWindowsConsoleIO_Check(self));
    if (self->handle >= 0) {
        if (self->closehandle) {
            /* Have to close the existing file first. */
            if (internal_close(self) < 0)
                return -1;
        }
        else
            self->handle = INVALID_HANDLE_VALUE;
    }

    if (PyFloat_Check(nameobj)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float");
        return -1;
    }

    fd = _PyLong_AsInt(nameobj);
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
        _Py_BEGIN_SUPPRESS_IPH
        self->handle = (HANDLE)_get_osfhandle(fd);
        _Py_END_SUPPRESS_IPH
        self->closehandle = 0;
    } else {
        DWORD access = GENERIC_READ;

        self->closehandle = 1;
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
        self->handle = CreateFileW(name, GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (self->handle == INVALID_HANDLE_VALUE)
            self->handle = CreateFileW(name, access,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        Py_END_ALLOW_THREADS

        if (self->handle == INVALID_HANDLE_VALUE) {
            PyErr_SetExcFromWindowsErrWithFilenameObject(PyExc_OSError, GetLastError(), nameobj);
            goto error;
        }
    }

    if (console_type == '\0')
        console_type = _get_console_type(self->handle);

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

    if (_PyObject_SetAttrId((PyObject *)self, &PyId_name, nameobj) < 0)
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
    self->finalizing = 1;
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
err_mode(const char *action)
{
    _PyIO_State *state = IO_STATE();
    if (state != NULL)
        PyErr_Format(state->unsupported_operation,
                     "Console buffer does not support %s", action);
    return NULL;
}

/*[clinic input]
_io._WindowsConsoleIO.fileno

Return the underlying file descriptor (an integer).

fileno is only set when a file descriptor is used to open
one of the standard streams.

[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_fileno_impl(winconsoleio *self)
/*[clinic end generated code: output=006fa74ce3b5cfbf input=079adc330ddaabe6]*/
{
    if (self->fd < 0 && self->handle != INVALID_HANDLE_VALUE) {
        _Py_BEGIN_SUPPRESS_IPH
        if (self->writable)
            self->fd = _open_osfhandle((intptr_t)self->handle, _O_WRONLY | _O_BINARY);
        else
            self->fd = _open_osfhandle((intptr_t)self->handle, _O_RDONLY | _O_BINARY);
        _Py_END_SUPPRESS_IPH
    }
    if (self->fd < 0)
        return err_mode("fileno");
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
    if (self->handle == INVALID_HANDLE_VALUE)
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
    if (self->handle == INVALID_HANDLE_VALUE)
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
        DWORD n, len = min(maxlen - off, BUFSIZ);
        SetLastError(0);
        BOOL res = ReadConsoleW(handle, &buf[off], len, &n, NULL);

        if (!res) {
            err = GetLastError();
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
readinto(winconsoleio *self, char *buf, Py_ssize_t len)
{
    if (self->handle == INVALID_HANDLE_VALUE) {
        err_closed();
        return -1;
    }
    if (!self->readable) {
        err_mode("reading");
        return -1;
    }
    if (len == 0)
        return 0;
    if (len > BUFMAX) {
        PyErr_Format(PyExc_ValueError, "cannot read more than %d bytes", BUFMAX);
        return -1;
    }

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
    wchar_t *wbuf = read_console_w(self->handle, wlen, &n);
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
            "Buffer had room for %d bytes but %d bytes required",
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
    buffer: Py_buffer(accept={rwbuffer})
    /

Same as RawIOBase.readinto().
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_readinto_impl(winconsoleio *self, Py_buffer *buffer)
/*[clinic end generated code: output=66d1bdfa3f20af39 input=4ed68da48a6baffe]*/
{
    Py_ssize_t len = readinto(self, buffer->buf, buffer->len);
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

    if (self->handle == INVALID_HANDLE_VALUE)
        return err_closed();

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

            buf = PyMem_Realloc(buf, (bufsize + 1) * sizeof(wchar_t));
            if (!buf) {
                PyMem_Free(buf);
                return NULL;
            }
        }

        subbuf = read_console_w(self->handle, bufsize - len, &n);

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
    size: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read at most size bytes, returned as bytes.

Only makes one system call when size is a positive integer,
so less data may be returned than requested.
Return an empty bytes object at EOF.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_read_impl(winconsoleio *self, Py_ssize_t size)
/*[clinic end generated code: output=57df68af9f4b22d0 input=8bc73bc15d0fa072]*/
{
    PyObject *bytes;
    Py_ssize_t bytes_size;

    if (self->handle == INVALID_HANDLE_VALUE)
        return err_closed();
    if (!self->readable)
        return err_mode("reading");

    if (size < 0)
        return _io__WindowsConsoleIO_readall_impl(self);
    if (size > BUFMAX) {
        PyErr_Format(PyExc_ValueError, "cannot read more than %d bytes", BUFMAX);
        return NULL;
    }

    bytes = PyBytes_FromStringAndSize(NULL, size);
    if (bytes == NULL)
        return NULL;

    bytes_size = readinto(self, PyBytes_AS_STRING(bytes), PyBytes_GET_SIZE(bytes));
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
    b: Py_buffer
    /

Write buffer b to file, return number of bytes written.

Only makes one system call, so not all of the data may be written.
The number of bytes actually written is returned.
[clinic start generated code]*/

static PyObject *
_io__WindowsConsoleIO_write_impl(winconsoleio *self, Py_buffer *b)
/*[clinic end generated code: output=775bdb16fbf9137b input=be35fb624f97c941]*/
{
    BOOL res = TRUE;
    wchar_t *wbuf;
    DWORD len, wlen, n = 0;

    if (self->handle == INVALID_HANDLE_VALUE)
        return err_closed();
    if (!self->writable)
        return err_mode("writing");

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
        wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len, NULL, 0);
    }
    Py_END_ALLOW_THREADS

    if (!wlen)
        return PyErr_SetFromWindowsErr(0);

    wbuf = (wchar_t*)PyMem_Malloc(wlen * sizeof(wchar_t));

    Py_BEGIN_ALLOW_THREADS
    wlen = MultiByteToWideChar(CP_UTF8, 0, b->buf, len, wbuf, wlen);
    if (wlen) {
        res = WriteConsoleW(self->handle, wbuf, wlen, &n, NULL);
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
    if (self->handle == INVALID_HANDLE_VALUE)
        return PyUnicode_FromFormat("<_io._WindowsConsoleIO [closed]>");

    if (self->readable)
        return PyUnicode_FromFormat("<_io._WindowsConsoleIO mode='rb' closefd=%s>",
            self->closehandle ? "True" : "False");
    if (self->writable)
        return PyUnicode_FromFormat("<_io._WindowsConsoleIO mode='wb' closefd=%s>",
            self->closehandle ? "True" : "False");

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
    if (self->handle == INVALID_HANDLE_VALUE)
        return err_closed();

    Py_RETURN_TRUE;
}

static PyObject *
winconsoleio_getstate(winconsoleio *self)
{
    PyErr_Format(PyExc_TypeError,
                 "cannot serialize '%s' object", Py_TYPE(self)->tp_name);
    return NULL;
}

#include "clinic/winconsoleio.c.h"

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
    {"__getstate__", (PyCFunction)winconsoleio_getstate, METH_NOARGS, NULL},
    {NULL,           NULL}             /* sentinel */
};

/* 'closed' and 'mode' are attributes for compatibility with FileIO. */

static PyObject *
get_closed(winconsoleio *self, void *closure)
{
    return PyBool_FromLong((long)(self->handle == INVALID_HANDLE_VALUE));
}

static PyObject *
get_closefd(winconsoleio *self, void *closure)
{
    return PyBool_FromLong((long)(self->closehandle));
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
    {"_blksize", T_UINT, offsetof(winconsoleio, blksize), 0},
    {"_finalizing", T_BOOL, offsetof(winconsoleio, finalizing), 0},
    {NULL}
};

PyTypeObject PyWindowsConsoleIO_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io._WindowsConsoleIO",
    sizeof(winconsoleio),
    0,
    (destructor)winconsoleio_dealloc,           /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)winconsoleio_repr,                /* tp_repr */
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
        | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_HAVE_FINALIZE,       /* tp_flags */
    _io__WindowsConsoleIO___init____doc__,      /* tp_doc */
    (traverseproc)winconsoleio_traverse,        /* tp_traverse */
    (inquiry)winconsoleio_clear,                /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(winconsoleio, weakreflist),        /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    winconsoleio_methods,                       /* tp_methods */
    winconsoleio_members,                       /* tp_members */
    winconsoleio_getsetlist,                    /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(winconsoleio, dict),               /* tp_dictoffset */
    _io__WindowsConsoleIO___init__,             /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    winconsoleio_new,                           /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
    0,                                          /* tp_is_gc */
    0,                                          /* tp_bases */
    0,                                          /* tp_mro */
    0,                                          /* tp_cache */
    0,                                          /* tp_subclasses */
    0,                                          /* tp_weaklist */
    0,                                          /* tp_del */
    0,                                          /* tp_version_tag */
    0,                                          /* tp_finalize */
};

PyAPI_DATA(PyObject *) _PyWindowsConsoleIO_Type = (PyObject*)&PyWindowsConsoleIO_Type;

#endif /* MS_WINDOWS */
