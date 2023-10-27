#ifndef Py_INTERNAL_SYSMODULE_H
#define Py_INTERNAL_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_pickle' shared extension
PyAPI_FUNC(PyObject*) _PySys_GetAttr(PyThreadState *tstate, PyObject *name);

// Export for '_pickle' shared extension
PyAPI_FUNC(size_t) _PySys_GetSizeOf(PyObject *);

extern int _PySys_Audit(
    PyThreadState *tstate,
    const char *event,
    const char *argFormat,
    ...);

// _PySys_ClearAuditHooks() must not be exported: use extern rather than
// PyAPI_FUNC(). We want minimal exposure of this function.
extern void _PySys_ClearAuditHooks(PyThreadState *tstate);

extern int _PySys_SetAttr(PyObject *, PyObject *);

extern int _PySys_ClearAttrString(PyInterpreterState *interp,
                                  const char *name, int verbose);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SYSMODULE_H */
