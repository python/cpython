#ifndef Py_INTERNAL_AST_H
#define Py_INTERNAL_AST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "Python-ast.h"           // expr_ty

extern int _PyAST_Validate(mod_ty);

/* _PyAST_ExprAsUnicode is defined in ast_unparse.c */
extern PyObject* _PyAST_ExprAsUnicode(expr_ty);

/* Return the borrowed reference to the first literal string in the
   sequence of statements or NULL if it doesn't start from a literal string.
   Doesn't set exception. */
extern PyObject* _PyAST_GetDocString(asdl_stmt_seq *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_AST_H */

