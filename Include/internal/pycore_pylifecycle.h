#ifndef Py_INTERNAL_LIFECYCLE_H
#define Py_INTERNAL_LIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

/* True if the main interpreter thread exited due to an unhandled
 * KeyboardInterrupt exception, suggesting the user pressed ^C. */
PyAPI_DATA(int) _Py_UnhandledKeyboardInterrupt;

PyAPI_FUNC(int) _Py_UnixMain(int argc, char **argv);

PyAPI_FUNC(int) _Py_SetFileSystemEncoding(
    const char *encoding,
    const char *errors);
PyAPI_FUNC(void) _Py_ClearFileSystemEncoding(void);

PyAPI_FUNC(void) _Py_ClearStandardStreamEncoding(void);

PyAPI_FUNC(int) _Py_IsLocaleCoercionTarget(const char *ctype_loc);

/* Various one-time initializers */

extern _PyInitError _PyUnicode_Init(void);
extern int _PyStructSequence_Init(void);
extern int _PyLong_Init(void);
extern _PyInitError _PyFaulthandler_Init(int enable);
extern int _PyTraceMalloc_Init(int enable);
extern PyObject * _PyBuiltin_Init(void);
extern _PyInitError _PySys_Create(
    PyInterpreterState *interp,
    PyObject **sysmod_p);
extern _PyInitError _PySys_SetPreliminaryStderr(PyObject *sysdict);
extern int _PySys_InitMain(PyInterpreterState *interp);
extern _PyInitError _PyImport_Init(PyInterpreterState *interp);
extern _PyInitError _PyExc_Init(void);
extern _PyInitError _PyBuiltins_AddExceptions(PyObject * bltinmod);
extern _PyInitError _PyImportHooks_Init(void);
extern int _PyFloat_Init(void);
extern _PyInitError _Py_HashRandomization_Init(const _PyCoreConfig *);

extern _PyInitError _PyTypes_Init(void);

/* Various internal finalizers */

extern void PyMethod_Fini(void);
extern void PyFrame_Fini(void);
extern void PyCFunction_Fini(void);
extern void PyDict_Fini(void);
extern void PyTuple_Fini(void);
extern void PyList_Fini(void);
extern void PySet_Fini(void);
extern void PyBytes_Fini(void);
extern void PyFloat_Fini(void);
extern void PyOS_FiniInterrupts(void);
extern void PySlice_Fini(void);
extern void PyAsyncGen_Fini(void);

extern void _PyExc_Fini(void);
extern void _PyImport_Fini(void);
extern void _PyImport_Fini2(void);
extern void _PyGC_Fini(void);
extern void _PyType_Fini(void);
extern void _Py_HashRandomization_Fini(void);
extern void _PyUnicode_Fini(void);
extern void PyLong_Fini(void);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern int _PyTraceMalloc_Fini(void);

extern void _PyGILState_Init(PyInterpreterState *, PyThreadState *);
extern void _PyGILState_Fini(void);

PyAPI_FUNC(void) _PyGC_DumpShutdownStats(void);

PyAPI_FUNC(_PyInitError) _Py_PreInitializeFromCoreConfig(
    const _PyCoreConfig *coreconfig);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LIFECYCLE_H */
