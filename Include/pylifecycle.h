
/* Interfaces to configure, query, create & destroy the Python runtime */

#ifndef Py_PYLIFECYCLE_H
#define Py_PYLIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(void) Py_SetProgramName(wchar_t *);
PyAPI_FUNC(wchar_t *) Py_GetProgramName(void);

PyAPI_FUNC(void) Py_SetPythonHome(wchar_t *);
PyAPI_FUNC(wchar_t *) Py_GetPythonHome(void);

#ifndef Py_LIMITED_API
/* Only used by applications that embed the interpreter and need to
 * override the standard encoding determination mechanism
 */
PyAPI_FUNC(int) Py_SetStandardStreamEncoding(const char *encoding,
                                             const char *errors);
#endif

PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(void) Py_InitializeEx(int);
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_InitializeEx_Private(int, int);
#endif
PyAPI_FUNC(void) Py_Finalize(void);
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03060000
PyAPI_FUNC(int) Py_FinalizeEx(void);
#endif
PyAPI_FUNC(int) Py_IsInitialized(void);
PyAPI_FUNC(PyThreadState *) Py_NewInterpreter(void);
PyAPI_FUNC(void) Py_EndInterpreter(PyThreadState *);


/* Py_PyAtExit is for the atexit module, Py_AtExit is for low-level
 * exit functions.
 */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_PyAtExit(void (*func)(void));
#endif
PyAPI_FUNC(int) Py_AtExit(void (*func)(void));

PyAPI_FUNC(void) Py_Exit(int);

/* Restore signals that the interpreter has called SIG_IGN on to SIG_DFL. */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_RestoreSignals(void);

PyAPI_FUNC(int) Py_FdIsInteractive(FILE *, const char *);
#endif

/* Bootstrap __main__ (defined in Modules/main.c) */
PyAPI_FUNC(int) Py_Main(int argc, wchar_t **argv);

/* In getpath.c */
PyAPI_FUNC(wchar_t *) Py_GetProgramFullPath(void);
PyAPI_FUNC(wchar_t *) Py_GetPrefix(void);
PyAPI_FUNC(wchar_t *) Py_GetExecPrefix(void);
PyAPI_FUNC(wchar_t *) Py_GetPath(void);
PyAPI_FUNC(void)      Py_SetPath(const wchar_t *);
#ifdef MS_WINDOWS
int _Py_CheckPython3();
#endif

/* In their own files */
PyAPI_FUNC(const char *) Py_GetVersion(void);
PyAPI_FUNC(const char *) Py_GetPlatform(void);
PyAPI_FUNC(const char *) Py_GetCopyright(void);
PyAPI_FUNC(const char *) Py_GetCompiler(void);
PyAPI_FUNC(const char *) Py_GetBuildInfo(void);
#ifndef Py_LIMITED_API
PyAPI_FUNC(const char *) _Py_gitidentifier(void);
PyAPI_FUNC(const char *) _Py_gitversion(void);
#endif

/* Internal -- various one-time initializations */
#ifndef Py_LIMITED_API
PyAPI_FUNC(PyObject *) _PyBuiltin_Init(void);
PyAPI_FUNC(PyObject *) _PySys_Init(void);
PyAPI_FUNC(void) _PyImport_Init(void);
PyAPI_FUNC(void) _PyExc_Init(PyObject * bltinmod);
PyAPI_FUNC(void) _PyImportHooks_Init(void);
PyAPI_FUNC(int) _PyFrame_Init(void);
PyAPI_FUNC(int) _PyFloat_Init(void);
PyAPI_FUNC(int) PyByteArray_Init(void);
PyAPI_FUNC(void) _PyRandom_Init(void);
#endif

/* Various internal finalizers */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyExc_Fini(void);
PyAPI_FUNC(void) _PyImport_Fini(void);
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
PyAPI_FUNC(void) _PyGC_DumpShutdownStats(void);
PyAPI_FUNC(void) _PyGC_Fini(void);
PyAPI_FUNC(void) PySlice_Fini(void);
PyAPI_FUNC(void) _PyType_Fini(void);
PyAPI_FUNC(void) _PyRandom_Fini(void);
PyAPI_FUNC(void) PyAsyncGen_Fini(void);

PyAPI_DATA(PyThreadState *) _Py_Finalizing;
#endif

/* Signals */
typedef void (*PyOS_sighandler_t)(int);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_getsig(int);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_setsig(int, PyOS_sighandler_t);

#ifndef Py_LIMITED_API
/* Random */
PyAPI_FUNC(int) _PyOS_URandom(void *buffer, Py_ssize_t size);
PyAPI_FUNC(int) _PyOS_URandomNonblock(void *buffer, Py_ssize_t size);
#endif /* !Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYLIFECYCLE_H */
