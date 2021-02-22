#ifndef Py_LIMITED_API
#ifndef Py_AST_H
#define Py_AST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python-ast.h"   /* mod_ty */

PyAPI_FUNC(int) PyAST_Validate(mod_ty);

/* _PyAST_ExprAsUnicode is defined in ast_unparse.c */
PyAPI_FUNC(PyObject *) _PyAST_ExprAsUnicode(expr_ty);

/* Return the borrowed reference to the first literal string in the
   sequence of statements or NULL if it doesn't start from a literal string.
   Doesn't set exception. */
PyAPI_FUNC(PyObject *) _PyAST_GetDocString(asdl_stmt_seq *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_AST_H */
#endif /* !Py_LIMITED_API */
