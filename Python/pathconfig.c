/* Path configuration like module_search_path (sys.path) */

#include "Python.h"
#include "osdefs.h"
#include "internal/pystate.h"

#ifdef __cplusplus
extern "C" {
#endif


_PyPathConfig _Py_path_config = _PyPathConfig_INIT;


void
_PyPathConfig_Clear(_PyPathConfig *config)
{
    /* _PyMem_SetDefaultAllocator() is needed to get a known memory allocator,
       since Py_SetPath(), Py_SetPythonHome() and Py_SetProgramName() can be
       called before Py_Initialize() which can changes the memory allocator. */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

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
    CLEAR(config->home);
    CLEAR(config->program_name);
#undef CLEAR

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


/* Initialize paths for Py_GetPath(), Py_GetPrefix(), Py_GetExecPrefix()
   and Py_GetProgramFullPath() */
_PyInitError
_PyPathConfig_Init(const _PyMainInterpreterConfig *main_config)
{
    if (_Py_path_config.module_search_path) {
        /* Already initialized */
        return _Py_INIT_OK();
    }

    _PyInitError err;
    _PyPathConfig new_config = _PyPathConfig_INIT;

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* Calculate program_full_path, prefix, exec_prefix (Unix)
       or dll_path (Windows), and module_search_path */
    err = _PyPathConfig_Calculate(&new_config, main_config);
    if (_Py_INIT_FAILED(err)) {
        _PyPathConfig_Clear(&new_config);
        goto done;
    }

    /* Copy home and program_name from main_config */
    if (main_config->home != NULL) {
        new_config.home = _PyMem_RawWcsdup(main_config->home);
        if (new_config.home == NULL) {
            err = _Py_INIT_NO_MEMORY();
            goto done;
        }
    }
    else {
        new_config.home = NULL;
    }

    new_config.program_name = _PyMem_RawWcsdup(main_config->program_name);
    if (new_config.program_name == NULL) {
        err = _Py_INIT_NO_MEMORY();
        goto done;
    }

    _PyPathConfig_Clear(&_Py_path_config);
    _Py_path_config = new_config;

    err = _Py_INIT_OK();

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return err;
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

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    _PyPathConfig new_config;
    new_config.program_full_path = _PyMem_RawWcsdup(Py_GetProgramName());
    new_config.prefix = _PyMem_RawWcsdup(L"");
#ifdef MS_WINDOWS
    new_config.dll_path = _PyMem_RawWcsdup(L"");
#else
    new_config.exec_prefix = _PyMem_RawWcsdup(L"");
#endif
    new_config.module_search_path = _PyMem_RawWcsdup(path);

    /* steal the home and program_name values (to leave them unchanged) */
    new_config.home = _Py_path_config.home;
    _Py_path_config.home = NULL;
    new_config.program_name = _Py_path_config.program_name;
    _Py_path_config.program_name = NULL;

    _PyPathConfig_Clear(&_Py_path_config);
    _Py_path_config = new_config;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


void
Py_SetPythonHome(wchar_t *home)
{
    if (home == NULL) {
        return;
    }

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.home);
    _Py_path_config.home = _PyMem_RawWcsdup(home);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_path_config.home == NULL) {
        Py_FatalError("Py_SetPythonHome() failed: out of memory");
    }
}


void
Py_SetProgramName(wchar_t *program_name)
{
    if (program_name == NULL || program_name[0] == L'\0') {
        return;
    }

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.program_name);
    _Py_path_config.program_name = _PyMem_RawWcsdup(program_name);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_path_config.program_name == NULL) {
        Py_FatalError("Py_SetProgramName() failed: out of memory");
    }
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


wchar_t*
Py_GetPythonHome(void)
{
    pathconfig_global_init();
    return _Py_path_config.home;
}


wchar_t *
Py_GetProgramName(void)
{
    pathconfig_global_init();
    return _Py_path_config.program_name;
}


#ifdef __cplusplus
}
#endif
