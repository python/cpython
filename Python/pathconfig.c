/* Path configuration like module_search_path (sys.path) */

#include "Python.h"
#include "osdefs.h"
#include "internal/pystate.h"

#ifdef __cplusplus
extern "C" {
#endif


_PyPathConfig _Py_path_config = _PyPathConfig_INIT;
#ifdef MS_WINDOWS
static wchar_t *progname = L"python";
#else
static wchar_t *progname = L"python3";
#endif
static wchar_t *default_home = NULL;


void
_PyPathConfig_Clear(_PyPathConfig *config)
{
#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(ATTR); \
        ATTR = NULL; \
    } while (0)

    CLEAR(config->prefix);
    CLEAR(config->program_full_path);
#ifdef MS_WINDOWS
    CLEAR(config->dll_path);
#else
    CLEAR(config->exec_prefix);
#endif
    CLEAR(config->module_search_path);
#undef CLEAR
}


void
Py_SetProgramName(wchar_t *pn)
{
    if (pn && *pn)
        progname = pn;
}


wchar_t *
Py_GetProgramName(void)
{
    return progname;
}


void
Py_SetPythonHome(wchar_t *home)
{
    default_home = home;
}


wchar_t*
Py_GetPythonHome(void)
{
    /* Use a static buffer to avoid heap memory allocation failure.
       Py_GetPythonHome() doesn't allow to report error, and the caller
       doesn't release memory. */
    static wchar_t buffer[MAXPATHLEN+1];

    if (default_home) {
        return default_home;
    }

    char *home = Py_GETENV("PYTHONHOME");
    if (!home) {
        return NULL;
    }

    size_t size = Py_ARRAY_LENGTH(buffer);
    size_t r = mbstowcs(buffer, home, size);
    if (r == (size_t)-1 || r >= size) {
        /* conversion failed or the static buffer is too small */
        return NULL;
    }

    return buffer;
}


static void
pathconfig_global_init(void)
{
    if (_Py_path_config.module_search_path) {
        /* Already initialized */
        return;
    }

    _PyInitError err;
    _PyMainInterpreterConfig config = _PyMainInterpreterConfig_INIT;

    err = _PyMainInterpreterConfig_ReadEnv(&config);
    if (_Py_INIT_FAILED(err)) {
        goto error;
    }

    err = _PyMainInterpreterConfig_Read(&config);
    if (_Py_INIT_FAILED(err)) {
        goto error;
    }

    err = _PyPathConfig_Init(&config);
    if (_Py_INIT_FAILED(err)) {
        goto error;
    }

    _PyMainInterpreterConfig_Clear(&config);
    return;

error:
    _PyMainInterpreterConfig_Clear(&config);
    _Py_FatalInitError(err);
}


/* External interface */

void
Py_SetPath(const wchar_t *path)
{
    if (path == NULL) {
        _PyPathConfig_Clear(&_Py_path_config);
        return;
    }

    _PyPathConfig new_config;
    new_config.program_full_path = _PyMem_RawWcsdup(Py_GetProgramName());
    new_config.prefix = _PyMem_RawWcsdup(L"");
#ifdef MS_WINDOWS
    new_config.dll_path = _PyMem_RawWcsdup(L"");
#else
    new_config.exec_prefix = _PyMem_RawWcsdup(L"");
#endif
    new_config.module_search_path = _PyMem_RawWcsdup(path);

    _PyPathConfig_Clear(&_Py_path_config);
    _Py_path_config = new_config;
}


wchar_t *
Py_GetPath(void)
{
    pathconfig_global_init();
    return _Py_path_config.module_search_path;
}


wchar_t *
Py_GetPrefix(void)
{
    pathconfig_global_init();
    return _Py_path_config.prefix;
}


wchar_t *
Py_GetExecPrefix(void)
{
#ifdef MS_WINDOWS
    return Py_GetPrefix();
#else
    pathconfig_global_init();
    return _Py_path_config.exec_prefix;
#endif
}


wchar_t *
Py_GetProgramFullPath(void)
{
    pathconfig_global_init();
    return _Py_path_config.program_full_path;
}

#ifdef __cplusplus
}
#endif
