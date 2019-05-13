#ifndef Py_CPYTHON_SYSMODULE_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) _PySys_GetObjectId(_Py_Identifier *key);
PyAPI_FUNC(int) _PySys_SetObjectId(_Py_Identifier *key, PyObject *);

PyAPI_FUNC(size_t) _PySys_GetSizeOf(PyObject *);

PyAPI_FUNC(int) PySys_Audit(const char*, const char *, ...);
PyAPI_FUNC(int) PySys_AddAuditHook(void*, void*);

#ifdef __cplusplus
}
#endif
