#ifndef Py_INTERNAL_CORECONFIG_H
#define Py_INTERNAL_CORECONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_pystate.h"   /* _PyRuntimeState */

/* --- _PyInitError ----------------------------------------------- */

/* Almost all errors causing Python initialization to fail */
#ifdef _MSC_VER
   /* Visual Studio 2015 doesn't implement C99 __func__ in C */
#  define _Py_INIT_GET_FUNC() __FUNCTION__
#else
#  define _Py_INIT_GET_FUNC() __func__
#endif

#define _Py_INIT_OK() \
    (_PyInitError){._type = _Py_INIT_ERR_TYPE_OK,}
    /* other fields are set to 0 */
#define _Py_INIT_ERR(ERR_MSG) \
    (_PyInitError){ \
        ._type = _Py_INIT_ERR_TYPE_ERROR, \
        ._func = _Py_INIT_GET_FUNC(), \
        .err_msg = (ERR_MSG)}
        /* other fields are set to 0 */
#define _Py_INIT_NO_MEMORY() _Py_INIT_ERR("memory allocation failed")
#define _Py_INIT_EXIT(EXITCODE) \
    (_PyInitError){ \
        ._type = _Py_INIT_ERR_TYPE_EXIT, \
        .exitcode = (EXITCODE)}
#define _Py_INIT_IS_ERROR(err) \
    (err._type == _Py_INIT_ERR_TYPE_ERROR)
#define _Py_INIT_IS_EXIT(err) \
    (err._type == _Py_INIT_ERR_TYPE_EXIT)
#define _Py_INIT_FAILED(err) \
    (err._type != _Py_INIT_ERR_TYPE_OK)

/* --- _PyWstrList ------------------------------------------------ */

#define _PyWstrList_INIT (_PyWstrList){.length = 0, .items = NULL}

#ifndef NDEBUG
PyAPI_FUNC(int) _PyWstrList_CheckConsistency(const _PyWstrList *list);
#endif
PyAPI_FUNC(void) _PyWstrList_Clear(_PyWstrList *list);
PyAPI_FUNC(int) _PyWstrList_Copy(_PyWstrList *list,
    const _PyWstrList *list2);
PyAPI_FUNC(int) _PyWstrList_Append(_PyWstrList *list,
    const wchar_t *item);
PyAPI_FUNC(PyObject*) _PyWstrList_AsList(const _PyWstrList *list);
PyAPI_FUNC(int) _PyWstrList_Extend(_PyWstrList *list,
    const _PyWstrList *list2);


/* --- _PyArgv ---------------------------------------------------- */

typedef struct {
    Py_ssize_t argc;
    int use_bytes_argv;
    char **bytes_argv;
    wchar_t **wchar_argv;
} _PyArgv;

PyAPI_FUNC(_PyInitError) _PyArgv_AsWstrList(const _PyArgv *args,
    _PyWstrList *list);


/* --- Helper functions ------------------------------------------- */

PyAPI_FUNC(int) _Py_str_to_int(
    const char *str,
    int *result);
PyAPI_FUNC(const wchar_t*) _Py_get_xoption(
    const _PyWstrList *xoptions,
    const wchar_t *name);
PyAPI_FUNC(const char*) _Py_GetEnv(
    int use_environment,
    const char *name);
PyAPI_FUNC(void) _Py_get_env_flag(
    int use_environment,
    int *flag,
    const char *name);

/* Py_GetArgcArgv() helper */
PyAPI_FUNC(void) _Py_ClearArgcArgv(void);


/* --- _PyPreCmdline ------------------------------------------------- */

typedef struct {
    _PyWstrList argv;
    _PyWstrList xoptions;     /* "-X value" option */
    int isolated;             /* -I option */
    int use_environment;      /* -E option */
    int dev_mode;             /* -X dev and PYTHONDEVMODE */
} _PyPreCmdline;

#define _PyPreCmdline_INIT \
    (_PyPreCmdline){ \
        .use_environment = -1, \
        .isolated = -1, \
        .dev_mode = -1}
/* Note: _PyPreCmdline_INIT sets other fields to 0/NULL */

PyAPI_FUNC(void) _PyPreCmdline_Clear(_PyPreCmdline *cmdline);
PyAPI_FUNC(_PyInitError) _PyPreCmdline_SetArgv(_PyPreCmdline *cmdline,
    const _PyArgv *args);
PyAPI_FUNC(int) _PyPreCmdline_SetCoreConfig(
    const _PyPreCmdline *cmdline,
    _PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyPreCmdline_Read(_PyPreCmdline *cmdline,
    const _PyPreConfig *preconfig);


/* --- _PyPreConfig ----------------------------------------------- */

PyAPI_FUNC(void) _PyPreConfig_Init(_PyPreConfig *config);
PyAPI_FUNC(void) _PyPreConfig_Copy(_PyPreConfig *config,
    const _PyPreConfig *config2);
PyAPI_FUNC(PyObject*) _PyPreConfig_AsDict(const _PyPreConfig *config);
PyAPI_FUNC(void) _PyPreConfig_GetCoreConfig(_PyPreConfig *config,
    const _PyCoreConfig *core_config);
PyAPI_FUNC(_PyInitError) _PyPreConfig_Read(_PyPreConfig *config,
    const _PyArgv *args);
PyAPI_FUNC(_PyInitError) _PyPreConfig_Write(const _PyPreConfig *config);


/* --- _PyCoreConfig ---------------------------------------------- */

PyAPI_FUNC(void) _PyCoreConfig_Init(_PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_Copy(
    _PyCoreConfig *config,
    const _PyCoreConfig *config2);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_InitPathConfig(_PyCoreConfig *config);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_SetPathConfig(
    const _PyCoreConfig *config);
PyAPI_FUNC(void) _PyCoreConfig_Write(const _PyCoreConfig *config,
    _PyRuntimeState *runtime);
PyAPI_FUNC(_PyInitError) _PyCoreConfig_SetPyArgv(
    _PyCoreConfig *config,
    const _PyArgv *args);


/* --- Function used for testing ---------------------------------- */

PyAPI_FUNC(PyObject*) _Py_GetConfigsAsDict(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CORECONFIG_H */
