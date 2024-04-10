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
    int fn = -1;

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

    fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        goto exit;
    }

    result = PyRun_FileExFlags(fp, filename, start, globals, locals, closeit, pflags);

/* Test fails in WASI */
#if !defined(__wasi__)
    fn = fileno(fp);
    if (result) {
        if (closeit && fn >= 0) {
            PyErr_SetString(PyExc_AssertionError, "File was not closed after excution");
            goto exit;
        }
        else if (!closeit && fn < 0) {
            PyErr_SetString(PyExc_AssertionError, "Bad file descriptor after excution");
            goto exit;
        }
    }
#endif

exit:

#if !defined(__wasi__)
    if (fn >= 0) {
        fclose(fp); /* don't need open file any more*/
    }
#endif

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
