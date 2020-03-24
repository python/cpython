#include <pegen_interface.h>

#include "../tokenizer.h"
#include "pegen.h"

mod_ty
PyPegen_ASTFromString(const char *str, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString("<string>");
    if (filename_ob == NULL) {
        return NULL;
    }

    mod_ty result = run_parser_from_string(str, START, filename_ob, arena);
    Py_XDECREF(filename_ob);
    return result;
}

mod_ty
PyPegen_ASTFromFile(const char *filename, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        return NULL;
    }

    mod_ty result = run_parser_from_file(filename, START, filename_ob, arena);
    Py_XDECREF(filename_ob);
    return result;
}

PyCodeObject *
PyPegen_CodeObjectFromString(const char *str)
{
    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyCodeObject *result = NULL;

    PyObject *filename_ob = PyUnicode_FromString("<string>");
    if (filename_ob == NULL) {
        goto error;
    }

    mod_ty res = PyPegen_ASTFromString(str, arena);
    if (res == NULL) {
        goto error;
    }

    result = PyAST_CompileObject(res, filename_ob, NULL, -1, arena);

error:
    Py_XDECREF(filename_ob);
    PyArena_Free(arena);
    return result;
}

PyCodeObject *
PyPegen_CodeObjectFromFile(const char *filename)
{
    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyCodeObject *result = NULL;

    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        goto error;
    }

    mod_ty res = PyPegen_ASTFromFile(filename, arena);
    if (res == NULL) {
        goto error;
    }

    result = PyAST_CompileObject(res, filename_ob, NULL, -1, arena);

error:
    Py_XDECREF(filename_ob);
    PyArena_Free(arena);
    return result;
}
