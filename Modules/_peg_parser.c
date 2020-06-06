#include <Python.h>
#include "pegen_interface.h"

static int
_mode_str_to_int(char *mode_str)
{
    int mode;
    if (strcmp(mode_str, "exec") == 0) {
        mode = Py_file_input;
    }
    else if (strcmp(mode_str, "eval") == 0) {
        mode = Py_eval_input;
    }
    else if (strcmp(mode_str, "single") == 0) {
        mode = Py_single_input;
    }
    else {
        mode = -1;
    }
    return mode;
}

static mod_ty
_run_parser(char *str, char *filename, int mode, PyCompilerFlags *flags, PyArena *arena, int oldparser)
{
    mod_ty mod;
    if (!oldparser) {
        mod = PyPegen_ASTFromString(str, filename, mode, flags, arena);
    }
    else {
        mod = PyParser_ASTFromString(str, filename, mode, flags, arena);
    }
    return mod;
}

PyObject *
_Py_compile_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"string", "filename", "mode", "oldparser", NULL};
    char *the_string;
    char *filename = "<string>";
    char *mode_str = "exec";
    int oldparser = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|ssp", keywords,
            &the_string, &filename, &mode_str, &oldparser)) {
        return NULL;
    }

    int mode = _mode_str_to_int(mode_str);
    if (mode == -1) {
        return PyErr_Format(PyExc_ValueError, "mode must be either 'exec' or 'eval' or 'single'");
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    flags.cf_flags = PyCF_IGNORE_COOKIE;

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod = _run_parser(the_string, filename, mode, &flags, arena, oldparser);
    if (mod == NULL) {
        PyArena_Free(arena);
        return NULL;
    }

    PyObject *filename_ob = PyUnicode_DecodeFSDefault(filename);
    if (filename_ob == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    PyCodeObject *result = PyAST_CompileObject(mod, filename_ob, &flags, -1, arena);
    Py_XDECREF(filename_ob);
    PyArena_Free(arena);
    return (PyObject *)result;
}

PyObject *
_Py_parse_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"string", "filename", "mode", "oldparser", "ast", NULL};
    char *the_string;
    char *filename = "<string>";
    char *mode_str = "exec";
    int oldparser = 0;
    int ast = 1;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|sspp", keywords,
            &the_string, &filename, &mode_str, &oldparser, &ast)) {
        return NULL;
    }

    int mode = _mode_str_to_int(mode_str);
    if (mode == -1) {
        return PyErr_Format(PyExc_ValueError, "mode must be either 'exec' or 'eval' or 'single'");
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    flags.cf_flags = PyCF_IGNORE_COOKIE;

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod = _run_parser(the_string, filename, mode, &flags, arena, oldparser);
    if (mod == NULL) {
        PyArena_Free(arena);
        return NULL;
    }

    PyObject *result;
    if (ast) {
        result = PyAST_mod2obj(mod);
    }
    else {
        Py_INCREF(Py_None);
        result = Py_None;
    }
    PyArena_Free(arena);
    return result;
}

static PyMethodDef ParseMethods[] = {
    {
        "parse_string",
        (PyCFunction)(void (*)(void))_Py_parse_string,
        METH_VARARGS|METH_KEYWORDS,
        "Parse a string, return an AST."
    },
    {
        "compile_string",
        (PyCFunction)(void (*)(void))_Py_compile_string,
        METH_VARARGS|METH_KEYWORDS,
        "Compile a string, return a code object."
    },
    {NULL, NULL, 0, NULL} /* Sentinel */
};

static struct PyModuleDef parsemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "peg_parser",
    .m_doc = "A parser.",
    .m_methods = ParseMethods,
};

PyMODINIT_FUNC
PyInit__peg_parser(void)
{
    return PyModule_Create(&parsemodule);
}
