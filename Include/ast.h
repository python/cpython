#ifndef Py_AST_H
#define Py_AST_H
#ifdef __cplusplus
extern "C" {
#endif

extern DL_IMPORT(mod_ty) PyAST_FromNode(const node *, PyCompilerFlags *flags,
					const char *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_AST_H */
