#ifndef Py_PYCORECONFIG_H
#define Py_PYCORECONFIG_H
#ifndef Py_LIMITED_API
#ifdef __cplusplus
extern "C" {
#endif

/* --- _PyInitError ----------------------------------------------- */

typedef struct {
    const char *prefix;
    const char *msg;
    int user_err;
    int exitcode;
} _PyInitError;

/* Almost all errors causing Python initialization to fail */
#ifdef _MSC_VER
   /* Visual Studio 2015 doesn't implement C99 __func__ in C */
#  define _Py_INIT_GET_FUNC() __FUNCTION__
#else
#  define _Py_INIT_GET_FUNC() __func__
#endif

#define _Py_INIT_OK() \
    (_PyInitError){.prefix = NULL, .msg = NULL, .user_err = 0, .exitcode = -1}
#define _Py_INIT_ERR(MSG) \
    (_PyInitError){.prefix = _Py_INIT_GET_FUNC(), .msg = (MSG), .user_err = 0, .exitcode = -1}
/* Error that can be fixed by the user like invalid input parameter.
   Don't abort() the process on such error. */
#define _Py_INIT_USER_ERR(MSG) \
    (_PyInitError){.prefix = _Py_INIT_GET_FUNC(), .msg = (MSG), .user_err = 1, .exitcode = -1}
#define _Py_INIT_NO_MEMORY() _Py_INIT_USER_ERR("memory allocation failed")
#define _Py_INIT_EXIT(EXITCODE) \
    (_PyInitError){.prefix = NULL, .msg = NULL, .user_err = 0, .exitcode = (EXITCODE)}
#define _Py_INIT_FAILED(err) \
    (err.msg != NULL || err.exitcode != -1)

/* --- _PyWstrList ------------------------------------------------ */

typedef struct {
    /* If length is greater than zero, items must be non-NULL
       and all items strings must be non-NULL */
    Py_ssize_t length;
    wchar_t **items;
} _PyWstrList;

#define _PyWstrList_INIT (_PyWstrList){.length = 0, .items = NULL}


/* --- _PyPreConfig ----------------------------------------------- */

typedef struct {
    /* If greater than 0, enable isolated mode: sys.path contains
       neither the script's directory nor the user's site-packages directory.

       Set to 1 by the -I command line option. If set to -1 (default), inherit
       Py_IsolatedFlag value. */
    int isolated;

    /* If greater than 0: use environment variables.
       Set to 0 by -E command line option. If set to -1 (default), it is
       set to !Py_IgnoreEnvironmentFlag. */
    int use_environment;

    /* Coerce the LC_CTYPE locale if it's equal to "C"? (PEP 538)

       Set to 0 by PYTHONCOERCECLOCALE=0. Set to 1 by PYTHONCOERCECLOCALE=1.
       Set to 2 if the user preferred LC_CTYPE locale is "C".

       If it is equal to 1, LC_CTYPE locale is read to decide it it should be
       coerced or not (ex: PYTHONCOERCECLOCALE=1). Internally, it is set to 2
       if the LC_CTYPE locale must be coerced. */
    int coerce_c_locale;

    /* Emit a warning if the LC_CTYPE locale is coerced?

       Disabled by default. Set to 1 by PYTHONCOERCECLOCALE=warn. */
    int coerce_c_locale_warn;

#ifdef MS_WINDOWS
    /* If greater than 1, use the "mbcs" encoding instead of the UTF-8
       encoding for the filesystem encoding.

       Set to 1 if the PYTHONLEGACYWINDOWSFSENCODING environment variable is
       set to a non-empty string. If set to -1 (default), inherit
       Py_LegacyWindowsFSEncodingFlag value.

       See PEP 529 for more details. */
    int legacy_windows_fs_encoding;
#endif

    /* Enable UTF-8 mode? (PEP 540)

       Disabled by default (equals to 0).

       Set to 1 by "-X utf8" and "-X utf8=1" command line options.
       Set to 1 by PYTHONUTF8=1 environment variable.

       Set to 0 by "-X utf8=0" and PYTHONUTF8=0.

       If equals to -1, it is set to 1 if the LC_CTYPE locale is "C" or
       "POSIX", otherwise inherit Py_UTF8Mode value. */
    int utf8_mode;

    int dev_mode;           /* Development mode. PYTHONDEVMODE, -X dev */
    char *allocator;        /* Memory allocator: PYTHONMALLOC */
} _PyPreConfig;

#ifdef MS_WINDOWS
#  define _PyPreConfig_WINDOWS_INIT \
        .legacy_windows_fs_encoding = -1,
#else
#  define _PyPreConfig_WINDOWS_INIT
#endif

#define _PyPreConfig_INIT \
    (_PyPreConfig){ \
        _PyPreConfig_WINDOWS_INIT \
        .isolated = -1, \
        .use_environment = -1, \
        .dev_mode = -1, \
        .allocator = NULL}


/* --- _PyCoreConfig ---------------------------------------------- */

typedef struct {
    int isolated;
    int use_environment;
    int dev_mode;

    /* Install signal handlers? Yes by default. */
    int install_signal_handlers;

    int use_hash_seed;      /* PYTHONHASHSEED=x */
    unsigned long hash_seed;

    /* Enable faulthandler?
       Set to 1 by -X faulthandler and PYTHONFAULTHANDLER. -1 means unset. */
    int faulthandler;

    /* Enable tracemalloc?
       Set by -X tracemalloc=N and PYTHONTRACEMALLOC. -1 means unset */
    int tracemalloc;

    int import_time;        /* PYTHONPROFILEIMPORTTIME, -X importtime */
    int show_ref_count;     /* -X showrefcount */
    int show_alloc_count;   /* -X showalloccount */
    int dump_refs;          /* PYTHONDUMPREFS */
    int malloc_stats;       /* PYTHONMALLOCSTATS */

    /* Python filesystem encoding and error handler:
       sys.getfilesystemencoding() and sys.getfilesystemencodeerrors().

       Default encoding and error handler:

       * if Py_SetStandardStreamEncoding() has been called: they have the
         highest priority;
       * PYTHONIOENCODING environment variable;
       * The UTF-8 Mode uses UTF-8/surrogateescape;
       * If Python forces the usage of the ASCII encoding (ex: C locale
         or POSIX locale on FreeBSD or HP-UX), use ASCII/surrogateescape;
       * locale encoding: ANSI code page on Windows, UTF-8 on Android and
         VxWorks, LC_CTYPE locale encoding on other platforms;
       * On Windows, "surrogateescape" error handler;
       * "surrogateescape" error handler if the LC_CTYPE locale is "C" or "POSIX";
       * "surrogateescape" error handler if the LC_CTYPE locale has been coerced
         (PEP 538);
       * "strict" error handler.

       Supported error handlers: "strict", "surrogateescape" and
       "surrogatepass". The surrogatepass error handler is only supported
       if Py_DecodeLocale() and Py_EncodeLocale() use directly the UTF-8 codec;
       it's only used on Windows.

       initfsencoding() updates the encoding to the Python codec name.
       For example, "ANSI_X3.4-1968" is replaced with "ascii".

       On Windows, sys._enablelegacywindowsfsencoding() sets the
       encoding/errors to mbcs/replace at runtime.


       See Py_FileSystemDefaultEncoding and Py_FileSystemDefaultEncodeErrors.
       */
    char *filesystem_encoding;
    char *filesystem_errors;

    wchar_t *pycache_prefix;  /* PYTHONPYCACHEPREFIX, -X pycache_prefix=PATH */
    wchar_t *program_name;    /* Program name, see also Py_GetProgramName() */
    _PyWstrList argv;         /* Command line arguments */
    wchar_t *program;         /* argv[0] or "" */
    _PyWstrList xoptions;     /* Command line -X options */
    _PyWstrList warnoptions;  /* Warnings options */

    /* Path configuration inputs */
    wchar_t *module_search_path_env; /* PYTHONPATH environment variable */
    wchar_t *home;          /* PYTHONHOME environment variable,
                               see also Py_SetPythonHome(). */

    /* Path configuration outputs */
    int use_module_search_paths;  /* If non-zero, use module_search_paths */
    _PyWstrList module_search_paths;  /* sys.path paths. Computed if
                                       use_module_search_paths is equal
                                       to zero. */

    wchar_t *executable;    /* sys.executable */
    wchar_t *prefix;        /* sys.prefix */
    wchar_t *base_prefix;   /* sys.base_prefix */
    wchar_t *exec_prefix;   /* sys.exec_prefix */
    wchar_t *base_exec_prefix;  /* sys.base_exec_prefix */
#ifdef MS_WINDOWS
    wchar_t *dll_path;      /* Windows DLL path */
#endif

    /* If equal to zero, disable the import of the module site and the
       site-dependent manipulations of sys.path that it entails. Also disable
       these manipulations if site is explicitly imported later (call
       site.main() if you want them to be triggered).

       Set to 0 by the -S command line option. If set to -1 (default), it is
       set to !Py_NoSiteFlag. */
    int site_import;

    /* Bytes warnings:

       * If equal to 1, issue a warning when comparing bytes or bytearray with
         str or bytes with int.
       * If equal or greater to 2, issue an error.

       Incremented by the -b command line option. If set to -1 (default), inherit
       Py_BytesWarningFlag value. */
    int bytes_warning;

    /* If greater than 0, enable inspect: when a script is passed as first
       argument or the -c option is used, enter interactive mode after
       executing the script or the command, even when sys.stdin does not appear
       to be a terminal.

       Incremented by the -i command line option. Set to 1 if the PYTHONINSPECT
       environment variable is non-empty. If set to -1 (default), inherit
       Py_InspectFlag value. */
    int inspect;

    /* If greater than 0: enable the interactive mode (REPL).

       Incremented by the -i command line option. If set to -1 (default),
       inherit Py_InteractiveFlag value. */
    int interactive;

    /* Optimization level.

       Incremented by the -O command line option. Set by the PYTHONOPTIMIZE
       environment variable. If set to -1 (default), inherit Py_OptimizeFlag
       value. */
    int optimization_level;

    /* If greater than 0, enable the debug mode: turn on parser debugging
       output (for expert only, depending on compilation options).

       Incremented by the -d command line option. Set by the PYTHONDEBUG
       environment variable. If set to -1 (default), inherit Py_DebugFlag
       value. */
    int parser_debug;

    /* If equal to 0, Python won't try to write ``.pyc`` files on the
       import of source modules.

       Set to 0 by the -B command line option and the PYTHONDONTWRITEBYTECODE
       environment variable. If set to -1 (default), it is set to
       !Py_DontWriteBytecodeFlag. */
    int write_bytecode;

    /* If greater than 0, enable the verbose mode: print a message each time a
       module is initialized, showing the place (filename or built-in module)
       from which it is loaded.

       If greater or equal to 2, print a message for each file that is checked
       for when searching for a module. Also provides information on module
       cleanup at exit.

       Incremented by the -v option. Set by the PYTHONVERBOSE environment
       variable. If set to -1 (default), inherit Py_VerboseFlag value. */
    int verbose;

    /* If greater than 0, enable the quiet mode: Don't display the copyright
       and version messages even in interactive mode.

       Incremented by the -q option. If set to -1 (default), inherit
       Py_QuietFlag value. */
    int quiet;

   /* If greater than 0, don't add the user site-packages directory to
      sys.path.

      Set to 0 by the -s and -I command line options , and the PYTHONNOUSERSITE
      environment variable. If set to -1 (default), it is set to
      !Py_NoUserSiteDirectory. */
    int user_site_directory;

    /* If equal to 0, enable unbuffered mode: force the stdout and stderr
       streams to be unbuffered.

       Set to 0 by the -u option. Set by the PYTHONUNBUFFERED environment
       variable.
       If set to -1 (default), it is set to !Py_UnbufferedStdioFlag. */
    int buffered_stdio;

    /* Encoding of sys.stdin, sys.stdout and sys.stderr.
       Value set from PYTHONIOENCODING environment variable and
       Py_SetStandardStreamEncoding() function.
       See also 'stdio_errors' attribute. */
    char *stdio_encoding;

    /* Error handler of sys.stdin and sys.stdout.
       Value set from PYTHONIOENCODING environment variable and
       Py_SetStandardStreamEncoding() function.
       See also 'stdio_encoding' attribute. */
    char *stdio_errors;

#ifdef MS_WINDOWS
    /* If greater than zero, use io.FileIO instead of WindowsConsoleIO for sys
       standard streams.

       Set to 1 if the PYTHONLEGACYWINDOWSSTDIO environment variable is set to
       a non-empty string. If set to -1 (default), inherit
       Py_LegacyWindowsStdioFlag value.

       See PEP 528 for more details. */
    int legacy_windows_stdio;
#endif

    /* --- Parameter only used by Py_Main() ---------- */

    /* Skip the first line of the source ('run_filename' parameter), allowing use of non-Unix forms of
       "#!cmd".  This is intended for a DOS specific hack only.

       Set by the -x command line option. */
    int skip_source_first_line;

    wchar_t *run_command;   /* -c command line argument */
    wchar_t *run_module;    /* -m command line argument */
    wchar_t *run_filename;  /* Trailing command line argument without -c or -m */

    /* --- Private fields ---------------------------- */

    /* Install importlib? If set to 0, importlib is not initialized at all.
       Needed by freeze_importlib. */
    int _install_importlib;

    /* Value of the --check-hash-based-pycs configure option. Valid values:

       - "default" means the 'check_source' flag in hash-based pycs
         determines invalidation
       - "always" causes the interpreter to hash the source file for
         invalidation regardless of value of 'check_source' bit
       - "never" causes the interpreter to always assume hash-based pycs are
         valid

       Set by the --check-hash-based-pycs command line option.
       The default value is "default".

       See PEP 552 "Deterministic pycs" for more details. */
    const char *_check_hash_pycs_mode;

    /* If greater than 0, suppress _PyPathConfig_Calculate() warnings.

       If set to -1 (default), inherit Py_FrozenFlag value. */
    int _frozen;

    /* If non-zero, use "main" Python initialization */
    int _init_main;

} _PyCoreConfig;

#ifdef MS_WINDOWS
#  define _PyCoreConfig_WINDOWS_INIT \
        .legacy_windows_stdio = -1,
#else
#  define _PyCoreConfig_WINDOWS_INIT
#endif

#define _PyCoreConfig_INIT \
    (_PyCoreConfig){ \
        _PyCoreConfig_WINDOWS_INIT \
        .isolated = -1, \
        .use_environment = -1, \
        .dev_mode = -1, \
        .install_signal_handlers = 1, \
        .use_hash_seed = -1, \
        .faulthandler = -1, \
        .tracemalloc = -1, \
        .use_module_search_paths = 0, \
        .site_import = -1, \
        .bytes_warning = -1, \
        .inspect = -1, \
        .interactive = -1, \
        .optimization_level = -1, \
        .parser_debug= -1, \
        .write_bytecode = -1, \
        .verbose = -1, \
        .quiet = -1, \
        .user_site_directory = -1, \
        .buffered_stdio = -1, \
        ._install_importlib = 1, \
        ._check_hash_pycs_mode = "default", \
        ._frozen = -1, \
        ._init_main = 1}
/* Note: _PyCoreConfig_INIT sets other fields to 0/NULL */


/* --- Function used for testing ---------------------------------- */

PyAPI_FUNC(PyObject*) _Py_GetConfigsAsDict(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_LIMITED_API */
#endif /* !Py_PYCORECONFIG_H */
