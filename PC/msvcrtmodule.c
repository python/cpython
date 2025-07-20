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
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
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

/*[python input]
class HANDLE_converter(CConverter):
    type = 'void *'
    format_unit = '"_Py_PARSE_UINTPTR"'

    def parse_arg(self, argname, displayname, *, limited_capi):
        return self.format_code("""
            {paramname} = PyLong_AsVoidPtr({argname});
            if (!{paramname} && PyErr_Occurred()) {{{{
                goto exit;
            }}}}
            """,
            argname=argname)

class HANDLE_return_converter(CReturnConverter):
    type = 'void *'

    def render(self, function, data):
        self.declare(data)
        self.err_occurred_if(
            "_return_value == NULL || _return_value == INVALID_HANDLE_VALUE",
            data)
        data.return_conversion.append(
            'return_value = PyLong_FromVoidPtr(_return_value);\n')

class byte_char_return_converter(CReturnConverter):
    type = 'int'

    def render(self, function, data):
        data.declarations.append('char s[1];')
        data.return_value = 's[0]'
        data.return_conversion.append(
            'return_value = PyBytes_FromStringAndSize(s, 1);\n')

class wchar_t_return_converter(CReturnConverter):
    type = 'wchar_t'

    def render(self, function, data):
        self.declare(data)
        data.return_conversion.append(
            'return_value = PyUnicode_FromOrdinal(_return_value);\n')
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=ff031be44ab3250d]*/

/*[clinic input]
module msvcrt
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=f31a87a783d036cd]*/

#include "clinic/msvcrtmodule.c.h"

/*[clinic input]
msvcrt.heapmin

Minimize the malloc() heap.

Force the malloc() heap to clean itself up and return unused blocks
to the operating system. On failure, this raises OSError.
[clinic start generated code]*/

static PyObject *
msvcrt_heapmin_impl(PyObject *module)
/*[clinic end generated code: output=1ba00f344782dc19 input=82e1771d21bde2d8]*/
{
    if (_heapmin() != 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_RETURN_NONE;
}
/*[clinic input]
msvcrt.locking

    fd: int
    mode: int
    nbytes: long
    /

Lock part of a file based on file descriptor fd from the C runtime.

Raises OSError on failure. The locked region of the file extends from
the current file position for nbytes bytes, and may continue beyond
the end of the file. mode must be one of the LK_* constants listed
below. Multiple regions in a file may be locked at the same time, but
may not overlap. Adjacent regions are not merged; they must be unlocked
individually.
[clinic start generated code]*/

static PyObject *
msvcrt_locking_impl(PyObject *module, int fd, int mode, long nbytes)
/*[clinic end generated code: output=a4a90deca9785a03 input=e97bd15fc4a04fef]*/
{
    int err;

    if (PySys_Audit("msvcrt.locking", "iil", fd, mode, nbytes) < 0) {
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    _Py_BEGIN_SUPPRESS_IPH
    err = _locking(fd, mode, nbytes);
    _Py_END_SUPPRESS_IPH
    Py_END_ALLOW_THREADS
    if (err != 0)
        return PyErr_SetFromErrno(PyExc_OSError);

    Py_RETURN_NONE;
}

/*[clinic input]
msvcrt.setmode -> long

    fd: int
    mode as flags: int
    /

Set the line-end translation mode for the file descriptor fd.

To set it to text mode, flags should be os.O_TEXT; for binary, it
should be os.O_BINARY.

Return value is the previous mode.
[clinic start generated code]*/

static long
msvcrt_setmode_impl(PyObject *module, int fd, int flags)
/*[clinic end generated code: output=24a9be5ea07ccb9b input=76e7c01f6b137f75]*/
{
    _Py_BEGIN_SUPPRESS_IPH
    flags = _setmode(fd, flags);
    _Py_END_SUPPRESS_IPH
    if (flags == -1)
        PyErr_SetFromErrno(PyExc_OSError);

    return flags;
}

/*[clinic input]
msvcrt.open_osfhandle -> long

    handle: HANDLE
    flags: int
    /

Create a C runtime file descriptor from the file handle handle.

The flags parameter should be a bitwise OR of os.O_APPEND, os.O_RDONLY,
and os.O_TEXT. The returned file descriptor may be used as a parameter
to os.fdopen() to create a file object.
[clinic start generated code]*/

static long
msvcrt_open_osfhandle_impl(PyObject *module, void *handle, int flags)
/*[clinic end generated code: output=b2fb97c4b515e4e6 input=d5db190a307cf4bb]*/
{
    if (PySys_Audit("msvcrt.open_osfhandle", "Ki", handle, flags) < 0) {
        return -1;
    }

    return _Py_open_osfhandle(handle, flags);
}

/*[clinic input]
msvcrt.get_osfhandle -> HANDLE

    fd: int
    /

Return the file handle for the file descriptor fd.

Raises OSError if fd is not recognized.
[clinic start generated code]*/

static void *
msvcrt_get_osfhandle_impl(PyObject *module, int fd)
/*[clinic end generated code: output=aca01dfe24637374 input=5fcfde9b17136aa2]*/
{
    if (PySys_Audit("msvcrt.get_osfhandle", "(i)", fd) < 0) {
        return NULL;
    }

    return _Py_get_osfhandle(fd);
}

/* Console I/O */
/*[clinic input]
msvcrt.kbhit -> long

Returns a nonzero value if a keypress is waiting to be read. Otherwise, return 0.
[clinic start generated code]*/

static long
msvcrt_kbhit_impl(PyObject *module)
/*[clinic end generated code: output=940dfce6587c1890 input=d0f4cb3289ff51e2]*/
{
    return _kbhit();
}

/*[clinic input]
msvcrt.getch -> byte_char

Read a keypress and return the resulting character as a byte string.

Nothing is echoed to the console. This call will block if a keypress is
not already available, but will not wait for Enter to be pressed. If the
pressed key was a special function key, this will return '\000' or
'\xe0'; the next call will return the keycode. The Control-C keypress
cannot be read with this function.
[clinic start generated code]*/

static int
msvcrt_getch_impl(PyObject *module)
/*[clinic end generated code: output=a4e51f0565064a7d input=37a40cf0ed0d1153]*/
{
    int ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getch();
    Py_END_ALLOW_THREADS
    return ch;
}

#ifdef MS_WINDOWS_DESKTOP

/*[clinic input]
msvcrt.getwch -> wchar_t

Wide char variant of getch(), returning a Unicode value.
[clinic start generated code]*/

static wchar_t
msvcrt_getwch_impl(PyObject *module)
/*[clinic end generated code: output=be9937494e22f007 input=27b3dec8ad823d7c]*/
{
    wchar_t ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwch();
    Py_END_ALLOW_THREADS
    return ch;
}

#endif /* MS_WINDOWS_DESKTOP */

/*[clinic input]
msvcrt.getche -> byte_char

Similar to getch(), but the keypress will be echoed if possible.
[clinic start generated code]*/

static int
msvcrt_getche_impl(PyObject *module)
/*[clinic end generated code: output=d8f7db4fd2990401 input=43311ade9ed4a9c0]*/
{
    int ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getche();
    Py_END_ALLOW_THREADS
    return ch;
}

#ifdef MS_WINDOWS_DESKTOP

/*[clinic input]
msvcrt.getwche -> wchar_t

Wide char variant of getche(), returning a Unicode value.
[clinic start generated code]*/

static wchar_t
msvcrt_getwche_impl(PyObject *module)
/*[clinic end generated code: output=d0dae5ba3829d596 input=49337d59d1a591f8]*/
{
    wchar_t ch;

    Py_BEGIN_ALLOW_THREADS
    ch = _getwche();
    Py_END_ALLOW_THREADS
    return ch;
}

#endif /* MS_WINDOWS_DESKTOP */

/*[clinic input]
msvcrt.putch

    char: char
    /

Print the byte string char to the console without buffering.
[clinic start generated code]*/

static PyObject *
msvcrt_putch_impl(PyObject *module, char char_value)
/*[clinic end generated code: output=92ec9b81012d8f60 input=ec078dd10cb054d6]*/
{
    _Py_BEGIN_SUPPRESS_IPH
    _putch(char_value);
    _Py_END_SUPPRESS_IPH
    Py_RETURN_NONE;
}

#ifdef MS_WINDOWS_DESKTOP

/*[clinic input]
msvcrt.putwch

    unicode_char: int(accept={str})
    /

Wide char variant of putch(), accepting a Unicode value.
[clinic start generated code]*/

static PyObject *
msvcrt_putwch_impl(PyObject *module, int unicode_char)
/*[clinic end generated code: output=a3bd1a8951d28eee input=996ccd0bbcbac4c3]*/
{
    _Py_BEGIN_SUPPRESS_IPH
    _putwch(unicode_char);
    _Py_END_SUPPRESS_IPH
    Py_RETURN_NONE;

}

#endif /* MS_WINDOWS_DESKTOP */

/*[clinic input]
msvcrt.ungetch

    char: char
    /

Opposite of getch.

Cause the byte string char to be "pushed back" into the
console buffer; it will be the next character read by
getch() or getche().
[clinic start generated code]*/

static PyObject *
msvcrt_ungetch_impl(PyObject *module, char char_value)
/*[clinic end generated code: output=c6942a0efa119000 input=22f07ee9001bbf0f]*/
{
    int res;

    _Py_BEGIN_SUPPRESS_IPH
    res = _ungetch(char_value);
    _Py_END_SUPPRESS_IPH

    if (res == EOF)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}

#ifdef MS_WINDOWS_DESKTOP

/*[clinic input]
msvcrt.ungetwch

    unicode_char: int(accept={str})
    /

Wide char variant of ungetch(), accepting a Unicode value.
[clinic start generated code]*/

static PyObject *
msvcrt_ungetwch_impl(PyObject *module, int unicode_char)
/*[clinic end generated code: output=e63af05438b8ba3d input=83ec0492be04d564]*/
{
    int res;

    _Py_BEGIN_SUPPRESS_IPH
    res = _ungetwch(unicode_char);
    _Py_END_SUPPRESS_IPH

    if (res == WEOF)
        return PyErr_SetFromErrno(PyExc_OSError);
    Py_RETURN_NONE;
}

#endif /* MS_WINDOWS_DESKTOP */

#ifdef _DEBUG
/*[clinic input]
msvcrt.CrtSetReportFile -> HANDLE

    type: int
    file: HANDLE
    /

Wrapper around _CrtSetReportFile.

Only available on Debug builds.
[clinic start generated code]*/

static void *
msvcrt_CrtSetReportFile_impl(PyObject *module, int type, void *file)
/*[clinic end generated code: output=9393e8c77088bbe9 input=290809b5f19e65b9]*/
{
    HANDLE res;

    _Py_BEGIN_SUPPRESS_IPH
    res = _CrtSetReportFile(type, file);
    _Py_END_SUPPRESS_IPH

    return res;
}

/*[clinic input]
msvcrt.CrtSetReportMode -> long

    type: int
    mode: int
    /

Wrapper around _CrtSetReportMode.

Only available on Debug builds.
[clinic start generated code]*/

static long
msvcrt_CrtSetReportMode_impl(PyObject *module, int type, int mode)
/*[clinic end generated code: output=b2863761523de317 input=9319d29b4319426b]*/
{
    int res;

    _Py_BEGIN_SUPPRESS_IPH
    res = _CrtSetReportMode(type, mode);
    _Py_END_SUPPRESS_IPH
    if (res == -1)
        PyErr_SetFromErrno(PyExc_OSError);
    return res;
}

/*[clinic input]
msvcrt.set_error_mode -> long

    mode: int
    /

Wrapper around _set_error_mode.

Only available on Debug builds.
[clinic start generated code]*/

static long
msvcrt_set_error_mode_impl(PyObject *module, int mode)
/*[clinic end generated code: output=ac4a09040d8ac4e3 input=046fca59c0f20872]*/
{
    long res;

    _Py_BEGIN_SUPPRESS_IPH
    res = _set_error_mode(mode);
    _Py_END_SUPPRESS_IPH

    return res;
}
#endif /* _DEBUG */

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM)

/*[clinic input]
msvcrt.GetErrorMode

Wrapper around GetErrorMode.
[clinic start generated code]*/

static PyObject *
msvcrt_GetErrorMode_impl(PyObject *module)
/*[clinic end generated code: output=3103fc6145913591 input=5a7fb083b6dd71fd]*/
{
    unsigned int res;

    _Py_BEGIN_SUPPRESS_IPH
    res = GetErrorMode();
    _Py_END_SUPPRESS_IPH

    return PyLong_FromUnsignedLong(res);
}

#endif /* MS_WINDOWS_APP || MS_WINDOWS_SYSTEM */

/*[clinic input]
msvcrt.SetErrorMode

    mode: unsigned_int(bitwise=True)
    /

Wrapper around SetErrorMode.
[clinic start generated code]*/

static PyObject *
msvcrt_SetErrorMode_impl(PyObject *module, unsigned int mode)
/*[clinic end generated code: output=01d529293f00da8f input=d8b167258d32d907]*/
{
    unsigned int res;

    _Py_BEGIN_SUPPRESS_IPH
    res = SetErrorMode(mode);
    _Py_END_SUPPRESS_IPH

    return PyLong_FromUnsignedLong(res);
}

/*[clinic input]
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=da39a3ee5e6b4b0d]*/

/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
    MSVCRT_HEAPMIN_METHODDEF
    MSVCRT_LOCKING_METHODDEF
    MSVCRT_SETMODE_METHODDEF
    MSVCRT_OPEN_OSFHANDLE_METHODDEF
    MSVCRT_GET_OSFHANDLE_METHODDEF
    MSVCRT_KBHIT_METHODDEF
    MSVCRT_GETCH_METHODDEF
    MSVCRT_GETCHE_METHODDEF
    MSVCRT_PUTCH_METHODDEF
    MSVCRT_UNGETCH_METHODDEF
    MSVCRT_GETERRORMODE_METHODDEF
    MSVCRT_SETERRORMODE_METHODDEF
    MSVCRT_CRTSETREPORTFILE_METHODDEF
    MSVCRT_CRTSETREPORTMODE_METHODDEF
    MSVCRT_SET_ERROR_MODE_METHODDEF
    MSVCRT_GETWCH_METHODDEF
    MSVCRT_GETWCHE_METHODDEF
    MSVCRT_PUTWCH_METHODDEF
    MSVCRT_UNGETWCH_METHODDEF
    {NULL,                      NULL}
};

static int
insertptr(PyObject *mod, const char *name, void *value)
{
    return PyModule_Add(mod, name, PyLong_FromVoidPtr(value));
}

#define INSERTINT(MOD, NAME, VAL) do {                  \
    if (PyModule_AddIntConstant(MOD, NAME, VAL) < 0) {  \
        return -1;                                      \
    }                                                   \
} while (0)

#define INSERTPTR(MOD, NAME, PTR) do {      \
    if (insertptr(MOD, NAME, PTR) < 0) {    \
        return -1;                          \
    }                                       \
} while (0)

#define INSERTSTR(MOD, NAME, CONST) do {                    \
    if (PyModule_AddStringConstant(MOD, NAME, CONST) < 0) { \
        return -1;                                          \
    }                                                       \
} while (0)

static int
exec_module(PyObject* m)
{
    /* constants for the locking() function's mode argument */
    INSERTINT(m, "LK_LOCK", _LK_LOCK);
    INSERTINT(m, "LK_NBLCK", _LK_NBLCK);
    INSERTINT(m, "LK_NBRLCK", _LK_NBRLCK);
    INSERTINT(m, "LK_RLCK", _LK_RLCK);
    INSERTINT(m, "LK_UNLCK", _LK_UNLCK);
#ifdef MS_WINDOWS_DESKTOP
    INSERTINT(m, "SEM_FAILCRITICALERRORS", SEM_FAILCRITICALERRORS);
    INSERTINT(m, "SEM_NOALIGNMENTFAULTEXCEPT", SEM_NOALIGNMENTFAULTEXCEPT);
    INSERTINT(m, "SEM_NOGPFAULTERRORBOX", SEM_NOGPFAULTERRORBOX);
    INSERTINT(m, "SEM_NOOPENFILEERRORBOX", SEM_NOOPENFILEERRORBOX);
#endif
#ifdef _DEBUG
    INSERTINT(m, "CRT_WARN", _CRT_WARN);
    INSERTINT(m, "CRT_ERROR", _CRT_ERROR);
    INSERTINT(m, "CRT_ASSERT", _CRT_ASSERT);
    INSERTINT(m, "CRTDBG_MODE_DEBUG", _CRTDBG_MODE_DEBUG);
    INSERTINT(m, "CRTDBG_MODE_FILE", _CRTDBG_MODE_FILE);
    INSERTINT(m, "CRTDBG_MODE_WNDW", _CRTDBG_MODE_WNDW);
    INSERTINT(m, "CRTDBG_REPORT_MODE", _CRTDBG_REPORT_MODE);
    INSERTPTR(m, "CRTDBG_FILE_STDERR", _CRTDBG_FILE_STDERR);
    INSERTPTR(m, "CRTDBG_FILE_STDOUT", _CRTDBG_FILE_STDOUT);
    INSERTPTR(m, "CRTDBG_REPORT_FILE", _CRTDBG_REPORT_FILE);
    INSERTINT(m, "OUT_TO_DEFAULT", _OUT_TO_DEFAULT);
    INSERTINT(m, "OUT_TO_STDERR", _OUT_TO_STDERR);
    INSERTINT(m, "OUT_TO_MSGBOX", _OUT_TO_MSGBOX);
    INSERTINT(m, "REPORT_ERRMODE", _REPORT_ERRMODE);
#endif

#undef INSERTINT
#undef INSERTPTR

    /* constants for the crt versions */
#ifdef _VC_ASSEMBLY_PUBLICKEYTOKEN
    INSERTSTR(m, "VC_ASSEMBLY_PUBLICKEYTOKEN", _VC_ASSEMBLY_PUBLICKEYTOKEN);
#endif
#ifdef _CRT_ASSEMBLY_VERSION
    INSERTSTR(m, "CRT_ASSEMBLY_VERSION", _CRT_ASSEMBLY_VERSION);
#endif
#ifdef __LIBRARIES_ASSEMBLY_NAME_PREFIX
    INSERTSTR(m, "LIBRARIES_ASSEMBLY_NAME_PREFIX",
              __LIBRARIES_ASSEMBLY_NAME_PREFIX);
#endif

#undef INSERTSTR

    /* constants for the 2010 crt versions */
#if defined(_VC_CRT_MAJOR_VERSION) && defined (_VC_CRT_MINOR_VERSION) && defined(_VC_CRT_BUILD_VERSION) && defined(_VC_CRT_RBUILD_VERSION)
    PyObject *version = PyUnicode_FromFormat("%d.%d.%d.%d",
                                             _VC_CRT_MAJOR_VERSION,
                                             _VC_CRT_MINOR_VERSION,
                                             _VC_CRT_BUILD_VERSION,
                                             _VC_CRT_RBUILD_VERSION);
    if (PyModule_Add(m, "CRT_ASSEMBLY_VERSION", version) < 0) {
        return -1;
    }
#endif

    return 0;
}

static PyModuleDef_Slot msvcrt_slots[] = {
    {Py_mod_exec, exec_module},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef msvcrtmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "msvcrt",
    .m_methods = msvcrt_functions,
    .m_slots = msvcrt_slots,
};

PyMODINIT_FUNC
PyInit_msvcrt(void)
{
    return PyModuleDef_Init(&msvcrtmodule);
}
