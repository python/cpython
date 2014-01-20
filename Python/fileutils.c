#include "Python.h"
#include "osdefs.h"
#include <locale.h>

#ifdef MS_WINDOWS
#  include <windows.h>
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */

#ifdef __APPLE__
extern wchar_t* _Py_DecodeUTF8_surrogateescape(const char *s, Py_ssize_t size);
#endif

#ifdef O_CLOEXEC
/* Does open() support the O_CLOEXEC flag? Possible values:

   -1: unknown
    0: open() ignores O_CLOEXEC flag, ex: Linux kernel older than 2.6.23
    1: open() supports O_CLOEXEC flag, close-on-exec is set

   The flag is used by _Py_open(), io.FileIO and os.open() */
int _Py_open_cloexec_works = -1;
#endif

PyObject *
_Py_device_encoding(int fd)
{
#if defined(MS_WINDOWS)
    UINT cp;
#endif
    if (!_PyVerify_fd(fd) || !isatty(fd)) {
        Py_RETURN_NONE;
    }
#if defined(MS_WINDOWS)
    if (fd == 0)
        cp = GetConsoleCP();
    else if (fd == 1 || fd == 2)
        cp = GetConsoleOutputCP();
    else
        cp = 0;
    /* GetConsoleCP() and GetConsoleOutputCP() return 0 if the application
       has no console */
    if (cp != 0)
        return PyUnicode_FromFormat("cp%u", (unsigned int)cp);
#elif defined(CODESET)
    {
        char *codeset = nl_langinfo(CODESET);
        if (codeset != NULL && codeset[0] != 0)
            return PyUnicode_FromString(codeset);
    }
#endif
    Py_RETURN_NONE;
}

#if !defined(__APPLE__) && !defined(MS_WINDOWS)
extern int _Py_normalize_encoding(const char *, char *, size_t);

/* Workaround FreeBSD and OpenIndiana locale encoding issue with the C locale.
   On these operating systems, nl_langinfo(CODESET) announces an alias of the
   ASCII encoding, whereas mbstowcs() and wcstombs() functions use the
   ISO-8859-1 encoding. The problem is that os.fsencode() and os.fsdecode() use
   locale.getpreferredencoding() codec. For example, if command line arguments
   are decoded by mbstowcs() and encoded back by os.fsencode(), we get a
   UnicodeEncodeError instead of retrieving the original byte string.

   The workaround is enabled if setlocale(LC_CTYPE, NULL) returns "C",
   nl_langinfo(CODESET) announces "ascii" (or an alias to ASCII), and at least
   one byte in range 0x80-0xff can be decoded from the locale encoding. The
   workaround is also enabled on error, for example if getting the locale
   failed.

   Values of force_ascii:

       1: the workaround is used: _Py_wchar2char() uses
          encode_ascii_surrogateescape() and _Py_char2wchar() uses
          decode_ascii_surrogateescape()
       0: the workaround is not used: _Py_wchar2char() uses wcstombs() and
          _Py_char2wchar() uses mbstowcs()
      -1: unknown, need to call check_force_ascii() to get the value
*/
static int force_ascii = -1;

static int
check_force_ascii(void)
{
    char *loc;
#if defined(HAVE_LANGINFO_H) && defined(CODESET)
    char *codeset, **alias;
    char encoding[100];
    int is_ascii;
    unsigned int i;
    char* ascii_aliases[] = {
        "ascii",
        "646",
        "ansi-x3.4-1968",
        "ansi-x3-4-1968",
        "ansi-x3.4-1986",
        "cp367",
        "csascii",
        "ibm367",
        "iso646-us",
        "iso-646.irv-1991",
        "iso-ir-6",
        "us",
        "us-ascii",
        NULL
    };
#endif

    loc = setlocale(LC_CTYPE, NULL);
    if (loc == NULL)
        goto error;
    if (strcmp(loc, "C") != 0) {
        /* the LC_CTYPE locale is different than C */
        return 0;
    }

#if defined(HAVE_LANGINFO_H) && defined(CODESET)
    codeset = nl_langinfo(CODESET);
    if (!codeset || codeset[0] == '\0') {
        /* CODESET is not set or empty */
        goto error;
    }
    if (!_Py_normalize_encoding(codeset, encoding, sizeof(encoding)))
        goto error;

    is_ascii = 0;
    for (alias=ascii_aliases; *alias != NULL; alias++) {
        if (strcmp(encoding, *alias) == 0) {
            is_ascii = 1;
            break;
        }
    }
    if (!is_ascii) {
        /* nl_langinfo(CODESET) is not "ascii" or an alias of ASCII */
        return 0;
    }

    for (i=0x80; i<0xff; i++) {
        unsigned char ch;
        wchar_t wch;
        size_t res;

        ch = (unsigned char)i;
        res = mbstowcs(&wch, (char*)&ch, 1);
        if (res != (size_t)-1) {
            /* decoding a non-ASCII character from the locale encoding succeed:
               the locale encoding is not ASCII, force ASCII */
            return 1;
        }
    }
    /* None of the bytes in the range 0x80-0xff can be decoded from the locale
       encoding: the locale encoding is really ASCII */
    return 0;
#else
    /* nl_langinfo(CODESET) is not available: always force ASCII */
    return 1;
#endif

error:
    /* if an error occured, force the ASCII encoding */
    return 1;
}

static char*
encode_ascii_surrogateescape(const wchar_t *text, size_t *error_pos)
{
    char *result = NULL, *out;
    size_t len, i;
    wchar_t ch;

    if (error_pos != NULL)
        *error_pos = (size_t)-1;

    len = wcslen(text);

    result = PyMem_Malloc(len + 1);  /* +1 for NUL byte */
    if (result == NULL)
        return NULL;

    out = result;
    for (i=0; i<len; i++) {
        ch = text[i];

        if (ch <= 0x7f) {
            /* ASCII character */
            *out++ = (char)ch;
        }
        else if (0xdc80 <= ch && ch <= 0xdcff) {
            /* UTF-8b surrogate */
            *out++ = (char)(ch - 0xdc00);
        }
        else {
            if (error_pos != NULL)
                *error_pos = i;
            PyMem_Free(result);
            return NULL;
        }
    }
    *out = '\0';
    return result;
}
#endif   /* !defined(__APPLE__) && !defined(MS_WINDOWS) */

#if !defined(__APPLE__) && (!defined(MS_WINDOWS) || !defined(HAVE_MBRTOWC))
static wchar_t*
decode_ascii_surrogateescape(const char *arg, size_t *size)
{
    wchar_t *res;
    unsigned char *in;
    wchar_t *out;

    res = PyMem_RawMalloc((strlen(arg)+1)*sizeof(wchar_t));
    if (!res)
        return NULL;

    in = (unsigned char*)arg;
    out = res;
    while(*in)
        if(*in < 128)
            *out++ = *in++;
        else
            *out++ = 0xdc00 + *in++;
    *out = 0;
    if (size != NULL)
        *size = out - res;
    return res;
}
#endif


/* Decode a byte string from the locale encoding with the
   surrogateescape error handler (undecodable bytes are decoded as characters
   in range U+DC80..U+DCFF). If a byte sequence can be decoded as a surrogate
   character, escape the bytes using the surrogateescape error handler instead
   of decoding them.

   Use _Py_wchar2char() to encode the character string back to a byte string.

   Return a pointer to a newly allocated wide character string (use
   PyMem_RawFree() to free the memory) and write the number of written wide
   characters excluding the null character into *size if size is not NULL, or
   NULL on error (decoding or memory allocation error). If size is not NULL,
   *size is set to (size_t)-1 on memory error and (size_t)-2 on decoding
   error.

   Conversion errors should never happen, unless there is a bug in the C
   library. */
wchar_t*
_Py_char2wchar(const char* arg, size_t *size)
{
#ifdef __APPLE__
    wchar_t *wstr;
    wstr = _Py_DecodeUTF8_surrogateescape(arg, strlen(arg));
    if (size != NULL) {
        if (wstr != NULL)
            *size = wcslen(wstr);
        else
            *size = (size_t)-1;
    }
    return wstr;
#else
    wchar_t *res;
    size_t argsize;
    size_t count;
#ifdef HAVE_MBRTOWC
    unsigned char *in;
    wchar_t *out;
    mbstate_t mbs;
#endif

#ifndef MS_WINDOWS
    if (force_ascii == -1)
        force_ascii = check_force_ascii();

    if (force_ascii) {
        /* force ASCII encoding to workaround mbstowcs() issue */
        res = decode_ascii_surrogateescape(arg, size);
        if (res == NULL)
            goto oom;
        return res;
    }
#endif

#ifdef HAVE_BROKEN_MBSTOWCS
    /* Some platforms have a broken implementation of
     * mbstowcs which does not count the characters that
     * would result from conversion.  Use an upper bound.
     */
    argsize = strlen(arg);
#else
    argsize = mbstowcs(NULL, arg, 0);
#endif
    if (argsize != (size_t)-1) {
        res = (wchar_t *)PyMem_RawMalloc((argsize+1)*sizeof(wchar_t));
        if (!res)
            goto oom;
        count = mbstowcs(res, arg, argsize+1);
        if (count != (size_t)-1) {
            wchar_t *tmp;
            /* Only use the result if it contains no
               surrogate characters. */
            for (tmp = res; *tmp != 0 &&
                         !Py_UNICODE_IS_SURROGATE(*tmp); tmp++)
                ;
            if (*tmp == 0) {
                if (size != NULL)
                    *size = count;
                return res;
            }
        }
        PyMem_RawFree(res);
    }
    /* Conversion failed. Fall back to escaping with surrogateescape. */
#ifdef HAVE_MBRTOWC
    /* Try conversion with mbrtwoc (C99), and escape non-decodable bytes. */

    /* Overallocate; as multi-byte characters are in the argument, the
       actual output could use less memory. */
    argsize = strlen(arg) + 1;
    res = (wchar_t*)PyMem_RawMalloc(argsize*sizeof(wchar_t));
    if (!res)
        goto oom;
    in = (unsigned char*)arg;
    out = res;
    memset(&mbs, 0, sizeof mbs);
    while (argsize) {
        size_t converted = mbrtowc(out, (char*)in, argsize, &mbs);
        if (converted == 0)
            /* Reached end of string; null char stored. */
            break;
        if (converted == (size_t)-2) {
            /* Incomplete character. This should never happen,
               since we provide everything that we have -
               unless there is a bug in the C library, or I
               misunderstood how mbrtowc works. */
            PyMem_RawFree(res);
            if (size != NULL)
                *size = (size_t)-2;
            return NULL;
        }
        if (converted == (size_t)-1) {
            /* Conversion error. Escape as UTF-8b, and start over
               in the initial shift state. */
            *out++ = 0xdc00 + *in++;
            argsize--;
            memset(&mbs, 0, sizeof mbs);
            continue;
        }
        if (Py_UNICODE_IS_SURROGATE(*out)) {
            /* Surrogate character.  Escape the original
               byte sequence with surrogateescape. */
            argsize -= converted;
            while (converted--)
                *out++ = 0xdc00 + *in++;
            continue;
        }
        /* successfully converted some bytes */
        in += converted;
        argsize -= converted;
        out++;
    }
    if (size != NULL)
        *size = out - res;
#else   /* HAVE_MBRTOWC */
    /* Cannot use C locale for escaping; manually escape as if charset
       is ASCII (i.e. escape all bytes > 128. This will still roundtrip
       correctly in the locale's charset, which must be an ASCII superset. */
    res = decode_ascii_surrogateescape(arg, size);
    if (res == NULL)
        goto oom;
#endif   /* HAVE_MBRTOWC */
    return res;
oom:
    if (size != NULL)
        *size = (size_t)-1;
    return NULL;
#endif   /* __APPLE__ */
}

/* Encode a (wide) character string to the locale encoding with the
   surrogateescape error handler (characters in range U+DC80..U+DCFF are
   converted to bytes 0x80..0xFF).

   This function is the reverse of _Py_char2wchar().

   Return a pointer to a newly allocated byte string (use PyMem_Free() to free
   the memory), or NULL on encoding or memory allocation error.

   If error_pos is not NULL: *error_pos is the index of the invalid character
   on encoding error, or (size_t)-1 otherwise. */
char*
_Py_wchar2char(const wchar_t *text, size_t *error_pos)
{
#ifdef __APPLE__
    Py_ssize_t len;
    PyObject *unicode, *bytes = NULL;
    char *cpath;

    unicode = PyUnicode_FromWideChar(text, wcslen(text));
    if (unicode == NULL)
        return NULL;

    bytes = _PyUnicode_AsUTF8String(unicode, "surrogateescape");
    Py_DECREF(unicode);
    if (bytes == NULL) {
        PyErr_Clear();
        if (error_pos != NULL)
            *error_pos = (size_t)-1;
        return NULL;
    }

    len = PyBytes_GET_SIZE(bytes);
    cpath = PyMem_Malloc(len+1);
    if (cpath == NULL) {
        PyErr_Clear();
        Py_DECREF(bytes);
        if (error_pos != NULL)
            *error_pos = (size_t)-1;
        return NULL;
    }
    memcpy(cpath, PyBytes_AsString(bytes), len + 1);
    Py_DECREF(bytes);
    return cpath;
#else   /* __APPLE__ */
    const size_t len = wcslen(text);
    char *result = NULL, *bytes = NULL;
    size_t i, size, converted;
    wchar_t c, buf[2];

#ifndef MS_WINDOWS
    if (force_ascii == -1)
        force_ascii = check_force_ascii();

    if (force_ascii)
        return encode_ascii_surrogateescape(text, error_pos);
#endif

    /* The function works in two steps:
       1. compute the length of the output buffer in bytes (size)
       2. outputs the bytes */
    size = 0;
    buf[1] = 0;
    while (1) {
        for (i=0; i < len; i++) {
            c = text[i];
            if (c >= 0xdc80 && c <= 0xdcff) {
                /* UTF-8b surrogate */
                if (bytes != NULL) {
                    *bytes++ = c - 0xdc00;
                    size--;
                }
                else
                    size++;
                continue;
            }
            else {
                buf[0] = c;
                if (bytes != NULL)
                    converted = wcstombs(bytes, buf, size);
                else
                    converted = wcstombs(NULL, buf, 0);
                if (converted == (size_t)-1) {
                    if (result != NULL)
                        PyMem_Free(result);
                    if (error_pos != NULL)
                        *error_pos = i;
                    return NULL;
                }
                if (bytes != NULL) {
                    bytes += converted;
                    size -= converted;
                }
                else
                    size += converted;
            }
        }
        if (result != NULL) {
            *bytes = '\0';
            break;
        }

        size += 1; /* nul byte at the end */
        result = PyMem_Malloc(size);
        if (result == NULL) {
            if (error_pos != NULL)
                *error_pos = (size_t)-1;
            return NULL;
        }
        bytes = result;
    }
    return result;
#endif   /* __APPLE__ */
}

/* In principle, this should use HAVE__WSTAT, and _wstat
   should be detected by autoconf. However, no current
   POSIX system provides that function, so testing for
   it is pointless.
   Not sure whether the MS_WINDOWS guards are necessary:
   perhaps for cygwin/mingw builds?
*/
#if defined(HAVE_STAT) && !defined(MS_WINDOWS)

/* Get file status. Encode the path to the locale encoding. */

int
_Py_wstat(const wchar_t* path, struct stat *buf)
{
    int err;
    char *fname;
    fname = _Py_wchar2char(path, NULL);
    if (fname == NULL) {
        errno = EINVAL;
        return -1;
    }
    err = stat(fname, buf);
    PyMem_Free(fname);
    return err;
}
#endif

#ifdef HAVE_STAT

/* Call _wstat() on Windows, or encode the path to the filesystem encoding and
   call stat() otherwise. Only fill st_mode attribute on Windows.

   Return 0 on success, -1 on _wstat() / stat() error, -2 if an exception was
   raised. */

int
_Py_stat(PyObject *path, struct stat *statbuf)
{
#ifdef MS_WINDOWS
    int err;
    struct _stat wstatbuf;
    wchar_t *wpath;

    wpath = PyUnicode_AsUnicode(path);
    if (wpath == NULL)
        return -2;
    err = _wstat(wpath, &wstatbuf);
    if (!err)
        statbuf->st_mode = wstatbuf.st_mode;
    return err;
#else
    int ret;
    PyObject *bytes = PyUnicode_EncodeFSDefault(path);
    if (bytes == NULL)
        return -2;
    ret = stat(PyBytes_AS_STRING(bytes), statbuf);
    Py_DECREF(bytes);
    return ret;
#endif
}

#endif

static int
get_inheritable(int fd, int raise)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    DWORD flags;

    if (!_PyVerify_fd(fd)) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }

    if (!GetHandleInformation(handle, &flags)) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }

    return (flags & HANDLE_FLAG_INHERIT);
#else
    int flags;

    flags = fcntl(fd, F_GETFD, 0);
    if (flags == -1) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return !(flags & FD_CLOEXEC);
#endif
}

/* Get the inheritable flag of the specified file descriptor.
   Return 1 if the file descriptor can be inherited, 0 if it cannot,
   raise an exception and return -1 on error. */
int
_Py_get_inheritable(int fd)
{
    return get_inheritable(fd, 1);
}

static int
set_inheritable(int fd, int inheritable, int raise, int *atomic_flag_works)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    DWORD flags;
#elif defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    int request;
    int err;
#elif defined(HAVE_FCNTL_H)
    int flags;
    int res;
#endif

    /* atomic_flag_works can only be used to make the file descriptor
       non-inheritable */
    assert(!(atomic_flag_works != NULL && inheritable));

    if (atomic_flag_works != NULL && !inheritable) {
        if (*atomic_flag_works == -1) {
            int inheritable = get_inheritable(fd, raise);
            if (inheritable == -1)
                return -1;
            *atomic_flag_works = !inheritable;
        }

        if (*atomic_flag_works)
            return 0;
    }

#ifdef MS_WINDOWS
    if (!_PyVerify_fd(fd)) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }

    if (inheritable)
        flags = HANDLE_FLAG_INHERIT;
    else
        flags = 0;
    if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, flags)) {
        if (raise)
            PyErr_SetFromWindowsErr(0);
        return -1;
    }
    return 0;

#elif defined(HAVE_SYS_IOCTL_H) && defined(FIOCLEX) && defined(FIONCLEX)
    if (inheritable)
        request = FIONCLEX;
    else
        request = FIOCLEX;
    err = ioctl(fd, request, NULL);
    if (err) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;

#else
    flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (inheritable)
        flags &= ~FD_CLOEXEC;
    else
        flags |= FD_CLOEXEC;
    res = fcntl(fd, F_SETFD, flags);
    if (res < 0) {
        if (raise)
            PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }
    return 0;
#endif
}

/* Make the file descriptor non-inheritable.
   Return 0 on success, set errno and return -1 on error. */
static int
make_non_inheritable(int fd)
{
    return set_inheritable(fd, 0, 0, NULL);
}

/* Set the inheritable flag of the specified file descriptor.
   On success: return 0, on error: raise an exception if raise is nonzero
   and return -1.

   If atomic_flag_works is not NULL:

    * if *atomic_flag_works==-1, check if the inheritable is set on the file
      descriptor: if yes, set *atomic_flag_works to 1, otherwise set to 0 and
      set the inheritable flag
    * if *atomic_flag_works==1: do nothing
    * if *atomic_flag_works==0: set inheritable flag to False

   Set atomic_flag_works to NULL if no atomic flag was used to create the
   file descriptor.

   atomic_flag_works can only be used to make a file descriptor
   non-inheritable: atomic_flag_works must be NULL if inheritable=1. */
int
_Py_set_inheritable(int fd, int inheritable, int *atomic_flag_works)
{
    return set_inheritable(fd, inheritable, 1, atomic_flag_works);
}

/* Open a file with the specified flags (wrapper to open() function).
   The file descriptor is created non-inheritable. */
int
_Py_open(const char *pathname, int flags)
{
    int fd;
#ifdef MS_WINDOWS
    fd = open(pathname, flags | O_NOINHERIT);
    if (fd < 0)
        return fd;
#else

    int *atomic_flag_works;
#ifdef O_CLOEXEC
    atomic_flag_works = &_Py_open_cloexec_works;
    flags |= O_CLOEXEC;
#else
    atomic_flag_works = NULL;
#endif
    fd = open(pathname, flags);
    if (fd < 0)
        return fd;

    if (set_inheritable(fd, 0, 0, atomic_flag_works) < 0) {
        close(fd);
        return -1;
    }
#endif   /* !MS_WINDOWS */
    return fd;
}

/* Open a file. Use _wfopen() on Windows, encode the path to the locale
   encoding and use fopen() otherwise. The file descriptor is created
   non-inheritable. */
FILE *
_Py_wfopen(const wchar_t *path, const wchar_t *mode)
{
    FILE *f;
#ifndef MS_WINDOWS
    char *cpath;
    char cmode[10];
    size_t r;
    r = wcstombs(cmode, mode, 10);
    if (r == (size_t)-1 || r >= 10) {
        errno = EINVAL;
        return NULL;
    }
    cpath = _Py_wchar2char(path, NULL);
    if (cpath == NULL)
        return NULL;
    f = fopen(cpath, cmode);
    PyMem_Free(cpath);
#else
    f = _wfopen(path, mode);
#endif
    if (f == NULL)
        return NULL;
    if (make_non_inheritable(fileno(f)) < 0) {
        fclose(f);
        return NULL;
    }
    return f;
}

/* Wrapper to fopen(). The file descriptor is created non-inheritable. */
FILE*
_Py_fopen(const char *pathname, const char *mode)
{
    FILE *f = fopen(pathname, mode);
    if (f == NULL)
        return NULL;
    if (make_non_inheritable(fileno(f)) < 0) {
        fclose(f);
        return NULL;
    }
    return f;
}

/* Open a file. Call _wfopen() on Windows, or encode the path to the filesystem
   encoding and call fopen() otherwise. The file descriptor is created
   non-inheritable.

   Return the new file object on success, or NULL if the file cannot be open or
   (if PyErr_Occurred()) on unicode error. */
FILE*
_Py_fopen_obj(PyObject *path, const char *mode)
{
    FILE *f;
#ifdef MS_WINDOWS
    wchar_t *wpath;
    wchar_t wmode[10];
    int usize;

    if (!PyUnicode_Check(path)) {
        PyErr_Format(PyExc_TypeError,
                     "str file path expected under Windows, got %R",
                     Py_TYPE(path));
        return NULL;
    }
    wpath = PyUnicode_AsUnicode(path);
    if (wpath == NULL)
        return NULL;

    usize = MultiByteToWideChar(CP_ACP, 0, mode, -1, wmode, sizeof(wmode));
    if (usize == 0)
        return NULL;

    f = _wfopen(wpath, wmode);
#else
    PyObject *bytes;
    if (!PyUnicode_FSConverter(path, &bytes))
        return NULL;
    f = fopen(PyBytes_AS_STRING(bytes), mode);
    Py_DECREF(bytes);
#endif
    if (f == NULL)
        return NULL;
    if (make_non_inheritable(fileno(f)) < 0) {
        fclose(f);
        return NULL;
    }
    return f;
}

#ifdef HAVE_READLINK

/* Read value of symbolic link. Encode the path to the locale encoding, decode
   the result from the locale encoding. Return -1 on error. */

int
_Py_wreadlink(const wchar_t *path, wchar_t *buf, size_t bufsiz)
{
    char *cpath;
    char cbuf[MAXPATHLEN];
    wchar_t *wbuf;
    int res;
    size_t r1;

    cpath = _Py_wchar2char(path, NULL);
    if (cpath == NULL) {
        errno = EINVAL;
        return -1;
    }
    res = (int)readlink(cpath, cbuf, Py_ARRAY_LENGTH(cbuf));
    PyMem_Free(cpath);
    if (res == -1)
        return -1;
    if (res == Py_ARRAY_LENGTH(cbuf)) {
        errno = EINVAL;
        return -1;
    }
    cbuf[res] = '\0'; /* buf will be null terminated */
    wbuf = _Py_char2wchar(cbuf, &r1);
    if (wbuf == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (bufsiz <= r1) {
        PyMem_RawFree(wbuf);
        errno = EINVAL;
        return -1;
    }
    wcsncpy(buf, wbuf, bufsiz);
    PyMem_RawFree(wbuf);
    return (int)r1;
}
#endif

#ifdef HAVE_REALPATH

/* Return the canonicalized absolute pathname. Encode path to the locale
   encoding, decode the result from the locale encoding.
   Return NULL on error. */

wchar_t*
_Py_wrealpath(const wchar_t *path,
              wchar_t *resolved_path, size_t resolved_path_size)
{
    char *cpath;
    char cresolved_path[MAXPATHLEN];
    wchar_t *wresolved_path;
    char *res;
    size_t r;
    cpath = _Py_wchar2char(path, NULL);
    if (cpath == NULL) {
        errno = EINVAL;
        return NULL;
    }
    res = realpath(cpath, cresolved_path);
    PyMem_Free(cpath);
    if (res == NULL)
        return NULL;

    wresolved_path = _Py_char2wchar(cresolved_path, &r);
    if (wresolved_path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (resolved_path_size <= r) {
        PyMem_RawFree(wresolved_path);
        errno = EINVAL;
        return NULL;
    }
    wcsncpy(resolved_path, wresolved_path, resolved_path_size);
    PyMem_RawFree(wresolved_path);
    return resolved_path;
}
#endif

/* Get the current directory. size is the buffer size in wide characters
   including the null character. Decode the path from the locale encoding.
   Return NULL on error. */

wchar_t*
_Py_wgetcwd(wchar_t *buf, size_t size)
{
#ifdef MS_WINDOWS
    int isize = (int)Py_MIN(size, INT_MAX);
    return _wgetcwd(buf, isize);
#else
    char fname[MAXPATHLEN];
    wchar_t *wname;
    size_t len;

    if (getcwd(fname, Py_ARRAY_LENGTH(fname)) == NULL)
        return NULL;
    wname = _Py_char2wchar(fname, &len);
    if (wname == NULL)
        return NULL;
    if (size <= len) {
        PyMem_RawFree(wname);
        return NULL;
    }
    wcsncpy(buf, wname, size);
    PyMem_RawFree(wname);
    return buf;
#endif
}

/* Duplicate a file descriptor. The new file descriptor is created as
   non-inheritable. Return a new file descriptor on success, raise an OSError
   exception and return -1 on error.

   The GIL is released to call dup(). The caller must hold the GIL. */
int
_Py_dup(int fd)
{
#ifdef MS_WINDOWS
    HANDLE handle;
    DWORD ftype;
#endif

    if (!_PyVerify_fd(fd)) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

#ifdef MS_WINDOWS
    handle = (HANDLE)_get_osfhandle(fd);
    if (handle == INVALID_HANDLE_VALUE) {
        PyErr_SetFromWindowsErr(0);
        return -1;
    }

    /* get the file type, ignore the error if it failed */
    ftype = GetFileType(handle);

    Py_BEGIN_ALLOW_THREADS
    fd = dup(fd);
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    /* Character files like console cannot be make non-inheritable */
    if (ftype != FILE_TYPE_CHAR) {
        if (_Py_set_inheritable(fd, 0, NULL) < 0) {
            close(fd);
            return -1;
        }
    }
#elif defined(HAVE_FCNTL_H) && defined(F_DUPFD_CLOEXEC)
    Py_BEGIN_ALLOW_THREADS
    fd = fcntl(fd, F_DUPFD_CLOEXEC, 0);
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

#else
    Py_BEGIN_ALLOW_THREADS
    fd = dup(fd);
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return -1;
    }

    if (_Py_set_inheritable(fd, 0, NULL) < 0) {
        close(fd);
        return -1;
    }
#endif
    return fd;
}

