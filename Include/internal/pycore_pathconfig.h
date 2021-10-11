#ifndef Py_INTERNAL_PATHCONFIG_H
#define Py_INTERNAL_PATHCONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#define LOCATION_UNKNOWN 0
#define LOCATION_EXISTS 2<<0
#define LOCATION_FORCED 2<<1        /* trusted to exist */
#define LOCATION_DEFAULT 2<<2       /* the default value */
#define LOCATION_CUSTOM 2<<3        /* from a file or env var */
/* build-defined */
#define LOCATION_PREFIX 2<<10
#define LOCATION_EXEC_PREFIX 2<<11
/* build-related */
#define LOCATION_IN_BUILD_DIR 2<<15
#define LOCATION_IN_SOURCE_TREE 2<<16
/* relative */
#define LOCATION_WITH_FILE 2<<20    /* in the tree under dirname(<file>)
                                       (combined with LOCATION_NEAR_*) */
#define LOCATION_NEAR_ARGV0 2<<21   /* based on a parent of argv[0] */
#define LOCATION_NEAR_LIB 2<<22     /* based on a parent of DLL/SO file */

typedef struct _PyPathConfig {
    /* Full path to the Python program */
    wchar_t *program_full_path;
    wchar_t *prefix;
    wchar_t *exec_prefix;
    wchar_t *stdlib_dir;
    /* Set by Py_SetPath(), or computed by _PyConfig_InitPathConfig() */
    wchar_t *module_search_path;
    /* Python program name */
    wchar_t *program_name;
    /* Set by Py_SetPythonHome() or PYTHONHOME environment variable */
    wchar_t *home;
#ifdef MS_WINDOWS
    /* isolated and site_import are used to set Py_IsolatedFlag and
       Py_NoSiteFlag flags on Windows in read_pth_file(). These fields
       are ignored when their value are equal to -1 (unset). */
    int isolated;
    int site_import;
    /* Set when a venv is detected */
    wchar_t *base_executable;
#endif
} _PyPathConfig;

#ifdef MS_WINDOWS
#  define _PyPathConfig_INIT \
      {.module_search_path = NULL, \
       .isolated = -1, \
       .site_import = -1}
#else
#  define _PyPathConfig_INIT \
      {.module_search_path = NULL}
#endif
/* Note: _PyPathConfig_INIT sets other fields to 0/NULL */

PyAPI_DATA(_PyPathConfig) _Py_path_config;
#ifdef MS_WINDOWS
PyAPI_DATA(wchar_t*) _Py_dll_path;
#endif

extern void _PyPathConfig_ClearGlobal(void);

extern PyStatus _PyPathConfig_Calculate(
    _PyPathConfig *pathconfig,
    const PyConfig *config);
extern int _PyPathConfig_ComputeSysPath0(
    const PyWideStringList *argv,
    PyObject **path0);
extern PyStatus _Py_FindEnvConfigValue(
    FILE *env_file,
    const wchar_t *key,
    wchar_t **value_p);

#ifdef MS_WINDOWS
extern wchar_t* _Py_GetDLLPath(void);
#endif

extern PyStatus _PyConfig_WritePathConfig(const PyConfig *config);
extern void _Py_DumpPathConfig(PyThreadState *tstate);
extern PyObject* _PyPathConfig_AsDict(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PATHCONFIG_H */
