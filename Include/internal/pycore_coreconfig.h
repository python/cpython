#ifndef Py_INTERNAL_CORECONFIG_H
#define Py_INTERNAL_CORECONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN defined"
#endif

/* helpers */
PyAPI_FUNC(const wchar_t*) _Py_GetXOption(int nxoption, wchar_t **xoptions, wchar_t *name);
PyAPI_FUNC(const wchar_t*) _Py_GetProgramFromArgv(int argc, wchar_t * const *argv);
PyAPI_FUNC(_PyInitError) _PyArgv_Decode(const _PyArgv *args, int *argc_p, wchar_t*** argv_p);

/* _PyPreConfig */
PyAPI_FUNC(const char*) _PyPreConfig_GetEnv(const _PyPreConfig *config, const char *name);
PyAPI_FUNC(_PyInitError) _PyPreConfig_Read(_PyPreConfig *config);
PyAPI_FUNC(void) _PyPreConfig_Clear(_PyPreConfig *config);
PyAPI_FUNC(int) _PyPreConfig_Copy(_PyPreConfig *config, const _PyPreConfig *config2);
PyAPI_FUNC(void) _PyPreConfig_GetGlobalConfig(_PyPreConfig *config);
PyAPI_FUNC(void) _PyPreConfig_SetGlobalConfig(const _PyPreConfig *config);
PyAPI_FUNC(_PyInitError) _PyPreConfig_InitFromArgv(_PyPreConfig *config, const _PyArgv *args);
PyAPI_FUNC(_PyInitError) _PyPreConfig_SetAllocator(_PyPreConfig *config);

/* _PyCoreConfig */
PyAPI_FUNC(_PyInitError) _PyCoreConfig_Read(_PyCoreConfig *config, const _PyPreConfig *preconfig);
PyAPI_FUNC(void) _PyCoreConfig_Clear(_PyCoreConfig *);
PyAPI_FUNC(int) _PyCoreConfig_Copy(
    _PyCoreConfig *config,
    const _PyCoreConfig *config2);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_InitPathConfig(_PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_SetPathConfig(
    const _PyCoreConfig *config);
PyAPI_FUNC(void) _PyCoreConfig_GetGlobalConfig(_PyCoreConfig *config);
PyAPI_FUNC(void) _PyCoreConfig_SetGlobalConfig(const _PyCoreConfig *config);
PyAPI_FUNC(const char*) _PyCoreConfig_GetEnv(
    const _PyCoreConfig *config,
    const char *name);
PyAPI_FUNC(int) _PyCoreConfig_GetEnvDup(
    const _PyCoreConfig *config,
    wchar_t **dest,
    wchar_t *wname,
    char *name);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_ReadFromArgv(
    _PyCoreConfig *config,
    const _PyArgv *args,
    const _PyPreConfig *preconfig);
PyAPI_FUNC(void) _Py_ClearArgcArgv(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CORECONFIG_H */
