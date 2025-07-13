/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(msvcrt_heapmin__doc__,
"heapmin($module, /)\n"
"--\n"
"\n"
"Minimize the malloc() heap.\n"
"\n"
"Force the malloc() heap to clean itself up and return unused blocks\n"
"to the operating system. On failure, this raises OSError.");

#define MSVCRT_HEAPMIN_METHODDEF    \
    {"heapmin", (PyCFunction)msvcrt_heapmin, METH_NOARGS, msvcrt_heapmin__doc__},

static PyObject *
msvcrt_heapmin_impl(PyObject *module);

static PyObject *
msvcrt_heapmin(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return msvcrt_heapmin_impl(module);
}

PyDoc_STRVAR(msvcrt_locking__doc__,
"locking($module, fd, mode, nbytes, /)\n"
"--\n"
"\n"
"Lock part of a file based on file descriptor fd from the C runtime.\n"
"\n"
"Raises OSError on failure. The locked region of the file extends from\n"
"the current file position for nbytes bytes, and may continue beyond\n"
"the end of the file. mode must be one of the LK_* constants listed\n"
"below. Multiple regions in a file may be locked at the same time, but\n"
"may not overlap. Adjacent regions are not merged; they must be unlocked\n"
"individually.");

#define MSVCRT_LOCKING_METHODDEF    \
    {"locking", _PyCFunction_CAST(msvcrt_locking), METH_FASTCALL, msvcrt_locking__doc__},

static PyObject *
msvcrt_locking_impl(PyObject *module, int fd, int mode, long nbytes);

static PyObject *
msvcrt_locking(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int mode;
    long nbytes;

    if (!_PyArg_CheckPositional("locking", nargs, 3, 3)) {
        goto exit;
    }
    fd = PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    mode = PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    nbytes = PyLong_AsLong(args[2]);
    if (nbytes == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = msvcrt_locking_impl(module, fd, mode, nbytes);

exit:
    return return_value;
}

PyDoc_STRVAR(msvcrt_setmode__doc__,
"setmode($module, fd, mode, /)\n"
"--\n"
"\n"
"Set the line-end translation mode for the file descriptor fd.\n"
"\n"
"To set it to text mode, flags should be os.O_TEXT; for binary, it\n"
"should be os.O_BINARY.\n"
"\n"
"Return value is the previous mode.");

#define MSVCRT_SETMODE_METHODDEF    \
    {"setmode", _PyCFunction_CAST(msvcrt_setmode), METH_FASTCALL, msvcrt_setmode__doc__},

static long
msvcrt_setmode_impl(PyObject *module, int fd, int flags);

static PyObject *
msvcrt_setmode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int flags;
    long _return_value;

    if (!_PyArg_CheckPositional("setmode", nargs, 2, 2)) {
        goto exit;
    }
    fd = PyLong_AsInt(args[0]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_setmode_impl(module, fd, flags);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(msvcrt_open_osfhandle__doc__,
"open_osfhandle($module, handle, flags, /)\n"
"--\n"
"\n"
"Create a C runtime file descriptor from the file handle handle.\n"
"\n"
"The flags parameter should be a bitwise OR of os.O_APPEND, os.O_RDONLY,\n"
"and os.O_TEXT. The returned file descriptor may be used as a parameter\n"
"to os.fdopen() to create a file object.");

#define MSVCRT_OPEN_OSFHANDLE_METHODDEF    \
    {"open_osfhandle", _PyCFunction_CAST(msvcrt_open_osfhandle), METH_FASTCALL, msvcrt_open_osfhandle__doc__},

static long
msvcrt_open_osfhandle_impl(PyObject *module, void *handle, int flags);

static PyObject *
msvcrt_open_osfhandle(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    void *handle;
    int flags;
    long _return_value;

    if (!_PyArg_CheckPositional("open_osfhandle", nargs, 2, 2)) {
        goto exit;
    }
    handle = PyLong_AsVoidPtr(args[0]);
    if (!handle && PyErr_Occurred()) {
        goto exit;
    }
    flags = PyLong_AsInt(args[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_open_osfhandle_impl(module, handle, flags);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(msvcrt_get_osfhandle__doc__,
"get_osfhandle($module, fd, /)\n"
"--\n"
"\n"
"Return the file handle for the file descriptor fd.\n"
"\n"
"Raises OSError if fd is not recognized.");

#define MSVCRT_GET_OSFHANDLE_METHODDEF    \
    {"get_osfhandle", (PyCFunction)msvcrt_get_osfhandle, METH_O, msvcrt_get_osfhandle__doc__},

static void *
msvcrt_get_osfhandle_impl(PyObject *module, int fd);

static PyObject *
msvcrt_get_osfhandle(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;
    void *_return_value;

    fd = PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_get_osfhandle_impl(module, fd);
    if ((_return_value == NULL || _return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromVoidPtr(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(msvcrt_kbhit__doc__,
"kbhit($module, /)\n"
"--\n"
"\n"
"Returns a nonzero value if a keypress is waiting to be read. Otherwise, return 0.");

#define MSVCRT_KBHIT_METHODDEF    \
    {"kbhit", (PyCFunction)msvcrt_kbhit, METH_NOARGS, msvcrt_kbhit__doc__},

static long
msvcrt_kbhit_impl(PyObject *module);

static PyObject *
msvcrt_kbhit(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    long _return_value;

    _return_value = msvcrt_kbhit_impl(module);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(msvcrt_getch__doc__,
"getch($module, /)\n"
"--\n"
"\n"
"Read a keypress and return the resulting character as a byte string.\n"
"\n"
"Nothing is echoed to the console. This call will block if a keypress is\n"
"not already available, but will not wait for Enter to be pressed. If the\n"
"pressed key was a special function key, this will return \'\\000\' or\n"
"\'\\xe0\'; the next call will return the keycode. The Control-C keypress\n"
"cannot be read with this function.");

#define MSVCRT_GETCH_METHODDEF    \
    {"getch", (PyCFunction)msvcrt_getch, METH_NOARGS, msvcrt_getch__doc__},

static int
msvcrt_getch_impl(PyObject *module);

static PyObject *
msvcrt_getch(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    char s[1];

    s[0] = msvcrt_getch_impl(module);
    return_value = PyBytes_FromStringAndSize(s, 1);

    return return_value;
}

#if defined(MS_WINDOWS_DESKTOP)

PyDoc_STRVAR(msvcrt_getwch__doc__,
"getwch($module, /)\n"
"--\n"
"\n"
"Wide char variant of getch(), returning a Unicode value.");

#define MSVCRT_GETWCH_METHODDEF    \
    {"getwch", (PyCFunction)msvcrt_getwch, METH_NOARGS, msvcrt_getwch__doc__},

static wchar_t
msvcrt_getwch_impl(PyObject *module);

static PyObject *
msvcrt_getwch(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    wchar_t _return_value;

    _return_value = msvcrt_getwch_impl(module);
    return_value = PyUnicode_FromOrdinal(_return_value);

    return return_value;
}

#endif /* defined(MS_WINDOWS_DESKTOP) */

PyDoc_STRVAR(msvcrt_getche__doc__,
"getche($module, /)\n"
"--\n"
"\n"
"Similar to getch(), but the keypress will be echoed if possible.");

#define MSVCRT_GETCHE_METHODDEF    \
    {"getche", (PyCFunction)msvcrt_getche, METH_NOARGS, msvcrt_getche__doc__},

static int
msvcrt_getche_impl(PyObject *module);

static PyObject *
msvcrt_getche(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    char s[1];

    s[0] = msvcrt_getche_impl(module);
    return_value = PyBytes_FromStringAndSize(s, 1);

    return return_value;
}

#if defined(MS_WINDOWS_DESKTOP)

PyDoc_STRVAR(msvcrt_getwche__doc__,
"getwche($module, /)\n"
"--\n"
"\n"
"Wide char variant of getche(), returning a Unicode value.");

#define MSVCRT_GETWCHE_METHODDEF    \
    {"getwche", (PyCFunction)msvcrt_getwche, METH_NOARGS, msvcrt_getwche__doc__},

static wchar_t
msvcrt_getwche_impl(PyObject *module);

static PyObject *
msvcrt_getwche(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    wchar_t _return_value;

    _return_value = msvcrt_getwche_impl(module);
    return_value = PyUnicode_FromOrdinal(_return_value);

    return return_value;
}

#endif /* defined(MS_WINDOWS_DESKTOP) */

PyDoc_STRVAR(msvcrt_putch__doc__,
"putch($module, char, /)\n"
"--\n"
"\n"
"Print the byte string char to the console without buffering.");

#define MSVCRT_PUTCH_METHODDEF    \
    {"putch", (PyCFunction)msvcrt_putch, METH_O, msvcrt_putch__doc__},

static PyObject *
msvcrt_putch_impl(PyObject *module, char char_value);

static PyObject *
msvcrt_putch(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    char char_value;

    if (PyBytes_Check(arg)) {
        if (PyBytes_GET_SIZE(arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "putch(): argument must be a byte string of length 1, "
                "not a bytes object of length %zd",
                PyBytes_GET_SIZE(arg));
            goto exit;
        }
        char_value = PyBytes_AS_STRING(arg)[0];
    }
    else if (PyByteArray_Check(arg)) {
        if (PyByteArray_GET_SIZE(arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "putch(): argument must be a byte string of length 1, "
                "not a bytearray object of length %zd",
                PyByteArray_GET_SIZE(arg));
            goto exit;
        }
        char_value = PyByteArray_AS_STRING(arg)[0];
    }
    else {
        _PyArg_BadArgument("putch", "argument", "a byte string of length 1", arg);
        goto exit;
    }
    return_value = msvcrt_putch_impl(module, char_value);

exit:
    return return_value;
}

#if defined(MS_WINDOWS_DESKTOP)

PyDoc_STRVAR(msvcrt_putwch__doc__,
"putwch($module, unicode_char, /)\n"
"--\n"
"\n"
"Wide char variant of putch(), accepting a Unicode value.");

#define MSVCRT_PUTWCH_METHODDEF    \
    {"putwch", (PyCFunction)msvcrt_putwch, METH_O, msvcrt_putwch__doc__},

static PyObject *
msvcrt_putwch_impl(PyObject *module, int unicode_char);

static PyObject *
msvcrt_putwch(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int unicode_char;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("putwch", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        PyErr_Format(PyExc_TypeError,
            "putwch(): argument must be a unicode character, "
            "not a string of length %zd",
            PyUnicode_GET_LENGTH(arg));
        goto exit;
    }
    unicode_char = PyUnicode_READ_CHAR(arg, 0);
    return_value = msvcrt_putwch_impl(module, unicode_char);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS_DESKTOP) */

PyDoc_STRVAR(msvcrt_ungetch__doc__,
"ungetch($module, char, /)\n"
"--\n"
"\n"
"Opposite of getch.\n"
"\n"
"Cause the byte string char to be \"pushed back\" into the\n"
"console buffer; it will be the next character read by\n"
"getch() or getche().");

#define MSVCRT_UNGETCH_METHODDEF    \
    {"ungetch", (PyCFunction)msvcrt_ungetch, METH_O, msvcrt_ungetch__doc__},

static PyObject *
msvcrt_ungetch_impl(PyObject *module, char char_value);

static PyObject *
msvcrt_ungetch(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    char char_value;

    if (PyBytes_Check(arg)) {
        if (PyBytes_GET_SIZE(arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "ungetch(): argument must be a byte string of length 1, "
                "not a bytes object of length %zd",
                PyBytes_GET_SIZE(arg));
            goto exit;
        }
        char_value = PyBytes_AS_STRING(arg)[0];
    }
    else if (PyByteArray_Check(arg)) {
        if (PyByteArray_GET_SIZE(arg) != 1) {
            PyErr_Format(PyExc_TypeError,
                "ungetch(): argument must be a byte string of length 1, "
                "not a bytearray object of length %zd",
                PyByteArray_GET_SIZE(arg));
            goto exit;
        }
        char_value = PyByteArray_AS_STRING(arg)[0];
    }
    else {
        _PyArg_BadArgument("ungetch", "argument", "a byte string of length 1", arg);
        goto exit;
    }
    return_value = msvcrt_ungetch_impl(module, char_value);

exit:
    return return_value;
}

#if defined(MS_WINDOWS_DESKTOP)

PyDoc_STRVAR(msvcrt_ungetwch__doc__,
"ungetwch($module, unicode_char, /)\n"
"--\n"
"\n"
"Wide char variant of ungetch(), accepting a Unicode value.");

#define MSVCRT_UNGETWCH_METHODDEF    \
    {"ungetwch", (PyCFunction)msvcrt_ungetwch, METH_O, msvcrt_ungetwch__doc__},

static PyObject *
msvcrt_ungetwch_impl(PyObject *module, int unicode_char);

static PyObject *
msvcrt_ungetwch(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int unicode_char;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("ungetwch", "argument", "a unicode character", arg);
        goto exit;
    }
    if (PyUnicode_GET_LENGTH(arg) != 1) {
        PyErr_Format(PyExc_TypeError,
            "ungetwch(): argument must be a unicode character, "
            "not a string of length %zd",
            PyUnicode_GET_LENGTH(arg));
        goto exit;
    }
    unicode_char = PyUnicode_READ_CHAR(arg, 0);
    return_value = msvcrt_ungetwch_impl(module, unicode_char);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS_DESKTOP) */

#if defined(_DEBUG)

PyDoc_STRVAR(msvcrt_CrtSetReportFile__doc__,
"CrtSetReportFile($module, type, file, /)\n"
"--\n"
"\n"
"Wrapper around _CrtSetReportFile.\n"
"\n"
"Only available on Debug builds.");

#define MSVCRT_CRTSETREPORTFILE_METHODDEF    \
    {"CrtSetReportFile", _PyCFunction_CAST(msvcrt_CrtSetReportFile), METH_FASTCALL, msvcrt_CrtSetReportFile__doc__},

static void *
msvcrt_CrtSetReportFile_impl(PyObject *module, int type, void *file);

static PyObject *
msvcrt_CrtSetReportFile(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int type;
    void *file;
    void *_return_value;

    if (!_PyArg_CheckPositional("CrtSetReportFile", nargs, 2, 2)) {
        goto exit;
    }
    type = PyLong_AsInt(args[0]);
    if (type == -1 && PyErr_Occurred()) {
        goto exit;
    }
    file = PyLong_AsVoidPtr(args[1]);
    if (!file && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_CrtSetReportFile_impl(module, type, file);
    if ((_return_value == NULL || _return_value == INVALID_HANDLE_VALUE) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromVoidPtr(_return_value);

exit:
    return return_value;
}

#endif /* defined(_DEBUG) */

#if defined(_DEBUG)

PyDoc_STRVAR(msvcrt_CrtSetReportMode__doc__,
"CrtSetReportMode($module, type, mode, /)\n"
"--\n"
"\n"
"Wrapper around _CrtSetReportMode.\n"
"\n"
"Only available on Debug builds.");

#define MSVCRT_CRTSETREPORTMODE_METHODDEF    \
    {"CrtSetReportMode", _PyCFunction_CAST(msvcrt_CrtSetReportMode), METH_FASTCALL, msvcrt_CrtSetReportMode__doc__},

static long
msvcrt_CrtSetReportMode_impl(PyObject *module, int type, int mode);

static PyObject *
msvcrt_CrtSetReportMode(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int type;
    int mode;
    long _return_value;

    if (!_PyArg_CheckPositional("CrtSetReportMode", nargs, 2, 2)) {
        goto exit;
    }
    type = PyLong_AsInt(args[0]);
    if (type == -1 && PyErr_Occurred()) {
        goto exit;
    }
    mode = PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_CrtSetReportMode_impl(module, type, mode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#endif /* defined(_DEBUG) */

#if defined(_DEBUG)

PyDoc_STRVAR(msvcrt_set_error_mode__doc__,
"set_error_mode($module, mode, /)\n"
"--\n"
"\n"
"Wrapper around _set_error_mode.\n"
"\n"
"Only available on Debug builds.");

#define MSVCRT_SET_ERROR_MODE_METHODDEF    \
    {"set_error_mode", (PyCFunction)msvcrt_set_error_mode, METH_O, msvcrt_set_error_mode__doc__},

static long
msvcrt_set_error_mode_impl(PyObject *module, int mode);

static PyObject *
msvcrt_set_error_mode(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int mode;
    long _return_value;

    mode = PyLong_AsInt(arg);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = msvcrt_set_error_mode_impl(module, mode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#endif /* defined(_DEBUG) */

#if (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM))

PyDoc_STRVAR(msvcrt_GetErrorMode__doc__,
"GetErrorMode($module, /)\n"
"--\n"
"\n"
"Wrapper around GetErrorMode.");

#define MSVCRT_GETERRORMODE_METHODDEF    \
    {"GetErrorMode", (PyCFunction)msvcrt_GetErrorMode, METH_NOARGS, msvcrt_GetErrorMode__doc__},

static PyObject *
msvcrt_GetErrorMode_impl(PyObject *module);

static PyObject *
msvcrt_GetErrorMode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return msvcrt_GetErrorMode_impl(module);
}

#endif /* (defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM)) */

PyDoc_STRVAR(msvcrt_SetErrorMode__doc__,
"SetErrorMode($module, mode, /)\n"
"--\n"
"\n"
"Wrapper around SetErrorMode.");

#define MSVCRT_SETERRORMODE_METHODDEF    \
    {"SetErrorMode", (PyCFunction)msvcrt_SetErrorMode, METH_O, msvcrt_SetErrorMode__doc__},

static PyObject *
msvcrt_SetErrorMode_impl(PyObject *module, unsigned int mode);

static PyObject *
msvcrt_SetErrorMode(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned int mode;

    mode = (unsigned int)PyLong_AsUnsignedLongMask(arg);
    if (mode == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = msvcrt_SetErrorMode_impl(module, mode);

exit:
    return return_value;
}

#ifndef MSVCRT_GETWCH_METHODDEF
    #define MSVCRT_GETWCH_METHODDEF
#endif /* !defined(MSVCRT_GETWCH_METHODDEF) */

#ifndef MSVCRT_GETWCHE_METHODDEF
    #define MSVCRT_GETWCHE_METHODDEF
#endif /* !defined(MSVCRT_GETWCHE_METHODDEF) */

#ifndef MSVCRT_PUTWCH_METHODDEF
    #define MSVCRT_PUTWCH_METHODDEF
#endif /* !defined(MSVCRT_PUTWCH_METHODDEF) */

#ifndef MSVCRT_UNGETWCH_METHODDEF
    #define MSVCRT_UNGETWCH_METHODDEF
#endif /* !defined(MSVCRT_UNGETWCH_METHODDEF) */

#ifndef MSVCRT_CRTSETREPORTFILE_METHODDEF
    #define MSVCRT_CRTSETREPORTFILE_METHODDEF
#endif /* !defined(MSVCRT_CRTSETREPORTFILE_METHODDEF) */

#ifndef MSVCRT_CRTSETREPORTMODE_METHODDEF
    #define MSVCRT_CRTSETREPORTMODE_METHODDEF
#endif /* !defined(MSVCRT_CRTSETREPORTMODE_METHODDEF) */

#ifndef MSVCRT_SET_ERROR_MODE_METHODDEF
    #define MSVCRT_SET_ERROR_MODE_METHODDEF
#endif /* !defined(MSVCRT_SET_ERROR_MODE_METHODDEF) */

#ifndef MSVCRT_GETERRORMODE_METHODDEF
    #define MSVCRT_GETERRORMODE_METHODDEF
#endif /* !defined(MSVCRT_GETERRORMODE_METHODDEF) */
/*[clinic end generated code: output=692c6f52bb9193ce input=a9049054013a1b77]*/
