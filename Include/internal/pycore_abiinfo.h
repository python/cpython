#ifndef Py_INTERNAL_ABIINFO_H
#define Py_INTERNAL_ABIINFO_H
#ifdef __cplusplus
extern "C" {
#endif

extern PyObject *_PyAbiInfo_GetInfo(void);
extern PyStatus _PyAbiInfo_InitTypes(PyInterpreterState *);
extern void _PyAbiInfo_FiniTypes(PyInterpreterState *interp);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ABIINFO_H */
