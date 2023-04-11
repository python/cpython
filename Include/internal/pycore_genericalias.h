#ifdef __cplusplus
extern "C" {
#endif

/* runtime lifecycle */

extern PyStatus _PyGenericAlias_Init(PyInterpreterState *interp);
extern void _PyGenericAlias_Fini(PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
