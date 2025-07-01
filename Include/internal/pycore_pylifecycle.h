#ifndef Py_INTERNAL_LIFECYCLE_H
#define Py_INTERNAL_LIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_typedefs.h"      // _PyRuntimeState

/* Forward declarations */
struct _PyArgv;

extern int _Py_SetFileSystemEncoding(
    const char *encoding,
    const char *errors);
extern void _Py_ClearFileSystemEncoding(void);
extern PyStatus _PyUnicode_InitEncodings(PyThreadState *tstate);
#ifdef MS_WINDOWS
extern int _PyUnicode_EnableLegacyWindowsFSEncoding(void);
#endif

extern int _Py_IsLocaleCoercionTarget(const char *ctype_loc);

/* Various one-time initializers */

extern void _Py_InitVersion(void);
extern PyStatus _PyFaulthandler_Init(int enable);
extern PyObject * _PyBuiltin_Init(PyInterpreterState *interp);
extern PyStatus _PySys_Create(
    PyThreadState *tstate,
    PyObject **sysmod_p);
extern PyStatus _PySys_ReadPreinitWarnOptions(PyWideStringList *options);
extern PyStatus _PySys_ReadPreinitXOptions(PyConfig *config);
extern int _PySys_UpdateConfig(PyThreadState *tstate);
extern void _PySys_FiniTypes(PyInterpreterState *interp);
extern int _PyBuiltins_AddExceptions(PyObject * bltinmod);
extern PyStatus _Py_HashRandomization_Init(const PyConfig *);

extern PyStatus _PyGC_Init(PyInterpreterState *interp);
extern PyStatus _PyAtExit_Init(PyInterpreterState *interp);

/* Various internal finalizers */

extern int _PySignal_Init(int install_signal_handlers);
extern void _PySignal_Fini(void);

extern void _PyGC_Fini(PyInterpreterState *interp);
extern void _Py_HashRandomization_Fini(void);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern void _PyTraceMalloc_Fini(void);
extern void _PyWarnings_Fini(PyInterpreterState *interp);
extern void _PyAST_Fini(PyInterpreterState *interp);
extern void _PyAtExit_Fini(PyInterpreterState *interp);
extern void _PyThread_FiniType(PyInterpreterState *interp);
extern void _PyArg_Fini(void);
extern void _Py_FinalizeAllocatedBlocks(_PyRuntimeState *);

extern PyStatus _PyGILState_Init(PyInterpreterState *interp);
extern void _PyGILState_SetTstate(PyThreadState *tstate);
extern void _PyGILState_Fini(PyInterpreterState *interp);

extern void _PyGC_DumpShutdownStats(PyInterpreterState *interp);

extern PyStatus _Py_PreInitializeFromPyArgv(
    const PyPreConfig *src_config,
    const struct _PyArgv *args);
extern PyStatus _Py_PreInitializeFromConfig(
    const PyConfig *config,
    const struct _PyArgv *args);

extern wchar_t * _Py_GetStdlibDir(void);

extern int _Py_HandleSystemExitAndKeyboardInterrupt(int *exitcode_p);

extern PyObject* _PyErr_WriteUnraisableDefaultHook(PyObject *unraisable);

extern void _PyErr_Print(PyThreadState *tstate);
extern void _PyErr_Display(PyObject *file, PyObject *exception,
                                PyObject *value, PyObject *tb);
extern void _PyErr_DisplayException(PyObject *file, PyObject *exc);

extern void _PyThreadState_DeleteCurrent(PyThreadState *tstate);

extern void _PyAtExit_Call(PyInterpreterState *interp);

extern int _Py_IsCoreInitialized(void);

extern int _Py_FdIsInteractive(FILE *fp, PyObject *filename);

extern const char* _Py_gitidentifier(void);
extern const char* _Py_gitversion(void);

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _Py_IsInterpreterFinalizing(PyInterpreterState *interp);

/* Random */
extern int _PyOS_URandom(void *buffer, Py_ssize_t size);

// Export for '_random' shared extension
PyAPI_FUNC(int) _PyOS_URandomNonblock(void *buffer, Py_ssize_t size);

/* Legacy locale support */
extern int _Py_CoerceLegacyLocale(int warn);
extern int _Py_LegacyLocaleDetected(int warn);

// Export for 'readline' shared extension
PyAPI_FUNC(char*) _Py_SetLocaleFromEnv(int category);

// Export for special main.c string compiling with source tracebacks
int _PyRun_SimpleStringFlagsWithName(const char *command, const char* name, PyCompilerFlags *flags);


/* interpreter config */

// Export for _testinternalcapi shared extension
PyAPI_FUNC(int) _PyInterpreterConfig_InitFromState(
    PyInterpreterConfig *,
    PyInterpreterState *);
PyAPI_FUNC(PyObject *) _PyInterpreterConfig_AsDict(PyInterpreterConfig *);
PyAPI_FUNC(int) _PyInterpreterConfig_InitFromDict(
    PyInterpreterConfig *,
    PyObject *);
PyAPI_FUNC(int) _PyInterpreterConfig_UpdateFromDict(
    PyInterpreterConfig *,
    PyObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LIFECYCLE_H */
