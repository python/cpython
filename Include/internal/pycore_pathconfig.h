#ifndef Py_INTERNAL_PATHCONFIG_H
#define Py_INTERNAL_PATHCONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct _PyPathConfig {
    /* Full path to the Python program */
    wchar_t *program_full_path;
    wchar_t *prefix;
    wchar_t *exec_prefix;
#ifdef MS_WINDOWS
    wchar_t *dll_path;
#endif
    /* Set by Py_SetPath(), or computed by _PyPathConfig_Init() */
    wchar_t *module_search_path;
    /* Python program name */
    wchar_t *program_name;
    /* Set by Py_SetPythonHome() or PYTHONHOME environment variable */
    wchar_t *home;
    /* isolated and site_import are used to set Py_IsolatedFlag and
       Py_NoSiteFlag flags on Windows in read_pth_file(). These fields
       are ignored when their value are equal to -1 (unset). */
    int isolated;
    int site_import;
} _PyPathConfig;

#define _PyPathConfig_INIT \
    {.module_search_path = NULL, \
     .isolated = -1, \
     .site_import = -1}
/* Note: _PyPathConfig_INIT sets other fields to 0/NULL */

PyAPI_DATA(_PyPathConfig) _Py_path_config;

extern void _PyPathConfig_ClearGlobal(void);
extern PyStatus _PyPathConfig_SetGlobal(
    const struct _PyPathConfig *pathconfig);

extern PyStatus _PyPathConfig_Calculate(
    _PyPathConfig *pathconfig,
    const PyConfig *config);
extern int _PyPathConfig_ComputeSysPath0(
    const PyWideStringList *argv,
    PyObject **path0);
extern int _Py_FindEnvConfigValue(
    FILE *env_file,
    const wchar_t *key,
    wchar_t *value,
    size_t value_size);

#ifdef MS_WINDOWS
extern wchar_t* _Py_GetDLLPath(void);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PATHCONFIG_H */
