/* Path configuration like module_search_path (sys.path) */

#include "Python.h"
#include "osdefs.h"
#include "pycore_initconfig.h"
#include "pycore_fileutils.h"
#include "pycore_pathconfig.h"
#include "pycore_pymem.h"
#include "pycore_pystate.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif


_PyPathConfig _Py_path_config = _PyPathConfig_INIT;


static int
copy_wstr(wchar_t **dst, const wchar_t *src)
{
    if (src != NULL) {
        *dst = _PyMem_RawWcsdup(src);
        if (*dst == NULL) {
            return -1;
        }
    }
    else {
        *dst = NULL;
    }
    return 0;
}


static void
pathconfig_clear(_PyPathConfig *config)
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
    CLEAR(config->exec_prefix);
#ifdef MS_WINDOWS
    CLEAR(config->dll_path);
#endif
    CLEAR(config->module_search_path);
    CLEAR(config->home);
    CLEAR(config->program_name);
#undef CLEAR

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


/* Calculate the path configuration: initialize pathconfig from config */
static PyStatus
pathconfig_calculate(_PyPathConfig *pathconfig, const PyConfig *config)
{
    PyStatus status;
    _PyPathConfig new_config = _PyPathConfig_INIT;

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* Calculate program_full_path, prefix, exec_prefix,
       dll_path (Windows), and module_search_path */
    status = _PyPathConfig_Calculate(&new_config, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    /* Copy home and program_name from config */
    if (copy_wstr(&new_config.home, config->home) < 0) {
        status = _PyStatus_NO_MEMORY();
        goto error;
    }
    if (copy_wstr(&new_config.program_name, config->program_name) < 0) {
        status = _PyStatus_NO_MEMORY();
        goto error;
    }

    pathconfig_clear(pathconfig);
    *pathconfig = new_config;

    status = _PyStatus_OK();
    goto done;

error:
    pathconfig_clear(&new_config);

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}


PyStatus
_PyPathConfig_SetGlobal(const _PyPathConfig *config)
{
    PyStatus status;
    _PyPathConfig new_config = _PyPathConfig_INIT;

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

#define COPY_ATTR(ATTR) \
    do { \
        if (copy_wstr(&new_config.ATTR, config->ATTR) < 0) { \
            pathconfig_clear(&new_config); \
            status = _PyStatus_NO_MEMORY(); \
            goto done; \
        } \
    } while (0)

    COPY_ATTR(program_full_path);
    COPY_ATTR(prefix);
    COPY_ATTR(exec_prefix);
#ifdef MS_WINDOWS
    COPY_ATTR(dll_path);
#endif
    COPY_ATTR(module_search_path);
    COPY_ATTR(program_name);
    COPY_ATTR(home);

    pathconfig_clear(&_Py_path_config);
    /* Steal new_config strings; don't clear new_config */
    _Py_path_config = new_config;

    status = _PyStatus_OK();

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}


void
_PyPathConfig_ClearGlobal(void)
{
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    pathconfig_clear(&_Py_path_config);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


static wchar_t*
_PyWideStringList_Join(const PyWideStringList *list, wchar_t sep)
{
    size_t len = 1;   /* NUL terminator */
    for (Py_ssize_t i=0; i < list->length; i++) {
        if (i != 0) {
            len++;
        }
        len += wcslen(list->items[i]);
    }

    wchar_t *text = PyMem_RawMalloc(len * sizeof(wchar_t));
    if (text == NULL) {
        return NULL;
    }
    wchar_t *str = text;
    for (Py_ssize_t i=0; i < list->length; i++) {
        wchar_t *path = list->items[i];
        if (i != 0) {
            *str++ = SEP;
        }
        len = wcslen(path);
        memcpy(str, path, len * sizeof(wchar_t));
        str += len;
    }
    *str = L'\0';

    return text;
}


/* Set the global path configuration from config. */
PyStatus
_PyConfig_SetPathConfig(const PyConfig *config)
{
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyStatus status;
    _PyPathConfig pathconfig = _PyPathConfig_INIT;

    pathconfig.module_search_path = _PyWideStringList_Join(&config->module_search_paths, DELIM);
    if (pathconfig.module_search_path == NULL) {
        goto no_memory;
    }

    if (copy_wstr(&pathconfig.program_full_path, config->executable) < 0) {
        goto no_memory;
    }
    if (copy_wstr(&pathconfig.prefix, config->prefix) < 0) {
        goto no_memory;
    }
    if (copy_wstr(&pathconfig.exec_prefix, config->exec_prefix) < 0) {
        goto no_memory;
    }
#ifdef MS_WINDOWS
    pathconfig.dll_path = _Py_GetDLLPath();
    if (pathconfig.dll_path == NULL) {
        goto no_memory;
    }
#endif
    if (copy_wstr(&pathconfig.program_name, config->program_name) < 0) {
        goto no_memory;
    }
    if (copy_wstr(&pathconfig.home, config->home) < 0) {
        goto no_memory;
    }

    status = _PyPathConfig_SetGlobal(&pathconfig);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = _PyStatus_OK();
    goto done;

no_memory:
    status = _PyStatus_NO_MEMORY();

done:
    pathconfig_clear(&pathconfig);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}


static PyStatus
config_init_module_search_paths(PyConfig *config, _PyPathConfig *pathconfig)
{
    assert(!config->module_search_paths_set);

    _PyWideStringList_Clear(&config->module_search_paths);

    const wchar_t *sys_path = pathconfig->module_search_path;
    const wchar_t delim = DELIM;
    const wchar_t *p = sys_path;
    while (1) {
        p = wcschr(sys_path, delim);
        if (p == NULL) {
            p = sys_path + wcslen(sys_path); /* End of string */
        }

        size_t path_len = (p - sys_path);
        wchar_t *path = PyMem_RawMalloc((path_len + 1) * sizeof(wchar_t));
        if (path == NULL) {
            return _PyStatus_NO_MEMORY();
        }
        memcpy(path, sys_path, path_len * sizeof(wchar_t));
        path[path_len] = L'\0';

        PyStatus status = PyWideStringList_Append(&config->module_search_paths, path);
        PyMem_RawFree(path);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }

        if (*p == '\0') {
            break;
        }
        sys_path = p + 1;
    }
    config->module_search_paths_set = 1;
    return _PyStatus_OK();
}


static PyStatus
config_calculate_pathconfig(PyConfig *config)
{
    _PyPathConfig pathconfig = _PyPathConfig_INIT;
    PyStatus status;

    status = pathconfig_calculate(&pathconfig, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    if (!config->module_search_paths_set) {
        status = config_init_module_search_paths(config, &pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            goto error;
        }
    }

    if (config->executable == NULL) {
        if (copy_wstr(&config->executable,
                      pathconfig.program_full_path) < 0) {
            goto no_memory;
        }
    }

    if (config->prefix == NULL) {
        if (copy_wstr(&config->prefix, pathconfig.prefix) < 0) {
            goto no_memory;
        }
    }

    if (config->exec_prefix == NULL) {
        if (copy_wstr(&config->exec_prefix,
                      pathconfig.exec_prefix) < 0) {
            goto no_memory;
        }
    }

    if (pathconfig.isolated != -1) {
        config->isolated = pathconfig.isolated;
    }
    if (pathconfig.site_import != -1) {
        config->site_import = pathconfig.site_import;
    }

    pathconfig_clear(&pathconfig);
    return _PyStatus_OK();

no_memory:
    status = _PyStatus_NO_MEMORY();

error:
    pathconfig_clear(&pathconfig);
    return status;
}


PyStatus
_PyConfig_InitPathConfig(PyConfig *config)
{
    /* Do we need to calculate the path? */
    if (!config->module_search_paths_set
        || (config->executable == NULL)
        || (config->prefix == NULL)
        || (config->exec_prefix == NULL))
    {
        PyStatus status = config_calculate_pathconfig(config);
        if (_PyStatus_EXCEPTION(status)) {
            return status;
        }
    }

    if (config->base_prefix == NULL) {
        if (copy_wstr(&config->base_prefix, config->prefix) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }

    if (config->base_exec_prefix == NULL) {
        if (copy_wstr(&config->base_exec_prefix,
                      config->exec_prefix) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }
    return _PyStatus_OK();
}


static void
pathconfig_global_init(void)
{
    if (_Py_path_config.module_search_path != NULL) {
        /* Already initialized */
        return;
    }

    PyStatus status;
    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    status = PyConfig_Read(&config);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    status = _PyConfig_SetPathConfig(&config);
    if (_PyStatus_EXCEPTION(status)) {
        goto error;
    }

    PyConfig_Clear(&config);
    return;

error:
    PyConfig_Clear(&config);
    Py_ExitStatusException(status);
}


/* External interface */

void
Py_SetPath(const wchar_t *path)
{
    if (path == NULL) {
        pathconfig_clear(&_Py_path_config);
        return;
    }

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    _PyPathConfig new_config;
    new_config.program_full_path = _PyMem_RawWcsdup(Py_GetProgramName());
    int alloc_error = (new_config.program_full_path == NULL);
    new_config.prefix = _PyMem_RawWcsdup(L"");
    alloc_error |= (new_config.prefix == NULL);
    new_config.exec_prefix = _PyMem_RawWcsdup(L"");
    alloc_error |= (new_config.exec_prefix == NULL);
#ifdef MS_WINDOWS
    new_config.dll_path = _Py_GetDLLPath();
    alloc_error |= (new_config.dll_path == NULL);
#endif
    new_config.module_search_path = _PyMem_RawWcsdup(path);
    alloc_error |= (new_config.module_search_path == NULL);

    /* steal the home and program_name values (to leave them unchanged) */
    new_config.home = _Py_path_config.home;
    _Py_path_config.home = NULL;
    new_config.program_name = _Py_path_config.program_name;
    _Py_path_config.program_name = NULL;

    pathconfig_clear(&_Py_path_config);
    _Py_path_config = new_config;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (alloc_error) {
        Py_FatalError("Py_SetPath() failed: out of memory");
    }
}


void
Py_SetPythonHome(const wchar_t *home)
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
Py_SetProgramName(const wchar_t *program_name)
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

void
_Py_SetProgramFullPath(const wchar_t *program_full_path)
{
    if (program_full_path == NULL || program_full_path[0] == L'\0') {
        return;
    }

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.program_full_path);
    _Py_path_config.program_full_path = _PyMem_RawWcsdup(program_full_path);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_path_config.program_full_path == NULL) {
        Py_FatalError("_Py_SetProgramFullPath() failed: out of memory");
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
    pathconfig_global_init();
    return _Py_path_config.exec_prefix;
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

/* Compute module search path from argv[0] or the current working
   directory ("-m module" case) which will be prepended to sys.argv:
   sys.path[0].

   Return 1 if the path is correctly resolved and written into *path0_p.

   Return 0 if it fails to resolve the full path. For example, return 0 if the
   current working directory has been removed (bpo-36236) or if argv is empty.

   Raise an exception and return -1 on error.
   */
int
_PyPathConfig_ComputeSysPath0(const PyWideStringList *argv, PyObject **path0_p)
{
    assert(_PyWideStringList_CheckConsistency(argv));

    if (argv->length == 0) {
        /* Leave sys.path unchanged if sys.argv is empty */
        return 0;
    }

    wchar_t *argv0 = argv->items[0];
    int have_module_arg = (wcscmp(argv0, L"-m") == 0);
    int have_script_arg = (!have_module_arg && (wcscmp(argv0, L"-c") != 0));

    wchar_t *path0 = argv0;
    Py_ssize_t n = 0;

#ifdef HAVE_REALPATH
    wchar_t fullpath[MAXPATHLEN];
#elif defined(MS_WINDOWS)
    wchar_t fullpath[MAX_PATH];
#endif

    if (have_module_arg) {
#if defined(HAVE_REALPATH) || defined(MS_WINDOWS)
        if (!_Py_wgetcwd(fullpath, Py_ARRAY_LENGTH(fullpath))) {
            return 0;
        }
        path0 = fullpath;
#else
        path0 = L".";
#endif
        n = wcslen(path0);
    }

#ifdef HAVE_READLINK
    wchar_t link[MAXPATHLEN + 1];
    int nr = 0;

    if (have_script_arg) {
        nr = _Py_wreadlink(path0, link, Py_ARRAY_LENGTH(link));
    }
    if (nr > 0) {
        /* It's a symlink */
        link[nr] = '\0';
        if (link[0] == SEP) {
            path0 = link; /* Link to absolute path */
        }
        else if (wcschr(link, SEP) == NULL) {
            /* Link without path */
        }
        else {
            /* Must join(dirname(path0), link) */
            wchar_t *q = wcsrchr(path0, SEP);
            if (q == NULL) {
                /* path0 without path */
                path0 = link;
            }
            else {
                /* Must make a copy, path0copy has room for 2 * MAXPATHLEN */
                wchar_t path0copy[2 * MAXPATHLEN + 1];
                wcsncpy(path0copy, path0, MAXPATHLEN);
                q = wcsrchr(path0copy, SEP);
                wcsncpy(q+1, link, MAXPATHLEN);
                q[MAXPATHLEN + 1] = L'\0';
                path0 = path0copy;
            }
        }
    }
#endif /* HAVE_READLINK */

    wchar_t *p = NULL;

#if SEP == '\\'
    /* Special case for Microsoft filename syntax */
    if (have_script_arg) {
        wchar_t *q;
#if defined(MS_WINDOWS)
        /* Replace the first element in argv with the full path. */
        wchar_t *ptemp;
        if (GetFullPathNameW(path0,
                           Py_ARRAY_LENGTH(fullpath),
                           fullpath,
                           &ptemp)) {
            path0 = fullpath;
        }
#endif
        p = wcsrchr(path0, SEP);
        /* Test for alternate separator */
        q = wcsrchr(p ? p : path0, '/');
        if (q != NULL)
            p = q;
        if (p != NULL) {
            n = p + 1 - path0;
            if (n > 1 && p[-1] != ':')
                n--; /* Drop trailing separator */
        }
    }
#else
    /* All other filename syntaxes */
    if (have_script_arg) {
#if defined(HAVE_REALPATH)
        if (_Py_wrealpath(path0, fullpath, Py_ARRAY_LENGTH(fullpath))) {
            path0 = fullpath;
        }
#endif
        p = wcsrchr(path0, SEP);
    }
    if (p != NULL) {
        n = p + 1 - path0;
#if SEP == '/' /* Special case for Unix filename syntax */
        if (n > 1) {
            /* Drop trailing separator */
            n--;
        }
#endif /* Unix */
    }
#endif /* All others */

    PyObject *path0_obj = PyUnicode_FromWideChar(path0, n);
    if (path0_obj == NULL) {
        return -1;
    }

    *path0_p = path0_obj;
    return 1;
}


#ifdef MS_WINDOWS
#define WCSTOK wcstok_s
#else
#define WCSTOK wcstok
#endif

/* Search for a prefix value in an environment file (pyvenv.cfg).
   If found, copy it into the provided buffer. */
int
_Py_FindEnvConfigValue(FILE *env_file, const wchar_t *key,
                       wchar_t *value, size_t value_size)
{
    int result = 0; /* meaning not found */
    char buffer[MAXPATHLEN * 2 + 1];  /* allow extra for key, '=', etc. */
    buffer[Py_ARRAY_LENGTH(buffer)-1] = '\0';

    fseek(env_file, 0, SEEK_SET);
    while (!feof(env_file)) {
        char * p = fgets(buffer, Py_ARRAY_LENGTH(buffer) - 1, env_file);

        if (p == NULL) {
            break;
        }

        size_t n = strlen(p);
        if (p[n - 1] != '\n') {
            /* line has overflowed - bail */
            break;
        }
        if (p[0] == '#') {
            /* Comment - skip */
            continue;
        }

        wchar_t *tmpbuffer = _Py_DecodeUTF8_surrogateescape(buffer, n, NULL);
        if (tmpbuffer) {
            wchar_t * state;
            wchar_t * tok = WCSTOK(tmpbuffer, L" \t\r\n", &state);
            if ((tok != NULL) && !wcscmp(tok, key)) {
                tok = WCSTOK(NULL, L" \t", &state);
                if ((tok != NULL) && !wcscmp(tok, L"=")) {
                    tok = WCSTOK(NULL, L"\r\n", &state);
                    if (tok != NULL) {
                        wcsncpy(value, tok, value_size - 1);
                        value[value_size - 1] = L'\0';
                        result = 1;
                        PyMem_RawFree(tmpbuffer);
                        break;
                    }
                }
            }
            PyMem_RawFree(tmpbuffer);
        }
    }
    return result;
}

#ifdef __cplusplus
}
#endif
