#ifndef Py_LIMITED_API
#ifndef Py_AST_H
#define Py_AST_H
#ifdef __cplusplus
extern "C" {
#endif

#include "Python-ast.h"   /* mod_ty */

PyAPI_FUNC(int) PyAST_Validate(mod_ty);

#ifdef __cplusplus
}
#endif
#endif /* !Py_AST_H */
#endif /* !Py_LIMITED_API */
