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

#ifdef _MSC_VER
#if _MSC_VER >= 1500 && _MSC_VER < 1600
#include <crtassem.h>
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

    return PyInt_FromLong(flags);
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
    long handle;
    int flags;
    int fd;

    if (!PyArg_ParseTuple(args, "li:open_osfhandle", &handle, &flags))
        return NULL;

    fd = _open_osfhandle(handle, flags);
    if (fd == -1)
        return PyErr_SetFromErrno(PyExc_IOError);

    return PyInt_FromLong(fd);
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
    return PyInt_FromLong(ok);
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
    return PyString_FromStringAndSize(s, 1);
}

PyDoc_STRVAR(getch_doc,
"getch() -> key character\n\
\n\
Read a keypress and return the resulting character. Nothing is echoed to\n\
the console. This call will block if a keypress is not already\n\
available, but will not wait for Enter to be pressed. If the pressed key\n\
was a special function key, this will return '\\000' or '\\xe0'; the next\n\
call will return the keycode. The Control-C keypress cannot be read with\n\
this function.");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_getwch(PyObject *self, PyObject *args)
{
    Py_UNICODE ch;
    Py_UNICODE u[1];

    if (!PyArg_ParseTuple(args, ":getwch"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwch();
    Py_END_ALLOW_THREADS
    u[0] = ch;
    return PyUnicode_FromUnicode(u, 1);
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
    return PyString_FromStringAndSize(s, 1);
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
    Py_UNICODE ch;
    Py_UNICODE s[1];

    if (!PyArg_ParseTuple(args, ":getwche"))
        return NULL;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwche();
    Py_END_ALLOW_THREADS
    s[0] = ch;
    return PyUnicode_FromUnicode(s, 1);
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
Print the character char to the console without buffering.");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_putwch(PyObject *self, PyObject *args)
{
    Py_UNICODE *ch;
    int size;

    if (!PyArg_ParseTuple(args, "u#:putwch", &ch, &size))
        return NULL;

    if (size == 0) {
        PyErr_SetString(PyExc_ValueError,
            "Expected unicode string of length 1");
        return NULL;
    }
    _putwch(*ch);
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
Cause the character char to be \"pushed back\" into the console buffer;\n\
it will be the next character read by getch() or getche().");

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_ungetwch(PyObject *self, PyObject *args)
{
    Py_UNICODE ch;

    if (!PyArg_ParseTuple(args, "u:ungetwch", &ch))
        return NULL;

    if (_ungetch(ch) == EOF)
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
    PyObject *v = PyInt_FromLong((long) value);
    if (v == NULL) {
        /* Don't bother reporting this error */
        PyErr_Clear();
    }
    else {
        PyDict_SetItemString(d, name, v);
        Py_DECREF(v);
    }
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
#ifdef _WCONIO_DEFINED
    {"getwch",                  msvcrt_getwch, METH_VARARGS, getwch_doc},
    {"getwche",                 msvcrt_getwche, METH_VARARGS, getwche_doc},
    {"putwch",                  msvcrt_putwch, METH_VARARGS, putwch_doc},
    {"ungetwch",                msvcrt_ungetwch, METH_VARARGS, ungetwch_doc},
#endif
    {NULL,                      NULL}
};

PyMODINIT_FUNC
initmsvcrt(void)
{
    int st;
    PyObject *d;
    PyObject *m = Py_InitModule("msvcrt", msvcrt_functions);
    if (m == NULL)
        return;
    d = PyModule_GetDict(m);

    /* constants for the locking() function's mode argument */
    insertint(d, "LK_LOCK", _LK_LOCK);
    insertint(d, "LK_NBLCK", _LK_NBLCK);
    insertint(d, "LK_NBRLCK", _LK_NBRLCK);
    insertint(d, "LK_RLCK", _LK_RLCK);
    insertint(d, "LK_UNLCK", _LK_UNLCK);

    /* constants for the crt versions */
#ifdef _VC_ASSEMBLY_PUBLICKEYTOKEN
    st = PyModule_AddStringConstant(m, "VC_ASSEMBLY_PUBLICKEYTOKEN",
                                    _VC_ASSEMBLY_PUBLICKEYTOKEN);
    if (st < 0)return;
#endif
#ifdef _CRT_ASSEMBLY_VERSION
    st = PyModule_AddStringConstant(m, "CRT_ASSEMBLY_VERSION",
                                    _CRT_ASSEMBLY_VERSION);
    if (st < 0)return;
#endif
#ifdef __LIBRARIES_ASSEMBLY_NAME_PREFIX
    st = PyModule_AddStringConstant(m, "LIBRARIES_ASSEMBLY_NAME_PREFIX",
                                    __LIBRARIES_ASSEMBLY_NAME_PREFIX);
    if (st < 0)return;
#endif
}
