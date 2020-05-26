
/* Testing module for multi-phase initialization of extension modules (PEP 489)
 */

#include "Python.h"

#ifdef MS_WINDOWS

#include "..\modules\_io\_iomodule.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>

 /* The full definition is in iomodule. We reproduce
 enough here to get the handle, which is all we want. */
typedef struct {
    PyObject_HEAD
    HANDLE handle;
} winconsoleio;


static int execfunc(PyObject *m)
{
    return 0;
}

PyModuleDef_Slot testconsole_slots[] = {
    {Py_mod_exec, execfunc},
    {0, NULL},
};

/*[clinic input]
module _testconsole

_testconsole.write_input
    file: object
    s: PyBytesObject

Writes UTF-16-LE encoded bytes to the console as if typed by a user.
[clinic start generated code]*/

static PyObject *
_testconsole_write_input_impl(PyObject *module, PyObject *file,
                              PyBytesObject *s)
/*[clinic end generated code: output=48f9563db34aedb3 input=4c774f2d05770bc6]*/
{
    INPUT_RECORD *rec = NULL;

    if (!PyWindowsConsoleIO_Check(file)) {
        PyErr_SetString(PyExc_TypeError, "expected raw console object");
        return NULL;
    }

    const wchar_t *p = (const wchar_t *)PyBytes_AS_STRING(s);
    DWORD size = (DWORD)PyBytes_GET_SIZE(s) / sizeof(wchar_t);

    rec = (INPUT_RECORD*)PyMem_Calloc(size, sizeof(INPUT_RECORD));
    if (!rec)
        goto error;

    INPUT_RECORD *prec = rec;
    for (DWORD i = 0; i < size; ++i, ++p, ++prec) {
        prec->EventType = KEY_EVENT;
        prec->Event.KeyEvent.bKeyDown = TRUE;
        prec->Event.KeyEvent.wRepeatCount = 10;
        prec->Event.KeyEvent.uChar.UnicodeChar = *p;
    }

    HANDLE hInput = ((winconsoleio*)file)->handle;
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
