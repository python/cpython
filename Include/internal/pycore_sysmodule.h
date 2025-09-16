#ifndef Py_INTERNAL_SYSMODULE_H
#define Py_INTERNAL_SYSMODULE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_pickle' shared extension
PyAPI_FUNC(size_t) _PySys_GetSizeOf(PyObject *);

extern int _PySys_SetAttr(PyObject *, PyObject *);

extern int _PySys_ClearAttrString(PyInterpreterState *interp,
                                  const char *name, int verbose);

extern int _PySys_SetFlagObj(Py_ssize_t pos, PyObject *new_value);
extern int _PySys_SetIntMaxStrDigits(int maxdigits);

extern int _PySysRemoteDebug_SendExec(int pid, int tid, const char *debugger_script_path);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_SYSMODULE_H */
