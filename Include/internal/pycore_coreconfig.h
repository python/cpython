#ifndef Py_INTERNAL_CORECONFIG_H
#define Py_INTERNAL_CORECONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN defined"
#endif


/* --- _PyPreCmdline ------------------------------------------------- */

typedef struct {
    _PyWstrList argv;
    _PyWstrList xoptions;     /* "-X value" option */
    int use_environment;      /* -E option */
    int isolated;             /* -I option */
} _PyPreCmdline;

#define _PyPreCmdline_INIT \
    (_PyPreCmdline){ \
        .use_environment = -1, \
        .isolated = -1}
/* Note: _PyPreCmdline_INIT sets other fields to 0/NULL */

PyAPI_FUNC(void) _PyPreCmdline_Clear(_PyPreCmdline *cmdline);
PyAPI_FUNC(int) _PyPreCmdline_Copy(_PyPreCmdline *cmdline,
    const _PyPreCmdline *cmdline2);
PyAPI_FUNC(_PyInitError) _PyPreCmdline_SetArgv(_PyPreCmdline *cmdline,
    const _PyArgv *args);
PyAPI_FUNC(void) _PyPreCmdline_GetPreConfig(
    _PyPreCmdline *cmdline,
    const _PyPreConfig *config);
PyAPI_FUNC(void) _PyPreCmdline_SetPreConfig(
    const _PyPreCmdline *cmdline,
    _PyPreConfig *config);
PyAPI_FUNC(void) _PyPreCmdline_GetCoreConfig(
    _PyPreCmdline *cmdline,
    const _PyCoreConfig *config);
PyAPI_FUNC(void) _PyPreCmdline_SetCoreConfig(
    const _PyPreCmdline *cmdline,
    _PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyPreCmdline_Read(_PyPreCmdline *cmdline);


/* --- _PyWstrList ------------------------------------------------ */

#ifndef NDEBUG
PyAPI_FUNC(int) _PyWstrList_CheckConsistency(const _PyWstrList *list);
#endif
PyAPI_FUNC(void) _PyWstrList_Clear(_PyWstrList *list);
PyAPI_FUNC(int) _PyWstrList_Copy(_PyWstrList *list,
    const _PyWstrList *list2);
PyAPI_FUNC(int) _PyWstrList_Append(_PyWstrList *list,
    const wchar_t *item);
PyAPI_FUNC(PyObject*) _PyWstrList_AsList(const _PyWstrList *list);


/* --- _PyArgv ---------------------------------------------------- */

PyAPI_FUNC(_PyInitError) _PyArgv_AsWstrList(const _PyArgv *args,
    _PyWstrList *list);


/* --- Py_GetArgcArgv() helpers ----------------------------------- */

PyAPI_FUNC(void) _Py_ClearArgcArgv(void);


/* --- _PyPreConfig ----------------------------------------------- */

PyAPI_FUNC(int) _Py_str_to_int(
    const char *str,
    int *result);
PyAPI_FUNC(const wchar_t*) _Py_get_xoption(
    const _PyWstrList *xoptions,
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
PyAPI_FUNC(_PyInitError) _PyPreConfig_Read(_PyPreConfig *config,
    const _PyArgv *args,
    const _PyCoreConfig *coreconfig);
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
PyAPI_FUNC(_PyInitError) _PyCoreConfig_Read(_PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_ReadFromArgv(_PyCoreConfig *config,
    const _PyArgv *args);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_Write(const _PyCoreConfig *config);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CORECONFIG_H */
