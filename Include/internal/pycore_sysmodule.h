#ifndef Py_INTERNAL_SYSMODULE_H
#define Py_INTERNAL_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

PyAPI_FUNC(int) _PySys_GetOptionalAttr(PyObject *, PyObject **);
PyAPI_FUNC(int) _PySys_GetOptionalAttrString(const char *, PyObject **);
PyAPI_FUNC(PyObject *) _PySys_GetRequiredAttr(PyObject *);
PyAPI_FUNC(PyObject *) _PySys_GetRequiredAttrString(const char *);

PyAPI_FUNC(int) _PySys_Audit(
    PyThreadState *tstate,
    const char *event,
    const char *argFormat,
    ...);

/* We want minimal exposure of this function, so use extern rather than
   PyAPI_FUNC() to not export the symbol. */
extern void _PySys_ClearAuditHooks(PyThreadState *tstate);

PyAPI_FUNC(int) _PySys_SetAttr(PyObject *, PyObject *);

extern int _PySys_ClearAttrString(PyInterpreterState *interp,
                                  const char *name, int verbose);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SYSMODULE_H */
