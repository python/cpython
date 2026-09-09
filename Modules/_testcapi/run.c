#define PYTESTCAPI_NEED_INTERNAL_API
#include "parts.h"
#include "util.h"
#include "pycore_fileutils.h"     // _Py_IsValidFD()

#include <stdio.h>
#include <errno.h>


// Test functions, not macros
#undef PyRun_AnyFile
#undef PyRun_AnyFileEx
#undef PyRun_AnyFileFlags
#undef PyRun_File
#undef PyRun_FileEx
#undef PyRun_FileFlags
#undef PyRun_SimpleFile
#undef PyRun_SimpleFileEx
#undef PyRun_String
#undef PyRun_SimpleString
#undef Py_CompileString
#undef Py_CompileStringFlags
#undef PyRun_InteractiveOne
#undef PyRun_InteractiveLoop


// Test PyRun_String()
static PyObject*
run_string(PyObject *mod, PyObject *args)
{
    const char *str;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;

    if (!PyArg_ParseTuple(args, "yi|OO", &str, &start, &globals, &locals)) {
        return NULL;
    }
    NULLABLE(globals);
    NULLABLE(locals);

    return PyRun_String(str, start, globals, locals);
}

// Test PyRun_StringFlags()
static PyObject*
run_stringflags(PyObject *mod, PyObject *args)
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

    if (!PyArg_ParseTuple(args, "z#iO|Oii",
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

static FILE*
open_file(PyObject *filename, int *closeit)
{
    if (PyUnicode_EqualToUTF8(filename, "<stdin>")) {
        if (closeit) {
            // override closeit
            *closeit = 0;
        }
        return stdin;
    }
    return Py_fopen(filename, "r");
}


// Test PyRun_SimpleFile()
static PyObject*
run_simplefile(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;

    if (!PyArg_ParseTuple(args, "Oy", &open_filename, &filename)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_SimpleFile(fp, filename);

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

static int
close_fd(FILE *fp, int fd, int closeit)
{
    if (closeit && _Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "File was not closed after execution");
        fclose(fp);
        return -1;
    }

    if (!closeit && !_Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "Bad file descriptor after execution");
        return -1;
    }

    if (!closeit) {
        fclose(fp);
    }
    return 0;
}

static int
close_fd_result(FILE *fp, int fd, int closeit, PyObject *result)
{
    if (closeit && result && _Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "File was not closed after execution");
        Py_DECREF(result);
        fclose(fp);
        return -1;
    }

    if (!closeit && !_Py_IsValidFD(fd)) {
        PyErr_SetString(PyExc_AssertionError, "Bad file descriptor after execution");
        Py_XDECREF(result);
        return -1;
    }

    if (!closeit) {
        fclose(fp);
    }
    return 0;
}

// Test PyRun_SimpleFileEx()
static PyObject*
run_simplefileex(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;
    int closeit = 0;

    if (!PyArg_ParseTuple(args, "Oy|i",
                          &open_filename, &filename, &closeit)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, &closeit);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_SimpleFileEx(fp, filename, closeit);

    if (close_fd(fp, fd, closeit) < 0) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

// Test PyRun_SimpleFileExFlags()
static PyObject*
run_simplefileexflags(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;
    int closeit = 0;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "Oy|ii",
                          &open_filename, &filename, &closeit, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, &closeit);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_SimpleFileExFlags(fp, filename, closeit, pflags);

    if (close_fd(fp, fd, closeit) < 0) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

// Test PyRun_AnyFile()
static PyObject*
run_anyfile(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;

    if (!PyArg_ParseTuple(args, "Oy", &open_filename, &filename)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_AnyFile(fp, filename);

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

// Test PyRun_AnyFileFlags()
static PyObject*
run_anyfileflags(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "Oy|i",
                          &open_filename, &filename, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_AnyFileFlags(fp, filename, pflags);

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

// Test PyRun_AnyFileEx()
static PyObject*
run_anyfileex(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;
    int closeit = 0;

    if (!PyArg_ParseTuple(args, "Oy|i",
                          &open_filename, &filename, &closeit)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, &closeit);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_AnyFileEx(fp, filename, closeit);

    if (close_fd(fp, fd, closeit) < 0) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

// Test PyRun_AnyFileExFlags()
static PyObject*
run_anyfileexflags(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    const char *filename;
    int closeit = 0;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "Oy|ii",
                          &open_filename, &filename, &closeit, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, &closeit);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_AnyFileExFlags(fp, filename, closeit, pflags);

    if (close_fd(fp, fd, closeit) < 0) {
        return NULL;
    }

    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}


// Test PyRun_InteractiveOne()
static PyObject*
run_interactiveone(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    char *filename;

    if (!PyArg_ParseTuple(args, "Oy", &open_filename, &filename)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_InteractiveOne(fp, filename);
    assert(!PyErr_Occurred());

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return PyLong_FromLong(res);
}

// Test PyRun_InteractiveOneFlags()
static PyObject*
run_interactiveoneflags(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    char *filename;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "Oy|i",
                          &open_filename, &filename, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_InteractiveOneFlags(fp, filename, pflags);
    assert(!PyErr_Occurred());

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return PyLong_FromLong(res);
}

// Test PyRun_InteractiveOneObject()
static PyObject*
run_interactiveoneobject(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    PyObject *filename;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "OO|i",
                          &open_filename, &filename, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_InteractiveOneObject(fp, filename, pflags);
    assert(!PyErr_Occurred());

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return PyLong_FromLong(res);
}

// Test PyRun_File()
static PyObject*
run_file(PyObject *mod, PyObject *args)
{
    PyObject *result = NULL;
    const char *filename = NULL;
    Py_ssize_t filename_size;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;

    if (!PyArg_ParseTuple(args, "z#iO|O",
                          &filename, &filename_size, &start,
                          &globals, &locals)) {
        return NULL;
    }
    NULLABLE(globals);
    NULLABLE(locals);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }
    int fd = fileno(fp);

    result = PyRun_File(fp, filename, start, globals, locals);

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return result;
}

// Test PyRun_FileEx()
static PyObject*
run_fileex(PyObject *mod, PyObject *args)
{
    const char *filename = NULL;
    Py_ssize_t filename_size;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    int closeit = 0;

    if (!PyArg_ParseTuple(args, "z#iO|Oi",
                          &filename, &filename_size, &start, &globals, &locals,
                          &closeit)) {
        return NULL;
    }
    NULLABLE(globals);
    NULLABLE(locals);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }
    int fd = fileno(fp);

    PyObject *result = PyRun_FileEx(fp, filename, start, globals, locals,
                                    closeit);

    if (close_fd_result(fp, fd, closeit, result) < 0) {
        return NULL;
    }

    return result;
}

// Test PyRun_FileFlags
static PyObject*
run_fileflags(PyObject *mod, PyObject *args)
{
    const char *filename = NULL;
    Py_ssize_t filename_size;
    int start;
    PyObject *globals = NULL;
    PyObject *locals = NULL;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;

    if (!PyArg_ParseTuple(args, "z#iO|Oii",
                          &filename, &filename_size, &start, &globals, &locals,
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

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }
    int fd = fileno(fp);

    PyObject *result = PyRun_FileFlags(fp, filename, start, globals, locals,
                                       pflags);

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return result;
}

static PyObject*
run_fileexflags(PyObject *mod, PyObject *args)
{
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

    if (!PyArg_ParseTuple(args, "z#iO|Oiii",
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

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        PyErr_SetFromErrnoWithFilename(PyExc_OSError, filename);
        return NULL;
    }
    int fd = fileno(fp);

    PyObject *result = PyRun_FileExFlags(fp, filename, start, globals, locals,
                                         closeit, pflags);

    if (close_fd_result(fp, fd, closeit, result) < 0) {
        return NULL;
    }

    return result;
}

static PyObject*
run_simplestring(PyObject *mod, PyObject *args)
{
    const char *str;
    if (!PyArg_ParseTuple(args, "y", &str)) {
        return NULL;
    }

    int res = PyRun_SimpleString(str);
    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}

static PyObject*
run_simplestringflags(PyObject *mod, PyObject *args)
{
    const char *str;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "y|i", &str, &flags)) {
        return NULL;
    }

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_SimpleStringFlags(str, pflags);
    assert(!PyErr_Occurred());
    return PyLong_FromLong(res);
}


// Test PyRun_InteractiveLoop()
static PyObject*
run_interactiveloop(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    char *filename;

    if (!PyArg_ParseTuple(args, "Oy|i", &open_filename, &filename)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    int res = PyRun_InteractiveLoop(fp, filename);
    assert(!PyErr_Occurred());

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return PyLong_FromLong(res);
}


// Test PyRun_InteractiveLoopFlags()
static PyObject*
run_interactiveloopflags(PyObject *mod, PyObject *args)
{
    PyObject *open_filename;
    char *filename;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;

    if (!PyArg_ParseTuple(args, "Oy|i",
                          &open_filename, &filename, &cf_flags)) {
        return NULL;
    }

    FILE *fp = open_file(open_filename, NULL);
    if (fp == NULL) {
        return NULL;
    }
    int fd = fileno(fp);

    if (cf_flags) {
        flags.cf_flags = cf_flags;
        pflags = &flags;
    }

    int res = PyRun_InteractiveLoopFlags(fp, filename, pflags);
    assert(!PyErr_Occurred());

    assert(_Py_IsValidFD(fd));
    fclose(fp);

    return PyLong_FromLong(res);
}


// Test Py_CompileStringFlags()
static PyObject*
run_compilestringflags(PyObject *mod, PyObject *args)
{
    const char *str;
    const char *filename;
    int start;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;

    if (!PyArg_ParseTuple(args, "yyi|ii", &str, &filename, &start,
                          &cf_flags, &cf_feature_version)) {
        return NULL;
    }
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    return Py_CompileStringFlags(str, filename, start, pflags);
}


// Test Py_CompileStringExFlags()
static PyObject*
run_compilestringexflags(PyObject *mod, PyObject *args)
{
    const char *str;
    const char *filename;
    int start;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;
    int optimize = -1;

    if (!PyArg_ParseTuple(args, "yyi|iii", &str, &filename, &start,
                          &cf_flags, &cf_feature_version, &optimize)) {
        return NULL;
    }
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    return Py_CompileStringExFlags(str, filename, start, pflags, optimize);
}


// Test Py_CompileStringObject()
static PyObject*
run_compilestringobject(PyObject *mod, PyObject *args)
{
    const char *str;
    PyObject *filename;
    int start;
    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    PyCompilerFlags *pflags = NULL;
    int cf_flags = 0;
    int cf_feature_version = 0;
    int optimize = -1;

    if (!PyArg_ParseTuple(args, "yOi|iii", &str, &filename, &start,
                          &cf_flags, &cf_feature_version, &optimize)) {
        return NULL;
    }
    if (cf_flags || cf_feature_version) {
        flags.cf_flags = cf_flags;
        flags.cf_feature_version = cf_feature_version;
        pflags = &flags;
    }

    return Py_CompileStringObject(str, filename, start, pflags, optimize);
}


static PyMethodDef test_methods[] = {
    {"run_string", run_string, METH_VARARGS},
    {"run_stringflags", run_stringflags, METH_VARARGS},
    {"run_simplefile", run_simplefile, METH_VARARGS},
    {"run_simplefileex", run_simplefileex, METH_VARARGS},
    {"run_simplefileexflags", run_simplefileexflags, METH_VARARGS},
    {"run_anyfile", run_anyfile, METH_VARARGS},
    {"run_anyfileflags", run_anyfileflags, METH_VARARGS},
    {"run_anyfileex", run_anyfileex, METH_VARARGS},
    {"run_anyfileexflags", run_anyfileexflags, METH_VARARGS},
    {"run_interactiveone", run_interactiveone, METH_VARARGS},
    {"run_interactiveoneflags", run_interactiveoneflags, METH_VARARGS},
    {"run_interactiveoneobject", run_interactiveoneobject, METH_VARARGS},
    {"run_file", run_file, METH_VARARGS},
    {"run_fileex", run_fileex, METH_VARARGS},
    {"run_fileflags", run_fileflags, METH_VARARGS},
    {"run_fileexflags", run_fileexflags, METH_VARARGS},
    {"run_simplestring", run_simplestring, METH_VARARGS},
    {"run_simplestringflags", run_simplestringflags, METH_VARARGS},
    {"run_interactiveloop", run_interactiveloop, METH_VARARGS},
    {"run_interactiveloopflags", run_interactiveloopflags, METH_VARARGS},
    {"run_compilestringflags", run_compilestringflags, METH_VARARGS},
    {"run_compilestringexflags", run_compilestringexflags, METH_VARARGS},
    {"run_compilestringobject", run_compilestringobject, METH_VARARGS},
    {NULL},
};

int
_PyTestCapi_Init_Run(PyObject *mod)
{
    if (PyModule_AddFunctions(mod, test_methods) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(mod, PyCF_ONLY_AST) < 0) {
        return -1;
    }
    if (PyModule_AddIntMacro(mod, PyCF_IGNORE_COOKIE) < 0) {
        return -1;
    }
    return 0;
}
