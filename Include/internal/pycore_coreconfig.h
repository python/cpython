#ifndef Py_INTERNAL_CORECONFIG_H
#define Py_INTERNAL_CORECONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN defined"
#endif

/* --- _Py_wstrlist ----------------------------------------------- */

PyAPI_FUNC(void) _Py_wstrlist_clear(
    int len,
    wchar_t **list);
PyAPI_FUNC(wchar_t**) _Py_wstrlist_copy(
    int len,
    wchar_t * const *list);
PyAPI_FUNC(_PyInitError) _Py_wstrlist_append(
    int *len,
    wchar_t ***list,
    const wchar_t *str);
PyAPI_FUNC(PyObject*) _Py_wstrlist_as_pylist(
    int len,
    wchar_t **list);

/* --- _PyArgv ---------------------------------------------------- */

PyAPI_FUNC(_PyInitError) _PyArgv_Decode(const _PyArgv *args,
    wchar_t*** argv_p);

/* --- Py_GetArgcArgv() helpers ----------------------------------- */

PyAPI_FUNC(void) _Py_ClearArgcArgv(void);

/* --- _PyPreConfig ----------------------------------------------- */

PyAPI_FUNC(int) _Py_str_to_int(
    const char *str,
    int *result);
PyAPI_FUNC(const wchar_t*) _Py_get_xoption(
    int nxoption,
    wchar_t * const *xoptions,
    const wchar_t *name);

PyAPI_FUNC(void) _PyPreConfig_Clear(_PyPreConfig *config);
PyAPI_FUNC(int) _PyPreConfig_Copy(_PyPreConfig *config,
    const _PyPreConfig *config2);
PyAPI_FUNC(void) _PyPreConfig_GetGlobalConfig(_PyPreConfig *config);
PyAPI_FUNC(void) _PyPreConfig_SetGlobalConfig(const _PyPreConfig *config);
PyAPI_FUNC(const char*) _PyPreConfig_GetEnv(const _PyPreConfig *config,
    const char *name);
PyAPI_FUNC(void) _Py_get_env_flag(_PyPreConfig *config,
    int *flag,
    const char *name);
PyAPI_FUNC(_PyInitError) _PyPreConfig_Read(_PyPreConfig *config);
PyAPI_FUNC(int) _PyPreConfig_AsDict(const _PyPreConfig *config,
    PyObject *dict);
PyAPI_FUNC(_PyInitError) _PyPreConfig_ReadFromArgv(_PyPreConfig *config,
    const _PyArgv *args);
PyAPI_FUNC(_PyInitError) _PyPreConfig_Write(_PyPreConfig *config);


/* --- _PyCoreConfig ---------------------------------------------- */

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
PyAPI_FUNC(_PyInitError) _PyCoreConfig_Read(_PyCoreConfig *config,
    const _PyPreConfig *preconfig);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_ReadFromArgv(_PyCoreConfig *config,
    const _PyArgv *args,
    const _PyPreConfig *preconfig);
PyAPI_FUNC(void) _PyCoreConfig_Write(const _PyCoreConfig *config);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CORECONFIG_H */
