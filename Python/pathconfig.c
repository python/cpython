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
    assert(*dst == NULL);
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

    CLEAR(config->program_full_path);
    CLEAR(config->prefix);
    CLEAR(config->exec_prefix);
    CLEAR(config->module_search_path);
    CLEAR(config->program_name);
    CLEAR(config->home);
#ifdef MS_WINDOWS
    CLEAR(config->base_executable);
#endif

#undef CLEAR

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


static PyStatus
pathconfig_copy(_PyPathConfig *config, const _PyPathConfig *config2)
{
    pathconfig_clear(config);

#define COPY_ATTR(ATTR) \
    do { \
        if (copy_wstr(&config->ATTR, config2->ATTR) < 0) { \
            return _PyStatus_NO_MEMORY(); \
        } \
    } while (0)

    COPY_ATTR(program_full_path);
    COPY_ATTR(prefix);
    COPY_ATTR(exec_prefix);
    COPY_ATTR(module_search_path);
    COPY_ATTR(program_name);
    COPY_ATTR(home);
#ifdef MS_WINDOWS
    config->isolated = config2->isolated;
    config->site_import = config2->site_import;
    COPY_ATTR(base_executable);
#endif

#undef COPY_ATTR

    return _PyStatus_OK();
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
            *str++ = sep;
        }
        len = wcslen(path);
        memcpy(str, path, len * sizeof(wchar_t));
        str += len;
    }
    *str = L'\0';

    return text;
}


static PyStatus
pathconfig_set_from_config(_PyPathConfig *pathconfig, const PyConfig *config)
{
    PyStatus status;
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (config->module_search_paths_set) {
        PyMem_RawFree(pathconfig->module_search_path);
        pathconfig->module_search_path = _PyWideStringList_Join(&config->module_search_paths, DELIM);
        if (pathconfig->module_search_path == NULL) {
            goto no_memory;
        }
    }

#define COPY_CONFIG(PATH_ATTR, CONFIG_ATTR) \
        if (config->CONFIG_ATTR) { \
            PyMem_RawFree(pathconfig->PATH_ATTR); \
            pathconfig->PATH_ATTR = NULL; \
            if (copy_wstr(&pathconfig->PATH_ATTR, config->CONFIG_ATTR) < 0) { \
                goto no_memory; \
            } \
        }

    COPY_CONFIG(program_full_path, executable);
    COPY_CONFIG(prefix, prefix);
    COPY_CONFIG(exec_prefix, exec_prefix);
    COPY_CONFIG(program_name, program_name);
    COPY_CONFIG(home, home);
#ifdef MS_WINDOWS
    COPY_CONFIG(base_executable, base_executable);
#endif

#undef COPY_CONFIG

    status = _PyStatus_OK();
    goto done;

no_memory:
    status = _PyStatus_NO_MEMORY();

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}


PyStatus
_PyConfig_WritePathConfig(const PyConfig *config)
{
    return pathconfig_set_from_config(&_Py_path_config, config);
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


/* Calculate the path configuration:

   - exec_prefix
   - module_search_path
   - prefix
   - program_full_path

   On Windows, more fields are calculated:

   - base_executable
   - isolated
   - site_import

   On other platforms, isolated and site_import are left unchanged, and
   _PyConfig_InitPathConfig() copies executable to base_executable (if it's not
   set).

   Priority, highest to lowest:

   - PyConfig
   - _Py_path_config: set by Py_SetPath(), Py_SetPythonHome()
     and Py_SetProgramName()
   - _PyPathConfig_Calculate()
*/
static PyStatus
pathconfig_calculate(_PyPathConfig *pathconfig, const PyConfig *config)
{
    PyStatus status;

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    status = pathconfig_copy(pathconfig, &_Py_path_config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = pathconfig_set_from_config(pathconfig, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    if (_Py_path_config.module_search_path == NULL) {
        status = _PyPathConfig_Calculate(pathconfig, config);
    }
    else {
        /* Py_SetPath() has been called: avoid _PyPathConfig_Calculate() */
    }

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return status;
}


static PyStatus
config_calculate_pathconfig(PyConfig *config)
{
    _PyPathConfig pathconfig = _PyPathConfig_INIT;
    PyStatus status;

    status = pathconfig_calculate(&pathconfig, config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    if (!config->module_search_paths_set) {
        status = config_init_module_search_paths(config, &pathconfig);
        if (_PyStatus_EXCEPTION(status)) {
            goto done;
        }
    }

#define COPY_ATTR(PATH_ATTR, CONFIG_ATTR) \
        if (config->CONFIG_ATTR == NULL) { \
            if (copy_wstr(&config->CONFIG_ATTR, pathconfig.PATH_ATTR) < 0) { \
                goto no_memory; \
            } \
        }

#ifdef MS_WINDOWS
    if (config->executable != NULL && config->base_executable == NULL) {
        /* If executable is set explicitly in the configuration,
           ignore calculated base_executable: _PyConfig_InitPathConfig()
           will copy executable to base_executable */
    }
    else {
        COPY_ATTR(base_executable, base_executable);
    }
#endif

    COPY_ATTR(program_full_path, executable);
    COPY_ATTR(prefix, prefix);
    COPY_ATTR(exec_prefix, exec_prefix);

#undef COPY_ATTR

#ifdef MS_WINDOWS
    /* If a ._pth file is found: isolated and site_import are overriden */
    if (pathconfig.isolated != -1) {
        config->isolated = pathconfig.isolated;
    }
    if (pathconfig.site_import != -1) {
        config->site_import = pathconfig.site_import;
    }
#endif

    status = _PyStatus_OK();
    goto done;

no_memory:
    status = _PyStatus_NO_MEMORY();

done:
    pathconfig_clear(&pathconfig);
    return status;
}


PyStatus
_PyConfig_InitPathConfig(PyConfig *config)
{
    /* Do we need to calculate the path? */
    if (!config->module_search_paths_set
        || config->executable == NULL
        || config->prefix == NULL
        || config->exec_prefix == NULL)
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

    if (config->base_executable == NULL) {
        if (copy_wstr(&config->base_executable,
                      config->executable) < 0) {
            return _PyStatus_NO_MEMORY();
        }
    }

    return _PyStatus_OK();
}


static PyStatus
pathconfig_global_read(_PyPathConfig *pathconfig)
{
    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    /* Call _PyConfig_InitPathConfig() */
    PyStatus status = PyConfig_Read(&config);
    if (_PyStatus_EXCEPTION(status)) {
        goto done;
    }

    status = pathconfig_set_from_config(pathconfig, &config);

done:
    PyConfig_Clear(&config);
    return status;
}


static void
pathconfig_global_init(void)
{
    PyStatus status;

    if (_Py_path_config.module_search_path == NULL) {
        status = pathconfig_global_read(&_Py_path_config);
        if (_PyStatus_EXCEPTION(status)) {
            Py_ExitStatusException(status);
        }
    }
    else {
        /* Global configuration already initialized */
    }

    assert(_Py_path_config.program_full_path != NULL);
    assert(_Py_path_config.prefix != NULL);
    assert(_Py_path_config.exec_prefix != NULL);
    assert(_Py_path_config.module_search_path != NULL);
    assert(_Py_path_config.program_name != NULL);
    /* home can be NULL */
#ifdef MS_WINDOWS
    assert(_Py_path_config.base_executable != NULL);
#endif
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

    /* Getting the program full path calls pathconfig_global_init() */
    wchar_t *program_full_path = _PyMem_RawWcsdup(Py_GetProgramFullPath());

    PyMem_RawFree(_Py_path_config.program_full_path);
    PyMem_RawFree(_Py_path_config.prefix);
    PyMem_RawFree(_Py_path_config.exec_prefix);
    PyMem_RawFree(_Py_path_config.module_search_path);

    _Py_path_config.program_full_path = program_full_path;
    _Py_path_config.prefix = _PyMem_RawWcsdup(L"");
    _Py_path_config.exec_prefix = _PyMem_RawWcsdup(L"");
    _Py_path_config.module_search_path = _PyMem_RawWcsdup(path);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_path_config.program_full_path == NULL
        || _Py_path_config.prefix == NULL
        || _Py_path_config.exec_prefix == NULL
        || _Py_path_config.module_search_path == NULL)
    {
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
    wchar_t path0copy[2 * MAXPATHLEN + 1];

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
