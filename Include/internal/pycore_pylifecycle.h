#ifndef Py_INTERNAL_LIFECYCLE_H
#define Py_INTERNAL_LIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_coreconfig.h"   /* _PyArgv */
#include "pycore_pystate.h"      /* _PyRuntimeState */

/* True if the main interpreter thread exited due to an unhandled
 * KeyboardInterrupt exception, suggesting the user pressed ^C. */
PyAPI_DATA(int) _Py_UnhandledKeyboardInterrupt;

PyAPI_FUNC(int) _Py_UnixMain(int argc, char **argv);

extern int _Py_SetFileSystemEncoding(
    const char *encoding,
    const char *errors);
extern void _Py_ClearFileSystemEncoding(void);
extern _PyInitError _PyUnicode_InitEncodings(PyInterpreterState *interp);
#ifdef MS_WINDOWS
extern int _PyUnicode_EnableLegacyWindowsFSEncoding(void);
#endif

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
    _PyRuntimeState *runtime,
    PyInterpreterState *interp,
    PyObject **sysmod_p);
extern _PyInitError _PySys_SetPreliminaryStderr(PyObject *sysdict);
extern int _PySys_InitMain(
    _PyRuntimeState *runtime,
    PyInterpreterState *interp);
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
extern void _PyGC_Fini(_PyRuntimeState *runtime);
extern void _PyType_Fini(void);
extern void _Py_HashRandomization_Fini(void);
extern void _PyUnicode_Fini(void);
extern void PyLong_Fini(void);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern int _PyTraceMalloc_Fini(void);
extern void _PyWarnings_Fini(_PyRuntimeState *runtime);

extern void _PyGILState_Init(
    _PyRuntimeState *runtime,
    PyInterpreterState *interp,
    PyThreadState *tstate);
extern void _PyGILState_Fini(_PyRuntimeState *runtime);

PyAPI_FUNC(void) _PyGC_DumpShutdownStats(_PyRuntimeState *runtime);

PyAPI_FUNC(_PyInitError) _Py_PreInitializeFromPyArgv(
    const _PyPreConfig *src_config,
    const _PyArgv *args);
PyAPI_FUNC(_PyInitError) _Py_PreInitializeFromCoreConfig(
    const _PyCoreConfig *coreconfig,
    const _PyArgv *args);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LIFECYCLE_H */
