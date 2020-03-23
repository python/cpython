#ifndef Py_LIMITED_API
#ifndef Py_PEGENINTERFACE
#define Py_PEGENINTERFACE
#ifdef __cplusplus
extern "C" {
#endif

#include "Python.h"
#include "Python-ast.h"

PyAPI_FUNC(mod_ty) PyPegen_ASTFromFile(const char *filename, PyArena *arena);
PyAPI_FUNC(mod_ty) PyPegen_ASTFromString(const char *str, PyArena *arena);
PyAPI_FUNC(PyCodeObject *) PyPegen_CodeObjectFromFile(const char *filename, PyArena *arena);
PyAPI_FUNC(PyCodeObject *) PyPegen_CodeObjectFromString(const char *str, PyArena *arena);

#ifdef __cplusplus
}
#endif
#endif /* !Py_PEGENINTERFACE*/
#endif /* !Py_LIMITED_API */
