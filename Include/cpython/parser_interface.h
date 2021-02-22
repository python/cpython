#ifndef Py_PEGENINTERFACE
#define Py_PEGENINTERFACE
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"

#ifndef Py_LIMITED_API
PyAPI_FUNC(struct _mod *) PyParser_ASTFromString(
    const char *str,
    const char *filename,
    int mode,
    PyCompilerFlags *flags,
    PyArena *arena);
PyAPI_FUNC(struct _mod *) PyParser_ASTFromStringObject(
    const char *str,
    PyObject* filename,
    int mode,
    PyCompilerFlags *flags,
    PyArena *arena);
PyAPI_FUNC(struct _mod *) PyParser_ASTFromFile(
    FILE *fp,
    const char *filename,
    const char* enc,
    int mode,
    const char *ps1,
    const char *ps2,
    PyCompilerFlags *flags,
    int *errcode,
    PyArena *arena);
PyAPI_FUNC(struct _mod *) PyParser_ASTFromFileObject(
    FILE *fp,
    PyObject *filename_ob,
    const char *enc,
    int mode,
    const char *ps1,
    const char *ps2,
    PyCompilerFlags *flags,
    int *errcode,
    PyArena *arena);
PyAPI_FUNC(struct _mod *) PyParser_ASTFromFilename(
    const char *filename,
    int mode,
    PyCompilerFlags *flags,
    PyArena *arena);
#endif /* !Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_PEGENINTERFACE */
