#include "Python.h"

#include "pegen.h"

mod_ty
_PyParser_ASTFromString(const char *str, PyObject* filename, int mode,
                        PyCompilerFlags *flags, PyArena *arena)
{
    if (PySys_Audit("compile", "yO", str, filename) < 0) {
        return NULL;
    }

    mod_ty result = _PyPegen_run_parser_from_string(str, mode, filename, flags, arena);
    return result;
}

mod_ty
_PyParser_ASTFromFile(FILE *fp, PyObject *filename_ob, const char *enc,
                      int mode, const char *ps1, const char* ps2,
                      PyCompilerFlags *flags, int *errcode, PyArena *arena)
{
    if (PySys_Audit("compile", "OO", Py_None, filename_ob) < 0) {
        return NULL;
    }
    return _PyPegen_run_parser_from_file_pointer(fp, mode, filename_ob, enc, ps1, ps2,
                                        flags, errcode, NULL, arena);
}

mod_ty
_PyParser_InteractiveASTFromFile(FILE *fp, PyObject *filename_ob, const char *enc,
                                 int mode, const char *ps1, const char* ps2,
                                 PyCompilerFlags *flags, int *errcode,
                                 PyObject **interactive_src, PyArena *arena)
{
    if (PySys_Audit("compile", "OO", Py_None, filename_ob) < 0) {
        return NULL;
    }
    return _PyPegen_run_parser_from_file_pointer(fp, mode, filename_ob, enc, ps1, ps2,
                                                 flags, errcode, interactive_src, arena);
}