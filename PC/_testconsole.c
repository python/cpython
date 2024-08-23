/* Testing module for multi-phase initialization of extension modules (PEP 489)
 */

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include "Python.h"

#ifdef MS_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>

 /* The full definition is in iomodule. We reproduce
 enough here to get the fd, which is all we want. */
typedef struct {
    PyObject_HEAD
    int fd;
} winconsoleio;


static int execfunc(PyObject *m)
{
    return 0;
}

PyModuleDef_Slot testconsole_slots[] = {
    {Py_mod_exec, execfunc},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

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
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=380aa5c91076742b]*/
/*[python end generated code:]*/

/*[clinic input]
module _testconsole

_testconsole.write_input
    file: object
    s: Py_buffer

Writes UTF-16-LE encoded bytes to the console as if typed by a user.
[clinic start generated code]*/

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file, Py_buffer *s)
/*[clinic end generated code: output=58631a8985426ad3 input=68062f1bb2e52206]*/
{
    INPUT_RECORD *rec = NULL;

    PyObject *mod = PyImport_ImportModule("_io");
    if (mod == NULL) {
        return NULL;
    }

    PyTypeObject *winconsoleio_type = (PyTypeObject *)PyObject_GetAttrString(mod, "_WindowsConsoleIO");
    Py_DECREF(mod);
    if (winconsoleio_type == NULL) {
        return NULL;
    }
    int is_subclass = PyObject_TypeCheck(file, winconsoleio_type);
    Py_DECREF(winconsoleio_type);
    if (!is_subclass) {
        PyErr_SetString(PyExc_TypeError, "expected raw console object");
        return NULL;
    }

    const wchar_t *p = (const wchar_t *)s->buf;
    DWORD size = (DWORD)s->len / sizeof(wchar_t);

    rec = (INPUT_RECORD*)PyMem_Calloc(size, sizeof(INPUT_RECORD));
    if (!rec)
        goto error;

    INPUT_RECORD *prec = rec;
    for (DWORD i = 0; i < size; ++i, ++p, ++prec) {
        prec->EventType = KEY_EVENT;
        prec->Event.KeyEvent.bKeyDown = TRUE;
        prec->Event.KeyEvent.wRepeatCount = 1;
        prec->Event.KeyEvent.uChar.UnicodeChar = *p;
    }

    HANDLE hInput = (HANDLE)_get_osfhandle(((winconsoleio*)file)->fd);
    if (hInput == INVALID_HANDLE_VALUE) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    DWORD total = 0;
    while (total < size) {
        DWORD wrote;
        if (!WriteConsoleInputW(hInput, &rec[total], (size - total), &wrote)) {
            PyErr_SetFromWindowsErr(0);
            goto error;
        }
        total += wrote;
    }

    PyMem_Free((void*)rec);

    Py_RETURN_NONE;
error:
    if (rec)
        PyMem_Free((void*)rec);
    return NULL;
}

/*[clinic input]
_testconsole.read_output
    file: object

Reads a str from the console as written to stdout.
[clinic start generated code]*/

static PyObject *
_testconsole_read_output_impl(PyObject *module, PyObject *file)
/*[clinic end generated code: output=876310d81a73e6d2 input=b3521f64b1b558e3]*/
{
    Py_RETURN_NONE;
}


#include "clinic\_testconsole.c.h"

PyMethodDef testconsole_methods[] = {
    _TESTCONSOLE_WRITE_INPUT_METHODDEF
    _TESTCONSOLE_READ_OUTPUT_METHODDEF
    {NULL, NULL}
};

static PyModuleDef testconsole_def = {
    PyModuleDef_HEAD_INIT,                      /* m_base */
    "_testconsole",                             /* m_name */
    PyDoc_STR("Test module for the Windows console"), /* m_doc */
    0,                                          /* m_size */
    testconsole_methods,                        /* m_methods */
    testconsole_slots,                          /* m_slots */
    NULL,                                       /* m_traverse */
    NULL,                                       /* m_clear */
    NULL,                                       /* m_free */
};

PyMODINIT_FUNC
PyInit__testconsole(PyObject *spec)
{
    return PyModuleDef_Init(&testconsole_def);
}

#endif /* MS_WINDOWS */
