/*********************************************************

    msvcrtmodule.c

    A Python interface to the Microsoft Visual C Runtime
    Library, providing access to those non-portable, but
    still useful routines.

    Only ever compiled with an MS compiler, so no attempt
    has been made to avoid MS language extensions, etc...

    This may only work on NT or 95...

    Author: Mark Hammond and Guido van Rossum.
    Maintenance: Guido van Rossum.

***********************************************************/

#include "Python.h"
#include "malloc.h"
#include <io.h>
#include <conio.h>
#include <sys/locking.h>
#include <crtdbg.h>
#include <windows.h>

#ifdef _MSC_VER
#if _MSC_VER >= 1500 && _MSC_VER < 1600
#include <crtassem.h>
#elif _MSC_VER >= 1600
#include <crtversion.h>
#endif
#endif

// Force the malloc heap to clean itself up, and free unused blocks
// back to the OS.  (According to the docs, only works on NT.)
static PyObject *
msvcrt_heapmin(PyObject *self, PyObject *args)
{
    if (!PyArg_ParseTuple(args, ":heapmin"))
        return NULL;

    if (_heapmin() != 0)
        return PyErr_SetFromErrno(PyExc_IOError);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(heapmin_doc,
"heapmin() -> None\n\
\n\
Force the malloc() heap to clean itself up and return unused blocks\n\
to the operating system. On failure, this raises IOError.");

// Perform locking operations on a C runtime file descriptor.
static PyObject *
msvcrt_locking(PyObject *self, PyObject *args)
{
    int fd;
    int mode;
    long nbytes;
    int err;

    if (!PyArg_ParseTuple(args, "iil:locking", &fd, &mode, &nbytes))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    err = _locking(fd, mode, nbytes);
    Py_END_ALLOW_THREADS
    if (err != 0)
        return PyErr_SetFromErrno(PyExc_IOError);

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(locking_doc,
"locking(fd, mode, nbytes) -> None\n\
\n\
Lock part of a file based on file descriptor fd from the C runtime.\n\
Raises IOError on failure. The locked region of the file extends from\n\
the current file position for nbytes bytes, and may continue beyond\n\
the end of the file. mode must be one of the LK_* constants listed\n\
below. Multiple regions in a file may be locked at the same time, but\n\
may not overlap. Adjacent regions are not merged; they must be unlocked\n\
individually.");

// Set the file translation mode for a C runtime file descriptor.
static PyObject *
msvcrt_setmode(PyObject *self, PyObject *args)
{
    int fd;
    int flags;
    if (!PyArg_ParseTuple(args,"ii:setmode", &fd, &flags))
        return NULL;

    flags = _setmode(fd, flags);
    if (flags == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    return PyLong_FromLong(flags);
}

PyDoc_STRVAR(setmode_doc,
"setmode(fd, mode) -> Previous mode\n\
\n\
Set the line-end translation mode for the file descriptor fd. To set\n\
it to text mode, flags should be os.O_TEXT; for binary, it should be\n\
os.O_BINARY.");

// Convert an OS file handle to a C runtime file descriptor.
static PyObject *
msvcrt_open_osfhandle(PyObject *self, PyObject *args)
{
    Py_intptr_t handle;
    int flags;
    int fd;

    if (!PyArg_ParseTuple(args, _Py_PARSE_INTPTR "i:open_osfhandle",
                          &handle, &flags))
        return NULL;

    fd = _open_osfhandle(handle, flags);
    if (fd == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    return PyLong_FromLong(fd);
}

PyDoc_STRVAR(open_osfhandle_doc,
"open_osfhandle(handle, flags) -> file descriptor\n\
\n\
Create a C runtime file descriptor from the file handle handle. The\n\
flags parameter should be a bitwise OR of os.O_APPEND, os.O_RDONLY,\n\
and os.O_TEXT. The returned file descriptor may be used as a parameter\n\
to os.fdopen() to create a file object.");

// Convert a C runtime file descriptor to an OS file handle.
static PyObject *
msvcrt_get_osfhandle(PyObject *self, PyObject *args)
{
    int fd;
    Py_intptr_t handle;

    if (!PyArg_ParseTuple(args,"i:get_osfhandle", &fd))
        return NULL;

    if (!_PyVerify_fd(fd))
        return PyErr_SetFromErrno(PyExc_IOError);

    handle = _get_osfhandle(fd);
    if (handle == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    /* technically 'handle' is not a pointer, but a integer as
       large as a pointer, Python's *VoidPtr interface is the
       most appropriate here */
    return PyLong_FromVoidPtr((void*)handle);
}

PyDoc_STRVAR(get_osfhandle_doc,
"get_osfhandle(fd) -> file handle\n\
\n\
Return the file handle for the file descriptor fd. Raises IOError\n\
if fd is not recognized.");

/* Console I/O */

static PyObject *
msvcrt_kbhit(PyObject *self, PyObject *args)
{
    int ok;

    if (!PyArg_ParseTuple(args, ":kbhit"))
        return NULL;

    ok = _kbhit();
    return PyLong_FromLong(ok);
}

PyDoc_STRVAR(kbhit_doc,
"kbhit() -> bool\n\
\n\
Return true if a keypress is waiting to be read.");

static PyObject *
msvcrt_getch(PyObject *self, PyObject *args)
{
    int ch;
    char s[1];

    if (!PyArg_ParseTuple(args, ":getch"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getch();
    Py_END_ALLOW_THREADS
    s[0] = ch;
    return PyBytes_FromStringAndSize(s, 1);
}

PyDoc_STRVAR(getch_doc,
"getch() -> key character\n\
\n\
Read a keypress and return the resulting character as a byte string.\n\
Nothing is echoed to the console. This call will block if a keypress is\n\
not already available, but will not wait for Enter to be pressed. If the\n\
pressed key was a special function key, this will return '\\000' or\n\
'\\xe0'; the next call will return the keycode. The Control-C keypress\n\
cannot be read with this function.");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_getwch(PyObject *self, PyObject *args)
{
    wchar_t ch;

    if (!PyArg_ParseTuple(args, ":getwch"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwch();
    Py_END_ALLOW_THREADS
    return PyUnicode_FromOrdinal(ch);
}

PyDoc_STRVAR(getwch_doc,
"getwch() -> Unicode key character\n\
\n\
Wide char variant of getch(), returning a Unicode value.");
#endif

static PyObject *
msvcrt_getche(PyObject *self, PyObject *args)
{
    int ch;
    char s[1];

    if (!PyArg_ParseTuple(args, ":getche"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getche();
    Py_END_ALLOW_THREADS
    s[0] = ch;
    return PyBytes_FromStringAndSize(s, 1);
}

PyDoc_STRVAR(getche_doc,
"getche() -> key character\n\
\n\
Similar to getch(), but the keypress will be echoed if it represents\n\
a printable character.");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_getwche(PyObject *self, PyObject *args)
{
    wchar_t ch;

    if (!PyArg_ParseTuple(args, ":getwche"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwche();
    Py_END_ALLOW_THREADS
    return PyUnicode_FromOrdinal(ch);
}

PyDoc_STRVAR(getwche_doc,
"getwche() -> Unicode key character\n\
\n\
Wide char variant of getche(), returning a Unicode value.");
#endif

static PyObject *
msvcrt_putch(PyObject *self, PyObject *args)
{
    char ch;

    if (!PyArg_ParseTuple(args, "c:putch", &ch))
        return NULL;

    _putch(ch);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(putch_doc,
"putch(char) -> None\n\
\n\
Print the byte string char to the console without buffering.");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_putwch(PyObject *self, PyObject *args)
{
    int ch;

    if (!PyArg_ParseTuple(args, "C:putwch", &ch))
        return NULL;

    _putwch(ch);
    Py_RETURN_NONE;

}

PyDoc_STRVAR(putwch_doc,
"putwch(unicode_char) -> None\n\
\n\
Wide char variant of putch(), accepting a Unicode value.");
#endif

static PyObject *
msvcrt_ungetch(PyObject *self, PyObject *args)
{
    char ch;

    if (!PyArg_ParseTuple(args, "c:ungetch", &ch))
        return NULL;

    if (_ungetch(ch) == EOF)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(ungetch_doc,
"ungetch(char) -> None\n\
\n\
Cause the byte string char to be \"pushed back\" into the\n\
console buffer; it will be the next character read by\n\
getch() or getche().");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_ungetwch(PyObject *self, PyObject *args)
{
    int ch;

    if (!PyArg_ParseTuple(args, "C:ungetwch", &ch))
        return NULL;

    if (_ungetwch(ch) == WEOF)
        return PyErr_SetFromErrno(PyExc_IOError);
    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(ungetwch_doc,
"ungetwch(unicode_char) -> None\n\
\n\
Wide char variant of ungetch(), accepting a Unicode value.");
#endif

static void
insertint(PyObject *d, char *name, int value)
{
    PyObject *v = PyLong_FromLong((long) value);
    if (v == NULL) {
        /* Don't bother reporting this error */
        PyErr_Clear();
    }
    else {
        PyDict_SetItemString(d, name, v);
        Py_DECREF(v);
    }
}

#ifdef _DEBUG

static PyObject*
msvcrt_setreportfile(PyObject *self, PyObject *args)
{
    int type, file;
    _HFILE res;

    if (!PyArg_ParseTuple(args, "ii", &type, &file))
        return NULL;
    res = _CrtSetReportFile(type, (_HFILE)file);
    return PyLong_FromLong((long)res);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
msvcrt_setreportmode(PyObject *self, PyObject *args)
{
    int type, mode;
    int res;

    if (!PyArg_ParseTuple(args, "ii", &type, &mode))
        return NULL;
    res = _CrtSetReportMode(type, mode);
    if (res == -1)
        return PyErr_SetFromErrno(PyExc_IOError);
    return PyLong_FromLong(res);
}

static PyObject*
msvcrt_seterrormode(PyObject *self, PyObject *args)
{
    int mode, res;

    if (!PyArg_ParseTuple(args, "i", &mode))
        return NULL;
    res = _set_error_mode(mode);
    return PyLong_FromLong(res);
}

#endif

static PyObject*
seterrormode(PyObject *self, PyObject *args)
{
    unsigned int mode, res;

    if (!PyArg_ParseTuple(args, "I", &mode))
        return NULL;
    res = SetErrorMode(mode);
    return PyLong_FromUnsignedLong(res);
}


/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
    {"heapmin",                 msvcrt_heapmin, METH_VARARGS, heapmin_doc},
    {"locking",             msvcrt_locking, METH_VARARGS, locking_doc},
    {"setmode",                 msvcrt_setmode, METH_VARARGS, setmode_doc},
    {"open_osfhandle",          msvcrt_open_osfhandle, METH_VARARGS, open_osfhandle_doc},
    {"get_osfhandle",           msvcrt_get_osfhandle, METH_VARARGS, get_osfhandle_doc},
    {"kbhit",                   msvcrt_kbhit, METH_VARARGS, kbhit_doc},
    {"getch",                   msvcrt_getch, METH_VARARGS, getch_doc},
    {"getche",                  msvcrt_getche, METH_VARARGS, getche_doc},
    {"putch",                   msvcrt_putch, METH_VARARGS, putch_doc},
    {"ungetch",                 msvcrt_ungetch, METH_VARARGS, ungetch_doc},
    {"SetErrorMode",            seterrormode, METH_VARARGS},
#ifdef _DEBUG
    {"CrtSetReportFile",        msvcrt_setreportfile, METH_VARARGS},
    {"CrtSetReportMode",        msvcrt_setreportmode, METH_VARARGS},
    {"set_error_mode",          msvcrt_seterrormode, METH_VARARGS},
#endif
#ifdef _WCONIO_DEFINED
    {"getwch",                  msvcrt_getwch, METH_VARARGS, getwch_doc},
    {"getwche",                 msvcrt_getwche, METH_VARARGS, getwche_doc},
    {"putwch",                  msvcrt_putwch, METH_VARARGS, putwch_doc},
    {"ungetwch",                msvcrt_ungetwch, METH_VARARGS, ungetwch_doc},
#endif
    {NULL,                      NULL}
};


static struct PyModuleDef msvcrtmodule = {
    PyModuleDef_HEAD_INIT,
    "msvcrt",
    NULL,
    -1,
    msvcrt_functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_msvcrt(void)
{
    int st;
    PyObject *d, *version;
    PyObject *m = PyModule_Create(&msvcrtmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    /* constants for the locking() function's mode argument */
    insertint(d, "LK_LOCK", _LK_LOCK);
    insertint(d, "LK_NBLCK", _LK_NBLCK);
    insertint(d, "LK_NBRLCK", _LK_NBRLCK);
    insertint(d, "LK_RLCK", _LK_RLCK);
    insertint(d, "LK_UNLCK", _LK_UNLCK);
    insertint(d, "SEM_FAILCRITICALERRORS", SEM_FAILCRITICALERRORS);
    insertint(d, "SEM_NOALIGNMENTFAULTEXCEPT", SEM_NOALIGNMENTFAULTEXCEPT);
    insertint(d, "SEM_NOGPFAULTERRORBOX", SEM_NOGPFAULTERRORBOX);
    insertint(d, "SEM_NOOPENFILEERRORBOX", SEM_NOOPENFILEERRORBOX);
#ifdef _DEBUG
    insertint(d, "CRT_WARN", _CRT_WARN);
    insertint(d, "CRT_ERROR", _CRT_ERROR);
    insertint(d, "CRT_ASSERT", _CRT_ASSERT);
    insertint(d, "CRTDBG_MODE_DEBUG", _CRTDBG_MODE_DEBUG);
    insertint(d, "CRTDBG_MODE_FILE", _CRTDBG_MODE_FILE);
    insertint(d, "CRTDBG_MODE_WNDW", _CRTDBG_MODE_WNDW);
    insertint(d, "CRTDBG_REPORT_MODE", _CRTDBG_REPORT_MODE);
    insertint(d, "CRTDBG_FILE_STDERR", (int)_CRTDBG_FILE_STDERR);
    insertint(d, "CRTDBG_FILE_STDOUT", (int)_CRTDBG_FILE_STDOUT);
    insertint(d, "CRTDBG_REPORT_FILE", (int)_CRTDBG_REPORT_FILE);
#endif

    /* constants for the crt versions */
    (void)st;
#ifdef _VC_ASSEMBLY_PUBLICKEYTOKEN
    st = PyModule_AddStringConstant(m, "VC_ASSEMBLY_PUBLICKEYTOKEN",
                                    _VC_ASSEMBLY_PUBLICKEYTOKEN);
    if (st < 0) return NULL;
#endif
#ifdef _CRT_ASSEMBLY_VERSION
    st = PyModule_AddStringConstant(m, "CRT_ASSEMBLY_VERSION",
                                    _CRT_ASSEMBLY_VERSION);
    if (st < 0) return NULL;
#endif
#ifdef __LIBRARIES_ASSEMBLY_NAME_PREFIX
    st = PyModule_AddStringConstant(m, "LIBRARIES_ASSEMBLY_NAME_PREFIX",
                                    __LIBRARIES_ASSEMBLY_NAME_PREFIX);
    if (st < 0) return NULL;
#endif

    /* constants for the 2010 crt versions */
#if defined(_VC_CRT_MAJOR_VERSION) && defined (_VC_CRT_MINOR_VERSION) && defined(_VC_CRT_BUILD_VERSION) && defined(_VC_CRT_RBUILD_VERSION)
    version = PyUnicode_FromFormat("%d.%d.%d.%d", _VC_CRT_MAJOR_VERSION,
                                                  _VC_CRT_MINOR_VERSION,
                                                  _VC_CRT_BUILD_VERSION,
                                                  _VC_CRT_RBUILD_VERSION);
    st = PyModule_AddObject(m, "CRT_ASSEMBLY_VERSION", version);
    if (st < 0) return NULL;
#endif

    return m;
}
