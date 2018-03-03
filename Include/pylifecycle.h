
/* Interfaces to configure, query, create & destroy the Python runtime */

#ifndef Py_PYLIFECYCLE_H
#define Py_PYLIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_LIMITED_API
typedef struct {
    const char *prefix;
    const char *msg;
    int user_err;
} _PyInitError;

/* Almost all errors causing Python initialization to fail */
#ifdef _MSC_VER
   /* Visual Studio 2015 doesn't implement C99 __func__ in C */
#  define _Py_INIT_GET_FUNC() __FUNCTION__
#else
#  define _Py_INIT_GET_FUNC() __func__
#endif

#define _Py_INIT_OK() \
    (_PyInitError){.prefix = NULL, .msg = NULL, .user_err = 0}
#define _Py_INIT_ERR(MSG) \
    (_PyInitError){.prefix = _Py_INIT_GET_FUNC(), .msg = (MSG), .user_err = 0}
/* Error that can be fixed by the user like invalid input parameter.
   Don't abort() the process on such error. */
#define _Py_INIT_USER_ERR(MSG) \
    (_PyInitError){.prefix = _Py_INIT_GET_FUNC(), .msg = (MSG), .user_err = 1}
#define _Py_INIT_NO_MEMORY() _Py_INIT_USER_ERR("memory allocation failed")
#define _Py_INIT_FAILED(err) \
    (err.msg != NULL)

#endif


PyAPI_FUNC(void) Py_SetProgramName(const wchar_t *);
PyAPI_FUNC(wchar_t *) Py_GetProgramName(void);

PyAPI_FUNC(void) Py_SetPythonHome(const wchar_t *);
PyAPI_FUNC(wchar_t *) Py_GetPythonHome(void);

#ifndef Py_LIMITED_API
/* Only used by applications that embed the interpreter and need to
 * override the standard encoding determination mechanism
 */
PyAPI_FUNC(int) Py_SetStandardStreamEncoding(const char *encoding,
                                             const char *errors);

/* PEP 432 Multi-phase initialization API (Private while provisional!) */
PyAPI_FUNC(_PyInitError) _Py_InitializeCore(const _PyCoreConfig *);
PyAPI_FUNC(int) _Py_IsCoreInitialized(void);

PyAPI_FUNC(_PyInitError) _PyCoreConfig_Read(_PyCoreConfig *);
PyAPI_FUNC(void) _PyCoreConfig_Clear(_PyCoreConfig *);
PyAPI_FUNC(int) _PyCoreConfig_Copy(
    _PyCoreConfig *config,
    const _PyCoreConfig *config2);

PyAPI_FUNC(_PyInitError) _PyMainInterpreterConfig_Read(
    _PyMainInterpreterConfig *config,
    const _PyCoreConfig *core_config);
PyAPI_FUNC(void) _PyMainInterpreterConfig_Clear(_PyMainInterpreterConfig *);
PyAPI_FUNC(int) _PyMainInterpreterConfig_Copy(
    _PyMainInterpreterConfig *config,
    const _PyMainInterpreterConfig *config2);

PyAPI_FUNC(_PyInitError) _Py_InitializeMainInterpreter(const _PyMainInterpreterConfig *);
#endif

/* Initialization and finalization */
PyAPI_FUNC(void) Py_Initialize(void);
PyAPI_FUNC(void) Py_InitializeEx(int);
#ifndef Py_LIMITED_API
PyAPI_FUNC(_PyInitError) _Py_InitializeEx_Private(int, int);
PyAPI_FUNC(void) _Py_FatalInitError(_PyInitError err) _Py_NO_RETURN;
#endif
PyAPI_FUNC(void) Py_Finalize(void);
PyAPI_FUNC(int) Py_FinalizeEx(void);
PyAPI_FUNC(int) Py_IsInitialized(void);

/* Subinterpreter support */
PyAPI_FUNC(PyThreadState *) Py_NewInterpreter(void);
PyAPI_FUNC(void) Py_EndInterpreter(PyThreadState *);


/* Py_PyAtExit is for the atexit module, Py_AtExit is for low-level
 * exit functions.
 */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_PyAtExit(void (*func)(PyObject *), PyObject *);
#endif
PyAPI_FUNC(int) Py_AtExit(void (*func)(void));

PyAPI_FUNC(void) Py_Exit(int) _Py_NO_RETURN;

/* Restore signals that the interpreter has called SIG_IGN on to SIG_DFL. */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_RestoreSignals(void);

PyAPI_FUNC(int) Py_FdIsInteractive(FILE *, const char *);
#endif

/* Bootstrap __main__ (defined in Modules/main.c) */
PyAPI_FUNC(int) Py_Main(int argc, wchar_t **argv);
#ifdef Py_BUILD_CORE
PyAPI_FUNC(int) _Py_UnixMain(int argc, char **argv);
#endif

/* In getpath.c */
PyAPI_FUNC(wchar_t *) Py_GetProgramFullPath(void);
PyAPI_FUNC(wchar_t *) Py_GetPrefix(void);
PyAPI_FUNC(wchar_t *) Py_GetExecPrefix(void);
PyAPI_FUNC(wchar_t *) Py_GetPath(void);
#ifdef Py_BUILD_CORE
PyAPI_FUNC(_PyInitError) _PyPathConfig_Init(const _PyCoreConfig *core_config);
PyAPI_FUNC(PyObject*) _PyPathConfig_ComputeArgv0(int argc, wchar_t **argv);
PyAPI_FUNC(int) _Py_FindEnvConfigValue(
    FILE *env_file,
    const wchar_t *key,
    wchar_t *value,
    size_t value_size);
#endif
PyAPI_FUNC(void)      Py_SetPath(const wchar_t *);
#ifdef MS_WINDOWS
int _Py_CheckPython3(void);
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
PyAPI_FUNC(_PyInitError) _PySys_BeginInit(PyObject **sysmod);
PyAPI_FUNC(int) _PySys_EndInit(PyObject *sysdict, _PyMainInterpreterConfig *config);
PyAPI_FUNC(_PyInitError) _PyImport_Init(PyInterpreterState *interp);
PyAPI_FUNC(void) _PyExc_Init(PyObject * bltinmod);
PyAPI_FUNC(_PyInitError) _PyImportHooks_Init(void);
PyAPI_FUNC(int) _PyFloat_Init(void);
PyAPI_FUNC(int) PyByteArray_Init(void);
PyAPI_FUNC(_PyInitError) _Py_HashRandomization_Init(const _PyCoreConfig *);
#endif
#ifdef Py_BUILD_CORE
PyAPI_FUNC(int) _Py_ReadHashSeed(
    const char *seed_text,
    int *use_hash_seed,
    unsigned long *hash_seed);
#endif

/* Various internal finalizers */

#ifdef Py_BUILD_CORE
PyAPI_FUNC(void) _PyExc_Fini(void);
PyAPI_FUNC(void) _PyImport_Fini(void);
PyAPI_FUNC(void) _PyImport_Fini2(void);
PyAPI_FUNC(void) _PyGC_DumpShutdownStats(void);
PyAPI_FUNC(void) _PyGC_Fini(void);
PyAPI_FUNC(void) _PyType_Fini(void);
PyAPI_FUNC(void) _Py_HashRandomization_Fini(void);
#endif   /* Py_BUILD_CORE */

#ifndef Py_LIMITED_API
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
#endif   /* !Py_LIMITED_API */

/* Signals */
typedef void (*PyOS_sighandler_t)(int);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_getsig(int);
PyAPI_FUNC(PyOS_sighandler_t) PyOS_setsig(int, PyOS_sighandler_t);

#ifndef Py_LIMITED_API
/* Random */
PyAPI_FUNC(int) _PyOS_URandom(void *buffer, Py_ssize_t size);
PyAPI_FUNC(int) _PyOS_URandomNonblock(void *buffer, Py_ssize_t size);
#endif /* !Py_LIMITED_API */

/* Legacy locale support */
#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _Py_CoerceLegacyLocale(const _PyCoreConfig *config);
PyAPI_FUNC(int) _Py_LegacyLocaleDetected(void);
PyAPI_FUNC(char *) _Py_SetLocaleFromEnv(int category);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYLIFECYCLE_H */
