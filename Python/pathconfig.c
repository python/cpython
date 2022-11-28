/* Path configuration like module_search_path (sys.path) */

#include "Python.h"
#include "marshal.h"              // PyMarshal_ReadObjectFromString
#include "osdefs.h"               // DELIM
#include "pycore_initconfig.h"
#include "pycore_fileutils.h"
#include "pycore_pathconfig.h"
#include "pycore_pymem.h"         // _PyMem_SetDefaultAllocator()
#include <wchar.h>
#ifdef MS_WINDOWS
#  include <windows.h>            // GetFullPathNameW(), MAX_PATH
#  include <pathcch.h>
#  include <shlwapi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/* External interface */

/* Stored values set by C API functions */
typedef struct _PyPathConfig {
    /* Full path to the Python program */
    wchar_t *program_full_path;
    wchar_t *prefix;
    wchar_t *exec_prefix;
    wchar_t *stdlib_dir;
    /* Set by Py_SetPath */
    wchar_t *module_search_path;
    /* Set by _PyPathConfig_UpdateGlobal */
    wchar_t *calculated_module_search_path;
    /* Python program name */
    wchar_t *program_name;
    /* Set by Py_SetPythonHome() or PYTHONHOME environment variable */
    wchar_t *home;
    int _is_python_build;
} _PyPathConfig;

#  define _PyPathConfig_INIT \
      {.module_search_path = NULL, ._is_python_build = 0}


_PyPathConfig _Py_path_config = _PyPathConfig_INIT;


const wchar_t *
_PyPathConfig_GetGlobalModuleSearchPath(void)
{
    return _Py_path_config.module_search_path;
}


void
_PyPathConfig_ClearGlobal(void)
{
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(_Py_path_config.ATTR); \
        _Py_path_config.ATTR = NULL; \
    } while (0)

    CLEAR(program_full_path);
    CLEAR(prefix);
    CLEAR(exec_prefix);
    CLEAR(stdlib_dir);
    CLEAR(module_search_path);
    CLEAR(calculated_module_search_path);
    CLEAR(program_name);
    CLEAR(home);
    _Py_path_config._is_python_build = 0;

#undef CLEAR

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}

PyStatus
_PyPathConfig_ReadGlobal(PyConfig *config)
{
    PyStatus status = _PyStatus_OK();

#define COPY(ATTR) \
    do { \
        if (_Py_path_config.ATTR && !config->ATTR) { \
            status = PyConfig_SetString(config, &config->ATTR, _Py_path_config.ATTR); \
            if (_PyStatus_EXCEPTION(status)) goto done; \
        } \
    } while (0)

#define COPY2(ATTR, SRCATTR) \
    do { \
        if (_Py_path_config.SRCATTR && !config->ATTR) { \
            status = PyConfig_SetString(config, &config->ATTR, _Py_path_config.SRCATTR); \
            if (_PyStatus_EXCEPTION(status)) goto done; \
        } \
    } while (0)

#define COPY_INT(ATTR) \
    do { \
        assert(_Py_path_config.ATTR >= 0); \
        if ((_Py_path_config.ATTR >= 0) && (config->ATTR <= 0)) { \
            config->ATTR = _Py_path_config.ATTR; \
        } \
    } while (0)

    COPY(prefix);
    COPY(exec_prefix);
    COPY(stdlib_dir);
    COPY(program_name);
    COPY(home);
    COPY2(executable, program_full_path);
    COPY_INT(_is_python_build);
    // module_search_path must be initialised - not read
#undef COPY
#undef COPY2
#undef COPY_INT

done:
    return status;
}

PyStatus
_PyPathConfig_UpdateGlobal(const PyConfig *config)
{
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

#define COPY(ATTR) \
    do { \
        if (config->ATTR) { \
            PyMem_RawFree(_Py_path_config.ATTR); \
            _Py_path_config.ATTR = _PyMem_RawWcsdup(config->ATTR); \
            if (!_Py_path_config.ATTR) goto error; \
        } \
    } while (0)

#define COPY2(ATTR, SRCATTR) \
    do { \
        if (config->SRCATTR) { \
            PyMem_RawFree(_Py_path_config.ATTR); \
            _Py_path_config.ATTR = _PyMem_RawWcsdup(config->SRCATTR); \
            if (!_Py_path_config.ATTR) goto error; \
        } \
    } while (0)

#define COPY_INT(ATTR) \
    do { \
        if (config->ATTR > 0) { \
            _Py_path_config.ATTR = config->ATTR; \
        } \
    } while (0)

    COPY(prefix);
    COPY(exec_prefix);
    COPY(stdlib_dir);
    COPY(program_name);
    COPY(home);
    COPY2(program_full_path, executable);
    COPY_INT(_is_python_build);
#undef COPY
#undef COPY2
#undef COPY_INT

    PyMem_RawFree(_Py_path_config.module_search_path);
    _Py_path_config.module_search_path = NULL;
    PyMem_RawFree(_Py_path_config.calculated_module_search_path);
    _Py_path_config.calculated_module_search_path = NULL;

    do {
        size_t cch = 1;
        for (Py_ssize_t i = 0; i < config->module_search_paths.length; ++i) {
            cch += 1 + wcslen(config->module_search_paths.items[i]);
        }

        wchar_t *path = (wchar_t*)PyMem_RawMalloc(sizeof(wchar_t) * cch);
        if (!path) {
            goto error;
        }
        wchar_t *p = path;
        for (Py_ssize_t i = 0; i < config->module_search_paths.length; ++i) {
            wcscpy(p, config->module_search_paths.items[i]);
            p = wcschr(p, L'\0');
            *p++ = DELIM;
            *p = L'\0';
        }

        do {
            *p = L'\0';
        } while (p != path && *--p == DELIM);
        _Py_path_config.calculated_module_search_path = path;
    } while (0);

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return _PyStatus_OK();

error:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    return _PyStatus_NO_MEMORY();
}


static void _Py_NO_RETURN
path_out_of_memory(const char *func)
{
    _Py_FatalErrorFunc(func, "out of memory");
}

void
Py_SetPath(const wchar_t *path)
{
    if (path == NULL) {
        _PyPathConfig_ClearGlobal();
        return;
    }

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.prefix);
    PyMem_RawFree(_Py_path_config.exec_prefix);
    PyMem_RawFree(_Py_path_config.stdlib_dir);
    PyMem_RawFree(_Py_path_config.module_search_path);
    PyMem_RawFree(_Py_path_config.calculated_module_search_path);

    _Py_path_config.prefix = _PyMem_RawWcsdup(L"");
    _Py_path_config.exec_prefix = _PyMem_RawWcsdup(L"");
    // XXX Copy this from the new module_search_path?
    if (_Py_path_config.home != NULL) {
        _Py_path_config.stdlib_dir = _PyMem_RawWcsdup(_Py_path_config.home);
    }
    else {
        _Py_path_config.stdlib_dir = _PyMem_RawWcsdup(L"");
    }
    _Py_path_config.module_search_path = _PyMem_RawWcsdup(path);
    _Py_path_config.calculated_module_search_path = NULL;

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_path_config.prefix == NULL
        || _Py_path_config.exec_prefix == NULL
        || _Py_path_config.stdlib_dir == NULL
        || _Py_path_config.module_search_path == NULL)
    {
        path_out_of_memory(__func__);
    }
}


void
Py_SetPythonHome(const wchar_t *home)
{
    int has_value = home && home[0];

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.home);
    _Py_path_config.home = NULL;

    if (has_value) {
        _Py_path_config.home = _PyMem_RawWcsdup(home);
    }

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (has_value && _Py_path_config.home == NULL) {
        path_out_of_memory(__func__);
    }
}


void
Py_SetProgramName(const wchar_t *program_name)
{
    int has_value = program_name && program_name[0];

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.program_name);
    _Py_path_config.program_name = NULL;

    if (has_value) {
        _Py_path_config.program_name = _PyMem_RawWcsdup(program_name);
    }

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (has_value && _Py_path_config.program_name == NULL) {
        path_out_of_memory(__func__);
    }
}

void
_Py_SetProgramFullPath(const wchar_t *program_full_path)
{
    int has_value = program_full_path && program_full_path[0];

    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    PyMem_RawFree(_Py_path_config.program_full_path);
    _Py_path_config.program_full_path = NULL;

    if (has_value) {
        _Py_path_config.program_full_path = _PyMem_RawWcsdup(program_full_path);
    }

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (has_value && _Py_path_config.program_full_path == NULL) {
        path_out_of_memory(__func__);
    }
}


wchar_t *
Py_GetPath(void)
{
    /* If the user has provided a path, return that */
    if (_Py_path_config.module_search_path) {
        return _Py_path_config.module_search_path;
    }
    /* If we have already done calculations, return the calculated path */
    return _Py_path_config.calculated_module_search_path;
}


wchar_t *
_Py_GetStdlibDir(void)
{
    wchar_t *stdlib_dir = _Py_path_config.stdlib_dir;
    if (stdlib_dir != NULL && stdlib_dir[0] != L'\0') {
        return stdlib_dir;
    }
    return NULL;
}


wchar_t *
Py_GetPrefix(void)
{
    return _Py_path_config.prefix;
}


wchar_t *
Py_GetExecPrefix(void)
{
    return _Py_path_config.exec_prefix;
}


wchar_t *
Py_GetProgramFullPath(void)
{
    return _Py_path_config.program_full_path;
}


wchar_t*
Py_GetPythonHome(void)
{
    return _Py_path_config.home;
}


wchar_t *
Py_GetProgramName(void)
{
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


#ifdef __cplusplus
}
#endif
