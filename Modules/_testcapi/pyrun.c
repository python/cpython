#include "parts.h"
#include "util.h"

#include <stdio.h>
#include <errno.h>


static PyObject *
run_fileexflags(PyObject *mod, PyObject *pos_args)
{
    PyObject *result = NULL;
    const char *filename = NULL;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    int closeit = 1;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;
    int fn;

    FILE *fp = NULL;

    if (!PyArg_ParseTuple(pos_args,
                        "ziO|Oiii:eval_pyrun_fileexflags",
                        &filename,
                        &start,
                        &globals,
                        &locals,
                        &closeit,
                        &cf_flags,
                        &cf_feature_version)) {
        goto exit;
    }

    NULLABLE(globals);
    NULLABLE(locals);
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    if (globals && !PyDict_Check(globals)) {
        PyErr_SetString(PyExc_TypeError, "globals must be a dict");
        goto exit;
    }

    if (locals && !PyMapping_Check(locals)) {
        PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
        goto exit;
    }

    fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        goto exit;
    }

    result = PyRun_FileExFlags(fp, filename, start, globals, locals, closeit, pflags);

    if (result) {
        fn = fileno(fp);
        if (closeit && fn > 0) {
            PyErr_SetString(PyExc_SystemError, "File was not closed after excution");
            goto exit;
        }
        else if (!closeit && fn < 0) {
            PyErr_SetString(PyExc_SystemError, "Bad file descriptor after excution");
            goto exit;
        }
    }

exit:

    return result;
}

static PyMethodDef test_methods[] = {
    {"run_fileexflags", run_fileexflags, METH_VARARGS},
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
