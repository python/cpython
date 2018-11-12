#ifndef Py_INTERNAL_LIFECYCLE_H
#define Py_INTERNAL_LIFECYCLE_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN define"
#endif

PyAPI_FUNC(int) _Py_UnixMain(int argc, char **argv);

PyAPI_FUNC(int) _Py_SetFileSystemEncoding(
    const char *encoding,
    const char *errors);
PyAPI_FUNC(void) _Py_ClearFileSystemEncoding(void);

PyAPI_FUNC(void) _Py_ClearStandardStreamEncoding(void);

PyAPI_FUNC(int) _Py_IsLocaleCoercionTarget(const char *ctype_loc);

extern int _PyUnicode_Init(void);
extern int _PyStructSequence_Init(void);
extern int _PyLong_Init(void);
extern _PyInitError _PyFaulthandler_Init(int enable);
extern int _PyTraceMalloc_Init(int enable);

extern void _Py_ReadyTypes(void);

PyAPI_FUNC(void) _PyExc_Fini(void);
PyAPI_FUNC(void) _PyImport_Fini(void);
PyAPI_FUNC(void) _PyImport_Fini2(void);
PyAPI_FUNC(void) _PyGC_Fini(void);
PyAPI_FUNC(void) _PyType_Fini(void);
PyAPI_FUNC(void) _Py_HashRandomization_Fini(void);
extern void _PyUnicode_Fini(void);
extern void PyLong_Fini(void);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern int _PyTraceMalloc_Fini(void);

extern void _PyGILState_Init(PyInterpreterState *, PyThreadState *);
extern void _PyGILState_Fini(void);

PyAPI_FUNC(void) _PyGC_DumpShutdownStats(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_LIFECYCLE_H */
