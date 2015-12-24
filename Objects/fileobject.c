/* File object implementation */

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef MS_WINDOWS
#define fileno _fileno
/* can simulate truncate with Win32 API functions; see file_truncate */
#define HAVE_FTRUNCATE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(PYOS_OS2) && defined(PYCC_GCC)
#include <io.h>
#endif

#define BUF(v) PyString_AS_STRING((PyStringObject *)v)

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_GETC_UNLOCKED
#define GETC(f) getc_unlocked(f)
#define FLOCKFILE(f) flockfile(f)
#define FUNLOCKFILE(f) funlockfile(f)
#else
#define GETC(f) getc(f)
#define FLOCKFILE(f)
#define FUNLOCKFILE(f)
#endif

/* Bits in f_newlinetypes */
#define NEWLINE_UNKNOWN 0       /* No newline seen, yet */
#define NEWLINE_CR 1            /* \r newline seen */
#define NEWLINE_LF 2            /* \n newline seen */
#define NEWLINE_CRLF 4          /* \r\n newline seen */

/*
 * These macros release the GIL while preventing the f_close() function being
 * called in the interval between them.  For that purpose, a running total of
 * the number of currently running unlocked code sections is kept in
 * the unlocked_count field of the PyFileObject. The close() method raises
 * an IOError if that field is non-zero.  See issue #815646, #595601.
 */

#define FILE_BEGIN_ALLOW_THREADS(fobj) \
{ \
    fobj->unlocked_count++; \
    Py_BEGIN_ALLOW_THREADS

#define FILE_END_ALLOW_THREADS(fobj) \
    Py_END_ALLOW_THREADS \
    fobj->unlocked_count--; \
    assert(fobj->unlocked_count >= 0); \
}

#define FILE_ABORT_ALLOW_THREADS(fobj) \
    Py_BLOCK_THREADS \
    fobj->unlocked_count--; \
    assert(fobj->unlocked_count >= 0);

#ifdef __cplusplus
extern "C" {
#endif

FILE *
PyFile_AsFile(PyObject *f)
{
    if (f == NULL || !PyFile_Check(f))
        return NULL;
    else
        return ((PyFileObject *)f)->f_fp;
}

void PyFile_IncUseCount(PyFileObject *fobj)
{
    fobj->unlocked_count++;
}

void PyFile_DecUseCount(PyFileObject *fobj)
{
    fobj->unlocked_count--;
    assert(fobj->unlocked_count >= 0);
}

PyObject *
PyFile_Name(PyObject *f)
{
    if (f == NULL || !PyFile_Check(f))
        return NULL;
    else
        return ((PyFileObject *)f)->f_name;
}

/* This is a safe wrapper around PyObject_Print to print to the FILE
   of a PyFileObject. PyObject_Print releases the GIL but knows nothing
   about PyFileObject. */
static int
file_PyObject_Print(PyObject *op, PyFileObject *f, int flags)
{
    int result;
    PyFile_IncUseCount(f);
    result = PyObject_Print(op, f->f_fp, flags);
    PyFile_DecUseCount(f);
    return result;
}

/* On Unix, fopen will succeed for directories.
   In Python, there should be no file objects referring to
   directories, so we need a check.  */

static PyFileObject*
dircheck(PyFileObject* f)
{
#if defined(HAVE_FSTAT) && defined(S_IFDIR) && defined(EISDIR)
    struct stat buf;
    if (f->f_fp == NULL)
        return f;
    if (fstat(fileno(f->f_fp), &buf) == 0 &&
        S_ISDIR(buf.st_mode)) {
        char *msg = strerror(EISDIR);
        PyObject *exc = PyObject_CallFunction(PyExc_IOError, "(isO)",
                                              EISDIR, msg, f->f_name);
        PyErr_SetObject(PyExc_IOError, exc);
        Py_XDECREF(exc);
        return NULL;
    }
#endif
    return f;
}


static PyObject *
fill_file_fields(PyFileObject *f, FILE *fp, PyObject *name, char *mode,
                 int (*close)(FILE *))
{
    assert(name != NULL);
    assert(f != NULL);
    assert(PyFile_Check(f));
    assert(f->f_fp == NULL);

    Py_DECREF(f->f_name);
    Py_DECREF(f->f_mode);
    Py_DECREF(f->f_encoding);
    Py_DECREF(f->f_errors);

    Py_INCREF(name);
    f->f_name = name;

    f->f_mode = PyString_FromString(mode);

    f->f_close = close;
    f->f_softspace = 0;
    f->f_binary = strchr(mode,'b') != NULL;
    f->f_buf = NULL;
    f->f_univ_newline = (strchr(mode, 'U') != NULL);
    f->f_newlinetypes = NEWLINE_UNKNOWN;
    f->f_skipnextlf = 0;
    Py_INCREF(Py_None);
    f->f_encoding = Py_None;
    Py_INCREF(Py_None);
    f->f_errors = Py_None;
    f->readable = f->writable = 0;
    if (strchr(mode, 'r') != NULL || f->f_univ_newline)
        f->readable = 1;
    if (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL)
        f->writable = 1;
    if (strchr(mode, '+') != NULL)
        f->readable = f->writable = 1;

    if (f->f_mode == NULL)
        return NULL;
    f->f_fp = fp;
    f = dircheck(f);
    return (PyObject *) f;
}

#if defined _MSC_VER && _MSC_VER >= 1400 && defined(__STDC_SECURE_LIB__)
#define Py_VERIFY_WINNT
/* The CRT on windows compiled with Visual Studio 2005 and higher may
 * assert if given invalid mode strings.  This is all fine and well
 * in static languages like C where the mode string is typcially hard
 * coded.  But in Python, were we pass in the mode string from the user,
 * we need to verify it first manually
 */
static int _PyVerify_Mode_WINNT(const char *mode)
{
    /* See if mode string is valid on Windows to avoid hard assertions */
    /* remove leading spacese */
    int singles = 0;
    int pairs = 0;
    int encoding = 0;
    const char *s, *c;

    while(*mode == ' ') /* strip initial spaces */
        ++mode;
    if (!strchr("rwa", *mode)) /* must start with one of these */
        return 0;
    while (*++mode) {
        if (*mode == ' ' || *mode == 'N') /* ignore spaces and N */
            continue;
        s = "+TD"; /* each of this can appear only once */
        c = strchr(s, *mode);
        if (c) {
            ptrdiff_t idx = s-c;
            if (singles & (1<<idx))
                return 0;
            singles |= (1<<idx);
            continue;
        }
        s = "btcnSR"; /* only one of each letter in the pairs allowed */
        c = strchr(s, *mode);
        if (c) {
            ptrdiff_t idx = (s-c)/2;
            if (pairs & (1<<idx))
                return 0;
            pairs |= (1<<idx);
            continue;
        }
        if (*mode == ',') {
            encoding = 1;
            break;
        }
        return 0; /* found an invalid char */
    }

    if (encoding) {
        char *e[] = {"UTF-8", "UTF-16LE", "UNICODE"};
        while (*mode == ' ')
            ++mode;
        /* find 'ccs =' */
        if (strncmp(mode, "ccs", 3))
            return 0;
        mode += 3;
        while (*mode == ' ')
            ++mode;
        if (*mode != '=')
            return 0;
        while (*mode == ' ')
            ++mode;
        for(encoding = 0; encoding<_countof(e); ++encoding) {
            size_t l = strlen(e[encoding]);
            if (!strncmp(mode, e[encoding], l)) {
                mode += l; /* found a valid encoding */
                break;
            }
        }
        if (encoding == _countof(e))
            return 0;
    }
    /* skip trailing spaces */
    while (*mode == ' ')
        ++mode;

    return *mode == '\0'; /* must be at the end of the string */
}
#endif

/* check for known incorrect mode strings - problem is, platforms are
   free to accept any mode characters they like and are supposed to
   ignore stuff they don't understand... write or append mode with
   universal newline support is expressly forbidden by PEP 278.
   Additionally, remove the 'U' from the mode string as platforms
   won't know what it is. Non-zero return signals an exception */
int
_PyFile_SanitizeMode(char *mode)
{
    char *upos;
    size_t len = strlen(mode);

    if (!len) {
        PyErr_SetString(PyExc_ValueError, "empty mode string");
        return -1;
    }

    upos = strchr(mode, 'U');
    if (upos) {
        memmove(upos, upos+1, len-(upos-mode)); /* incl null char */

        if (mode[0] == 'w' || mode[0] == 'a') {
            PyErr_Format(PyExc_ValueError, "universal newline "
                         "mode can only be used with modes "
                         "starting with 'r'");
            return -1;
        }

        if (mode[0] != 'r') {
            memmove(mode+1, mode, strlen(mode)+1);
            mode[0] = 'r';
        }

        if (!strchr(mode, 'b')) {
            memmove(mode+2, mode+1, strlen(mode));
            mode[1] = 'b';
        }
    } else if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a') {
        PyErr_Format(PyExc_ValueError, "mode string must begin with "
                    "one of 'r', 'w', 'a' or 'U', not '%.200s'", mode);
        return -1;
    }
#ifdef Py_VERIFY_WINNT
    /* additional checks on NT with visual studio 2005 and higher */
    if (!_PyVerify_Mode_WINNT(mode)) {
        PyErr_Format(PyExc_ValueError, "Invalid mode ('%.50s')", mode);
        return -1;
    }
#endif
    return 0;
}

static PyObject *
open_the_file(PyFileObject *f, char *name, char *mode)
{
    char *newmode;
    assert(f != NULL);
    assert(PyFile_Check(f));
#ifdef MS_WINDOWS
    /* windows ignores the passed name in order to support Unicode */
    assert(f->f_name != NULL);
#else
    assert(name != NULL);
#endif
    assert(mode != NULL);
    assert(f->f_fp == NULL);

    /* probably need to replace 'U' by 'rb' */
    newmode = PyMem_MALLOC(strlen(mode) + 3);
    if (!newmode) {
        PyErr_NoMemory();
        return NULL;
    }
    strcpy(newmode, mode);

    if (_PyFile_SanitizeMode(newmode)) {
        f = NULL;
        goto cleanup;
    }

    /* rexec.py can't stop a user from getting the file() constructor --
       all they have to do is get *any* file object f, and then do
       type(f).  Here we prevent them from doing damage with it. */
    if (PyEval_GetRestricted()) {
        PyErr_SetString(PyExc_IOError,
        "file() constructor not accessible in restricted mode");
        f = NULL;
        goto cleanup;
    }
    errno = 0;

#ifdef MS_WINDOWS
    if (PyUnicode_Check(f->f_name)) {
        PyObject *wmode;
        wmode = PyUnicode_DecodeASCII(newmode, strlen(newmode), NULL);
        if (f->f_name && wmode) {
            FILE_BEGIN_ALLOW_THREADS(f)
            /* PyUnicode_AS_UNICODE OK without thread
               lock as it is a simple dereference. */
            f->f_fp = _wfopen(PyUnicode_AS_UNICODE(f->f_name),
                              PyUnicode_AS_UNICODE(wmode));
            FILE_END_ALLOW_THREADS(f)
        }
        Py_XDECREF(wmode);
    }
#endif
    if (NULL == f->f_fp && NULL != name) {
        FILE_BEGIN_ALLOW_THREADS(f)
        f->f_fp = fopen(name, newmode);
        FILE_END_ALLOW_THREADS(f)
    }

    if (f->f_fp == NULL) {
#if defined  _MSC_VER && (_MSC_VER < 1400 || !defined(__STDC_SECURE_LIB__))
        /* MSVC 6 (Microsoft) leaves errno at 0 for bad mode strings,
         * across all Windows flavors.  When it sets EINVAL varies
         * across Windows flavors, the exact conditions aren't
         * documented, and the answer lies in the OS's implementation
         * of Win32's CreateFile function (whose source is secret).
         * Seems the best we can do is map EINVAL to ENOENT.
         * Starting with Visual Studio .NET 2005, EINVAL is correctly
         * set by our CRT error handler (set in exceptions.c.)
         */
        if (errno == 0)         /* bad mode string */
            errno = EINVAL;
        else if (errno == EINVAL) /* unknown, but not a mode string */
            errno = ENOENT;
#endif
        /* EINVAL is returned when an invalid filename or
         * an invalid mode is supplied. */
        if (errno == EINVAL) {
            PyObject *v;
            char message[100];
            PyOS_snprintf(message, 100,
                "invalid mode ('%.50s') or filename", mode);
            v = Py_BuildValue("(isO)", errno, message, f->f_name);
            if (v != NULL) {
                PyErr_SetObject(PyExc_IOError, v);
                Py_DECREF(v);
            }
        }
        else
            PyErr_SetFromErrnoWithFilenameObject(PyExc_IOError, f->f_name);
        f = NULL;
    }
    if (f != NULL)
        f = dircheck(f);

cleanup:
    PyMem_FREE(newmode);

    return (PyObject *)f;
}

static PyObject *
close_the_file(PyFileObject *f)
{
    int sts = 0;
    int (*local_close)(FILE *);
    FILE *local_fp = f->f_fp;
    char *local_setbuf = f->f_setbuf;
    if (local_fp != NULL) {
        local_close = f->f_close;
        if (local_close != NULL && f->unlocked_count > 0) {
            if (f->ob_refcnt > 0) {
                PyErr_SetString(PyExc_IOError,
                    "close() called during concurrent "
                    "operation on the same file object.");
            } else {
                /* This should not happen unless someone is
                 * carelessly playing with the PyFileObject
                 * struct fields and/or its associated FILE
                 * pointer. */
                PyErr_SetString(PyExc_SystemError,
                    "PyFileObject locking error in "
                    "destructor (refcnt <= 0 at close).");
            }
            return NULL;
        }
        /* NULL out the FILE pointer before releasing the GIL, because
         * it will not be valid anymore after the close() function is
         * called. */
        f->f_fp = NULL;
        if (local_close != NULL) {
            /* Issue #9295: must temporarily reset f_setbuf so that another
               thread doesn't free it when running file_close() concurrently.
               Otherwise this close() will crash when flushing the buffer. */
            f->f_setbuf = NULL;
            Py_BEGIN_ALLOW_THREADS
            errno = 0;
            sts = (*local_close)(local_fp);
            Py_END_ALLOW_THREADS
            f->f_setbuf = local_setbuf;
            if (sts == EOF)
                return PyErr_SetFromErrno(PyExc_IOError);
            if (sts != 0)
                return PyInt_FromLong((long)sts);
        }
    }
    Py_RETURN_NONE;
}

PyObject *
PyFile_FromFile(FILE *fp, char *name, char *mode, int (*close)(FILE *))
{
    PyFileObject *f;
    PyObject *o_name;

    f = (PyFileObject *)PyFile_Type.tp_new(&PyFile_Type, NULL, NULL);
    if (f == NULL)
        return NULL;
    o_name = PyString_FromString(name);
    if (o_name == NULL) {
        if (close != NULL && fp != NULL)
            close(fp);
        Py_DECREF(f);
        return NULL;
    }
    if (fill_file_fields(f, fp, o_name, mode, close) == NULL) {
        Py_DECREF(f);
        Py_DECREF(o_name);
        return NULL;
    }
    Py_DECREF(o_name);
    return (PyObject *)f;
}

PyObject *
PyFile_FromString(char *name, char *mode)
{
    extern int fclose(FILE *);
    PyFileObject *f;

    f = (PyFileObject *)PyFile_FromFile((FILE *)NULL, name, mode, fclose);
    if (f != NULL) {
        if (open_the_file(f, name, mode) == NULL) {
            Py_DECREF(f);
            f = NULL;
        }
    }
    return (PyObject *)f;
}

void
PyFile_SetBufSize(PyObject *f, int bufsize)
{
    PyFileObject *file = (PyFileObject *)f;
    if (bufsize >= 0) {
        int type;
        switch (bufsize) {
        case 0:
            type = _IONBF;
            break;
#ifdef HAVE_SETVBUF
        case 1:
            type = _IOLBF;
            bufsize = BUFSIZ;
            break;
#endif
        default:
            type = _IOFBF;
#ifndef HAVE_SETVBUF
            bufsize = BUFSIZ;
#endif
            break;
        }
        fflush(file->f_fp);
        if (type == _IONBF) {
            PyMem_Free(file->f_setbuf);
            file->f_setbuf = NULL;
        } else {
            file->f_setbuf = (char *)PyMem_Realloc(file->f_setbuf,
                                                    bufsize);
        }
#ifdef HAVE_SETVBUF
        setvbuf(file->f_fp, file->f_setbuf, type, bufsize);
#else /* !HAVE_SETVBUF */
        setbuf(file->f_fp, file->f_setbuf);
#endif /* !HAVE_SETVBUF */
    }
}

/* Set the encoding used to output Unicode strings.
   Return 1 on success, 0 on failure. */

int
PyFile_SetEncoding(PyObject *f, const char *enc)
{
    return PyFile_SetEncodingAndErrors(f, enc, NULL);
}

int
PyFile_SetEncodingAndErrors(PyObject *f, const char *enc, char* errors)
{
    PyFileObject *file = (PyFileObject*)f;
    PyObject *str, *oerrors;

    assert(PyFile_Check(f));
    str = PyString_FromString(enc);
    if (!str)
        return 0;
    if (errors) {
        oerrors = PyString_FromString(errors);
        if (!oerrors) {
            Py_DECREF(str);
            return 0;
        }
    } else {
        oerrors = Py_None;
        Py_INCREF(Py_None);
    }
    Py_SETREF(file->f_encoding, str);
    Py_SETREF(file->f_errors, oerrors);
    return 1;
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
    PyErr_Format(PyExc_IOError, "File not open for %s", action);
    return NULL;
}

/* Refuse regular file I/O if there's data in the iteration-buffer.
 * Mixing them would cause data to arrive out of order, as the read*
 * methods don't use the iteration buffer. */
static PyObject *
err_iterbuffered(void)
{
    PyErr_SetString(PyExc_ValueError,
        "Mixing iteration and read methods would lose data");
    return NULL;
}

static void drop_readahead(PyFileObject *);

/* Methods */

static void
file_dealloc(PyFileObject *f)
{
    PyObject *ret;
    if (f->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) f);
    ret = close_the_file(f);
    if (!ret) {
        PySys_WriteStderr("close failed in file object destructor:\n");
        PyErr_Print();
    }
    else {
        Py_DECREF(ret);
    }
    PyMem_Free(f->f_setbuf);
    Py_XDECREF(f->f_name);
    Py_XDECREF(f->f_mode);
    Py_XDECREF(f->f_encoding);
    Py_XDECREF(f->f_errors);
    drop_readahead(f);
    Py_TYPE(f)->tp_free((PyObject *)f);
}

static PyObject *
file_repr(PyFileObject *f)
{
    PyObject *ret = NULL;
    PyObject *name = NULL;
    if (PyUnicode_Check(f->f_name)) {
#ifdef Py_USING_UNICODE
        const char *name_str;
        name = PyUnicode_AsUnicodeEscapeString(f->f_name);
        name_str = name ? PyString_AsString(name) : "?";
        ret = PyString_FromFormat("<%s file u'%s', mode '%s' at %p>",
                           f->f_fp == NULL ? "closed" : "open",
                           name_str,
                           PyString_AsString(f->f_mode),
                           f);
        Py_XDECREF(name);
        return ret;
#endif
    } else {
        name = PyObject_Repr(f->f_name);
        if (name == NULL)
            return NULL;
        ret = PyString_FromFormat("<%s file %s, mode '%s' at %p>",
                           f->f_fp == NULL ? "closed" : "open",
                           PyString_AsString(name),
                           PyString_AsString(f->f_mode),
                           f);
        Py_XDECREF(name);
        return ret;
    }
}

static PyObject *
file_close(PyFileObject *f)
{
    PyObject *sts = close_the_file(f);
    if (sts) {
        PyMem_Free(f->f_setbuf);
        f->f_setbuf = NULL;
    }
    return sts;
}


/* Our very own off_t-like type, 64-bit if possible */
#if !defined(HAVE_LARGEFILE_SUPPORT)
typedef off_t Py_off_t;
#elif SIZEOF_OFF_T >= 8
typedef off_t Py_off_t;
#elif SIZEOF_FPOS_T >= 8
typedef fpos_t Py_off_t;
#else
#error "Large file support, but neither off_t nor fpos_t is large enough."
#endif


/* a portable fseek() function
   return 0 on success, non-zero on failure (with errno set) */
static int
_portable_fseek(FILE *fp, Py_off_t offset, int whence)
{
#if !defined(HAVE_LARGEFILE_SUPPORT)
    return fseek(fp, offset, whence);
#elif defined(HAVE_FSEEKO) && SIZEOF_OFF_T >= 8
    return fseeko(fp, offset, whence);
#elif defined(HAVE_FSEEK64)
    return fseek64(fp, offset, whence);
#elif defined(__BEOS__)
    return _fseek(fp, offset, whence);
#elif SIZEOF_FPOS_T >= 8
    /* lacking a 64-bit capable fseek(), use a 64-bit capable fsetpos()
       and fgetpos() to implement fseek()*/
    fpos_t pos;
    switch (whence) {
    case SEEK_END:
#ifdef MS_WINDOWS
        fflush(fp);
        if (_lseeki64(fileno(fp), 0, 2) == -1)
            return -1;
#else
        if (fseek(fp, 0, SEEK_END) != 0)
            return -1;
#endif
        /* fall through */
    case SEEK_CUR:
        if (fgetpos(fp, &pos) != 0)
            return -1;
        offset += pos;
        break;
    /* case SEEK_SET: break; */
    }
    return fsetpos(fp, &offset);
#else
#error "Large file support, but no way to fseek."
#endif
}


/* a portable ftell() function
   Return -1 on failure with errno set appropriately, current file
   position on success */
static Py_off_t
_portable_ftell(FILE* fp)
{
#if !defined(HAVE_LARGEFILE_SUPPORT)
    return ftell(fp);
#elif defined(HAVE_FTELLO) && SIZEOF_OFF_T >= 8
    return ftello(fp);
#elif defined(HAVE_FTELL64)
    return ftell64(fp);
#elif SIZEOF_FPOS_T >= 8
    fpos_t pos;
    if (fgetpos(fp, &pos) != 0)
        return -1;
    return pos;
#else
#error "Large file support, but no way to ftell."
#endif
}


static PyObject *
file_seek(PyFileObject *f, PyObject *args)
{
    int whence;
    int ret;
    Py_off_t offset;
    PyObject *offobj, *off_index;

    if (f->f_fp == NULL)
        return err_closed();
    drop_readahead(f);
    whence = 0;
    if (!PyArg_ParseTuple(args, "O|i:seek", &offobj, &whence))
        return NULL;
    off_index = PyNumber_Index(offobj);
    if (!off_index) {
        if (!PyFloat_Check(offobj))
            return NULL;
        /* Deprecated in 2.6 */
        PyErr_Clear();
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "integer argument expected, got float",
                         1) < 0)
            return NULL;
        off_index = offobj;
        Py_INCREF(offobj);
    }
#if !defined(HAVE_LARGEFILE_SUPPORT)
    offset = PyInt_AsLong(off_index);
#else
    offset = PyLong_Check(off_index) ?
        PyLong_AsLongLong(off_index) : PyInt_AsLong(off_index);
#endif
    Py_DECREF(off_index);
    if (PyErr_Occurred())
        return NULL;

    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    ret = _portable_fseek(f->f_fp, offset, whence);
    FILE_END_ALLOW_THREADS(f)

    if (ret != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        clearerr(f->f_fp);
        return NULL;
    }
    f->f_skipnextlf = 0;
    Py_INCREF(Py_None);
    return Py_None;
}


#ifdef HAVE_FTRUNCATE
static PyObject *
file_truncate(PyFileObject *f, PyObject *args)
{
    Py_off_t newsize;
    PyObject *newsizeobj = NULL;
    Py_off_t initialpos;
    int ret;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->writable)
        return err_mode("writing");
    if (!PyArg_UnpackTuple(args, "truncate", 0, 1, &newsizeobj))
        return NULL;

    /* Get current file position.  If the file happens to be open for
     * update and the last operation was an input operation, C doesn't
     * define what the later fflush() will do, but we promise truncate()
     * won't change the current position (and fflush() *does* change it
     * then at least on Windows).  The easiest thing is to capture
     * current pos now and seek back to it at the end.
     */
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    initialpos = _portable_ftell(f->f_fp);
    FILE_END_ALLOW_THREADS(f)
    if (initialpos == -1)
        goto onioerror;

    /* Set newsize to current postion if newsizeobj NULL, else to the
     * specified value.
     */
    if (newsizeobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
        newsize = PyInt_AsLong(newsizeobj);
#else
        newsize = PyLong_Check(newsizeobj) ?
                        PyLong_AsLongLong(newsizeobj) :
                PyInt_AsLong(newsizeobj);
#endif
        if (PyErr_Occurred())
            return NULL;
    }
    else /* default to current position */
        newsize = initialpos;

    /* Flush the stream.  We're mixing stream-level I/O with lower-level
     * I/O, and a flush may be necessary to synch both platform views
     * of the current file state.
     */
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    ret = fflush(f->f_fp);
    FILE_END_ALLOW_THREADS(f)
    if (ret != 0)
        goto onioerror;

#ifdef MS_WINDOWS
    /* MS _chsize doesn't work if newsize doesn't fit in 32 bits,
       so don't even try using it. */
    {
        HANDLE hFile;

        /* Have to move current pos to desired endpoint on Windows. */
        FILE_BEGIN_ALLOW_THREADS(f)
        errno = 0;
        ret = _portable_fseek(f->f_fp, newsize, SEEK_SET) != 0;
        FILE_END_ALLOW_THREADS(f)
        if (ret)
            goto onioerror;

        /* Truncate.  Note that this may grow the file! */
        FILE_BEGIN_ALLOW_THREADS(f)
        errno = 0;
        hFile = (HANDLE)_get_osfhandle(fileno(f->f_fp));
        ret = hFile == (HANDLE)-1;
        if (ret == 0) {
            ret = SetEndOfFile(hFile) == 0;
            if (ret)
                errno = EACCES;
        }
        FILE_END_ALLOW_THREADS(f)
        if (ret)
            goto onioerror;
    }
#else
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    ret = ftruncate(fileno(f->f_fp), newsize);
    FILE_END_ALLOW_THREADS(f)
    if (ret != 0)
        goto onioerror;
#endif /* !MS_WINDOWS */

    /* Restore original file position. */
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    ret = _portable_fseek(f->f_fp, initialpos, SEEK_SET) != 0;
    FILE_END_ALLOW_THREADS(f)
    if (ret)
        goto onioerror;

    Py_INCREF(Py_None);
    return Py_None;

onioerror:
    PyErr_SetFromErrno(PyExc_IOError);
    clearerr(f->f_fp);
    return NULL;
}
#endif /* HAVE_FTRUNCATE */

static PyObject *
file_tell(PyFileObject *f)
{
    Py_off_t pos;

    if (f->f_fp == NULL)
        return err_closed();
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    pos = _portable_ftell(f->f_fp);
    FILE_END_ALLOW_THREADS(f)

    if (pos == -1) {
        PyErr_SetFromErrno(PyExc_IOError);
        clearerr(f->f_fp);
        return NULL;
    }
    if (f->f_skipnextlf) {
        int c;
        c = GETC(f->f_fp);
        if (c == '\n') {
            f->f_newlinetypes |= NEWLINE_CRLF;
            pos++;
            f->f_skipnextlf = 0;
        } else if (c != EOF) ungetc(c, f->f_fp);
    }
#if !defined(HAVE_LARGEFILE_SUPPORT)
    return PyInt_FromLong(pos);
#else
    return PyLong_FromLongLong(pos);
#endif
}

static PyObject *
file_fileno(PyFileObject *f)
{
    if (f->f_fp == NULL)
        return err_closed();
    return PyInt_FromLong((long) fileno(f->f_fp));
}

static PyObject *
file_flush(PyFileObject *f)
{
    int res;

    if (f->f_fp == NULL)
        return err_closed();
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    res = fflush(f->f_fp);
    FILE_END_ALLOW_THREADS(f)
    if (res != 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        clearerr(f->f_fp);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
file_isatty(PyFileObject *f)
{
    long res;
    if (f->f_fp == NULL)
        return err_closed();
    FILE_BEGIN_ALLOW_THREADS(f)
    res = isatty((int)fileno(f->f_fp));
    FILE_END_ALLOW_THREADS(f)
    return PyBool_FromLong(res);
}


#if BUFSIZ < 8192
#define SMALLCHUNK 8192
#else
#define SMALLCHUNK BUFSIZ
#endif

static size_t
new_buffersize(PyFileObject *f, size_t currentsize)
{
#ifdef HAVE_FSTAT
    off_t pos, end;
    struct stat st;
    if (fstat(fileno(f->f_fp), &st) == 0) {
        end = st.st_size;
        /* The following is not a bug: we really need to call lseek()
           *and* ftell().  The reason is that some stdio libraries
           mistakenly flush their buffer when ftell() is called and
           the lseek() call it makes fails, thereby throwing away
           data that cannot be recovered in any way.  To avoid this,
           we first test lseek(), and only call ftell() if lseek()
           works.  We can't use the lseek() value either, because we
           need to take the amount of buffered data into account.
           (Yet another reason why stdio stinks. :-) */
        pos = lseek(fileno(f->f_fp), 0L, SEEK_CUR);
        if (pos >= 0) {
            pos = ftell(f->f_fp);
        }
        if (pos < 0)
            clearerr(f->f_fp);
        if (end > pos && pos >= 0)
            return currentsize + end - pos + 1;
        /* Add 1 so if the file were to grow we'd notice. */
    }
#endif
    /* Expand the buffer by an amount proportional to the current size,
       giving us amortized linear-time behavior. Use a less-than-double
       growth factor to avoid excessive allocation. */
    return currentsize + (currentsize >> 3) + 6;
}

#if defined(EWOULDBLOCK) && defined(EAGAIN) && EWOULDBLOCK != EAGAIN
#define BLOCKED_ERRNO(x) ((x) == EWOULDBLOCK || (x) == EAGAIN)
#else
#ifdef EWOULDBLOCK
#define BLOCKED_ERRNO(x) ((x) == EWOULDBLOCK)
#else
#ifdef EAGAIN
#define BLOCKED_ERRNO(x) ((x) == EAGAIN)
#else
#define BLOCKED_ERRNO(x) 0
#endif
#endif
#endif

static PyObject *
file_read(PyFileObject *f, PyObject *args)
{
    long bytesrequested = -1;
    size_t bytesread, buffersize, chunksize;
    PyObject *v;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->readable)
        return err_mode("reading");
    /* refuse to mix with f.next() */
    if (f->f_buf != NULL &&
        (f->f_bufend - f->f_bufptr) > 0 &&
        f->f_buf[0] != '\0')
        return err_iterbuffered();
    if (!PyArg_ParseTuple(args, "|l:read", &bytesrequested))
        return NULL;
    if (bytesrequested < 0)
        buffersize = new_buffersize(f, (size_t)0);
    else
        buffersize = bytesrequested;
    if (buffersize > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError,
    "requested number of bytes is more than a Python string can hold");
        return NULL;
    }
    v = PyString_FromStringAndSize((char *)NULL, buffersize);
    if (v == NULL)
        return NULL;
    bytesread = 0;
    for (;;) {
        int interrupted;
        FILE_BEGIN_ALLOW_THREADS(f)
        errno = 0;
        chunksize = Py_UniversalNewlineFread(BUF(v) + bytesread,
                  buffersize - bytesread, f->f_fp, (PyObject *)f);
        interrupted = ferror(f->f_fp) && errno == EINTR;
        FILE_END_ALLOW_THREADS(f)
        if (interrupted) {
            clearerr(f->f_fp);
            if (PyErr_CheckSignals()) {
                Py_DECREF(v);
                return NULL;
            }
        }
        if (chunksize == 0) {
            if (interrupted)
                continue;
            if (!ferror(f->f_fp))
                break;
            clearerr(f->f_fp);
            /* When in non-blocking mode, data shouldn't
             * be discarded if a blocking signal was
             * received. That will also happen if
             * chunksize != 0, but bytesread < buffersize. */
            if (bytesread > 0 && BLOCKED_ERRNO(errno))
                break;
            PyErr_SetFromErrno(PyExc_IOError);
            Py_DECREF(v);
            return NULL;
        }
        bytesread += chunksize;
        if (bytesread < buffersize && !interrupted) {
            clearerr(f->f_fp);
            break;
        }
        if (bytesrequested < 0) {
            buffersize = new_buffersize(f, buffersize);
            if (_PyString_Resize(&v, buffersize) < 0)
                return NULL;
        } else {
            /* Got what was requested. */
            break;
        }
    }
    if (bytesread != buffersize && _PyString_Resize(&v, bytesread))
        return NULL;
    return v;
}

static PyObject *
file_readinto(PyFileObject *f, PyObject *args)
{
    char *ptr;
    Py_ssize_t ntodo;
    Py_ssize_t ndone, nnow;
    Py_buffer pbuf;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->readable)
        return err_mode("reading");
    /* refuse to mix with f.next() */
    if (f->f_buf != NULL &&
        (f->f_bufend - f->f_bufptr) > 0 &&
        f->f_buf[0] != '\0')
        return err_iterbuffered();
    if (!PyArg_ParseTuple(args, "w*", &pbuf))
        return NULL;
    ptr = pbuf.buf;
    ntodo = pbuf.len;
    ndone = 0;
    while (ntodo > 0) {
        int interrupted;
        FILE_BEGIN_ALLOW_THREADS(f)
        errno = 0;
        nnow = Py_UniversalNewlineFread(ptr+ndone, ntodo, f->f_fp,
                                        (PyObject *)f);
        interrupted = ferror(f->f_fp) && errno == EINTR;
        FILE_END_ALLOW_THREADS(f)
        if (interrupted) {
            clearerr(f->f_fp);
            if (PyErr_CheckSignals()) {
                PyBuffer_Release(&pbuf);
                return NULL;
            }
        }
        if (nnow == 0) {
            if (interrupted)
                continue;
            if (!ferror(f->f_fp))
                break;
            PyErr_SetFromErrno(PyExc_IOError);
            clearerr(f->f_fp);
            PyBuffer_Release(&pbuf);
            return NULL;
        }
        ndone += nnow;
        ntodo -= nnow;
    }
    PyBuffer_Release(&pbuf);
    return PyInt_FromSsize_t(ndone);
}

/**************************************************************************
Routine to get next line using platform fgets().

Under MSVC 6:

+ MS threadsafe getc is very slow (multiple layers of function calls before+
  after each character, to lock+unlock the stream).
+ The stream-locking functions are MS-internal -- can't access them from user
  code.
+ There's nothing Tim could find in the MS C or platform SDK libraries that
  can worm around this.
+ MS fgets locks/unlocks only once per line; it's the only hook we have.

So we use fgets for speed(!), despite that it's painful.

MS realloc is also slow.

Reports from other platforms on this method vs getc_unlocked (which MS doesn't
have):
    Linux               a wash
    Solaris             a wash
    Tru64 Unix          getline_via_fgets significantly faster

CAUTION:  The C std isn't clear about this:  in those cases where fgets
writes something into the buffer, can it write into any position beyond the
required trailing null byte?  MSVC 6 fgets does not, and no platform is (yet)
known on which it does; and it would be a strange way to code fgets. Still,
getline_via_fgets may not work correctly if it does.  The std test
test_bufio.py should fail if platform fgets() routinely writes beyond the
trailing null byte.  #define DONT_USE_FGETS_IN_GETLINE to disable this code.
**************************************************************************/

/* Use this routine if told to, or by default on non-get_unlocked()
 * platforms unless told not to.  Yikes!  Let's spell that out:
 * On a platform with getc_unlocked():
 *     By default, use getc_unlocked().
 *     If you want to use fgets() instead, #define USE_FGETS_IN_GETLINE.
 * On a platform without getc_unlocked():
 *     By default, use fgets().
 *     If you don't want to use fgets(), #define DONT_USE_FGETS_IN_GETLINE.
 */
#if !defined(USE_FGETS_IN_GETLINE) && !defined(HAVE_GETC_UNLOCKED)
#define USE_FGETS_IN_GETLINE
#endif

#if defined(DONT_USE_FGETS_IN_GETLINE) && defined(USE_FGETS_IN_GETLINE)
#undef USE_FGETS_IN_GETLINE
#endif

#ifdef USE_FGETS_IN_GETLINE
static PyObject*
getline_via_fgets(PyFileObject *f, FILE *fp)
{
/* INITBUFSIZE is the maximum line length that lets us get away with the fast
 * no-realloc, one-fgets()-call path.  Boosting it isn't free, because we have
 * to fill this much of the buffer with a known value in order to figure out
 * how much of the buffer fgets() overwrites.  So if INITBUFSIZE is larger
 * than "most" lines, we waste time filling unused buffer slots.  100 is
 * surely adequate for most peoples' email archives, chewing over source code,
 * etc -- "regular old text files".
 * MAXBUFSIZE is the maximum line length that lets us get away with the less
 * fast (but still zippy) no-realloc, two-fgets()-call path.  See above for
 * cautions about boosting that.  300 was chosen because the worst real-life
 * text-crunching job reported on Python-Dev was a mail-log crawler where over
 * half the lines were 254 chars.
 */
#define INITBUFSIZE 100
#define MAXBUFSIZE 300
    char* p;            /* temp */
    char buf[MAXBUFSIZE];
    PyObject* v;        /* the string object result */
    char* pvfree;       /* address of next free slot */
    char* pvend;    /* address one beyond last free slot */
    size_t nfree;       /* # of free buffer slots; pvend-pvfree */
    size_t total_v_size;  /* total # of slots in buffer */
    size_t increment;           /* amount to increment the buffer */
    size_t prev_v_size;

    /* Optimize for normal case:  avoid _PyString_Resize if at all
     * possible via first reading into stack buffer "buf".
     */
    total_v_size = INITBUFSIZE;         /* start small and pray */
    pvfree = buf;
    for (;;) {
        FILE_BEGIN_ALLOW_THREADS(f)
        pvend = buf + total_v_size;
        nfree = pvend - pvfree;
        memset(pvfree, '\n', nfree);
        assert(nfree < INT_MAX); /* Should be atmost MAXBUFSIZE */
        p = fgets(pvfree, (int)nfree, fp);
        FILE_END_ALLOW_THREADS(f)

        if (p == NULL) {
            clearerr(fp);
            if (PyErr_CheckSignals())
                return NULL;
            v = PyString_FromStringAndSize(buf, pvfree - buf);
            return v;
        }
        /* fgets read *something* */
        p = memchr(pvfree, '\n', nfree);
        if (p != NULL) {
            /* Did the \n come from fgets or from us?
             * Since fgets stops at the first \n, and then writes
             * \0, if it's from fgets a \0 must be next.  But if
             * that's so, it could not have come from us, since
             * the \n's we filled the buffer with have only more
             * \n's to the right.
             */
            if (p+1 < pvend && *(p+1) == '\0') {
                /* It's from fgets:  we win!  In particular,
                 * we haven't done any mallocs yet, and can
                 * build the final result on the first try.
                 */
                ++p;                    /* include \n from fgets */
            }
            else {
                /* Must be from us:  fgets didn't fill the
                 * buffer and didn't find a newline, so it
                 * must be the last and newline-free line of
                 * the file.
                 */
                assert(p > pvfree && *(p-1) == '\0');
                --p;                    /* don't include \0 from fgets */
            }
            v = PyString_FromStringAndSize(buf, p - buf);
            return v;
        }
        /* yuck:  fgets overwrote all the newlines, i.e. the entire
         * buffer.  So this line isn't over yet, or maybe it is but
         * we're exactly at EOF.  If we haven't already, try using the
         * rest of the stack buffer.
         */
        assert(*(pvend-1) == '\0');
        if (pvfree == buf) {
            pvfree = pvend - 1;                 /* overwrite trailing null */
            total_v_size = MAXBUFSIZE;
        }
        else
            break;
    }

    /* The stack buffer isn't big enough; malloc a string object and read
     * into its buffer.
     */
    total_v_size = MAXBUFSIZE << 1;
    v = PyString_FromStringAndSize((char*)NULL, (int)total_v_size);
    if (v == NULL)
        return v;
    /* copy over everything except the last null byte */
    memcpy(BUF(v), buf, MAXBUFSIZE-1);
    pvfree = BUF(v) + MAXBUFSIZE - 1;

    /* Keep reading stuff into v; if it ever ends successfully, break
     * after setting p one beyond the end of the line.  The code here is
     * very much like the code above, except reads into v's buffer; see
     * the code above for detailed comments about the logic.
     */
    for (;;) {
        FILE_BEGIN_ALLOW_THREADS(f)
        pvend = BUF(v) + total_v_size;
        nfree = pvend - pvfree;
        memset(pvfree, '\n', nfree);
        assert(nfree < INT_MAX);
        p = fgets(pvfree, (int)nfree, fp);
        FILE_END_ALLOW_THREADS(f)

        if (p == NULL) {
            clearerr(fp);
            if (PyErr_CheckSignals()) {
                Py_DECREF(v);
                return NULL;
            }
            p = pvfree;
            break;
        }
        p = memchr(pvfree, '\n', nfree);
        if (p != NULL) {
            if (p+1 < pvend && *(p+1) == '\0') {
                /* \n came from fgets */
                ++p;
                break;
            }
            /* \n came from us; last line of file, no newline */
            assert(p > pvfree && *(p-1) == '\0');
            --p;
            break;
        }
        /* expand buffer and try again */
        assert(*(pvend-1) == '\0');
        increment = total_v_size >> 2;          /* mild exponential growth */
        prev_v_size = total_v_size;
        total_v_size += increment;
        /* check for overflow */
        if (total_v_size <= prev_v_size ||
            total_v_size > PY_SSIZE_T_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                "line is longer than a Python string can hold");
            Py_DECREF(v);
            return NULL;
        }
        if (_PyString_Resize(&v, (int)total_v_size) < 0)
            return NULL;
        /* overwrite the trailing null byte */
        pvfree = BUF(v) + (prev_v_size - 1);
    }
    if (BUF(v) + total_v_size != p && _PyString_Resize(&v, p - BUF(v)))
        return NULL;
    return v;
#undef INITBUFSIZE
#undef MAXBUFSIZE
}
#endif  /* ifdef USE_FGETS_IN_GETLINE */

/* Internal routine to get a line.
   Size argument interpretation:
   > 0: max length;
   <= 0: read arbitrary line
*/

static PyObject *
get_line(PyFileObject *f, int n)
{
    FILE *fp = f->f_fp;
    int c;
    char *buf, *end;
    size_t total_v_size;        /* total # of slots in buffer */
    size_t used_v_size;         /* # used slots in buffer */
    size_t increment;       /* amount to increment the buffer */
    PyObject *v;
    int newlinetypes = f->f_newlinetypes;
    int skipnextlf = f->f_skipnextlf;
    int univ_newline = f->f_univ_newline;

#if defined(USE_FGETS_IN_GETLINE)
    if (n <= 0 && !univ_newline )
        return getline_via_fgets(f, fp);
#endif
    total_v_size = n > 0 ? n : 100;
    v = PyString_FromStringAndSize((char *)NULL, total_v_size);
    if (v == NULL)
        return NULL;
    buf = BUF(v);
    end = buf + total_v_size;

    for (;;) {
        FILE_BEGIN_ALLOW_THREADS(f)
        FLOCKFILE(fp);
        if (univ_newline) {
            c = 'x'; /* Shut up gcc warning */
            while ( buf != end && (c = GETC(fp)) != EOF ) {
                if (skipnextlf ) {
                    skipnextlf = 0;
                    if (c == '\n') {
                        /* Seeing a \n here with
                         * skipnextlf true means we
                         * saw a \r before.
                         */
                        newlinetypes |= NEWLINE_CRLF;
                        c = GETC(fp);
                        if (c == EOF) break;
                    } else {
                        newlinetypes |= NEWLINE_CR;
                    }
                }
                if (c == '\r') {
                    skipnextlf = 1;
                    c = '\n';
                } else if ( c == '\n')
                    newlinetypes |= NEWLINE_LF;
                *buf++ = c;
                if (c == '\n') break;
            }
            if (c == EOF) {
                if (ferror(fp) && errno == EINTR) {
                    FUNLOCKFILE(fp);
                    FILE_ABORT_ALLOW_THREADS(f)
                    f->f_newlinetypes = newlinetypes;
                    f->f_skipnextlf = skipnextlf;

                    if (PyErr_CheckSignals()) {
                        Py_DECREF(v);
                        return NULL;
                    }
                    /* We executed Python signal handlers and got no exception.
                     * Now back to reading the line where we left off. */
                    clearerr(fp);
                    continue;
                }
                if (skipnextlf)
                    newlinetypes |= NEWLINE_CR;
            }
        } else /* If not universal newlines use the normal loop */
        while ((c = GETC(fp)) != EOF &&
               (*buf++ = c) != '\n' &&
            buf != end)
            ;
        FUNLOCKFILE(fp);
        FILE_END_ALLOW_THREADS(f)
        f->f_newlinetypes = newlinetypes;
        f->f_skipnextlf = skipnextlf;
        if (c == '\n')
            break;
        if (c == EOF) {
            if (ferror(fp)) {
                if (errno == EINTR) {
                    if (PyErr_CheckSignals()) {
                        Py_DECREF(v);
                        return NULL;
                    }
                    /* We executed Python signal handlers and got no exception.
                     * Now back to reading the line where we left off. */
                    clearerr(fp);
                    continue;
                }
                PyErr_SetFromErrno(PyExc_IOError);
                clearerr(fp);
                Py_DECREF(v);
                return NULL;
            }
            clearerr(fp);
            if (PyErr_CheckSignals()) {
                Py_DECREF(v);
                return NULL;
            }
            break;
        }
        /* Must be because buf == end */
        if (n > 0)
            break;
        used_v_size = total_v_size;
        increment = total_v_size >> 2; /* mild exponential growth */
        total_v_size += increment;
        if (total_v_size > PY_SSIZE_T_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                "line is longer than a Python string can hold");
            Py_DECREF(v);
            return NULL;
        }
        if (_PyString_Resize(&v, total_v_size) < 0)
            return NULL;
        buf = BUF(v) + used_v_size;
        end = BUF(v) + total_v_size;
    }

    used_v_size = buf - BUF(v);
    if (used_v_size != total_v_size && _PyString_Resize(&v, used_v_size))
        return NULL;
    return v;
}

/* External C interface */

PyObject *
PyFile_GetLine(PyObject *f, int n)
{
    PyObject *result;

    if (f == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (PyFile_Check(f)) {
        PyFileObject *fo = (PyFileObject *)f;
        if (fo->f_fp == NULL)
            return err_closed();
        if (!fo->readable)
            return err_mode("reading");
        /* refuse to mix with f.next() */
        if (fo->f_buf != NULL &&
            (fo->f_bufend - fo->f_bufptr) > 0 &&
            fo->f_buf[0] != '\0')
            return err_iterbuffered();
        result = get_line(fo, n);
    }
    else {
        PyObject *reader;
        PyObject *args;

        reader = PyObject_GetAttrString(f, "readline");
        if (reader == NULL)
            return NULL;
        if (n <= 0)
            args = PyTuple_New(0);
        else
            args = Py_BuildValue("(i)", n);
        if (args == NULL) {
            Py_DECREF(reader);
            return NULL;
        }
        result = PyEval_CallObject(reader, args);
        Py_DECREF(reader);
        Py_DECREF(args);
        if (result != NULL && !PyString_Check(result) &&
            !PyUnicode_Check(result)) {
            Py_DECREF(result);
            result = NULL;
            PyErr_SetString(PyExc_TypeError,
                       "object.readline() returned non-string");
        }
    }

    if (n < 0 && result != NULL && PyString_Check(result)) {
        char *s = PyString_AS_STRING(result);
        Py_ssize_t len = PyString_GET_SIZE(result);
        if (len == 0) {
            Py_DECREF(result);
            result = NULL;
            PyErr_SetString(PyExc_EOFError,
                            "EOF when reading a line");
        }
        else if (s[len-1] == '\n') {
            if (result->ob_refcnt == 1) {
                if (_PyString_Resize(&result, len-1))
                    return NULL;
            }
            else {
                PyObject *v;
                v = PyString_FromStringAndSize(s, len-1);
                Py_DECREF(result);
                result = v;
            }
        }
    }
#ifdef Py_USING_UNICODE
    if (n < 0 && result != NULL && PyUnicode_Check(result)) {
        Py_UNICODE *s = PyUnicode_AS_UNICODE(result);
        Py_ssize_t len = PyUnicode_GET_SIZE(result);
        if (len == 0) {
            Py_DECREF(result);
            result = NULL;
            PyErr_SetString(PyExc_EOFError,
                            "EOF when reading a line");
        }
        else if (s[len-1] == '\n') {
            if (result->ob_refcnt == 1)
                PyUnicode_Resize(&result, len-1);
            else {
                PyObject *v;
                v = PyUnicode_FromUnicode(s, len-1);
                Py_DECREF(result);
                result = v;
            }
        }
    }
#endif
    return result;
}

/* Python method */

static PyObject *
file_readline(PyFileObject *f, PyObject *args)
{
    int n = -1;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->readable)
        return err_mode("reading");
    /* refuse to mix with f.next() */
    if (f->f_buf != NULL &&
        (f->f_bufend - f->f_bufptr) > 0 &&
        f->f_buf[0] != '\0')
        return err_iterbuffered();
    if (!PyArg_ParseTuple(args, "|i:readline", &n))
        return NULL;
    if (n == 0)
        return PyString_FromString("");
    if (n < 0)
        n = 0;
    return get_line(f, n);
}

static PyObject *
file_readlines(PyFileObject *f, PyObject *args)
{
    long sizehint = 0;
    PyObject *list = NULL;
    PyObject *line;
    char small_buffer[SMALLCHUNK];
    char *buffer = small_buffer;
    size_t buffersize = SMALLCHUNK;
    PyObject *big_buffer = NULL;
    size_t nfilled = 0;
    size_t nread;
    size_t totalread = 0;
    char *p, *q, *end;
    int err;
    int shortread = 0;  /* bool, did the previous read come up short? */

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->readable)
        return err_mode("reading");
    /* refuse to mix with f.next() */
    if (f->f_buf != NULL &&
        (f->f_bufend - f->f_bufptr) > 0 &&
        f->f_buf[0] != '\0')
        return err_iterbuffered();
    if (!PyArg_ParseTuple(args, "|l:readlines", &sizehint))
        return NULL;
    if ((list = PyList_New(0)) == NULL)
        return NULL;
    for (;;) {
        if (shortread)
            nread = 0;
        else {
            FILE_BEGIN_ALLOW_THREADS(f)
            errno = 0;
            nread = Py_UniversalNewlineFread(buffer+nfilled,
                buffersize-nfilled, f->f_fp, (PyObject *)f);
            FILE_END_ALLOW_THREADS(f)
            shortread = (nread < buffersize-nfilled);
        }
        if (nread == 0) {
            sizehint = 0;
            if (!ferror(f->f_fp))
                break;
            if (errno == EINTR) {
                if (PyErr_CheckSignals()) {
                    goto error;
                }
                clearerr(f->f_fp);
                shortread = 0;
                continue;
            }
            PyErr_SetFromErrno(PyExc_IOError);
            clearerr(f->f_fp);
            goto error;
        }
        totalread += nread;
        p = (char *)memchr(buffer+nfilled, '\n', nread);
        if (p == NULL) {
            /* Need a larger buffer to fit this line */
            nfilled += nread;
            buffersize *= 2;
            if (buffersize > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_OverflowError,
                "line is longer than a Python string can hold");
                goto error;
            }
            if (big_buffer == NULL) {
                /* Create the big buffer */
                big_buffer = PyString_FromStringAndSize(
                    NULL, buffersize);
                if (big_buffer == NULL)
                    goto error;
                buffer = PyString_AS_STRING(big_buffer);
                memcpy(buffer, small_buffer, nfilled);
            }
            else {
                /* Grow the big buffer */
                if ( _PyString_Resize(&big_buffer, buffersize) < 0 )
                    goto error;
                buffer = PyString_AS_STRING(big_buffer);
            }
            continue;
        }
        end = buffer+nfilled+nread;
        q = buffer;
        do {
            /* Process complete lines */
            p++;
            line = PyString_FromStringAndSize(q, p-q);
            if (line == NULL)
                goto error;
            err = PyList_Append(list, line);
            Py_DECREF(line);
            if (err != 0)
                goto error;
            q = p;
            p = (char *)memchr(q, '\n', end-q);
        } while (p != NULL);
        /* Move the remaining incomplete line to the start */
        nfilled = end-q;
        memmove(buffer, q, nfilled);
        if (sizehint > 0)
            if (totalread >= (size_t)sizehint)
                break;
    }
    if (nfilled != 0) {
        /* Partial last line */
        line = PyString_FromStringAndSize(buffer, nfilled);
        if (line == NULL)
            goto error;
        if (sizehint > 0) {
            /* Need to complete the last line */
            PyObject *rest = get_line(f, 0);
            if (rest == NULL) {
                Py_DECREF(line);
                goto error;
            }
            PyString_Concat(&line, rest);
            Py_DECREF(rest);
            if (line == NULL)
                goto error;
        }
        err = PyList_Append(list, line);
        Py_DECREF(line);
        if (err != 0)
            goto error;
    }

cleanup:
    Py_XDECREF(big_buffer);
    return list;

error:
    Py_CLEAR(list);
    goto cleanup;
}

static PyObject *
file_write(PyFileObject *f, PyObject *args)
{
    Py_buffer pbuf;
    const char *s;
    Py_ssize_t n, n2;
    PyObject *encoded = NULL;
    int err_flag = 0, err;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->writable)
        return err_mode("writing");
    if (f->f_binary) {
        if (!PyArg_ParseTuple(args, "s*", &pbuf))
            return NULL;
        s = pbuf.buf;
        n = pbuf.len;
    }
    else {
        PyObject *text;
        if (!PyArg_ParseTuple(args, "O", &text))
            return NULL;

        if (PyString_Check(text)) {
            s = PyString_AS_STRING(text);
            n = PyString_GET_SIZE(text);
#ifdef Py_USING_UNICODE
        } else if (PyUnicode_Check(text)) {
            const char *encoding, *errors;
            if (f->f_encoding != Py_None)
                encoding = PyString_AS_STRING(f->f_encoding);
            else
                encoding = PyUnicode_GetDefaultEncoding();
            if (f->f_errors != Py_None)
                errors = PyString_AS_STRING(f->f_errors);
            else
                errors = "strict";
            encoded = PyUnicode_AsEncodedString(text, encoding, errors);
            if (encoded == NULL)
                return NULL;
            s = PyString_AS_STRING(encoded);
            n = PyString_GET_SIZE(encoded);
#endif
        } else {
            if (PyObject_AsCharBuffer(text, &s, &n))
                return NULL;
        }
    }
    f->f_softspace = 0;
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    n2 = fwrite(s, 1, n, f->f_fp);
    if (n2 != n || ferror(f->f_fp)) {
        err_flag = 1;
        err = errno;
    }
    FILE_END_ALLOW_THREADS(f)
    Py_XDECREF(encoded);
    if (f->f_binary)
        PyBuffer_Release(&pbuf);
    if (err_flag) {
        errno = err;
        PyErr_SetFromErrno(PyExc_IOError);
        clearerr(f->f_fp);
        return NULL;
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
file_writelines(PyFileObject *f, PyObject *seq)
{
#define CHUNKSIZE 1000
    PyObject *list, *line;
    PyObject *it;       /* iter(seq) */
    PyObject *result;
    int index, islist;
    Py_ssize_t i, j, nwritten, len;

    assert(seq != NULL);
    if (f->f_fp == NULL)
        return err_closed();
    if (!f->writable)
        return err_mode("writing");

    result = NULL;
    list = NULL;
    islist = PyList_Check(seq);
    if  (islist)
        it = NULL;
    else {
        it = PyObject_GetIter(seq);
        if (it == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "writelines() requires an iterable argument");
            return NULL;
        }
        /* From here on, fail by going to error, to reclaim "it". */
        list = PyList_New(CHUNKSIZE);
        if (list == NULL)
            goto error;
    }

    /* Strategy: slurp CHUNKSIZE lines into a private list,
       checking that they are all strings, then write that list
       without holding the interpreter lock, then come back for more. */
    for (index = 0; ; index += CHUNKSIZE) {
        if (islist) {
            Py_XDECREF(list);
            list = PyList_GetSlice(seq, index, index+CHUNKSIZE);
            if (list == NULL)
                goto error;
            j = PyList_GET_SIZE(list);
        }
        else {
            for (j = 0; j < CHUNKSIZE; j++) {
                line = PyIter_Next(it);
                if (line == NULL) {
                    if (PyErr_Occurred())
                        goto error;
                    break;
                }
                PyList_SetItem(list, j, line);
            }
            /* The iterator might have closed the file on us. */
            if (f->f_fp == NULL) {
                err_closed();
                goto error;
            }
        }
        if (j == 0)
            break;

        /* Check that all entries are indeed strings. If not,
           apply the same rules as for file.write() and
           convert the results to strings. This is slow, but
           seems to be the only way since all conversion APIs
           could potentially execute Python code. */
        for (i = 0; i < j; i++) {
            PyObject *v = PyList_GET_ITEM(list, i);
            if (!PyString_Check(v)) {
                const char *buffer;
                int res;
                if (f->f_binary) {
                    res = PyObject_AsReadBuffer(v, (const void**)&buffer, &len);
                } else {
                    res = PyObject_AsCharBuffer(v, &buffer, &len);
                }
                if (res) {
                    PyErr_SetString(PyExc_TypeError,
            "writelines() argument must be a sequence of strings");
                            goto error;
                }
                line = PyString_FromStringAndSize(buffer,
                                                  len);
                if (line == NULL)
                    goto error;
                Py_DECREF(v);
                PyList_SET_ITEM(list, i, line);
            }
        }

        /* Since we are releasing the global lock, the
           following code may *not* execute Python code. */
        f->f_softspace = 0;
        FILE_BEGIN_ALLOW_THREADS(f)
        errno = 0;
        for (i = 0; i < j; i++) {
            line = PyList_GET_ITEM(list, i);
            len = PyString_GET_SIZE(line);
            nwritten = fwrite(PyString_AS_STRING(line),
                              1, len, f->f_fp);
            if (nwritten != len) {
                FILE_ABORT_ALLOW_THREADS(f)
                PyErr_SetFromErrno(PyExc_IOError);
                clearerr(f->f_fp);
                goto error;
            }
        }
        FILE_END_ALLOW_THREADS(f)

        if (j < CHUNKSIZE)
            break;
    }

    Py_INCREF(Py_None);
    result = Py_None;
  error:
    Py_XDECREF(list);
    Py_XDECREF(it);
    return result;
#undef CHUNKSIZE
}

static PyObject *
file_self(PyFileObject *f)
{
    if (f->f_fp == NULL)
        return err_closed();
    Py_INCREF(f);
    return (PyObject *)f;
}

static PyObject *
file_xreadlines(PyFileObject *f)
{
    if (PyErr_WarnPy3k("f.xreadlines() not supported in 3.x, "
                       "try 'for line in f' instead", 1) < 0)
           return NULL;
    return file_self(f);
}

static PyObject *
file_exit(PyObject *f, PyObject *args)
{
    PyObject *ret = PyObject_CallMethod(f, "close", NULL);
    if (!ret)
        /* If error occurred, pass through */
        return NULL;
    Py_DECREF(ret);
    /* We cannot return the result of close since a true
     * value will be interpreted as "yes, swallow the
     * exception if one was raised inside the with block". */
    Py_RETURN_NONE;
}

PyDoc_STRVAR(readline_doc,
"readline([size]) -> next line from the file, as a string.\n"
"\n"
"Retain newline.  A non-negative size argument limits the maximum\n"
"number of bytes to return (an incomplete line may be returned then).\n"
"Return an empty string at EOF.");

PyDoc_STRVAR(read_doc,
"read([size]) -> read at most size bytes, returned as a string.\n"
"\n"
"If the size argument is negative or omitted, read until EOF is reached.\n"
"Notice that when in non-blocking mode, less data than what was requested\n"
"may be returned, even if no size parameter was given.");

PyDoc_STRVAR(write_doc,
"write(str) -> None.  Write string str to file.\n"
"\n"
"Note that due to buffering, flush() or close() may be needed before\n"
"the file on disk reflects the data written.");

PyDoc_STRVAR(fileno_doc,
"fileno() -> integer \"file descriptor\".\n"
"\n"
"This is needed for lower-level file interfaces, such os.read().");

PyDoc_STRVAR(seek_doc,
"seek(offset[, whence]) -> None.  Move to new file position.\n"
"\n"
"Argument offset is a byte count.  Optional argument whence defaults to\n"
"0 (offset from start of file, offset should be >= 0); other values are 1\n"
"(move relative to current position, positive or negative), and 2 (move\n"
"relative to end of file, usually negative, although many platforms allow\n"
"seeking beyond the end of a file).  If the file is opened in text mode,\n"
"only offsets returned by tell() are legal.  Use of other offsets causes\n"
"undefined behavior."
"\n"
"Note that not all file objects are seekable.");

#ifdef HAVE_FTRUNCATE
PyDoc_STRVAR(truncate_doc,
"truncate([size]) -> None.  Truncate the file to at most size bytes.\n"
"\n"
"Size defaults to the current file position, as returned by tell().");
#endif

PyDoc_STRVAR(tell_doc,
"tell() -> current file position, an integer (may be a long integer).");

PyDoc_STRVAR(readinto_doc,
"readinto() -> Undocumented.  Don't use this; it may go away.");

PyDoc_STRVAR(readlines_doc,
"readlines([size]) -> list of strings, each a line from the file.\n"
"\n"
"Call readline() repeatedly and return a list of the lines so read.\n"
"The optional size argument, if given, is an approximate bound on the\n"
"total number of bytes in the lines returned.");

PyDoc_STRVAR(xreadlines_doc,
"xreadlines() -> returns self.\n"
"\n"
"For backward compatibility. File objects now include the performance\n"
"optimizations previously implemented in the xreadlines module.");

PyDoc_STRVAR(writelines_doc,
"writelines(sequence_of_strings) -> None.  Write the strings to the file.\n"
"\n"
"Note that newlines are not added.  The sequence can be any iterable object\n"
"producing strings. This is equivalent to calling write() for each string.");

PyDoc_STRVAR(flush_doc,
"flush() -> None.  Flush the internal I/O buffer.");

PyDoc_STRVAR(close_doc,
"close() -> None or (perhaps) an integer.  Close the file.\n"
"\n"
"Sets data attribute .closed to True.  A closed file cannot be used for\n"
"further I/O operations.  close() may be called more than once without\n"
"error.  Some kinds of file objects (for example, opened by popen())\n"
"may return an exit status upon closing.");

PyDoc_STRVAR(isatty_doc,
"isatty() -> true or false.  True if the file is connected to a tty device.");

PyDoc_STRVAR(enter_doc,
             "__enter__() -> self.");

PyDoc_STRVAR(exit_doc,
             "__exit__(*excinfo) -> None.  Closes the file.");

static PyMethodDef file_methods[] = {
    {"readline",  (PyCFunction)file_readline, METH_VARARGS, readline_doc},
    {"read",      (PyCFunction)file_read,     METH_VARARGS, read_doc},
    {"write",     (PyCFunction)file_write,    METH_VARARGS, write_doc},
    {"fileno",    (PyCFunction)file_fileno,   METH_NOARGS,  fileno_doc},
    {"seek",      (PyCFunction)file_seek,     METH_VARARGS, seek_doc},
#ifdef HAVE_FTRUNCATE
    {"truncate",  (PyCFunction)file_truncate, METH_VARARGS, truncate_doc},
#endif
    {"tell",      (PyCFunction)file_tell,     METH_NOARGS,  tell_doc},
    {"readinto",  (PyCFunction)file_readinto, METH_VARARGS, readinto_doc},
    {"readlines", (PyCFunction)file_readlines, METH_VARARGS, readlines_doc},
    {"xreadlines",(PyCFunction)file_xreadlines, METH_NOARGS, xreadlines_doc},
    {"writelines",(PyCFunction)file_writelines, METH_O,     writelines_doc},
    {"flush",     (PyCFunction)file_flush,    METH_NOARGS,  flush_doc},
    {"close",     (PyCFunction)file_close,    METH_NOARGS,  close_doc},
    {"isatty",    (PyCFunction)file_isatty,   METH_NOARGS,  isatty_doc},
    {"__enter__", (PyCFunction)file_self,     METH_NOARGS,  enter_doc},
    {"__exit__",  (PyCFunction)file_exit,     METH_VARARGS, exit_doc},
    {NULL,            NULL}             /* sentinel */
};

#define OFF(x) offsetof(PyFileObject, x)

static PyMemberDef file_memberlist[] = {
    {"mode",            T_OBJECT,       OFF(f_mode),    RO,
     "file mode ('r', 'U', 'w', 'a', possibly with 'b' or '+' added)"},
    {"name",            T_OBJECT,       OFF(f_name),    RO,
     "file name"},
    {"encoding",        T_OBJECT,       OFF(f_encoding),        RO,
     "file encoding"},
    {"errors",          T_OBJECT,       OFF(f_errors),  RO,
     "Unicode error handler"},
    /* getattr(f, "closed") is implemented without this table */
    {NULL}      /* Sentinel */
};

static PyObject *
get_closed(PyFileObject *f, void *closure)
{
    return PyBool_FromLong((long)(f->f_fp == 0));
}
static PyObject *
get_newlines(PyFileObject *f, void *closure)
{
    switch (f->f_newlinetypes) {
    case NEWLINE_UNKNOWN:
        Py_INCREF(Py_None);
        return Py_None;
    case NEWLINE_CR:
        return PyString_FromString("\r");
    case NEWLINE_LF:
        return PyString_FromString("\n");
    case NEWLINE_CR|NEWLINE_LF:
        return Py_BuildValue("(ss)", "\r", "\n");
    case NEWLINE_CRLF:
        return PyString_FromString("\r\n");
    case NEWLINE_CR|NEWLINE_CRLF:
        return Py_BuildValue("(ss)", "\r", "\r\n");
    case NEWLINE_LF|NEWLINE_CRLF:
        return Py_BuildValue("(ss)", "\n", "\r\n");
    case NEWLINE_CR|NEWLINE_LF|NEWLINE_CRLF:
        return Py_BuildValue("(sss)", "\r", "\n", "\r\n");
    default:
        PyErr_Format(PyExc_SystemError,
                     "Unknown newlines value 0x%x\n",
                     f->f_newlinetypes);
        return NULL;
    }
}

static PyObject *
get_softspace(PyFileObject *f, void *closure)
{
    if (PyErr_WarnPy3k("file.softspace not supported in 3.x", 1) < 0)
        return NULL;
    return PyInt_FromLong(f->f_softspace);
}

static int
set_softspace(PyFileObject *f, PyObject *value)
{
    int new;
    if (PyErr_WarnPy3k("file.softspace not supported in 3.x", 1) < 0)
        return -1;

    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "can't delete softspace attribute");
        return -1;
    }

    new = PyInt_AsLong(value);
    if (new == -1 && PyErr_Occurred())
        return -1;
    f->f_softspace = new;
    return 0;
}

static PyGetSetDef file_getsetlist[] = {
    {"closed", (getter)get_closed, NULL, "True if the file is closed"},
    {"newlines", (getter)get_newlines, NULL,
     "end-of-line convention used in this file"},
    {"softspace", (getter)get_softspace, (setter)set_softspace,
     "flag indicating that a space needs to be printed; used by print"},
    {0},
};

static void
drop_readahead(PyFileObject *f)
{
    if (f->f_buf != NULL) {
        PyMem_Free(f->f_buf);
        f->f_buf = NULL;
    }
}

/* Make sure that file has a readahead buffer with at least one byte
   (unless at EOF) and no more than bufsize.  Returns negative value on
   error, will set MemoryError if bufsize bytes cannot be allocated. */
static int
readahead(PyFileObject *f, Py_ssize_t bufsize)
{
    Py_ssize_t chunksize;

    if (f->f_buf != NULL) {
        if( (f->f_bufend - f->f_bufptr) >= 1)
            return 0;
        else
            drop_readahead(f);
    }
    if ((f->f_buf = (char *)PyMem_Malloc(bufsize)) == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    FILE_BEGIN_ALLOW_THREADS(f)
    errno = 0;
    chunksize = Py_UniversalNewlineFread(
        f->f_buf, bufsize, f->f_fp, (PyObject *)f);
    FILE_END_ALLOW_THREADS(f)
    if (chunksize == 0) {
        if (ferror(f->f_fp)) {
            PyErr_SetFromErrno(PyExc_IOError);
            clearerr(f->f_fp);
            drop_readahead(f);
            return -1;
        }
    }
    f->f_bufptr = f->f_buf;
    f->f_bufend = f->f_buf + chunksize;
    return 0;
}

/* Used by file_iternext.  The returned string will start with 'skip'
   uninitialized bytes followed by the remainder of the line. Don't be
   horrified by the recursive call: maximum recursion depth is limited by
   logarithmic buffer growth to about 50 even when reading a 1gb line. */

static PyStringObject *
readahead_get_line_skip(PyFileObject *f, Py_ssize_t skip, Py_ssize_t bufsize)
{
    PyStringObject* s;
    char *bufptr;
    char *buf;
    Py_ssize_t len;

    if (f->f_buf == NULL)
        if (readahead(f, bufsize) < 0)
            return NULL;

    len = f->f_bufend - f->f_bufptr;
    if (len == 0)
        return (PyStringObject *)
            PyString_FromStringAndSize(NULL, skip);
    bufptr = (char *)memchr(f->f_bufptr, '\n', len);
    if (bufptr != NULL) {
        bufptr++;                               /* Count the '\n' */
        len = bufptr - f->f_bufptr;
        s = (PyStringObject *)
            PyString_FromStringAndSize(NULL, skip + len);
        if (s == NULL)
            return NULL;
        memcpy(PyString_AS_STRING(s) + skip, f->f_bufptr, len);
        f->f_bufptr = bufptr;
        if (bufptr == f->f_bufend)
            drop_readahead(f);
    } else {
        bufptr = f->f_bufptr;
        buf = f->f_buf;
        f->f_buf = NULL;                /* Force new readahead buffer */
        assert(len <= PY_SSIZE_T_MAX - skip);
        s = readahead_get_line_skip(f, skip + len, bufsize + (bufsize>>2));
        if (s == NULL) {
            PyMem_Free(buf);
            return NULL;
        }
        memcpy(PyString_AS_STRING(s) + skip, bufptr, len);
        PyMem_Free(buf);
    }
    return s;
}

/* A larger buffer size may actually decrease performance. */
#define READAHEAD_BUFSIZE 8192

static PyObject *
file_iternext(PyFileObject *f)
{
    PyStringObject* l;

    if (f->f_fp == NULL)
        return err_closed();
    if (!f->readable)
        return err_mode("reading");

    l = readahead_get_line_skip(f, 0, READAHEAD_BUFSIZE);
    if (l == NULL || PyString_GET_SIZE(l) == 0) {
        Py_XDECREF(l);
        return NULL;
    }
    return (PyObject *)l;
}


static PyObject *
file_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self;
    static PyObject *not_yet_string;

    assert(type != NULL && type->tp_alloc != NULL);

    if (not_yet_string == NULL) {
        not_yet_string = PyString_InternFromString("<uninitialized file>");
        if (not_yet_string == NULL)
            return NULL;
    }

    self = type->tp_alloc(type, 0);
    if (self != NULL) {
        /* Always fill in the name and mode, so that nobody else
           needs to special-case NULLs there. */
        Py_INCREF(not_yet_string);
        ((PyFileObject *)self)->f_name = not_yet_string;
        Py_INCREF(not_yet_string);
        ((PyFileObject *)self)->f_mode = not_yet_string;
        Py_INCREF(Py_None);
        ((PyFileObject *)self)->f_encoding = Py_None;
        Py_INCREF(Py_None);
        ((PyFileObject *)self)->f_errors = Py_None;
        ((PyFileObject *)self)->weakreflist = NULL;
        ((PyFileObject *)self)->unlocked_count = 0;
    }
    return self;
}

static int
file_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    PyFileObject *foself = (PyFileObject *)self;
    int ret = 0;
    static char *kwlist[] = {"name", "mode", "buffering", 0};
    char *name = NULL;
    char *mode = "r";
    int bufsize = -1;
    int wideargument = 0;
#ifdef MS_WINDOWS
    PyObject *po;
#endif

    assert(PyFile_Check(self));
    if (foself->f_fp != NULL) {
        /* Have to close the existing file first. */
        PyObject *closeresult = file_close(foself);
        if (closeresult == NULL)
            return -1;
        Py_DECREF(closeresult);
    }

#ifdef MS_WINDOWS
    if (PyArg_ParseTupleAndKeywords(args, kwds, "U|si:file",
                                    kwlist, &po, &mode, &bufsize)) {
        wideargument = 1;
        if (fill_file_fields(foself, NULL, po, mode,
                             fclose) == NULL)
            goto Error;
    } else {
        /* Drop the argument parsing error as narrow
           strings are also valid. */
        PyErr_Clear();
    }
#endif

    if (!wideargument) {
        PyObject *o_name;

        if (!PyArg_ParseTupleAndKeywords(args, kwds, "et|si:file", kwlist,
                                         Py_FileSystemDefaultEncoding,
                                         &name,
                                         &mode, &bufsize))
            return -1;

        /* We parse again to get the name as a PyObject */
        if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|si:file",
                                         kwlist, &o_name, &mode,
                                         &bufsize))
            goto Error;

        if (fill_file_fields(foself, NULL, o_name, mode,
                             fclose) == NULL)
            goto Error;
    }
    if (open_the_file(foself, name, mode) == NULL)
        goto Error;
    foself->f_setbuf = NULL;
    PyFile_SetBufSize(self, bufsize);
    goto Done;

Error:
    ret = -1;
    /* fall through */
Done:
    PyMem_Free(name); /* free the encoded string */
    return ret;
}

PyDoc_VAR(file_doc) =
PyDoc_STR(
"file(name[, mode[, buffering]]) -> file object\n"
"\n"
"Open a file.  The mode can be 'r', 'w' or 'a' for reading (default),\n"
"writing or appending.  The file will be created if it doesn't exist\n"
"when opened for writing or appending; it will be truncated when\n"
"opened for writing.  Add a 'b' to the mode for binary files.\n"
"Add a '+' to the mode to allow simultaneous reading and writing.\n"
"If the buffering argument is given, 0 means unbuffered, 1 means line\n"
"buffered, and larger numbers specify the buffer size.  The preferred way\n"
"to open a file is with the builtin open() function.\n"
)
PyDoc_STR(
"Add a 'U' to mode to open the file for input with universal newline\n"
"support.  Any line ending in the input file will be seen as a '\\n'\n"
"in Python.  Also, a file so opened gains the attribute 'newlines';\n"
"the value for this attribute is one of None (no newline read yet),\n"
"'\\r', '\\n', '\\r\\n' or a tuple containing all the newline types seen.\n"
"\n"
"'U' cannot be combined with 'w' or '+' mode.\n"
);

PyTypeObject PyFile_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "file",
    sizeof(PyFileObject),
    0,
    (destructor)file_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    (reprfunc)file_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    /* softspace is writable:  we must supply tp_setattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_WEAKREFS, /* tp_flags */
    file_doc,                                   /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(PyFileObject, weakreflist),        /* tp_weaklistoffset */
    (getiterfunc)file_self,                     /* tp_iter */
    (iternextfunc)file_iternext,                /* tp_iternext */
    file_methods,                               /* tp_methods */
    file_memberlist,                            /* tp_members */
    file_getsetlist,                            /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    file_init,                                  /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    file_new,                                   /* tp_new */
    PyObject_Del,                           /* tp_free */
};

/* Interface for the 'soft space' between print items. */

int
PyFile_SoftSpace(PyObject *f, int newflag)
{
    long oldflag = 0;
    if (f == NULL) {
        /* Do nothing */
    }
    else if (PyFile_Check(f)) {
        oldflag = ((PyFileObject *)f)->f_softspace;
        ((PyFileObject *)f)->f_softspace = newflag;
    }
    else {
        PyObject *v;
        v = PyObject_GetAttrString(f, "softspace");
        if (v == NULL)
            PyErr_Clear();
        else {
            if (PyInt_Check(v))
                oldflag = PyInt_AsLong(v);
            assert(oldflag < INT_MAX);
            Py_DECREF(v);
        }
        v = PyInt_FromLong((long)newflag);
        if (v == NULL)
            PyErr_Clear();
        else {
            if (PyObject_SetAttrString(f, "softspace", v) != 0)
                PyErr_Clear();
            Py_DECREF(v);
        }
    }
    return (int)oldflag;
}

/* Interfaces to write objects/strings to file-like objects */

int
PyFile_WriteObject(PyObject *v, PyObject *f, int flags)
{
    PyObject *writer, *value, *args, *result;
    if (f == NULL) {
        PyErr_SetString(PyExc_TypeError, "writeobject with NULL file");
        return -1;
    }
    else if (PyFile_Check(f)) {
        PyFileObject *fobj = (PyFileObject *) f;
#ifdef Py_USING_UNICODE
        PyObject *enc = fobj->f_encoding;
        int result;
#endif
        if (fobj->f_fp == NULL) {
            err_closed();
            return -1;
        }
#ifdef Py_USING_UNICODE
        if ((flags & Py_PRINT_RAW) &&
            PyUnicode_Check(v) && enc != Py_None) {
            char *cenc = PyString_AS_STRING(enc);
            char *errors = fobj->f_errors == Py_None ?
              "strict" : PyString_AS_STRING(fobj->f_errors);
            value = PyUnicode_AsEncodedString(v, cenc, errors);
            if (value == NULL)
                return -1;
        } else {
            value = v;
            Py_INCREF(value);
        }
        result = file_PyObject_Print(value, fobj, flags);
        Py_DECREF(value);
        return result;
#else
        return file_PyObject_Print(v, fobj, flags);
#endif
    }
    writer = PyObject_GetAttrString(f, "write");
    if (writer == NULL)
        return -1;
    if (flags & Py_PRINT_RAW) {
        if (PyUnicode_Check(v)) {
            value = v;
            Py_INCREF(value);
        } else
            value = PyObject_Str(v);
    }
    else
        value = PyObject_Repr(v);
    if (value == NULL) {
        Py_DECREF(writer);
        return -1;
    }
    args = PyTuple_Pack(1, value);
    if (args == NULL) {
        Py_DECREF(value);
        Py_DECREF(writer);
        return -1;
    }
    result = PyEval_CallObject(writer, args);
    Py_DECREF(args);
    Py_DECREF(value);
    Py_DECREF(writer);
    if (result == NULL)
        return -1;
    Py_DECREF(result);
    return 0;
}

int
PyFile_WriteString(const char *s, PyObject *f)
{

    if (f == NULL) {
        /* Should be caused by a pre-existing error */
        if (!PyErr_Occurred())
            PyErr_SetString(PyExc_SystemError,
                            "null file for PyFile_WriteString");
        return -1;
    }
    else if (PyFile_Check(f)) {
        PyFileObject *fobj = (PyFileObject *) f;
        FILE *fp = PyFile_AsFile(f);
        if (fp == NULL) {
            err_closed();
            return -1;
        }
        FILE_BEGIN_ALLOW_THREADS(fobj)
        fputs(s, fp);
        FILE_END_ALLOW_THREADS(fobj)
        return 0;
    }
    else if (!PyErr_Occurred()) {
        PyObject *v = PyString_FromString(s);
        int err;
        if (v == NULL)
            return -1;
        err = PyFile_WriteObject(v, f, Py_PRINT_RAW);
        Py_DECREF(v);
        return err;
    }
    else
        return -1;
}

/* Try to get a file-descriptor from a Python object.  If the object
   is an integer or long integer, its value is returned.  If not, the
   object's fileno() method is called if it exists; the method must return
   an integer or long integer, which is returned as the file descriptor value.
   -1 is returned on failure.
*/

int PyObject_AsFileDescriptor(PyObject *o)
{
    int fd;
    PyObject *meth;

    if (PyInt_Check(o)) {
        fd = _PyInt_AsInt(o);
    }
    else if (PyLong_Check(o)) {
        fd = _PyLong_AsInt(o);
    }
    else if ((meth = PyObject_GetAttrString(o, "fileno")) != NULL)
    {
        PyObject *fno = PyEval_CallObject(meth, NULL);
        Py_DECREF(meth);
        if (fno == NULL)
            return -1;

        if (PyInt_Check(fno)) {
            fd = _PyInt_AsInt(fno);
            Py_DECREF(fno);
        }
        else if (PyLong_Check(fno)) {
            fd = _PyLong_AsInt(fno);
            Py_DECREF(fno);
        }
        else {
            PyErr_SetString(PyExc_TypeError,
                            "fileno() returned a non-integer");
            Py_DECREF(fno);
            return -1;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "argument must be an int, or have a fileno() method.");
        return -1;
    }

    if (fd < 0) {
        PyErr_Format(PyExc_ValueError,
                     "file descriptor cannot be a negative integer (%i)",
                     fd);
        return -1;
    }
    return fd;
}

/* From here on we need access to the real fgets and fread */
#undef fgets
#undef fread

/*
** Py_UniversalNewlineFgets is an fgets variation that understands
** all of \r, \n and \r\n conventions.
** The stream should be opened in binary mode.
** If fobj is NULL the routine always does newline conversion, and
** it may peek one char ahead to gobble the second char in \r\n.
** If fobj is non-NULL it must be a PyFileObject. In this case there
** is no readahead but in stead a flag is used to skip a following
** \n on the next read. Also, if the file is open in binary mode
** the whole conversion is skipped. Finally, the routine keeps track of
** the different types of newlines seen.
** Note that we need no error handling: fgets() treats error and eof
** identically.
*/
char *
Py_UniversalNewlineFgets(char *buf, int n, FILE *stream, PyObject *fobj)
{
    char *p = buf;
    int c;
    int newlinetypes = 0;
    int skipnextlf = 0;
    int univ_newline = 1;

    if (fobj) {
        if (!PyFile_Check(fobj)) {
            errno = ENXIO;              /* What can you do... */
            return NULL;
        }
        univ_newline = ((PyFileObject *)fobj)->f_univ_newline;
        if ( !univ_newline )
            return fgets(buf, n, stream);
        newlinetypes = ((PyFileObject *)fobj)->f_newlinetypes;
        skipnextlf = ((PyFileObject *)fobj)->f_skipnextlf;
    }
    FLOCKFILE(stream);
    c = 'x'; /* Shut up gcc warning */
    while (--n > 0 && (c = GETC(stream)) != EOF ) {
        if (skipnextlf ) {
            skipnextlf = 0;
            if (c == '\n') {
                /* Seeing a \n here with skipnextlf true
                ** means we saw a \r before.
                */
                newlinetypes |= NEWLINE_CRLF;
                c = GETC(stream);
                if (c == EOF) break;
            } else {
                /*
                ** Note that c == EOF also brings us here,
                ** so we're okay if the last char in the file
                ** is a CR.
                */
                newlinetypes |= NEWLINE_CR;
            }
        }
        if (c == '\r') {
            /* A \r is translated into a \n, and we skip
            ** an adjacent \n, if any. We don't set the
            ** newlinetypes flag until we've seen the next char.
            */
            skipnextlf = 1;
            c = '\n';
        } else if ( c == '\n') {
            newlinetypes |= NEWLINE_LF;
        }
        *p++ = c;
        if (c == '\n') break;
    }
    if ( c == EOF && skipnextlf )
        newlinetypes |= NEWLINE_CR;
    FUNLOCKFILE(stream);
    *p = '\0';
    if (fobj) {
        ((PyFileObject *)fobj)->f_newlinetypes = newlinetypes;
        ((PyFileObject *)fobj)->f_skipnextlf = skipnextlf;
    } else if ( skipnextlf ) {
        /* If we have no file object we cannot save the
        ** skipnextlf flag. We have to readahead, which
        ** will cause a pause if we're reading from an
        ** interactive stream, but that is very unlikely
        ** unless we're doing something silly like
        ** execfile("/dev/tty").
        */
        c = GETC(stream);
        if ( c != '\n' )
            ungetc(c, stream);
    }
    if (p == buf)
        return NULL;
    return buf;
}

/*
** Py_UniversalNewlineFread is an fread variation that understands
** all of \r, \n and \r\n conventions.
** The stream should be opened in binary mode.
** fobj must be a PyFileObject. In this case there
** is no readahead but in stead a flag is used to skip a following
** \n on the next read. Also, if the file is open in binary mode
** the whole conversion is skipped. Finally, the routine keeps track of
** the different types of newlines seen.
*/
size_t
Py_UniversalNewlineFread(char *buf, size_t n,
                         FILE *stream, PyObject *fobj)
{
    char *dst = buf;
    PyFileObject *f = (PyFileObject *)fobj;
    int newlinetypes, skipnextlf;

    assert(buf != NULL);
    assert(stream != NULL);

    if (!fobj || !PyFile_Check(fobj)) {
        errno = ENXIO;          /* What can you do... */
        return 0;
    }
    if (!f->f_univ_newline)
        return fread(buf, 1, n, stream);
    newlinetypes = f->f_newlinetypes;
    skipnextlf = f->f_skipnextlf;
    /* Invariant:  n is the number of bytes remaining to be filled
     * in the buffer.
     */
    while (n) {
        size_t nread;
        int shortread;
        char *src = dst;

        nread = fread(dst, 1, n, stream);
        assert(nread <= n);
        if (nread == 0)
            break;

        n -= nread; /* assuming 1 byte out for each in; will adjust */
        shortread = n != 0;             /* true iff EOF or error */
        while (nread--) {
            char c = *src++;
            if (c == '\r') {
                /* Save as LF and set flag to skip next LF. */
                *dst++ = '\n';
                skipnextlf = 1;
            }
            else if (skipnextlf && c == '\n') {
                /* Skip LF, and remember we saw CR LF. */
                skipnextlf = 0;
                newlinetypes |= NEWLINE_CRLF;
                ++n;
            }
            else {
                /* Normal char to be stored in buffer.  Also
                 * update the newlinetypes flag if either this
                 * is an LF or the previous char was a CR.
                 */
                if (c == '\n')
                    newlinetypes |= NEWLINE_LF;
                else if (skipnextlf)
                    newlinetypes |= NEWLINE_CR;
                *dst++ = c;
                skipnextlf = 0;
            }
        }
        if (shortread) {
            /* If this is EOF, update type flags. */
            if (skipnextlf && feof(stream))
                newlinetypes |= NEWLINE_CR;
            break;
        }
    }
    f->f_newlinetypes = newlinetypes;
    f->f_skipnextlf = skipnextlf;
    return dst - buf;
}

#ifdef __cplusplus
}
#endif
