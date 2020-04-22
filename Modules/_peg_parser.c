#include <Python.h>
#include "pegen_interface.h"

PyObject *
_Py_parse_file(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"file", "mode", NULL};
    char *filename;
    char *mode_str = "exec";

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s", keywords, &filename, &mode_str)) {
        return NULL;
    }

    int mode;
    if (strcmp(mode_str, "exec") == 0) {
        mode = Py_file_input;
    }
    else if (strcmp(mode_str, "single") == 0) {
        mode = Py_single_input;
    }
    else {
        return PyErr_Format(PyExc_ValueError, "mode must be either 'exec' or 'single'");
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyObject *result = NULL;

    mod_ty res = PyPegen_ASTFromFile(filename, mode, arena);
    if (res == NULL) {
        goto error;
    }
    result = PyAST_mod2obj(res);

error:
    PyArena_Free(arena);
    return result;
}

PyObject *
_Py_parse_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"string", "mode", NULL};
    char *the_string;
    char *mode_str = "exec";

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|s", keywords, &the_string, &mode_str)) {
        return NULL;
    }

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
        return PyErr_Format(PyExc_ValueError, "mode must be either 'exec' or 'eval' or 'single'");
    }

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyObject *result = NULL;

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    flags.cf_flags = PyCF_IGNORE_COOKIE;

    mod_ty res = PyPegen_ASTFromString(the_string, mode, &flags, arena);
    if (res == NULL) {
        goto error;
    }
    result = PyAST_mod2obj(res);

error:
    PyArena_Free(arena);
    return result;
}

static PyMethodDef ParseMethods[] = {
    {"parse_file", (PyCFunction)(void (*)(void))_Py_parse_file, METH_VARARGS|METH_KEYWORDS, "Parse a file."},
    {"parse_string", (PyCFunction)(void (*)(void))_Py_parse_string, METH_VARARGS|METH_KEYWORDS,"Parse a string."},
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
