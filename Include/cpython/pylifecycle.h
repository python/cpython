#ifndef Py_CPYTHON_PYLIFECYCLE_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Only used by applications that embed the interpreter and need to
 * override the standard encoding determination mechanism
 */
PyAPI_FUNC(int) Py_SetStandardStreamEncoding(const char *encoding,
                                             const char *errors);

/* PEP 432 Multi-phase initialization API (Private while provisional!) */

PyAPI_FUNC(_PyInitError) _Py_InitializeCore(
    PyInterpreterState **interp,
    const _PyCoreConfig *);
PyAPI_FUNC(int) _Py_IsCoreInitialized(void);


PyAPI_FUNC(_PyInitError) _PyMainInterpreterConfig_Read(
    _PyMainInterpreterConfig *config,
    const _PyCoreConfig *core_config);
PyAPI_FUNC(void) _PyMainInterpreterConfig_Clear(_PyMainInterpreterConfig *);
PyAPI_FUNC(int) _PyMainInterpreterConfig_Copy(
    _PyMainInterpreterConfig *config,
    const _PyMainInterpreterConfig *config2);
/* Used by _testcapi.get_main_config() */
PyAPI_FUNC(PyObject*) _PyMainInterpreterConfig_AsDict(
    const _PyMainInterpreterConfig *config);

PyAPI_FUNC(_PyInitError) _Py_InitializeMainInterpreter(
    PyInterpreterState *interp,
    const _PyMainInterpreterConfig *);

/* Initialization and finalization */

PyAPI_FUNC(_PyInitError) _Py_InitializeFromConfig(
    const _PyCoreConfig *config);
PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalInitError(_PyInitError err);

/* Py_PyAtExit is for the atexit module, Py_AtExit is for low-level
 * exit functions.
 */
PyAPI_FUNC(void) _Py_PyAtExit(void (*func)(PyObject *), PyObject *);

/* Restore signals that the interpreter has called SIG_IGN on to SIG_DFL. */
PyAPI_FUNC(void) _Py_RestoreSignals(void);

PyAPI_FUNC(int) Py_FdIsInteractive(FILE *, const char *);

PyAPI_FUNC(void) _Py_SetProgramFullPath(const wchar_t *);

PyAPI_FUNC(const char *) _Py_gitidentifier(void);
PyAPI_FUNC(const char *) _Py_gitversion(void);

/* Internal -- various one-time initializations */
PyAPI_FUNC(PyObject *) _PyBuiltin_Init(void);
PyAPI_FUNC(_PyInitError) _PySys_BeginInit(PyObject **sysmod);
PyAPI_FUNC(int) _PySys_EndInit(PyObject *sysdict, PyInterpreterState *interp);
PyAPI_FUNC(_PyInitError) _PyImport_Init(PyInterpreterState *interp);
PyAPI_FUNC(void) _PyExc_Init(PyObject * bltinmod);
PyAPI_FUNC(_PyInitError) _PyImportHooks_Init(void);
PyAPI_FUNC(int) _PyFloat_Init(void);
PyAPI_FUNC(int) PyByteArray_Init(void);
PyAPI_FUNC(_PyInitError) _Py_HashRandomization_Init(const _PyCoreConfig *);

/* Various internal finalizers */

PyAPI_FUNC(void) PyMethod_Fini(void);
PyAPI_FUNC(void) PyFrame_Fini(void);
PyAPI_FUNC(void) PyCFunction_Fini(void);
PyAPI_FUNC(void) PyDict_Fini(void);
PyAPI_FUNC(void) PyTuple_Fini(void);
PyAPI_FUNC(void) PyList_Fini(void);
PyAPI_FUNC(void) PySet_Fini(void);
PyAPI_FUNC(void) PyBytes_Fini(void);
PyAPI_FUNC(void) PyByteArray_Fini(void);
PyAPI_FUNC(void) PyFloat_Fini(void);
PyAPI_FUNC(void) PyOS_FiniInterrupts(void);
PyAPI_FUNC(void) PySlice_Fini(void);
PyAPI_FUNC(void) PyAsyncGen_Fini(void);

PyAPI_FUNC(int) _Py_IsFinalizing(void);

/* Random */
PyAPI_FUNC(int) _PyOS_URandom(void *buffer, Py_ssize_t size);
PyAPI_FUNC(int) _PyOS_URandomNonblock(void *buffer, Py_ssize_t size);

/* Legacy locale support */
PyAPI_FUNC(void) _Py_CoerceLegacyLocale(int warn);
PyAPI_FUNC(int) _Py_LegacyLocaleDetected(void);
PyAPI_FUNC(char *) _Py_SetLocaleFromEnv(int category);

#ifdef __cplusplus
}
#endif
