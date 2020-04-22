#ifndef Py_LIMITED_API
#ifndef Py_PEGENINTERFACE
#define Py_PEGENINTERFACE
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include "Python-ast.h"

PyAPI_FUNC(mod_ty) PyPegen_ASTFromFile(const char *filename, int mode, PyArena *arena);
PyAPI_FUNC(mod_ty) PyPegen_ASTFromString(const char *str, int mode, PyCompilerFlags *flags,
                                         PyArena *arena);
PyAPI_FUNC(mod_ty) PyPegen_ASTFromStringObject(const char *str, PyObject* filename, int mode,
                                               PyCompilerFlags *flags, PyArena *arena);
PyAPI_FUNC(mod_ty) PyPegen_ASTFromFileObject(FILE *fp, PyObject *filename_ob,
                                             int mode, const char *enc, const char *ps1,
                                             const char *ps2, int *errcode, PyArena *arena);
PyAPI_FUNC(PyCodeObject *) PyPegen_CodeObjectFromFile(const char *filename, int mode);
PyAPI_FUNC(PyCodeObject *) PyPegen_CodeObjectFromString(const char *str, int mode,
                                                        PyCompilerFlags *flags);
PyAPI_FUNC(PyCodeObject *) PyPegen_CodeObjectFromFileObject(FILE *, PyObject *filename_ob,
                                                            int mode, const char *enc,
                                                            const char *ps1,
                                                            const char *ps2,
                                                            int *errcode);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PEGENINTERFACE*/
#endif /* !Py_LIMITED_API */
