#include <pegen_interface.h>

#include "../tokenizer.h"
#include "pegen.h"

mod_ty
PyPegen_ASTFromString(const char *str, int mode, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString("<string>");
    if (filename_ob == NULL) {
        return NULL;
    }

    mod_ty result = run_parser_from_string(str, mode, filename_ob, arena);
    Py_XDECREF(filename_ob);
    return result;
}

mod_ty
PyPegen_ASTFromFile(const char *filename, int mode, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        return NULL;
    }

    mod_ty result = run_parser_from_file(filename, mode, filename_ob, arena);
    Py_XDECREF(filename_ob);
    return result;
}

mod_ty
PyPegen_ASTFromFileObject(FILE *fp, PyObject *filename_ob, int mode,
                          const char *enc, const char *ps1, const char* ps2,
                          int *errcode, PyArena *arena)
{
    return run_parser_from_file_pointer(fp, mode, filename_ob, enc, ps1, ps2,
                                        errcode, arena);
}

PyCodeObject *
PyPegen_CodeObjectFromString(const char *str, int mode)
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

    mod_ty res = PyPegen_ASTFromString(str, mode, arena);
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
PyPegen_CodeObjectFromFile(const char *filename, int mode)
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

    mod_ty res = PyPegen_ASTFromFile(filename, mode, arena);
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
PyPegen_CodeObjectFromFileObject(FILE *fp, PyObject *filename_ob, int mode,
                                 const char *ps1, const char *ps2, const char *enc,
                                 int *errcode)
{
    PyArena *arena = PyArena_New();
    if (arena == NULL) {
        return NULL;
    }

    PyCodeObject *result = NULL;

    mod_ty res = PyPegen_ASTFromFileObject(fp, filename_ob, mode, enc, ps1, ps2,
                                           errcode, arena);
    if (res == NULL) {
        goto error;
    }

    result = PyAST_CompileObject(res, filename_ob, NULL, -1, arena);

error:
    PyArena_Free(arena);
    return result;
}
