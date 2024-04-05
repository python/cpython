//#include <unistd.h>
//#include <stdio.h>
#include "parts.h"

char *filename = "pyrun_sample.py";

static FILE *create_tempscript(void) {
    FILE *fp;
    const char *script = "import sysconfig\n\nconfig = sysconfig.get_config_vars()\n";

    fp = tmpfile();
    if (fp == NULL) {
        PyErr_SetString(PyExc_LookupError, "tmpname");
        return NULL;
    }
    fprintf(fp, script);
    rewind(fp);

    return fp;
}

static PyObject *
test_pyrun_fileex(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *pdict, *pResult;
    FILE *fp;

    pdict = PyDict_New();
    if (pdict == NULL) {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
        return NULL;
    }

    fp = create_tempscript();
    if (fp == NULL) {
        return NULL;
    }

    pResult = PyRun_FileEx(fp, filename, Py_file_input, pdict, pdict, 1);
    if (pResult == NULL) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
test_pyrun_fileex_null_locals(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *pdict, *pResult;
    FILE *fp;

    pdict = PyDict_New();
    if (pdict == NULL) {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
        return NULL;
    }

    fp = create_tempscript();
    if (fp == NULL) {
        return NULL;
    }

    pResult = PyRun_FileEx(fp, filename, Py_file_input, pdict, NULL, 1);
    if (pResult == NULL) {
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject *
test_pyrun_fileex_null_globals(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *pdict, *pResult, *exc;
    FILE *fp;
    char *filename = "pyrun_sample.py";

    pdict = PyDict_New();
    if (pdict == NULL) {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
        return NULL;
    }

    fp = create_tempscript();
    if (fp == NULL) {
        return NULL;
    }

    pResult = PyRun_FileEx(fp, filename, Py_file_input, NULL, pdict, 1);

    exc = PyErr_GetRaisedException();
    assert(exc);
    assert(PyErr_GivenExceptionMatches(exc, PyExc_SystemError));
    PyErr_Clear();

    Py_RETURN_NONE;
}

static PyObject *
test_pyrun_fileex_null_globals_locals(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *pdict, *pResult, *exc;
    FILE *fp;
    char *filename = "pyrun_sample.py";

    pdict = PyDict_New();
    if (pdict == NULL) {
        assert(PyErr_ExceptionMatches(PyExc_MemoryError));
        return NULL;
    }

    fp = create_tempscript();
    if (fp == NULL) {
        return NULL;
    }

    pResult = PyRun_FileEx(fp, filename, Py_file_input, NULL, NULL, 1);
    exc = PyErr_GetRaisedException();
    assert(exc);
    assert(PyErr_GivenExceptionMatches(exc, PyExc_SystemError));
    PyErr_Clear();

    Py_RETURN_NONE;
}

static PyMethodDef test_methods[] = {
    {"test_pyrun_fileex", test_pyrun_fileex, METH_NOARGS},
    {"test_pyrun_fileex_null_locals", test_pyrun_fileex_null_locals, METH_NOARGS},
    {"test_pyrun_fileex_null_globals", test_pyrun_fileex_null_globals, METH_NOARGS},
    {"test_pyrun_fileex_null_globals_locals", test_pyrun_fileex_null_globals_locals, METH_NOARGS},
    {NULL},
};

int
_PyTestCapi_Init_PyRun(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
