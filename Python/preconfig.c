#include <Python.h>
#include "pycore_coreconfig.h"
#include "pycore_fileutils.h"
#include "pycore_getopt.h"
#include "pycore_pylifecycle.h"
#include "pycore_pystate.h"
#include <locale.h>
#ifdef HAVE_LANGINFO_H
#  include <langinfo.h>
#endif

#define DECODE_LOCALE_ERR(NAME, LEN) \
    (((LEN) == -2) \
     ? _Py_INIT_USER_ERR("cannot decode " NAME) \
     : _Py_INIT_NO_MEMORY())


/* The filesystem encoding is chosen by config_init_fs_encoding(),
   see also initfsencoding(). */
const char *Py_FileSystemDefaultEncoding = NULL;
int Py_HasFileSystemDefaultEncoding = 0;
const char *Py_FileSystemDefaultEncodeErrors = NULL;
int _Py_HasFileSystemDefaultEncodeErrors = 0;


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


const wchar_t*
_Py_GetProgramFromArgv(int argc, wchar_t * const *argv)
{
    if (argc >= 1 && argv != NULL) {
        return argv[0];
    }
    else {
        return L"";
    }
}


const wchar_t*
_Py_GetXOption(int nxoption, wchar_t **xoptions, wchar_t *name)
{
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


_PyInitError
_PyArgv_Decode(const _PyArgv *args, int *argc_p, wchar_t*** argv_p)
{
    wchar_t** argv;
    if (args->use_bytes_argv) {
        /* +1 for a the NULL terminator */
        size_t size = sizeof(wchar_t*) * (args->argc + 1);
        argv = (wchar_t **)PyMem_RawMalloc(size);
        if (argv == NULL) {
            return _Py_INIT_NO_MEMORY();
        }

        for (int i = 0; i < args->argc; i++) {
            size_t len;
            wchar_t *arg = Py_DecodeLocale(args->bytes_argv[i], &len);
            if (arg == NULL) {
                _Py_wstrlist_clear(i, argv);
                return DECODE_LOCALE_ERR("command line arguments",
                                                (Py_ssize_t)len);
            }
            argv[i] = arg;
        }
        argv[args->argc] = NULL;
    }
    else {
        argv = _Py_wstrlist_copy(args->argc, args->wchar_argv);
        if (argv == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    *argc_p = args->argc;
    *argv_p = argv;
    return _Py_INIT_OK();
}


#if !defined(__ANDROID__)
#  define _PyPreCmdline_USE_LOCALE
#endif

typedef struct {
    int argc;
    wchar_t **argv;
    int nxoption;           /* Number of -X options */
    wchar_t **xoptions;     /* -X options */
#ifdef _PyPreCmdline_USE_LOCALE
    char *ctype_locale;
#endif
} _PyPreCmdline;


static void
precmdline_clear(_PyPreCmdline *cmdline)
{
    _Py_wstrlist_clear(cmdline->argc, cmdline->argv);
    cmdline->argc = 0;
    cmdline->argv = NULL;

    _Py_wstrlist_clear(cmdline->nxoption, cmdline->xoptions);
    cmdline->nxoption = 0;
    cmdline->xoptions = NULL;

#ifdef _PyPreCmdline_USE_LOCALE
    PyMem_RawFree(cmdline->ctype_locale);
    cmdline->ctype_locale = NULL;
#endif
}


static _PyInitError
precmdline_init(const _PyArgv *args, _PyPreCmdline *cmdline)
{
    _PyInitError err = _PyArgv_Decode(args, &cmdline->argc, &cmdline->argv);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

#ifdef _PyPreCmdline_USE_LOCALE
    const char *loc = setlocale(LC_CTYPE, NULL);
    if (loc != NULL) {
        cmdline->ctype_locale = _PyMem_RawStrdup(loc);
        if (cmdline->ctype_locale == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }
#endif

    return _Py_INIT_OK();
}


void
_PyPreConfig_Clear(_PyPreConfig *config)
{
#define CLEAR(ATTR) \
    do { \
        PyMem_RawFree(ATTR); \
        ATTR = NULL; \
    } while (0)

    CLEAR(config->allocator);
    CLEAR(config->filesystem_encoding);
    CLEAR(config->filesystem_errors);
    CLEAR(config->stdio_encoding);
    CLEAR(config->stdio_errors);
#undef CLEAR
}


int
_PyPreConfig_Copy(_PyPreConfig *config, const _PyPreConfig *config2)
{
    _PyPreConfig_Clear(config);

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

    COPY_ATTR(use_environment);
    COPY_ATTR(isolated);
    COPY_ATTR(dev_mode);
    COPY_STR_ATTR(allocator);
    COPY_ATTR(utf8_mode);
    COPY_ATTR(coerce_c_locale);
    COPY_ATTR(coerce_c_locale_warn);
    COPY_STR_ATTR(filesystem_encoding);
    COPY_STR_ATTR(filesystem_errors);
    COPY_STR_ATTR(stdio_encoding);
    COPY_STR_ATTR(stdio_errors);
#ifdef MS_WINDOWS
    COPY_ATTR(legacy_windows_fs_encoding);
    COPY_ATTR(legacy_windows_stdio);
#endif
    return 0;

#undef COPY_ATTR
#undef COPY_STR_ATTR
}


void
_PyPreConfig_GetGlobalConfig(_PyPreConfig *config)
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
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_fs_encoding, Py_LegacyWindowsFSEncodingFlag);
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


void
_PyPreConfig_SetGlobalConfig(const _PyPreConfig *config)
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
#ifdef MS_WINDOWS
    COPY_FLAG(legacy_windows_fs_encoding, Py_LegacyWindowsFSEncodingFlag);
    COPY_FLAG(legacy_windows_stdio, Py_LegacyWindowsStdioFlag);
#endif
    COPY_NOT_FLAG(use_environment, Py_IgnoreEnvironmentFlag);

#undef COPY_FLAG
#undef COPY_NOT_FLAG
}


const char*
_PyPreConfig_GetEnv(const _PyPreConfig *config, const char *name)
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


static void
preconfig_init_locale(_PyPreConfig *config, const char *ctype_locale)
{
    /* Test also if coerce_c_locale equals 1: PYTHONCOERCECLOCALE=1 doesn't
       imply that the C locale is always coerced. It is only coerced if
       if the LC_CTYPE locale is "C". */
    if (config->coerce_c_locale != 0) {
        /* The C locale enables the C locale coercion (PEP 538) */
        if (_Py_LegacyLocaleDetected(ctype_locale)) {
            config->coerce_c_locale = 1;
        }
        else {
            config->coerce_c_locale = 0;
        }
    }

#ifndef MS_WINDOWS
    if (config->utf8_mode < 0) {
        /* The C locale and the POSIX locale enable the UTF-8 Mode (PEP 540) */
        if (ctype_locale != NULL
           && (strcmp(ctype_locale, "C") == 0
               || strcmp(ctype_locale, "POSIX") == 0))
        {
            config->utf8_mode = 1;
        }
    }
#endif
}


static _PyInitError
preconfig_init_utf8_mode(_PyPreConfig *config, const _PyPreCmdline *cmdline)
{
    const wchar_t *xopt;
    if (cmdline) {
        xopt = _Py_GetXOption(cmdline->nxoption, cmdline->xoptions, L"utf8");
    }
    else {
        xopt = NULL;
    }
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

    const char *opt = _PyPreConfig_GetEnv(config, "PYTHONUTF8");
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
preconfig_init_fs_encoding(_PyPreConfig *config)
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


static const char *
get_stdio_errors(void)
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
preconfig_init_stdio_encoding(_PyPreConfig *config)
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
    const char *opt = _PyPreConfig_GetEnv(config, "PYTHONIOENCODING");
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
        const char *errors = get_stdio_errors();
        config->stdio_errors = _PyMem_RawStrdup(errors);
        if (config->stdio_errors == NULL) {
            return _Py_INIT_NO_MEMORY();
        }
    }

    return _Py_INIT_OK();
}


void
_PreConfig_GetEnvFlag(_PyPreConfig *config, int *flag, const char *name)
{
    const char *var = _PyPreConfig_GetEnv(config, name);
    if (!var) {
        return;
    }
    int value;
    if (_Py_str_to_int(var, &value) < 0 || value < 0) {
        /* PYTHONDEBUG=text and PYTHONDEBUG=-2 behave as PYTHONDEBUG=1 */
        value = 1;
    }
    if (*flag < value) {
        *flag = value;
    }
}


static _PyInitError
preconfig_read_env_vars(_PyPreConfig *config)
{
    if (config->allocator == NULL) {
        const char *allocator = _PyPreConfig_GetEnv(config, "PYTHONMALLOC");
        if (allocator) {
            config->allocator = _PyMem_RawStrdup(allocator);
            if (config->allocator == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
    }

#ifdef MS_WINDOWS
    _PreConfig_GetEnvFlag(config, &config->legacy_windows_fs_encoding,
                 "PYTHONLEGACYWINDOWSFSENCODING");
    _PreConfig_GetEnvFlag(config, &config->legacy_windows_stdio,
                 "PYTHONLEGACYWINDOWSSTDIO");
#endif

    const char *env = _PyPreConfig_GetEnv(config, "PYTHONCOERCECLOCALE");
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

    return _Py_INIT_OK();
}


static _PyInitError
preconfig_read_from_cmdline(_PyPreConfig *config, const _PyPreCmdline *cmdline)
{
    assert(config->use_environment >= 0);

    _PyPreConfig_GetGlobalConfig(config);

    _PyInitError err;

    if (config->isolated > 0) {
        config->use_environment = 0;
    }

#ifdef MS_WINDOWS
    if (config->legacy_windows_fs_encoding) {
        config->utf8_mode = 0;
    }
#endif

    if (config->use_environment) {
        err = preconfig_read_env_vars(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if ((cmdline != NULL && _Py_GetXOption(cmdline->nxoption, cmdline->xoptions, L"dev")) ||
        _PyPreConfig_GetEnv(config, "PYTHONDEVMODE"))
    {
        config->dev_mode = 1;
    }

    if (config->utf8_mode < 0) {
        err = preconfig_init_utf8_mode(config, cmdline);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (config->coerce_c_locale != 0 || config->utf8_mode < 0) {
        const char *ctype_locale;
        if (cmdline) {
            ctype_locale = cmdline->ctype_locale;
        }
        else {
            ctype_locale = setlocale(LC_CTYPE, NULL);
        }
        preconfig_init_locale(config, ctype_locale);
    }

    if (config->coerce_c_locale < 0) {
        config->coerce_c_locale = 0;
    }
    if (config->utf8_mode < 0) {
        config->utf8_mode = 0;
    }

    if (config->filesystem_encoding == NULL || config->filesystem_errors == NULL) {
        err = preconfig_init_fs_encoding(config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    err = preconfig_init_stdio_encoding(config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* default values */
    if (config->dev_mode) {
        if (config->allocator == NULL) {
            config->allocator = _PyMem_RawStrdup("debug");
            if (config->allocator == NULL) {
                return _Py_INIT_NO_MEMORY();
            }
        }
    }

    assert(config->coerce_c_locale >= 0);
    assert(config->use_environment >= 0);
    assert(config->filesystem_encoding != NULL);
    assert(config->filesystem_errors != NULL);
    assert(config->stdio_encoding != NULL);
    assert(config->stdio_errors != NULL);

    return _Py_INIT_OK();
}


_PyInitError
_PyPreConfig_Read(_PyPreConfig *config)
{
    return preconfig_read_from_cmdline(config, NULL);
}


/* Parse _PyPreConfig command line options like -E */
static _PyInitError
preconfig_parse_cmdline(_PyPreConfig *config, _PyPreCmdline *cmdline)
{
    _PyOS_ResetGetOpt();
    _PyOS_opterr = 0;
    do {
        int longindex = -1;
        int c = _PyOS_GetOpt(cmdline->argc, cmdline->argv, &longindex);
        if (c == EOF || c == 'c' || c == 'm') {
            break;
        }

        switch (c) {
        case 'I':
            config->isolated++;
            break;

        case 'E':
            config->use_environment = 0;
            break;

        case 'X':
        {
            _PyInitError err;
            err = _Py_wstrlist_append(&cmdline->nxoption,
                                      &cmdline->xoptions,
                                      _PyOS_optarg);
            if (_Py_INIT_FAILED(err)) {
                return err;
            }
            break;
        }

        default:
            /* other arguments will be parsed by
               cmdline_parse_impl() */
            break;
        }
    } while (1);
    return _Py_INIT_OK();
}


static _PyInitError
_PyPreConfig_ReadFromArgv(_PyPreConfig *config, const _PyArgv *args)
{
    _PyInitError err;

    _PyPreCmdline cmdline;
    memset(&cmdline, 0, sizeof(cmdline));

    err = precmdline_init(args, &cmdline);
    if (_Py_INIT_FAILED(err)) {
        goto done;
    }

    err = preconfig_parse_cmdline(config, &cmdline);
    if (_Py_INIT_FAILED(err)) {
        goto done;
    }

    err = preconfig_read_from_cmdline(config, &cmdline);

done:
    precmdline_clear(&cmdline);
    return err;
}


/* Read the preconfiguration and initialize the LC_CTYPE locale:
   coerce the C locale (PEP 538). */
_PyInitError
_PyPreConfig_InitFromArgv(_PyPreConfig *config, const _PyArgv *args)
{
    _PyInitError err;

    err = _PyRuntime_Initialize();
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    int init_utf8_mode = Py_UTF8Mode;
#ifdef MS_WINDOWS
    int init_legacy_encoding = Py_LegacyWindowsFSEncodingFlag;
#endif
    _PyPreConfig save_config = _PyPreConfig_INIT;
    int locale_coerced = 0;
    int loops = 0;

    if (_PyPreConfig_Copy(&save_config, config) < 0) {
        err = _Py_INIT_NO_MEMORY();
        goto done;
    }

    /* Set LC_CTYPE to the user preferred locale */
    _Py_SetLocaleFromEnv(LC_CTYPE);

    while (1) {
        int utf8_mode = config->utf8_mode;

        /* Watchdog to prevent an infinite loop */
        loops++;
        if (loops == 3) {
            err = _Py_INIT_ERR("Encoding changed twice while "
                               "reading the configuration");
            goto done;
        }

        /* bpo-34207: Py_DecodeLocale() and Py_EncodeLocale() depend
           on Py_UTF8Mode and Py_LegacyWindowsFSEncodingFlag. */
        Py_UTF8Mode = config->utf8_mode;
#ifdef MS_WINDOWS
        Py_LegacyWindowsFSEncodingFlag = config->legacy_windows_fs_encoding;
#endif

        err = _PyPreConfig_ReadFromArgv(config, args);
        if (_Py_INIT_FAILED(err)) {
            goto done;
        }

        /* The legacy C locale assumes ASCII as the default text encoding, which
         * causes problems not only for the CPython runtime, but also other
         * components like GNU readline.
         *
         * Accordingly, when the CLI detects it, it attempts to coerce it to a
         * more capable UTF-8 based alternative.
         *
         * See the documentation of the PYTHONCOERCECLOCALE setting for more
         * details.
         */
        int encoding_changed = 0;
        if (config->coerce_c_locale && !locale_coerced) {
            locale_coerced = 1;
            _Py_CoerceLegacyLocale(config->coerce_c_locale_warn);
            encoding_changed = 1;
        }

        if (utf8_mode == -1) {
            if (config->utf8_mode == 1) {
                /* UTF-8 Mode enabled */
                encoding_changed = 1;
            }
        }
        else {
            if (config->utf8_mode != utf8_mode) {
                encoding_changed = 1;
            }
        }

        if (!encoding_changed) {
            break;
        }

        /* Reset the configuration before reading again the configuration,
           just keep UTF-8 Mode value. */
        int new_utf8_mode = config->utf8_mode;
        int new_coerce_c_locale = config->coerce_c_locale;
        if (_PyPreConfig_Copy(config, &save_config) < 0) {
            err = _Py_INIT_NO_MEMORY();
            goto done;
        }
        config->utf8_mode = new_utf8_mode;
        config->coerce_c_locale = new_coerce_c_locale;

        /* The encoding changed: read again the configuration
           with the new encoding */
    }
    err = _Py_INIT_OK();

done:
    _PyPreConfig_Clear(&save_config);
    Py_UTF8Mode = init_utf8_mode ;
#ifdef MS_WINDOWS
    Py_LegacyWindowsFSEncodingFlag = init_legacy_encoding;
#endif
    return err;
}


_PyInitError
_PyPreConfig_SetAllocator(_PyPreConfig *config)
{
    assert(!_PyRuntime.core_initialized);

    PyMemAllocatorEx old_alloc;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_PyMem_SetupAllocators(config->allocator) < 0) {
        return _Py_INIT_USER_ERR("Unknown PYTHONMALLOC allocator");
    }

    /* free config with the old allocator and re-allocate the config
       with the new allocator */
    PyMemAllocatorEx new_alloc;
    PyMem_GetAllocator(PYMEM_DOMAIN_RAW, &new_alloc);

    _PyInitError err;
    _PyPreConfig config2 = _PyPreConfig_INIT;
    if (_PyPreConfig_Copy(&config2, config) < 0) {
        err = _Py_INIT_NO_MEMORY();
        goto done;
    }

    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    _PyPreConfig_Clear(config);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &new_alloc);

    if (_PyPreConfig_Copy(config, &config2) < 0) {
        err = _Py_INIT_NO_MEMORY();
        goto done;
    }
    err = _Py_INIT_OK();

done:
    _PyPreConfig_Clear(&config2);
    return err;
}
