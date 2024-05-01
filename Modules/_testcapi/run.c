#define PYTESTCAPI_NEED_INTERNAL_API
#include "parts.h"
#include "util.h"
#include "pycore_fileutils.h"     // _Py_IsValidFD()

#include <stdio.h>
#include <errno.h>


static PyObject *
run_stringflags(PyObject *mod, PyObject *pos_args)
{
    const char *str;
    Py_ssize_t size;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;

    if (!PyArg_ParseTuple(pos_args, "z#iO|Oii",
                          &str, &size, &start, &globals, &locals,
                          &cf_flags, &cf_feature_version)) {
        return NULL;
    }

    NULLABLE(globals);
    NULLABLE(locals);
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    return PyRun_StringFlags(str, start, globals, locals, pflags);
}

static PyObject *
run_fileexflags(PyObject *mod, PyObject *pos_args)
{
    PyObject *result = NULL;
    const char *filename = NULL;
    Py_ssize_t filename_size;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    int closeit = 0;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;

    FILE *fp = NULL;

    if (!PyArg_ParseTuple(pos_args, "z#iO|Oiii",
                          &filename, &filename_size, &start, &globals, &locals,
                          &closeit, &cf_flags, &cf_feature_version)) {
        return NULL;
    }

    NULLABLE(globals);
    NULLABLE(locals);
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }
    int fd = fileno(fp);

    result = PyRun_FileExFlags(fp, filename, start, globals, locals, closeit, pflags);

    if (closeit && result && _Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "File was not closed after excution");
        Py_DECREF(result);
        fclose(fp);
        return NULL;
    }

    if (!closeit && !_Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "Bad file descriptor after excution");
        Py_XDECREF(result);
        return NULL;
    }

    if (!closeit) {
        fclose(fp); /* don't need open file any more*/
    }

    return result;
}

static PyMethodDef test_methods[] = {
    {"run_stringflags", run_stringflags, METH_VARARGS},
    {"run_fileexflags", run_fileexflags, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Run(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    return 0;
}
