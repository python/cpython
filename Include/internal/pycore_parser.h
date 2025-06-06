#ifndef Py_INTERNAL_PARSER_H
#define Py_INTERNAL_PARSER_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


#include "pycore_ast.h"             // struct _expr
#include "pycore_global_strings.h"  // _Py_DECLARE_STR()
#include "pycore_pyarena.h"         // PyArena

_Py_DECLARE_STR(empty, "")
#if defined(Py_DEBUG) && defined(Py_GIL_DISABLED)
#define _parser_runtime_state_INIT \
    { \
        .mutex = {0}, \
        .dummy_name = { \
            .kind = Name_kind, \
            .v.Name.id = &_Py_STR(empty), \
            .v.Name.ctx = Load, \
            .lineno = 1, \
            .col_offset = 0, \
            .end_lineno = 1, \
            .end_col_offset = 0, \
        }, \
    }
#else
#define _parser_runtime_state_INIT \
    { \
        .dummy_name = { \
            .kind = Name_kind, \
            .v.Name.id = &_Py_STR(empty), \
            .v.Name.ctx = Load, \
            .lineno = 1, \
            .col_offset = 0, \
            .end_lineno = 1, \
            .end_col_offset = 0, \
        }, \
    }
#endif

extern struct _mod* _PyParser_ASTFromString(
    const char *str,
    PyObject* filename,
    int mode,
    PyCompilerFlags *flags,
    PyArena *arena);

extern struct _mod* _PyParser_ASTFromFile(
    FILE *fp,
    PyObject *filename_ob,
    const char *enc,
    int mode,
    const char *ps1,
    const char *ps2,
    PyCompilerFlags *flags,
    int *errcode,
    PyArena *arena);
extern struct _mod* _PyParser_InteractiveASTFromFile(
    FILE *fp,
    PyObject *filename_ob,
    const char *enc,
    int mode,
    const char *ps1,
    const char *ps2,
    PyCompilerFlags *flags,
    int *errcode,
    PyObject **interactive_src,
    PyArena *arena);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PARSER_H */
