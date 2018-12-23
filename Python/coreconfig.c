#include "Python.h"
#include "pycore_fileutils.h"
#include "pycore_pylifecycle.h"
#include "pycore_pymem.h"
#include "pycore_pathconfig.h"
#include "pycore_pystate.h"
#include <locale.h>
#ifdef HAVE_LANGINFO_H
#  include <langinfo.h>
#endif

#include <locale.h>     /* setlocale() */
#ifdef HAVE_LANGINFO_H
#include <langinfo.h>   /* nl_langinfo(CODESET) */
#endif


#define DECODE_LOCALE_ERR(NAME, LEN) \
    (((LEN) == -2) \
     ? _Py_INIT_USER_ERR("cannot decode " NAME) \
     : _Py_INIT_NO_MEMORY())


/* Global configuration variables */

/* The filesystem encoding is chosen by config_init_fs_encoding(),
   see also initfsencoding(). */
const char *Py_FileSystemDefaultEncoding = NULL;
int Py_HasFileSystemDefaultEncoding = 0;
const char *Py_FileSystemDefaultEncodeErrors = NULL;
static int _Py_HasFileSystemDefaultEncodeErrors = 0;

/* UTF-8 mode (PEP 540): if equals to 1, use the UTF-8 encoding, and change
   stdin and stdout error handler to "surrogateescape". It is equal to
   -1 by default: unknown, will be set by Py_Main() */
int Py_UTF8Mode = -1;
int Py_DebugFlag = 0; /* Needed by parser.c */
int Py_VerboseFlag = 0; /* Needed by import.c */
int Py_QuietFlag = 0; /* Needed by sysmodule.c */
int Py_InteractiveFlag = 0; /* Needed by Py_FdIsInteractive() below */
int Py_InspectFlag = 0; /* Needed to determine whether to exit at SystemExit */
int Py_OptimizeFlag = 0; /* Needed by compile.c */
int Py_NoSiteFlag = 0; /* Suppress 'import site' */
int Py_BytesWarningFlag = 0; /* Warn on str(bytes) and str(buffer) */
int Py_FrozenFlag = 0; /* Needed by getpath.c */
int Py_IgnoreEnvironmentFlag = 0; /* e.g. PYTHONPATH, PYTHONHOME */
int Py_DontWriteBytecodeFlag = 0; /* Suppress writing bytecode files (*.pyc) */
int Py_NoUserSiteDirectory = 0; /* for -s and site.py */
int Py_UnbufferedStdioFlag = 0; /* Unbuffered binary std{in,out,err} */
int Py_HashRandomizationFlag = 0; /* for -R and PYTHONHASHSEED */
int Py_IsolatedFlag = 0; /* for -I, isolate from user's env */
#ifdef MS_WINDOWS
int Py_LegacyWindowsFSEncodingFlag = 0; /* Uses mbcs instead of utf-8 */
int Py_LegacyWindowsStdioFlag = 0; /* Uses FileIO instead of WindowsConsoleIO */
#endif


PyObject *
_Py_GetGlobalVariablesAsDict(void)
{
    PyObject *dict, *obj;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

#define SET_ITEM(KEY, EXPR) \
        do { \
            obj = (EXPR); \
            if (obj == NULL) { \
                return NULL; \
            } \
            int res = PyDict_SetItemString(dict, (KEY), obj); \
            Py_DECREF(obj); \
            if (res < 0) { \
                goto fail; \
            } \
        } while (0)
#define SET_ITEM_INT(VAR) \
    SET_ITEM(#VAR, PyLong_FromLong(VAR))
#define FROM_STRING(STR) \
    ((STR != NULL) ? \
        PyUnicode_FromString(STR) \
        : (Py_INCREF(Py_None), Py_None))
#define SET_ITEM_STR(VAR) \
    SET_ITEM(#VAR, FROM_STRING(VAR))

    SET_ITEM_STR(Py_FileSystemDefaultEncoding);
    SET_ITEM_INT(Py_HasFileSystemDefaultEncoding);
    SET_ITEM_STR(Py_FileSystemDefaultEncodeErrors);
    SET_ITEM_INT(_Py_HasFileSystemDefaultEncodeErrors);

    SET_ITEM_INT(Py_UTF8Mode);
    SET_ITEM_INT(Py_DebugFlag);
    SET_ITEM_INT(Py_VerboseFlag);
    SET_ITEM_INT(Py_QuietFlag);
    SET_ITEM_INT(Py_InteractiveFlag);
    SET_ITEM_INT(Py_InspectFlag);

    SET_ITEM_INT(Py_OptimizeFlag);
    SET_ITEM_INT(Py_NoSiteFlag);
    SET_ITEM_INT(Py_BytesWarningFlag);
    SET_ITEM_INT(Py_FrozenFlag);
    SET_ITEM_INT(Py_IgnoreEnvironmentFlag);
    SET_ITEM_INT(Py_DontWriteBytecodeFlag);
    SET_ITEM_INT(Py_NoUserSiteDirectory);
    SET_ITEM_INT(Py_UnbufferedStdioFlag);
    SET_ITEM_INT(Py_HashRandomizationFlag);
    SET_ITEM_INT(Py_IsolatedFlag);

#ifdef MS_WINDOWS
    SET_ITEM_INT(Py_LegacyWindowsFSEncodingFlag);
    SET_ITEM_INT(Py_LegacyWindowsStdioFlag);
#endif

    return dict;

fail:
    Py_DECREF(dict);
    return NULL;

#undef FROM_STRING
#undef SET_ITEM
#undef SET_ITEM_INT
#undef SET_ITEM_STR
}


void
_Py_wstrlist_clear(int len, wchar_t **list)
{
    for (int i=0; i < len; i++) {
        PyMem_RawFree(list[i]);
    }
    PyMem_RawFree(list);
}


wchar_t**
_Py_wstrlist_copy(int len, wchar_t **list)
{
    assert((len > 0 && list != NULL) || len == 0);
    size_t size = len * sizeof(list[0]);
    wchar_t **list_copy = PyMem_RawMalloc(size);
    if (list_copy == NULL) {
        return NULL;
    }
    for (int i=0; i < len; i++) {
        wchar_t* arg = _PyMem_RawWcsdup(list[i]);
        if (arg == NULL) {
            _Py_wstrlist_clear(i, list_copy);
            return NULL;
        }
        list_copy[i] = arg;
    }
    return list_copy;
}


void
_Py_ClearFileSystemEncoding(void)
{
    if (!Py_HasFileSystemDefaultEncoding && Py_FileSystemDefaultEncoding) {
        PyMem_RawFree((char*)Py_FileSystemDefaultEncoding);
        Py_FileSystemDefaultEncoding = NULL;
    }
    if (!_Py_HasFileSystemDefaultEncodeErrors && Py_FileSystemDefaultEncodeErrors) {
        PyMem_RawFree((char*)Py_FileSystemDefaultEncodeErrors);
        Py_FileSystemDefaultEncodeErrors = NULL;
    }
}


/* Set Py_FileSystemDefaultEncoding and Py_FileSystemDefaultEncodeErrors
   global configuration variables. */
int
_Py_SetFileSystemEncoding(const char *encoding, const char *errors)
{
    char *encoding2 = _PyMem_RawStrdup(encoding);
    if (encoding2 == NULL) {
        return -1;
    }

    char *errors2 = _PyMem_RawStrdup(errors);
    if (errors2 == NULL) {
        PyMem_RawFree(encoding2);
        return -1;
    }

    _Py_ClearFileSystemEncoding();

    Py_FileSystemDefaultEncoding = encoding2;
    Py_HasFileSystemDefaultEncoding = 0;

    Py_FileSystemDefaultEncodeErrors = errors2;
    _Py_HasFileSystemDefaultEncodeErrors = 0;
    return 0;
}


/* Helper to allow an embedding application to override the normal
 * mechanism that attempts to figure out an appropriate IO encoding
 */

static char *_Py_StandardStreamEncoding = NULL;
static char *_Py_StandardStreamErrors = NULL;

int
Py_SetStandardStreamEncoding(const char *encoding, const char *errors)
{
    if (Py_IsInitialized()) {
        /* This is too late to have any effect */
        return -1;
    }

    int res = 0;

    /* Py_SetStandardStreamEncoding() can be called before Py_Initialize(),
       but Py_Initialize() can change the allocator. Use a known allocator
       to be able to release the memory later. */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* Can't call PyErr_NoMemory() on errors, as Python hasn't been
     * initialised yet.
     *
     * However, the raw memory allocators are initialised appropriately
     * as C static variables, so _PyMem_RawStrdup is OK even though
     * Py_Initialize hasn't been called yet.
     */
    if (encoding) {
        _Py_StandardStreamEncoding = _PyMem_RawStrdup(encoding);
        if (!_Py_StandardStreamEncoding) {
            res = -2;
            goto done;
        }
    }
    if (errors) {
        _Py_StandardStreamErrors = _PyMem_RawStrdup(errors);
        if (!_Py_StandardStreamErrors) {
            if (_Py_StandardStreamEncoding) {
                PyMem_RawFree(_Py_StandardStreamEncoding);
            }
            res = -3;
            goto done;
        }
    }
#ifdef MS_WINDOWS
    if (_Py_StandardStreamEncoding) {
        /* Overriding the stream encoding implies legacy streams */
        Py_LegacyWindowsStdioFlag = 1;
    }
#endif

done:
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    return res;
}


void
_Py_ClearStandardStreamEncoding(void)
{
    /* Use the same allocator than Py_SetStandardStreamEncoding() */
    PyMemAllocatorEx old_alloc;
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    /* We won't need them anymore. */
    if (_Py_StandardStreamEncoding) {
        PyMem_RawFree(_Py_StandardStreamEncoding);
        _Py_StandardStreamEncoding = NULL;
    }
    if (_Py_StandardStreamErrors) {
        PyMem_RawFree(_Py_StandardStreamErrors);
        _Py_StandardStreamErrors = NULL;
    }

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
}


/* Free memory allocated in config, but don't clear all attributes */
void
_PyCoreConfig_Clear(_PyCoreConfig *config)
{
#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(ATTR); \
        ATTR = NULL; \
    } while (0)
#define CLEAR_WSTRLIST(LEN, LIST) \
    do { \
        _Py_wstrlist_clear(LEN, LIST); \
        LEN = 0; \
        LIST = NULL; \
    } while (0)

    CLEAR(config->pycache_prefix);
    CLEAR(config->module_search_path_env);
    CLEAR(config->home);
    CLEAR(config->program_name);
    CLEAR(config->program);

    CLEAR_WSTRLIST(config->argc, config->argv);
    config->argc = -1;

    CLEAR_WSTRLIST(config->nwarnoption, config->warnoptions);
    CLEAR_WSTRLIST(config->nxoption, config->xoptions);
    CLEAR_WSTRLIST(config->nmodule_search_path, config->module_search_paths);
    config->nmodule_search_path = -1;

    CLEAR(config->executable);
    CLEAR(config->prefix);
    CLEAR(config->base_prefix);
    CLEAR(config->exec_prefix);
#ifdef MS_WINDOWS
    CLEAR(config->dll_path);
#endif
    CLEAR(config->base_exec_prefix);

    CLEAR(config->filesystem_encoding);
    CLEAR(config->filesystem_errors);
    CLEAR(config->stdio_encoding);
    CLEAR(config->stdio_errors);
#undef CLEAR
#undef CLEAR_WSTRLIST
}


int
_PyCoreConfig_Copy(_PyCoreConfig *config, const _PyCoreConfig *config2)
{
    _PyCoreConfig_Clear(config);

#define COPY_ATTR(ATTR) config->ATTR = config2->ATTR
#define COPY_STR_ATTR(ATTR) \
    do { \
        if (config2->ATTR != NULL) { \
            config->ATTR = _PyMem_RawStrdup(config2->ATTR); \
            if (config->ATTR == NULL) { \
                return -1; \
            } \
        } \
    } while (0)
#define COPY_WSTR_ATTR(ATTR) \
    do { \
        if (config2->ATTR != NULL) { \
            config->ATTR = _PyMem_RawWcsdup(config2->ATTR); \
            if (config->ATTR == NULL) { \
                return -1; \
            } \
        } \
    } while (0)
#define COPY_WSTRLIST(LEN, LIST) \
    do { \
        if (config2->LIST != NULL) { \
            config->LIST = _Py_wstrlist_copy(config2->LEN, config2->LIST); \
            if (config->LIST == NULL) { \
                return -1; \
            } \
        } \
        config->LEN = config2->LEN; \
    } while (0)

    COPY_ATTR(install_signal_handlers);
    COPY_ATTR(use_environment);
    COPY_ATTR(use_hash_seed);
    COPY_ATTR(hash_seed);
    COPY_ATTR(_install_importlib);
    COPY_ATTR(allocator);
    COPY_ATTR(dev_mode);
    COPY_ATTR(faulthandler);
    COPY_ATTR(tracemalloc);
    COPY_ATTR(import_time);
    COPY_ATTR(show_ref_count);
    COPY_ATTR(show_alloc_count);
    COPY_ATTR(dump_refs);
    COPY_ATTR(malloc_stats);

    COPY_ATTR(coerce_c_locale);
    COPY_ATTR(coerce_c_locale_warn);
    COPY_ATTR(utf8_mode);

    COPY_WSTR_ATTR(pycache_prefix);
    COPY_WSTR_ATTR(module_search_path_env);
    COPY_WSTR_ATTR(home);
    COPY_WSTR_ATTR(program_name);
    COPY_WSTR_ATTR(program);

    COPY_WSTRLIST(argc, argv);
    COPY_WSTRLIST(nwarnoption, warnoptions);
    COPY_WSTRLIST(nxoption, xoptions);
    COPY_WSTRLIST(nmodule_search_path, module_search_paths);

    COPY_WSTR_ATTR(executable);
    COPY_WSTR_ATTR(prefix);
    COPY_WSTR_ATTR(base_prefix);
    COPY_WSTR_ATTR(exec_prefix);
#ifdef MS_WINDOWS
    COPY_WSTR_ATTR(dll_path);
#endif
    COPY_WSTR_ATTR(base_exec_prefix);

    COPY_ATTR(isolated);
    COPY_ATTR(site_import);
    COPY_ATTR(bytes_warning);
    COPY_ATTR(inspect);
    COPY_ATTR(interactive);
    COPY_ATTR(optimization_level);
    COPY_ATTR(parser_debug);
    COPY_ATTR(write_bytecode);
    COPY_ATTR(verbose);
    COPY_ATTR(quiet);
    COPY_ATTR(user_site_directory);
    COPY_ATTR(buffered_stdio);
    COPY_STR_ATTR(filesystem_encoding);
    COPY_STR_ATTR(filesystem_errors);
    COPY_STR_ATTR(stdio_encoding);
    COPY_STR_ATTR(stdio_errors);
#ifdef MS_WINDOWS
    COPY_ATTR(legacy_windows_fs_encoding);
    COPY_ATTR(legacy_windows_stdio);
#endif
    COPY_ATTR(_check_hash_pycs_mode);
    COPY_ATTR(_frozen);

#undef COPY_ATTR
#undef COPY_STR_ATTR
#undef COPY_WSTR_ATTR
#undef COPY_WSTRLIST
    return 0;
}


const char*
_PyCoreConfig_GetEnv(const _PyCoreConfig *config, const char *name)
{
    assert(config->use_environment >= 0);

    if (!config->use_environment) {
        return NULL;
    }

    const char *var = getenv(name);
    if (var && var[0] != '\0') {
        return var;
    }
    else {
        return NULL;
    }
}


int
_PyCoreConfig_GetEnvDup(const _PyCoreConfig *config,
                        wchar_t **dest,
                        wchar_t *wname, char *name)
{
    assert(config->use_environment >= 0);

    if (!config->use_environment) {
        *dest = NULL;
        return 0;
    }

#ifdef MS_WINDOWS
    const wchar_t *var = _wgetenv(wname);
    if (!var || var[0] == '\0') {
        *dest = NULL;
        return 0;
    }

    wchar_t *copy = _PyMem_RawWcsdup(var);
    if (copy == NULL) {
        return -1;
    }

    *dest = copy;
#else
    const char *var = getenv(name);
    if (!var || var[0] == '\0') {
        *dest = NULL;
        return 0;
    }

    size_t len;
    wchar_t *wvar = Py_DecodeLocale(var, &len);
    if (!wvar) {
        if (len == (size_t)-2) {
            return -2;
        }
        else {
            return -1;
        }
    }
    *dest = wvar;
#endif
    return 0;
}


void
_PyCoreConfig_GetGlobalConfig(_PyCoreConfig *config)
{
#define COPY_FLAG(ATTR, VALUE) \
        if (config->ATTR == -1) { \
            config->ATTR = VALUE; \
        }
#define COPY_NOT_FLAG(ATTR, VALUE) \
        if (config->ATTR == -1) { \
            config->ATTR = !(VALUE); \
        }

    COPY_FLAG(utf8_mode, Py_UTF8Mode);
    COPY_FLAG(isolated, Py_IsolatedFlag);
    COPY_FLAG(bytes_warning, Py_BytesWarningFlag);
    COPY_FLAG(inspect, Py_InspectFlag);
    COPY_FLAG(interactive, Py_InteractiveFlag);
    COPY_FLAG(optimization_level, Py_OptimizeFlag);
    COPY_FLAG(parser_debug, Py_DebugFlag);
    COPY_FLAG(verbose, Py_VerboseFlag);
    COPY_FLAG(quiet, Py_QuietFlag);
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_fs_encoding, Py_LegacyWindowsFSEncodingFlag);
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_FLAG(_frozen, Py_FrozenFlag);

    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);
    COPY_NOT_FLAG(buffered_stdio, Py_UnbufferedStdioFlag);
    COPY_NOT_FLAG(site_import, Py_NoSiteFlag);
    COPY_NOT_FLAG(write_bytecode, Py_DontWriteBytecodeFlag);
    COPY_NOT_FLAG(user_site_directory, Py_NoUserSiteDirectory);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


/* Set Py_xxx global configuration variables from 'config' configuration. */
void
_PyCoreConfig_SetGlobalConfig(const _PyCoreConfig *config)
{
#define COPY_FLAG(ATTR, VAR) \
        if (config->ATTR != -1) { \
            VAR = config->ATTR; \
        }
#define COPY_NOT_FLAG(ATTR, VAR) \
        if (config->ATTR != -1) { \
            VAR = !config->ATTR; \
        }

    COPY_FLAG(utf8_mode, Py_UTF8Mode);
    COPY_FLAG(isolated, Py_IsolatedFlag);
    COPY_FLAG(bytes_warning, Py_BytesWarningFlag);
    COPY_FLAG(inspect, Py_InspectFlag);
    COPY_FLAG(interactive, Py_InteractiveFlag);
    COPY_FLAG(optimization_level, Py_OptimizeFlag);
    COPY_FLAG(parser_debug, Py_DebugFlag);
    COPY_FLAG(verbose, Py_VerboseFlag);
    COPY_FLAG(quiet, Py_QuietFlag);
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_fs_encoding, Py_LegacyWindowsFSEncodingFlag);
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_FLAG(_frozen, Py_FrozenFlag);

    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);
    COPY_NOT_FLAG(buffered_stdio, Py_UnbufferedStdioFlag);
    COPY_NOT_FLAG(site_import, Py_NoSiteFlag);
    COPY_NOT_FLAG(write_bytecode, Py_DontWriteBytecodeFlag);
    COPY_NOT_FLAG(user_site_directory, Py_NoUserSiteDirectory);

    /* Random or non-zero hash seed */
    Py_HashRandomizationFlag = (config->use_hash_seed == 0 ||
                                config->hash_seed != 0);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


/* Get the program name: use PYTHONEXECUTABLE and __PYVENV_LAUNCHER__
   environment variables on macOS if available. */
static _PyInitError
config_init_program_name(_PyCoreConfig *config)
{
    assert(config->program_name == NULL);

    /* If Py_SetProgramName() was called, use its value */
    const wchar_t *program_name = _Py_path_config.program_name;
    if (program_name != NULL) {
        config->program_name = _PyMem_RawWcsdup(program_name);
        if (config->program_name == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
        return _Py_INIT_OK();
    }

#ifdef __APPLE__
    /* On MacOS X, when the Python interpreter is embedded in an
       application bundle, it gets executed by a bootstrapping script
       that does os.execve() with an argv[0] that's different from the
       actual Python executable. This is needed to keep the Finder happy,
       or rather, to work around Apple's overly strict requirements of
       the process name. However, we still need a usable sys.executable,
       so the actual executable path is passed in an environment variable.
       See Lib/plat-mac/bundlebuiler.py for details about the bootstrap
       script. */
    const char *p = _PyCoreConfig_GetEnv(config, "PYTHONEXECUTABLE");
    if (p != NULL) {
        size_t len;
        wchar_t* program_name = Py_DecodeLocale(p, &len);
        if (program_name == NULL) {
            return DECODE_LOCALE_ERR("PYTHONEXECUTABLE environment "
                                     "variable", (Py_ssize_t)len);
        }
        config->program_name = program_name;
        return _Py_INIT_OK();
    }
#ifdef WITH_NEXT_FRAMEWORK
    else {
        const char* pyvenv_launcher = getenv("__PYVENV_LAUNCHER__");
        if (pyvenv_launcher && *pyvenv_launcher) {
            /* Used by Mac/Tools/pythonw.c to forward
             * the argv0 of the stub executable
             */
            size_t len;
            wchar_t* program_name = Py_DecodeLocale(pyvenv_launcher, &len);
            if (program_name == NULL) {
                return DECODE_LOCALE_ERR("__PYVENV_LAUNCHER__ environment "
                                         "variable", (Py_ssize_t)len);
            }
            config->program_name = program_name;
            return _Py_INIT_OK();
        }
    }
#endif   /* WITH_NEXT_FRAMEWORK */
#endif   /* __APPLE__ */

    /* Use argv[0] by default, if available */
    if (config->program != NULL) {
        config->program_name = _PyMem_RawWcsdup(config->program);
        if (config->program_name == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
        return _Py_INIT_OK();
    }

    /* Last fall back: hardcoded string */
#ifdef MS_WINDOWS
    const wchar_t *default_program_name = L"python";
#else
    const wchar_t *default_program_name = L"python3";
#endif
    config->program_name = _PyMem_RawWcsdup(default_program_name);
    if (config->program_name == NULL) {
        return _Py_INIT_NO_MEMORY();
    }
    return _Py_INIT_OK();
}

static _PyInitError
config_init_executable(_PyCoreConfig *config)
{
    assert(config->executable == NULL);

    /* If Py_SetProgramFullPath() was called, use its value */
    const wchar_t *program_full_path = _Py_path_config.program_full_path;
    if (program_full_path != NULL) {
        config->executable = _PyMem_RawWcsdup(program_full_path);
        if (config->executable == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
        return _Py_INIT_OK();
    }

    return _Py_INIT_OK();
}

static const wchar_t*
config_get_xoption(const _PyCoreConfig *config, wchar_t *name)
{
    int nxoption = config->nxoption;
    wchar_t **xoptions = config->xoptions;
    for (int i=0; i < nxoption; i++) {
        wchar_t *option = xoptions[i];
        size_t len;
        wchar_t *sep = wcschr(option, L'=');
        if (sep != NULL) {
            len = (sep - option);
        }
        else {
            len = wcslen(option);
        }
        if (wcsncmp(option, name, len) == 0 && name[len] == L'\0') {
            return option;
        }
    }
    return NULL;
}


static _PyInitError
config_init_home(_PyCoreConfig *config)
{
    wchar_t *home;

    /* If Py_SetPythonHome() was called, use its value */
    home = _Py_path_config.home;
    if (home) {
        config->home = _PyMem_RawWcsdup(home);
        if (config->home == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
        return _Py_INIT_OK();
    }

    int res = _PyCoreConfig_GetEnvDup(config, &home,
                                      L"PYTHONHOME", "PYTHONHOME");
    if (res < 0) {
        return DECODE_LOCALE_ERR("PYTHONHOME", res);
    }
    config->home = home;
    return _Py_INIT_OK();
}


static _PyInitError
config_init_hash_seed(_PyCoreConfig *config)
{
    const char *seed_text = _PyCoreConfig_GetEnv(config, "PYTHONHASHSEED");

    Py_BUILD_ASSERT(sizeof(_Py_HashSecret_t) == sizeof(_Py_HashSecret.uc));
    /* Convert a text seed to a numeric one */
    if (seed_text && strcmp(seed_text, "random") != 0) {
        const char *endptr = seed_text;
        unsigned long seed;
        errno = 0;
        seed = strtoul(seed_text, (char **)&endptr, 10);
        if (*endptr != '\0'
            || seed > 4294967295UL
            || (errno == ERANGE && seed == ULONG_MAX))
        {
            return _Py_INIT_USER_ERR("PYTHONHASHSEED must be \"random\" "
                                     "or an integer in range [0; 4294967295]");
        }
        /* Use a specific hash */
        config->use_hash_seed = 1;
        config->hash_seed = seed;
    }
    else {
        /* Use a random hash */
        config->use_hash_seed = 0;
        config->hash_seed = 0;
    }
    return _Py_INIT_OK();
}


static _PyInitError
config_init_utf8_mode(_PyCoreConfig *config)
{
    const wchar_t *xopt = config_get_xoption(config, L"utf8");
    if (xopt) {
        wchar_t *sep = wcschr(xopt, L'=');
        if (sep) {
            xopt = sep + 1;
            if (wcscmp(xopt, L"1") == 0) {
                config->utf8_mode = 1;
            }
            else if (wcscmp(xopt, L"0") == 0) {
                config->utf8_mode = 0;
            }
            else {
                return _Py_INIT_USER_ERR("invalid -X utf8 option value");
            }
        }
        else {
            config->utf8_mode = 1;
        }
        return _Py_INIT_OK();
    }

    const char *opt = _PyCoreConfig_GetEnv(config, "PYTHONUTF8");
    if (opt) {
        if (strcmp(opt, "1") == 0) {
            config->utf8_mode = 1;
        }
        else if (strcmp(opt, "0") == 0) {
            config->utf8_mode = 0;
        }
        else {
            return _Py_INIT_USER_ERR("invalid PYTHONUTF8 environment "
                                     "variable value");
        }
        return _Py_INIT_OK();
    }

    return _Py_INIT_OK();
}


static int
config_str_to_int(const char *str, int *result)
{
    const char *endptr = str;
    errno = 0;
    long value = strtol(str, (char **)&endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) {
        return -1;
    }
    if (value < INT_MIN || value > INT_MAX) {
        return -1;
    }

    *result = (int)value;
    return 0;
}


static int
config_wstr_to_int(const wchar_t *wstr, int *result)
{
    const wchar_t *endptr = wstr;
    errno = 0;
    long value = wcstol(wstr, (wchar_t **)&endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) {
        return -1;
    }
    if (value < INT_MIN || value > INT_MAX) {
        return -1;
    }

    *result = (int)value;
    return 0;
}


static void
get_env_flag(_PyCoreConfig *config, int *flag, const char *name)
{
    const char *var = _PyCoreConfig_GetEnv(config, name);
    if (!var) {
        return;
    }
    int value;
    if (config_str_to_int(var, &value) < 0 || value < 0) {
        /* PYTHONDEBUG=text and PYTHONDEBUG=-2 behave as PYTHONDEBUG=1 */
        value = 1;
    }
    if (*flag < value) {
        *flag = value;
    }
}


static _PyInitError
config_read_env_vars(_PyCoreConfig *config)
{
    /* Get environment variables */
    get_env_flag(config, &config->parser_debug, "PYTHONDEBUG");
    get_env_flag(config, &config->verbose, "PYTHONVERBOSE");
    get_env_flag(config, &config->optimization_level, "PYTHONOPTIMIZE");
    get_env_flag(config, &config->inspect, "PYTHONINSPECT");

    int dont_write_bytecode = 0;
    get_env_flag(config, &dont_write_bytecode, "PYTHONDONTWRITEBYTECODE");
    if (dont_write_bytecode) {
        config->write_bytecode = 0;
    }

    int no_user_site_directory = 0;
    get_env_flag(config, &no_user_site_directory, "PYTHONNOUSERSITE");
    if (no_user_site_directory) {
        config->user_site_directory = 0;
    }

    int unbuffered_stdio = 0;
    get_env_flag(config, &unbuffered_stdio, "PYTHONUNBUFFERED");
    if (unbuffered_stdio) {
        config->buffered_stdio = 0;
    }

#ifdef MS_WINDOWS
    get_env_flag(config, &config->legacy_windows_fs_encoding,
                 "PYTHONLEGACYWINDOWSFSENCODING");
    get_env_flag(config, &config->legacy_windows_stdio,
                 "PYTHONLEGACYWINDOWSSTDIO");
#endif

    if (config->allocator == NULL) {
        config->allocator = _PyCoreConfig_GetEnv(config, "PYTHONMALLOC");
    }

    if (_PyCoreConfig_GetEnv(config, "PYTHONDUMPREFS")) {
        config->dump_refs = 1;
    }
    if (_PyCoreConfig_GetEnv(config, "PYTHONMALLOCSTATS")) {
        config->malloc_stats = 1;
    }

    const char *env = _PyCoreConfig_GetEnv(config, "PYTHONCOERCECLOCALE");
    if (env) {
        if (strcmp(env, "0") == 0) {
            if (config->coerce_c_locale < 0) {
                config->coerce_c_locale = 0;
            }
        }
        else if (strcmp(env, "warn") == 0) {
            config->coerce_c_locale_warn = 1;
        }
        else {
            if (config->coerce_c_locale < 0) {
                config->coerce_c_locale = 1;
            }
        }
    }

    wchar_t *path;
    int res = _PyCoreConfig_GetEnvDup(config, &path,
                                      L"PYTHONPATH", "PYTHONPATH");
    if (res < 0) {
        return DECODE_LOCALE_ERR("PYTHONPATH", res);
    }
    config->module_search_path_env = path;

    if (config->use_hash_seed < 0) {
        _PyInitError err = config_init_hash_seed(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    return _Py_INIT_OK();
}


static _PyInitError
config_init_tracemalloc(_PyCoreConfig *config)
{
    int nframe;
    int valid;

    const char *env = _PyCoreConfig_GetEnv(config, "PYTHONTRACEMALLOC");
    if (env) {
        if (!config_str_to_int(env, &nframe)) {
            valid = (nframe >= 0);
        }
        else {
            valid = 0;
        }
        if (!valid) {
            return _Py_INIT_USER_ERR("PYTHONTRACEMALLOC: invalid number "
                                     "of frames");
        }
        config->tracemalloc = nframe;
    }

    const wchar_t *xoption = config_get_xoption(config, L"tracemalloc");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep) {
            if (!config_wstr_to_int(sep + 1, &nframe)) {
                valid = (nframe >= 0);
            }
            else {
                valid = 0;
            }
            if (!valid) {
                return _Py_INIT_USER_ERR("-X tracemalloc=NFRAME: "
                                         "invalid number of frames");
            }
        }
        else {
            /* -X tracemalloc behaves as -X tracemalloc=1 */
            nframe = 1;
        }
        config->tracemalloc = nframe;
    }
    return _Py_INIT_OK();
}


static _PyInitError
config_init_pycache_prefix(_PyCoreConfig *config)
{
    assert(config->pycache_prefix == NULL);

    const wchar_t *xoption = config_get_xoption(config, L"pycache_prefix");
    if (xoption) {
        const wchar_t *sep = wcschr(xoption, L'=');
        if (sep && wcslen(sep) > 1) {
            config->pycache_prefix = _PyMem_RawWcsdup(sep + 1);
            if (config->pycache_prefix == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
        else {
            // -X pycache_prefix= can cancel the env var
            config->pycache_prefix = NULL;
        }
    }
    else {
        wchar_t *env;
        int res = _PyCoreConfig_GetEnvDup(config, &env,
                                          L"PYTHONPYCACHEPREFIX",
                                          "PYTHONPYCACHEPREFIX");
        if (res < 0) {
            return DECODE_LOCALE_ERR("PYTHONPYCACHEPREFIX", res);
        }

        if (env) {
            config->pycache_prefix = env;
        }
    }
    return _Py_INIT_OK();
}


static _PyInitError
config_read_complex_options(_PyCoreConfig *config)
{
    /* More complex options configured by env var and -X option */
    if (config->faulthandler < 0) {
        if (_PyCoreConfig_GetEnv(config, "PYTHONFAULTHANDLER")
           || config_get_xoption(config, L"faulthandler")) {
            config->faulthandler = 1;
        }
    }
    if (_PyCoreConfig_GetEnv(config, "PYTHONPROFILEIMPORTTIME")
       || config_get_xoption(config, L"importtime")) {
        config->import_time = 1;
    }
    if (config_get_xoption(config, L"dev" ) ||
        _PyCoreConfig_GetEnv(config, "PYTHONDEVMODE"))
    {
        config->dev_mode = 1;
    }

    _PyInitError err;
    if (config->tracemalloc < 0) {
        err = config_init_tracemalloc(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->pycache_prefix == NULL) {
        err = config_init_pycache_prefix(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    return _Py_INIT_OK();
}


static void
config_init_locale(_PyCoreConfig *config)
{
    /* Test also if coerce_c_locale equals 1: PYTHONCOERCECLOCALE=1 doesn't
       imply that the C locale is always coerced. It is only coerced if
       if the LC_CTYPE locale is "C". */
    if (config->coerce_c_locale != 0) {
        /* The C locale enables the C locale coercion (PEP 538) */
        if (_Py_LegacyLocaleDetected()) {
            config->coerce_c_locale = 1;
        }
        else {
            config->coerce_c_locale = 0;
        }
    }

#ifndef MS_WINDOWS
    if (config->utf8_mode < 0) {
        /* The C locale and the POSIX locale enable the UTF-8 Mode (PEP 540) */
        const char *ctype_loc = setlocale(LC_CTYPE, NULL);
        if (ctype_loc != NULL
           && (strcmp(ctype_loc, "C") == 0
               || strcmp(ctype_loc, "POSIX") == 0))
        {
            config->utf8_mode = 1;
        }
    }
#endif
}


static const char *
get_stdio_errors(const _PyCoreConfig *config)
{
#ifndef MS_WINDOWS
    const char *loc = setlocale(LC_CTYPE, NULL);
    if (loc != NULL) {
        /* surrogateescape is the default in the legacy C and POSIX locales */
        if (strcmp(loc, "C") == 0 || strcmp(loc, "POSIX") == 0) {
            return "surrogateescape";
        }

#ifdef PY_COERCE_C_LOCALE
        /* surrogateescape is the default in locale coercion target locales */
        if (_Py_IsLocaleCoercionTarget(loc)) {
            return "surrogateescape";
        }
#endif
    }

    return "strict";
#else
    /* On Windows, always use surrogateescape by default */
    return "surrogateescape";
#endif
}


static _PyInitError
get_locale_encoding(char **locale_encoding)
{
#ifdef MS_WINDOWS
    char encoding[20];
    PyOS_snprintf(encoding, sizeof(encoding), "cp%d", GetACP());
#elif defined(__ANDROID__)
    const char *encoding = "UTF-8";
#else
    const char *encoding = nl_langinfo(CODESET);
    if (!encoding || encoding[0] == '\0') {
        return _Py_INIT_USER_ERR("failed to get the locale encoding: "
                                 "nl_langinfo(CODESET) failed");
    }
#endif
    *locale_encoding = _PyMem_RawStrdup(encoding);
    if (*locale_encoding == NULL) {
        return _Py_INIT_NO_MEMORY();
    }
    return _Py_INIT_OK();
}


static _PyInitError
config_init_stdio_encoding(_PyCoreConfig *config)
{
    /* If Py_SetStandardStreamEncoding() have been called, use these
        parameters. */
    if (config->stdio_encoding == NULL && _Py_StandardStreamEncoding != NULL) {
        config->stdio_encoding = _PyMem_RawStrdup(_Py_StandardStreamEncoding);
        if (config->stdio_encoding == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    if (config->stdio_errors == NULL && _Py_StandardStreamErrors != NULL) {
        config->stdio_errors = _PyMem_RawStrdup(_Py_StandardStreamErrors);
        if (config->stdio_errors == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    if (config->stdio_encoding != NULL && config->stdio_errors != NULL) {
        return _Py_INIT_OK();
    }

    /* PYTHONIOENCODING environment variable */
    const char *opt = _PyCoreConfig_GetEnv(config, "PYTHONIOENCODING");
    if (opt) {
        char *pythonioencoding = _PyMem_RawStrdup(opt);
        if (pythonioencoding == NULL) {
            return _Py_INIT_NO_MEMORY();
        }

        char *err = strchr(pythonioencoding, ':');
        if (err) {
            *err = '\0';
            err++;
            if (!err[0]) {
                err = NULL;
            }
        }

        /* Does PYTHONIOENCODING contain an encoding? */
        if (pythonioencoding[0]) {
            if (config->stdio_encoding == NULL) {
                config->stdio_encoding = _PyMem_RawStrdup(pythonioencoding);
                if (config->stdio_encoding == NULL) {
                    PyMem_RawFree(pythonioencoding);
                    return _Py_INIT_NO_MEMORY();
                }
            }

            /* If the encoding is set but not the error handler,
               use "strict" error handler by default.
               PYTHONIOENCODING=latin1 behaves as
               PYTHONIOENCODING=latin1:strict. */
            if (!err) {
                err = "strict";
            }
        }

        if (config->stdio_errors == NULL && err != NULL) {
            config->stdio_errors = _PyMem_RawStrdup(err);
            if (config->stdio_errors == NULL) {
                PyMem_RawFree(pythonioencoding);
                return _Py_INIT_NO_MEMORY();
            }
        }

        PyMem_RawFree(pythonioencoding);
    }

    /* UTF-8 Mode uses UTF-8/surrogateescape */
    if (config->utf8_mode) {
        if (config->stdio_encoding == NULL) {
            config->stdio_encoding = _PyMem_RawStrdup("utf-8");
            if (config->stdio_encoding == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
        if (config->stdio_errors == NULL) {
            config->stdio_errors = _PyMem_RawStrdup("surrogateescape");
            if (config->stdio_errors == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
    }

    /* Choose the default error handler based on the current locale. */
    if (config->stdio_encoding == NULL) {
        _PyInitError err = get_locale_encoding(&config->stdio_encoding);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    if (config->stdio_errors == NULL) {
        const char *errors = get_stdio_errors(config);
        config->stdio_errors = _PyMem_RawStrdup(errors);
        if (config->stdio_errors == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    return _Py_INIT_OK();
}


static _PyInitError
config_init_fs_encoding(_PyCoreConfig *config)
{
#ifdef MS_WINDOWS
    if (config->legacy_windows_fs_encoding) {
        /* Legacy Windows filesystem encoding: mbcs/replace */
        if (config->filesystem_encoding == NULL) {
            config->filesystem_encoding = _PyMem_RawStrdup("mbcs");
            if (config->filesystem_encoding == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
        if (config->filesystem_errors == NULL) {
            config->filesystem_errors = _PyMem_RawStrdup("replace");
            if (config->filesystem_errors == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
    }

    /* Windows defaults to utf-8/surrogatepass (PEP 529).

       Note: UTF-8 Mode takes the same code path and the Legacy Windows FS
             encoding has the priortiy over UTF-8 Mode. */
    if (config->filesystem_encoding == NULL) {
        config->filesystem_encoding = _PyMem_RawStrdup("utf-8");
        if (config->filesystem_encoding == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    if (config->filesystem_errors == NULL) {
        config->filesystem_errors = _PyMem_RawStrdup("surrogatepass");
        if (config->filesystem_errors == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }
#else
    if (config->filesystem_encoding == NULL) {
        if (config->utf8_mode) {
            /* UTF-8 Mode use: utf-8/surrogateescape */
            config->filesystem_encoding = _PyMem_RawStrdup("utf-8");
            /* errors defaults to surrogateescape above */
        }
        else if (_Py_GetForceASCII()) {
            config->filesystem_encoding = _PyMem_RawStrdup("ascii");
        }
        else {
            /* macOS and Android use UTF-8,
               other platforms use the locale encoding. */
#if defined(__APPLE__) || defined(__ANDROID__)
            config->filesystem_encoding = _PyMem_RawStrdup("utf-8");
#else
            _PyInitError err = get_locale_encoding(&config->filesystem_encoding);
            if (_Py_INIT_FAILED(err)) {
                return err;
            }
#endif
        }

        if (config->filesystem_encoding == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    if (config->filesystem_errors == NULL) {
        /* by default, use the "surrogateescape" error handler */
        config->filesystem_errors = _PyMem_RawStrdup("surrogateescape");
        if (config->filesystem_errors == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }
#endif
    return _Py_INIT_OK();
}


/* Read configuration settings from standard locations
 *
 * This function doesn't make any changes to the interpreter state - it
 * merely populates any missing configuration settings. This allows an
 * embedding application to completely override a config option by
 * setting it before calling this function, or else modify the default
 * setting before passing the fully populated config to Py_EndInitialization.
 *
 * More advanced selective initialization tricks are possible by calling
 * this function multiple times with various preconfigured settings.
 */

_PyInitError
_PyCoreConfig_Read(_PyCoreConfig *config)
{
    _PyInitError err;

    _PyCoreConfig_GetGlobalConfig(config);
    assert(config->use_environment >= 0);

    if (config->isolated > 0) {
        config->use_environment = 0;
        config->user_site_directory = 0;
    }

#ifdef MS_WINDOWS
    if (config->legacy_windows_fs_encoding) {
        config->utf8_mode = 0;
    }
#endif

    if (config->use_environment) {
        err = config_read_env_vars(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    /* -X options */
    if (config_get_xoption(config, L"showrefcount")) {
        config->show_ref_count = 1;
    }
    if (config_get_xoption(config, L"showalloccount")) {
        config->show_alloc_count = 1;
    }

    err = config_read_complex_options(config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    if (config->utf8_mode < 0) {
        err = config_init_utf8_mode(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->home == NULL) {
        err = config_init_home(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->program_name == NULL) {
        err = config_init_program_name(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->executable == NULL) {
        err = config_init_executable(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->coerce_c_locale != 0 || config->utf8_mode < 0) {
        config_init_locale(config);
    }

    if (config->_install_importlib) {
        err = _PyCoreConfig_InitPathConfig(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    /* default values */
    if (config->dev_mode) {
        if (config->faulthandler < 0) {
            config->faulthandler = 1;
        }
        if (config->allocator == NULL) {
            config->allocator = "debug";
        }
    }
    if (config->use_hash_seed < 0) {
        config->use_hash_seed = 0;
        config->hash_seed = 0;
    }
    if (config->faulthandler < 0) {
        config->faulthandler = 0;
    }
    if (config->tracemalloc < 0) {
        config->tracemalloc = 0;
    }
    if (config->coerce_c_locale < 0) {
        config->coerce_c_locale = 0;
    }
    if (config->utf8_mode < 0) {
        config->utf8_mode = 0;
    }
    if (config->argc < 0) {
        config->argc = 0;
    }

    if (config->filesystem_encoding == NULL || config->filesystem_errors == NULL) {
        err = config_init_fs_encoding(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    err = config_init_stdio_encoding(config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    assert(config->coerce_c_locale >= 0);
    assert(config->use_environment >= 0);
    assert(config->filesystem_encoding != NULL);
    assert(config->filesystem_errors != NULL);
    assert(config->stdio_encoding != NULL);
    assert(config->stdio_errors != NULL);
    assert(config->_check_hash_pycs_mode != NULL);

    return _Py_INIT_OK();
}


PyObject *
_PyCoreConfig_AsDict(const _PyCoreConfig *config)
{
    PyObject *dict, *obj;

    dict = PyDict_New();
    if (dict == NULL) {
        return NULL;
    }

#define SET_ITEM(KEY, EXPR) \
        do { \
            obj = (EXPR); \
            if (obj == NULL) { \
                return NULL; \
            } \
            int res = PyDict_SetItemString(dict, (KEY), obj); \
            Py_DECREF(obj); \
            if (res < 0) { \
                goto fail; \
            } \
        } while (0)
#define FROM_STRING(STR) \
    ((STR != NULL) ? \
        PyUnicode_FromString(STR) \
        : (Py_INCREF(Py_None), Py_None))
#define SET_ITEM_INT(ATTR) \
    SET_ITEM(#ATTR, PyLong_FromLong(config->ATTR))
#define SET_ITEM_UINT(ATTR) \
    SET_ITEM(#ATTR, PyLong_FromUnsignedLong(config->ATTR))
#define SET_ITEM_STR(ATTR) \
    SET_ITEM(#ATTR, FROM_STRING(config->ATTR))
#define FROM_WSTRING(STR) \
    ((STR != NULL) ? \
        PyUnicode_FromWideChar(STR, -1) \
        : (Py_INCREF(Py_None), Py_None))
#define SET_ITEM_WSTR(ATTR) \
    SET_ITEM(#ATTR, FROM_WSTRING(config->ATTR))
#define SET_ITEM_WSTRLIST(NOPTION, OPTIONS) \
    SET_ITEM(#OPTIONS, _Py_wstrlist_as_pylist(config->NOPTION, config->OPTIONS))

    SET_ITEM_INT(install_signal_handlers);
    SET_ITEM_INT(use_environment);
    SET_ITEM_INT(use_hash_seed);
    SET_ITEM_UINT(hash_seed);
    SET_ITEM_STR(allocator);
    SET_ITEM_INT(dev_mode);
    SET_ITEM_INT(faulthandler);
    SET_ITEM_INT(tracemalloc);
    SET_ITEM_INT(import_time);
    SET_ITEM_INT(show_ref_count);
    SET_ITEM_INT(show_alloc_count);
    SET_ITEM_INT(dump_refs);
    SET_ITEM_INT(malloc_stats);
    SET_ITEM_INT(coerce_c_locale);
    SET_ITEM_INT(coerce_c_locale_warn);
    SET_ITEM_STR(filesystem_encoding);
    SET_ITEM_STR(filesystem_errors);
    SET_ITEM_INT(utf8_mode);
    SET_ITEM_WSTR(pycache_prefix);
    SET_ITEM_WSTR(program_name);
    SET_ITEM_WSTRLIST(argc, argv);
    SET_ITEM_WSTR(program);
    SET_ITEM_WSTRLIST(nxoption, xoptions);
    SET_ITEM_WSTRLIST(nwarnoption, warnoptions);
    SET_ITEM_WSTR(module_search_path_env);
    SET_ITEM_WSTR(home);
    SET_ITEM_WSTRLIST(nmodule_search_path, module_search_paths);
    SET_ITEM_WSTR(executable);
    SET_ITEM_WSTR(prefix);
    SET_ITEM_WSTR(base_prefix);
    SET_ITEM_WSTR(exec_prefix);
    SET_ITEM_WSTR(base_exec_prefix);
#ifdef MS_WINDOWS
    SET_ITEM_WSTR(dll_path);
#endif
    SET_ITEM_INT(isolated);
    SET_ITEM_INT(site_import);
    SET_ITEM_INT(bytes_warning);
    SET_ITEM_INT(inspect);
    SET_ITEM_INT(interactive);
    SET_ITEM_INT(optimization_level);
    SET_ITEM_INT(parser_debug);
    SET_ITEM_INT(write_bytecode);
    SET_ITEM_INT(verbose);
    SET_ITEM_INT(quiet);
    SET_ITEM_INT(user_site_directory);
    SET_ITEM_INT(buffered_stdio);
    SET_ITEM_STR(stdio_encoding);
    SET_ITEM_STR(stdio_errors);
#ifdef MS_WINDOWS
    SET_ITEM_INT(legacy_windows_fs_encoding);
    SET_ITEM_INT(legacy_windows_stdio);
#endif
    SET_ITEM_INT(_install_importlib);
    SET_ITEM_STR(_check_hash_pycs_mode);
    SET_ITEM_INT(_frozen);

    return dict;

fail:
    Py_DECREF(dict);
    return NULL;

#undef FROM_STRING
#undef FROM_WSTRING
#undef SET_ITEM
#undef SET_ITEM_INT
#undef SET_ITEM_UINT
#undef SET_ITEM_STR
#undef SET_ITEM_WSTR
#undef SET_ITEM_WSTRLIST
}
