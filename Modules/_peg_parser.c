#include <Python.h>
#include "pegen_interface.h"

PyObject *
_Py_parse_string(PyObject *self, PyObject *args, PyObject *kwds)
{
    static char *keywords[] = {"string", "mode", "oldparser", "bytecode", NULL};
    char *the_string;
    char *mode_str = "exec";
    int oldparser = 0;
    int bytecode = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "s|spp", keywords,
            &the_string, &mode_str, &oldparser, &bytecode)) {
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

    PyCompilerFlags flags = _PyCompilerFlags_INIT;
    flags.cf_flags = PyCF_IGNORE_COOKIE;

    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    mod_ty mod;
    if (!oldparser) {
        mod = PyPegen_ASTFromString(the_string, "<string>", mode, &flags, arena);
    }
    else {
        mod = PyParser_ASTFromString(the_string, "<string>", mode, &flags, arena);
    }
    if (mod == NULL) {
        goto error;
    }

    if (!bytecode) {
        PyObject *result = PyAST_mod2obj(mod);
        PyArena_Free(arena);
        return result;
    }

    PyObject *filename = PyUnicode_DecodeFSDefault("<string>");
    if (!filename) {
        goto error;
    }
    PyCodeObject *co = PyAST_CompileObject(mod, filename, &flags, -1, arena);
    Py_XDECREF(filename);
    PyArena_Free(arena);
    return (PyObject *)co;

error:
    PyArena_Free(arena);
    return NULL;
}

static PyMethodDef ParseMethods[] = {
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
