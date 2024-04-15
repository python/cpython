#include "parts.h"
#include "util.h"

#include <stdio.h>
#include <errno.h>


static PyObject *
run_fileexflags(PyObject *mod, PyObject *pos_args)
{
    PyObject *result = NULL;
    PyObject *filenamebytes;
    const char *filename = NULL;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    int closeit = 0;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;

    FILE *fp = NULL;

    if (!PyArg_ParseTuple(pos_args,
                        "O&iO|Oiii:eval_pyrun_fileexflags",
                        PyUnicode_FSConverter, &filenamebytes,
                        &start,
                        &globals,
                        &locals,
                        &closeit,
                        &cf_flags,
                        &cf_feature_version)) {
        return NULL;
    }

    NULLABLE(globals);
    NULLABLE(locals);
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    filename = PyBytes_AS_STRING(filenamebytes);
    fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        goto exit;
    }

    result = PyRun_FileExFlags(fp, filename, start, globals, locals, closeit, pflags);

#if !defined(__wasi__)
    /* The behavior of fileno() after fclose() is undefined. */
    if (closeit && result && fileno(fp) >= 0) {
        PyErr_SetString(PyExc_AssertionError, "File was not closed after excution");
        Py_CLEAR(result);
        fclose(fp);
        goto exit;
    }
#endif
    if (!closeit && fileno(fp) < 0) {
        PyErr_SetString(PyExc_AssertionError, "Bad file descriptor after excution");
        Py_CLEAR(result);
        goto exit;
    }

    if (!closeit) {
        fclose(fp); /* don't need open file any more*/
    }

exit:
    Py_DECREF(filenamebytes);
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
