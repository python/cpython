#include "pegen_interface.h"

#include "../tokenizer.h"
#include "pegen.h"

mod_ty
PyPegen_ASTFromString(const char *str, const char *filename, int mode,
                      PyCompilerFlags *flags, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        return NULL;
    }
    mod_ty result = PyPegen_ASTFromStringObject(str, filename_ob, mode, flags, arena);
    Py_XDECREF(filename_ob);
    return result;
}

mod_ty
PyPegen_ASTFromStringObject(const char *str, PyObject* filename, int mode,
                            PyCompilerFlags *flags, PyArena *arena)
{
    if (PySys_Audit("compile", "yO", str, filename) < 0) {
        return NULL;
    }

    mod_ty result = _PyPegen_run_parser_from_string(str, mode, filename, flags, arena);
    return result;
}

mod_ty
PyPegen_ASTFromFilename(const char *filename, int mode, PyCompilerFlags *flags, PyArena *arena)
{
    PyObject *filename_ob = PyUnicode_FromString(filename);
    if (filename_ob == NULL) {
        return NULL;
    }

    mod_ty result = _PyPegen_run_parser_from_file(filename, mode, filename_ob, flags, arena);
    Py_XDECREF(filename_ob);
    return result;
}

mod_ty
PyPegen_ASTFromFileObject(FILE *fp, PyObject *filename_ob, int mode,
                          const char *enc, const char *ps1, const char* ps2,
                          PyCompilerFlags *flags, int *errcode, PyArena *arena)
{
    if (PySys_Audit("compile", "OO", Py_None, filename_ob) < 0) {
        return NULL;
    }
    return _PyPegen_run_parser_from_file_pointer(fp, mode, filename_ob, enc, ps1, ps2,
                                        flags, errcode, arena);
}
