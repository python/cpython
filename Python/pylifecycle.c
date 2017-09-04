/* Python interpreter top-level routines, including init/exit */

#include "Python.h"

#include "Python-ast.h"
#undef Yield /* undefine macro conflicting with winbase.h */
#include "grammar.h"
#include "node.h"
#include "token.h"
#include "parsetok.h"
#include "errcode.h"
#include "code.h"
#include "symtable.h"
#include "ast.h"
#include "marshal.h"
#include "osdefs.h"
#include <locale.h>

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef MS_WINDOWS
#include "malloc.h" /* for alloca */
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef MS_WINDOWS
#undef BYTE
#include "windows.h"

extern PyTypeObject PyWindowsConsoleIO_Type;
#define PyWindowsConsoleIO_Check(op) (PyObject_TypeCheck((op), &PyWindowsConsoleIO_Type))
#endif

_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(name);
_Py_IDENTIFIER(stdin);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);

#ifdef __cplusplus
extern "C" {
#endif

extern wchar_t *Py_GetPath(void);

extern grammar _PyParser_Grammar; /* From graminit.c */

/* Forward */
static void initmain(PyInterpreterState *interp);
static int initfsencoding(PyInterpreterState *interp);
static void initsite(void);
static int initstdio(void);
static void initsigs(void);
static void call_py_exitfuncs(void);
static void wait_for_thread_shutdown(void);
static void call_ll_exitfuncs(void);
extern int _PyUnicode_Init(void);
extern int _PyStructSequence_Init(void);
extern void _PyUnicode_Fini(void);
extern int _PyLong_Init(void);
extern void PyLong_Fini(void);
extern int _PyFaulthandler_Init(void);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern int _PyTraceMalloc_Init(void);
extern int _PyTraceMalloc_Fini(void);
extern void _Py_ReadyTypes(void);

#ifdef WITH_THREAD
extern void _PyGILState_Init(PyInterpreterState *, PyThreadState *);
extern void _PyGILState_Fini(void);
#endif /* WITH_THREAD */

/* Global configuration variable declarations are in pydebug.h */
/* XXX (ncoghlan): move those declarations to pylifecycle.h? */
int Py_DebugFlag; /* Needed by parser.c */
int Py_VerboseFlag; /* Needed by import.c */
int Py_QuietFlag; /* Needed by sysmodule.c */
int Py_InteractiveFlag; /* Needed by Py_FdIsInteractive() below */
int Py_InspectFlag; /* Needed to determine whether to exit at SystemExit */
int Py_OptimizeFlag = 0; /* Needed by compile.c */
int Py_NoSiteFlag; /* Suppress 'import site' */
int Py_BytesWarningFlag; /* Warn on str(bytes) and str(buffer) */
int Py_UseClassExceptionsFlag = 1; /* Needed by bltinmodule.c: deprecated */
int Py_FrozenFlag; /* Needed by getpath.c */
int Py_IgnoreEnvironmentFlag; /* e.g. PYTHONPATH, PYTHONHOME */
int Py_DontWriteBytecodeFlag; /* Suppress writing bytecode files (*.pyc) */
int Py_NoUserSiteDirectory = 0; /* for -s and site.py */
int Py_UnbufferedStdioFlag = 0; /* Unbuffered binary std{in,out,err} */
int Py_HashRandomizationFlag = 0; /* for -R and PYTHONHASHSEED */
int Py_IsolatedFlag = 0; /* for -I, isolate from user's env */
#ifdef MS_WINDOWS
int Py_LegacyWindowsFSEncodingFlag = 0; /* Uses mbcs instead of utf-8 */
int Py_LegacyWindowsStdioFlag = 0; /* Uses FileIO instead of WindowsConsoleIO */
#endif

PyThreadState *_Py_Finalizing = NULL;

/* Hack to force loading of object files */
int (*_PyOS_mystrnicmp_hack)(const char *, const char *, Py_ssize_t) = \
    PyOS_mystrnicmp; /* Python/pystrcmp.o */

/* PyModule_GetWarningsModule is no longer necessary as of 2.6
since _warnings is builtin.  This API should not be used. */
PyObject *
PyModule_GetWarningsModule(void)
{
    return PyImport_ImportModule("warnings");
}


/* APIs to access the initialization flags
 *
 * Can be called prior to Py_Initialize.
 */
int _Py_CoreInitialized = 0;
int _Py_Initialized = 0;

int
_Py_IsCoreInitialized(void)
{
    return _Py_CoreInitialized;
}

int
Py_IsInitialized(void)
{
    return _Py_Initialized;
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
            return -2;
        }
    }
    if (errors) {
        _Py_StandardStreamErrors = _PyMem_RawStrdup(errors);
        if (!_Py_StandardStreamErrors) {
            if (_Py_StandardStreamEncoding) {
                PyMem_RawFree(_Py_StandardStreamEncoding);
            }
            return -3;
        }
    }
#ifdef MS_WINDOWS
    if (_Py_StandardStreamEncoding) {
        /* Overriding the stream encoding implies legacy streams */
        Py_LegacyWindowsStdioFlag = 1;
    }
#endif
    return 0;
}


/* Global initializations.  Can be undone by Py_FinalizeEx().  Don't
   call this twice without an intervening Py_FinalizeEx() call.  When
   initializations fail, a fatal error is issued and the function does
   not return.  On return, the first thread and interpreter state have
   been created.

   Locking: you must hold the interpreter lock while calling this.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/

static int
add_flag(int flag, const char *envs)
{
    int env = atoi(envs);
    if (flag < env)
        flag = env;
    if (flag < 1)
        flag = 1;
    return flag;
}

static char*
get_codec_name(const char *encoding)
{
    const char *name_utf8;
    char *name_str;
    PyObject *codec, *name = NULL;

    codec = _PyCodec_Lookup(encoding);
    if (!codec)
        goto error;

    name = _PyObject_GetAttrId(codec, &PyId_name);
    Py_CLEAR(codec);
    if (!name)
        goto error;

    name_utf8 = PyUnicode_AsUTF8(name);
    if (name_utf8 == NULL)
        goto error;
    name_str = _PyMem_RawStrdup(name_utf8);
    Py_DECREF(name);
    if (name_str == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    return name_str;

error:
    Py_XDECREF(codec);
    Py_XDECREF(name);
    return NULL;
}

static char*
get_locale_encoding(void)
{
#ifdef MS_WINDOWS
    char codepage[100];
    PyOS_snprintf(codepage, sizeof(codepage), "cp%d", GetACP());
    return get_codec_name(codepage);
#elif defined(HAVE_LANGINFO_H) && defined(CODESET)
    char* codeset = nl_langinfo(CODESET);
    if (!codeset || codeset[0] == '\0') {
        PyErr_SetString(PyExc_ValueError, "CODESET is not set or empty");
        return NULL;
    }
    return get_codec_name(codeset);
#elif defined(__ANDROID__)
    return get_codec_name("UTF-8");
#else
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
#endif
}

static void
initimport(PyInterpreterState *interp, PyObject *sysmod)
{
    PyObject *importlib;
    PyObject *impmod;
    PyObject *sys_modules;
    PyObject *value;

    /* Import _importlib through its frozen version, _frozen_importlib. */
    if (PyImport_ImportFrozenModule("_frozen_importlib") <= 0) {
        Py_FatalError("Py_Initialize: can't import _frozen_importlib");
    }
    else if (Py_VerboseFlag) {
        PySys_FormatStderr("import _frozen_importlib # frozen\n");
    }
    importlib = PyImport_AddModule("_frozen_importlib");
    if (importlib == NULL) {
        Py_FatalError("Py_Initialize: couldn't get _frozen_importlib from "
                      "sys.modules");
    }
    interp->importlib = importlib;
    Py_INCREF(interp->importlib);

    interp->import_func = PyDict_GetItemString(interp->builtins, "__import__");
    if (interp->import_func == NULL)
        Py_FatalError("Py_Initialize: __import__ not found");
    Py_INCREF(interp->import_func);

    /* Import the _imp module */
    impmod = PyInit_imp();
    if (impmod == NULL) {
        Py_FatalError("Py_Initialize: can't import _imp");
    }
    else if (Py_VerboseFlag) {
        PySys_FormatStderr("import _imp # builtin\n");
    }
    sys_modules = PyImport_GetModuleDict();
    if (Py_VerboseFlag) {
        PySys_FormatStderr("import sys # builtin\n");
    }
    if (PyDict_SetItemString(sys_modules, "_imp", impmod) < 0) {
        Py_FatalError("Py_Initialize: can't save _imp to sys.modules");
    }

    /* Install importlib as the implementation of import */
    value = PyObject_CallMethod(importlib, "_install", "OO", sysmod, impmod);
    if (value != NULL) {
        Py_DECREF(value);
        value = PyObject_CallMethod(importlib,
                                    "_install_external_importers", "");
    }
    if (value == NULL) {
        PyErr_Print();
        Py_FatalError("Py_Initialize: importlib install failed");
    }
    Py_DECREF(value);
    Py_DECREF(impmod);

    _PyImportZip_Init();
}

static void
initexternalimport(PyInterpreterState *interp)
{
    PyObject *value;
    value = PyObject_CallMethod(interp->importlib,
                                "_install_external_importers", "");
    if (value == NULL) {
        PyErr_Print();
        Py_FatalError("Py_EndInitialization: external importer setup failed");
    }
    Py_DECREF(value);
}

/* Helper functions to better handle the legacy C locale
 *
 * The legacy C locale assumes ASCII as the default text encoding, which
 * causes problems not only for the CPython runtime, but also other
 * components like GNU readline.
 *
 * Accordingly, when the CLI detects it, it attempts to coerce it to a
 * more capable UTF-8 based alternative as follows:
 *
 *     if (_Py_LegacyLocaleDetected()) {
 *         _Py_CoerceLegacyLocale();
 *     }
 *
 * See the documentation of the PYTHONCOERCECLOCALE setting for more details.
 *
 * Locale coercion also impacts the default error handler for the standard
 * streams: while the usual default is "strict", the default for the legacy
 * C locale and for any of the coercion target locales is "surrogateescape".
 */

int
_Py_LegacyLocaleDetected(void)
{
#ifndef MS_WINDOWS
    /* On non-Windows systems, the C locale is considered a legacy locale */
    /* XXX (ncoghlan): some platforms (notably Mac OS X) don't appear to treat
     *                 the POSIX locale as a simple alias for the C locale, so
     *                 we may also want to check for that explicitly.
     */
    const char *ctype_loc = setlocale(LC_CTYPE, NULL);
    return ctype_loc != NULL && strcmp(ctype_loc, "C") == 0;
#else
    /* Windows uses code pages instead of locales, so no locale is legacy */
    return 0;
#endif
}

static const char *_C_LOCALE_WARNING =
    "Python runtime initialized with LC_CTYPE=C (a locale with default ASCII "
    "encoding), which may cause Unicode compatibility problems. Using C.UTF-8, "
    "C.utf8, or UTF-8 (if available) as alternative Unicode-compatible "
    "locales is recommended.\n";

static int
_legacy_locale_warnings_enabled(void)
{
    const char *coerce_c_locale = getenv("PYTHONCOERCECLOCALE");
    return (coerce_c_locale != NULL &&
            strncmp(coerce_c_locale, "warn", 5) == 0);
}

static void
_emit_stderr_warning_for_legacy_locale(void)
{
    if (_legacy_locale_warnings_enabled()) {
        if (_Py_LegacyLocaleDetected()) {
            fprintf(stderr, "%s", _C_LOCALE_WARNING);
        }
    }
}

typedef struct _CandidateLocale {
    const char *locale_name; /* The locale to try as a coercion target */
} _LocaleCoercionTarget;

static _LocaleCoercionTarget _TARGET_LOCALES[] = {
    {"C.UTF-8"},
    {"C.utf8"},
    {"UTF-8"},
    {NULL}
};

static char *
get_default_standard_stream_error_handler(void)
{
    const char *ctype_loc = setlocale(LC_CTYPE, NULL);
    if (ctype_loc != NULL) {
        /* "surrogateescape" is the default in the legacy C locale */
        if (strcmp(ctype_loc, "C") == 0) {
            return "surrogateescape";
        }

#ifdef PY_COERCE_C_LOCALE
        /* "surrogateescape" is the default in locale coercion target locales */
        const _LocaleCoercionTarget *target = NULL;
        for (target = _TARGET_LOCALES; target->locale_name; target++) {
            if (strcmp(ctype_loc, target->locale_name) == 0) {
                return "surrogateescape";
            }
        }
#endif
   }

   /* Otherwise return NULL to request the typical default error handler */
   return NULL;
}

#ifdef PY_COERCE_C_LOCALE
static const char *_C_LOCALE_COERCION_WARNING =
    "Python detected LC_CTYPE=C: LC_CTYPE coerced to %.20s (set another locale "
    "or PYTHONCOERCECLOCALE=0 to disable this locale coercion behavior).\n";

static void
_coerce_default_locale_settings(const _LocaleCoercionTarget *target)
{
    const char *newloc = target->locale_name;

    /* Reset locale back to currently configured defaults */
    setlocale(LC_ALL, "");

    /* Set the relevant locale environment variable */
    if (setenv("LC_CTYPE", newloc, 1)) {
        fprintf(stderr,
                "Error setting LC_CTYPE, skipping C locale coercion\n");
        return;
    }
    if (_legacy_locale_warnings_enabled()) {
        fprintf(stderr, _C_LOCALE_COERCION_WARNING, newloc);
    }

    /* Reconfigure with the overridden environment variables */
    setlocale(LC_ALL, "");
}
#endif

void
_Py_CoerceLegacyLocale(void)
{
#ifdef PY_COERCE_C_LOCALE
    /* We ignore the Python -E and -I flags here, as the CLI needs to sort out
     * the locale settings *before* we try to do anything with the command
     * line arguments. For cross-platform debugging purposes, we also need
     * to give end users a way to force even scripts that are otherwise
     * isolated from their environment to use the legacy ASCII-centric C
     * locale.
     *
     * Ignoring -E and -I is safe from a security perspective, as we only use
     * the setting to turn *off* the implicit locale coercion, and anyone with
     * access to the process environment already has the ability to set
     * `LC_ALL=C` to override the C level locale settings anyway.
     */
    const char *coerce_c_locale = getenv("PYTHONCOERCECLOCALE");
    if (coerce_c_locale == NULL || strncmp(coerce_c_locale, "0", 2) != 0) {
        /* PYTHONCOERCECLOCALE is not set, or is set to something other than "0" */
        const char *locale_override = getenv("LC_ALL");
        if (locale_override == NULL || *locale_override == '\0') {
            /* LC_ALL is also not set (or is set to an empty string) */
            const _LocaleCoercionTarget *target = NULL;
            for (target = _TARGET_LOCALES; target->locale_name; target++) {
                const char *new_locale = setlocale(LC_CTYPE,
                                                   target->locale_name);
                if (new_locale != NULL) {
#if !defined(__APPLE__) && defined(HAVE_LANGINFO_H) && defined(CODESET)
                    /* Also ensure that nl_langinfo works in this locale */
                    char *codeset = nl_langinfo(CODESET);
                    if (!codeset || *codeset == '\0') {
                        /* CODESET is not set or empty, so skip coercion */
                        new_locale = NULL;
                        setlocale(LC_CTYPE, "");
                        continue;
                    }
#endif
                    /* Successfully configured locale, so make it the default */
                    _coerce_default_locale_settings(target);
                    return;
                }
            }
        }
    }
    /* No C locale warning here, as Py_Initialize will emit one later */
#endif
}


/* Global initializations.  Can be undone by Py_Finalize().  Don't
   call this twice without an intervening Py_Finalize() call.

   Every call to Py_InitializeCore, Py_Initialize or Py_InitializeEx
   must have a corresponding call to Py_Finalize.

   Locking: you must hold the interpreter lock while calling these APIs.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/

/* Begin interpreter initialization
 *
 * On return, the first thread and interpreter state have been created,
 * but the compiler, signal handling, multithreading and
 * multiple interpreter support, and codec infrastructure are not yet
 * available.
 *
 * The import system will support builtin and frozen modules only.
 * The only supported io is writing to sys.stderr
 *
 * If any operation invoked by this function fails, a fatal error is
 * issued and the function does not return.
 *
 * Any code invoked from this function should *not* assume it has access
 * to the Python C API (unless the API is explicitly listed as being
 * safe to call without calling Py_Initialize first)
 */

/* TODO: Progressively move functionality from Py_BeginInitialization to
 * Py_ReadConfig and Py_EndInitialization
 */

void _Py_InitializeCore(const _PyCoreConfig *config)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;
    PyObject *bimod, *sysmod, *pstderr;
    char *p;
    _PyCoreConfig core_config = _PyCoreConfig_INIT;
    _PyMainInterpreterConfig preinit_config = _PyMainInterpreterConfig_INIT;

    if (config != NULL) {
        core_config = *config;
    }

    if (_Py_Initialized) {
        Py_FatalError("Py_InitializeCore: main interpreter already initialized");
    }
    if (_Py_CoreInitialized) {
        Py_FatalError("Py_InitializeCore: runtime core already initialized");
    }

    /* Py_Finalize leaves _Py_Finalizing set in order to help daemon
     * threads behave a little more gracefully at interpreter shutdown.
     * We clobber it here so the new interpreter can start with a clean
     * slate.
     *
     * However, this may still lead to misbehaviour if there are daemon
     * threads still hanging around from a previous Py_Initialize/Finalize
     * pair :(
     */
    _Py_Finalizing = NULL;

#ifdef __ANDROID__
    /* Passing "" to setlocale() on Android requests the C locale rather
     * than checking environment variables, so request C.UTF-8 explicitly
     */
    setlocale(LC_CTYPE, "C.UTF-8");
#else
#ifndef MS_WINDOWS
    /* Set up the LC_CTYPE locale, so we can obtain
       the locale's charset without having to switch
       locales. */
    setlocale(LC_CTYPE, "");
    _emit_stderr_warning_for_legacy_locale();
#endif
#endif

    if ((p = Py_GETENV("PYTHONDEBUG")) && *p != '\0')
        Py_DebugFlag = add_flag(Py_DebugFlag, p);
    if ((p = Py_GETENV("PYTHONVERBOSE")) && *p != '\0')
        Py_VerboseFlag = add_flag(Py_VerboseFlag, p);
    if ((p = Py_GETENV("PYTHONOPTIMIZE")) && *p != '\0')
        Py_OptimizeFlag = add_flag(Py_OptimizeFlag, p);
    if ((p = Py_GETENV("PYTHONDONTWRITEBYTECODE")) && *p != '\0')
        Py_DontWriteBytecodeFlag = add_flag(Py_DontWriteBytecodeFlag, p);
    /* The variable is only tested for existence here;
       _Py_HashRandomization_Init will check its value further. */
    if ((p = Py_GETENV("PYTHONHASHSEED")) && *p != '\0')
        Py_HashRandomizationFlag = add_flag(Py_HashRandomizationFlag, p);
#ifdef MS_WINDOWS
    if ((p = Py_GETENV("PYTHONLEGACYWINDOWSFSENCODING")) && *p != '\0')
        Py_LegacyWindowsFSEncodingFlag = add_flag(Py_LegacyWindowsFSEncodingFlag, p);
    if ((p = Py_GETENV("PYTHONLEGACYWINDOWSSTDIO")) && *p != '\0')
        Py_LegacyWindowsStdioFlag = add_flag(Py_LegacyWindowsStdioFlag, p);
#endif

    _Py_HashRandomization_Init(&core_config);
    if (!core_config.use_hash_seed || core_config.hash_seed) {
        /* Random or non-zero hash seed */
        Py_HashRandomizationFlag = 1;
    }

    _PyInterpreterState_Init();
    interp = PyInterpreterState_New();
    if (interp == NULL)
        Py_FatalError("Py_InitializeCore: can't make main interpreter");
    interp->core_config = core_config;
    interp->config = preinit_config;

    tstate = PyThreadState_New(interp);
    if (tstate == NULL)
        Py_FatalError("Py_InitializeCore: can't make first thread");
    (void) PyThreadState_Swap(tstate);

#ifdef WITH_THREAD
    /* We can't call _PyEval_FiniThreads() in Py_FinalizeEx because
       destroying the GIL might fail when it is being referenced from
       another running thread (see issue #9901).
       Instead we destroy the previously created GIL here, which ensures
       that we can call Py_Initialize / Py_FinalizeEx multiple times. */
    _PyEval_FiniThreads();
    /* Auto-thread-state API */
    _PyGILState_Init(interp, tstate);
#endif /* WITH_THREAD */

    _Py_ReadyTypes();

    if (!_PyFrame_Init())
        Py_FatalError("Py_InitializeCore: can't init frames");

    if (!_PyLong_Init())
        Py_FatalError("Py_InitializeCore: can't init longs");

    if (!PyByteArray_Init())
        Py_FatalError("Py_InitializeCore: can't init bytearray");

    if (!_PyFloat_Init())
        Py_FatalError("Py_InitializeCore: can't init float");

    interp->modules = PyDict_New();
    if (interp->modules == NULL)
        Py_FatalError("Py_InitializeCore: can't make modules dictionary");

    /* Init Unicode implementation; relies on the codec registry */
    if (_PyUnicode_Init() < 0)
        Py_FatalError("Py_InitializeCore: can't initialize unicode");

    if (_PyStructSequence_Init() < 0)
        Py_FatalError("Py_InitializeCore: can't initialize structseq");

    bimod = _PyBuiltin_Init();
    if (bimod == NULL)
        Py_FatalError("Py_InitializeCore: can't initialize builtins modules");
    _PyImport_FixupBuiltin(bimod, "builtins");
    interp->builtins = PyModule_GetDict(bimod);
    if (interp->builtins == NULL)
        Py_FatalError("Py_InitializeCore: can't initialize builtins dict");
    Py_INCREF(interp->builtins);

    /* initialize builtin exceptions */
    _PyExc_Init(bimod);

    sysmod = _PySys_BeginInit();
    if (sysmod == NULL)
        Py_FatalError("Py_InitializeCore: can't initialize sys");
    interp->sysdict = PyModule_GetDict(sysmod);
    if (interp->sysdict == NULL)
        Py_FatalError("Py_InitializeCore: can't initialize sys dict");
    Py_INCREF(interp->sysdict);
    _PyImport_FixupBuiltin(sysmod, "sys");
    PyDict_SetItemString(interp->sysdict, "modules",
                         interp->modules);

    /* Set up a preliminary stderr printer until we have enough
       infrastructure for the io module in place. */
    pstderr = PyFile_NewStdPrinter(fileno(stderr));
    if (pstderr == NULL)
        Py_FatalError("Py_InitializeCore: can't set preliminary stderr");
    _PySys_SetObjectId(&PyId_stderr, pstderr);
    PySys_SetObject("__stderr__", pstderr);
    Py_DECREF(pstderr);

    _PyImport_Init();

    _PyImportHooks_Init();

    /* Initialize _warnings. */
    _PyWarnings_Init();

    /* This call sets up builtin and frozen import support */
    if (!interp->core_config._disable_importlib) {
        initimport(interp, sysmod);
    }

    /* Only when we get here is the runtime core fully initialized */
    _Py_CoreInitialized = 1;
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

int _Py_ReadMainInterpreterConfig(_PyMainInterpreterConfig *config)
{
    /* Signal handlers are installed by default */
    if (config->install_signal_handlers < 0) {
        config->install_signal_handlers = 1;
    }

    return 0;
}

/* Update interpreter state based on supplied configuration settings
 *
 * After calling this function, most of the restrictions on the interpreter
 * are lifted. The only remaining incomplete settings are those related
 * to the main module (sys.argv[0], __main__ metadata)
 *
 * Calling this when the interpreter is not initializing, is already
 * initialized or without a valid current thread state is a fatal error.
 * Other errors should be reported as normal Python exceptions with a
 * non-zero return code.
 */
int _Py_InitializeMainInterpreter(const _PyMainInterpreterConfig *config)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;

    if (!_Py_CoreInitialized) {
        Py_FatalError("Py_InitializeMainInterpreter: runtime core not initialized");
    }
    if (_Py_Initialized) {
        Py_FatalError("Py_InitializeMainInterpreter: main interpreter already initialized");
    }

    /* Get current thread state and interpreter pointer */
    tstate = PyThreadState_GET();
    if (!tstate)
        Py_FatalError("Py_InitializeMainInterpreter: failed to read thread state");
    interp = tstate->interp;
    if (!interp)
        Py_FatalError("Py_InitializeMainInterpreter: failed to get interpreter");

    /* Now finish configuring the main interpreter */
    interp->config = *config;

    if (interp->core_config._disable_importlib) {
        /* Special mode for freeze_importlib: run with no import system
         *
         * This means anything which needs support from extension modules
         * or pure Python code in the standard library won't work.
         */
        _Py_Initialized = 1;
        return 0;
    }
    /* TODO: Report exceptions rather than fatal errors below here */

    if (_PyTime_Init() < 0)
        Py_FatalError("Py_InitializeMainInterpreter: can't initialize time");

    /* Finish setting up the sys module and import system */
    /* GetPath may initialize state that _PySys_EndInit locks
       in, and so has to be called first. */
    /* TODO: Call Py_GetPath() in Py_ReadConfig, rather than here */
    PySys_SetPath(Py_GetPath());
    if (_PySys_EndInit(interp->sysdict) < 0)
        Py_FatalError("Py_InitializeMainInterpreter: can't finish initializing sys");
    initexternalimport(interp);

    /* initialize the faulthandler module */
    if (_PyFaulthandler_Init())
        Py_FatalError("Py_InitializeMainInterpreter: can't initialize faulthandler");

    if (initfsencoding(interp) < 0)
        Py_FatalError("Py_InitializeMainInterpreter: unable to load the file system codec");

    if (config->install_signal_handlers)
        initsigs(); /* Signal handling stuff, including initintr() */

    if (_PyTraceMalloc_Init() < 0)
        Py_FatalError("Py_InitializeMainInterpreter: can't initialize tracemalloc");

    initmain(interp); /* Module __main__ */
    if (initstdio() < 0)
        Py_FatalError(
            "Py_InitializeMainInterpreter: can't initialize sys standard streams");

    /* Initialize warnings. */
    if (PySys_HasWarnOptions()) {
        PyObject *warnings_module = PyImport_ImportModule("warnings");
        if (warnings_module == NULL) {
            fprintf(stderr, "'import warnings' failed; traceback:\n");
            PyErr_Print();
        }
        Py_XDECREF(warnings_module);
    }

    _Py_Initialized = 1;

    if (!Py_NoSiteFlag)
        initsite(); /* Module site */

    return 0;
}

#undef _INIT_DEBUG_PRINT

void
_Py_InitializeEx_Private(int install_sigs, int install_importlib)
{
    _PyCoreConfig core_config = _PyCoreConfig_INIT;
    _PyMainInterpreterConfig config = _PyMainInterpreterConfig_INIT;

    /* TODO: Moar config options! */
    core_config.ignore_environment = Py_IgnoreEnvironmentFlag;
    core_config._disable_importlib = !install_importlib;
    config.install_signal_handlers = install_sigs;
    _Py_InitializeCore(&core_config);
    /* TODO: Print any exceptions raised by these operations */
    if (_Py_ReadMainInterpreterConfig(&config))
        Py_FatalError("Py_Initialize: Py_ReadMainInterpreterConfig failed");
    if (_Py_InitializeMainInterpreter(&config))
        Py_FatalError("Py_Initialize: Py_InitializeMainInterpreter failed");
}


void
Py_InitializeEx(int install_sigs)
{
    _Py_InitializeEx_Private(install_sigs, 1);
}

void
Py_Initialize(void)
{
    Py_InitializeEx(1);
}


#ifdef COUNT_ALLOCS
extern void dump_counts(FILE*);
#endif

/* Flush stdout and stderr */

static int
file_is_closed(PyObject *fobj)
{
    int r;
    PyObject *tmp = PyObject_GetAttrString(fobj, "closed");
    if (tmp == NULL) {
        PyErr_Clear();
        return 0;
    }
    r = PyObject_IsTrue(tmp);
    Py_DECREF(tmp);
    if (r < 0)
        PyErr_Clear();
    return r > 0;
}

static int
flush_std_files(void)
{
    PyObject *fout = _PySys_GetObjectId(&PyId_stdout);
    PyObject *ferr = _PySys_GetObjectId(&PyId_stderr);
    PyObject *tmp;
    int status = 0;

    if (fout != NULL && fout != Py_None && !file_is_closed(fout)) {
        tmp = _PyObject_CallMethodId(fout, &PyId_flush, NULL);
        if (tmp == NULL) {
            PyErr_WriteUnraisable(fout);
            status = -1;
        }
        else
            Py_DECREF(tmp);
    }

    if (ferr != NULL && ferr != Py_None && !file_is_closed(ferr)) {
        tmp = _PyObject_CallMethodId(ferr, &PyId_flush, NULL);
        if (tmp == NULL) {
            PyErr_Clear();
            status = -1;
        }
        else
            Py_DECREF(tmp);
    }

    return status;
}

/* Undo the effect of Py_Initialize().

   Beware: if multiple interpreter and/or thread states exist, these
   are not wiped out; only the current thread and interpreter state
   are deleted.  But since everything else is deleted, those other
   interpreter and thread states should no longer be used.

   (XXX We should do better, e.g. wipe out all interpreters and
   threads.)

   Locking: as above.

*/

int
Py_FinalizeEx(void)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;
    int status = 0;

    if (!_Py_Initialized)
        return status;

    wait_for_thread_shutdown();

    /* The interpreter is still entirely intact at this point, and the
     * exit funcs may be relying on that.  In particular, if some thread
     * or exit func is still waiting to do an import, the import machinery
     * expects Py_IsInitialized() to return true.  So don't say the
     * interpreter is uninitialized until after the exit funcs have run.
     * Note that Threading.py uses an exit func to do a join on all the
     * threads created thru it, so this also protects pending imports in
     * the threads created via Threading.
     */
    call_py_exitfuncs();

    /* Get current thread state and interpreter pointer */
    tstate = PyThreadState_GET();
    interp = tstate->interp;

    /* Remaining threads (e.g. daemon threads) will automatically exit
       after taking the GIL (in PyEval_RestoreThread()). */
    _Py_Finalizing = tstate;
    _Py_Initialized = 0;
    _Py_CoreInitialized = 0;

    /* Flush sys.stdout and sys.stderr */
    if (flush_std_files() < 0) {
        status = -1;
    }

    /* Disable signal handling */
    PyOS_FiniInterrupts();

    /* Collect garbage.  This may call finalizers; it's nice to call these
     * before all modules are destroyed.
     * XXX If a __del__ or weakref callback is triggered here, and tries to
     * XXX import a module, bad things can happen, because Python no
     * XXX longer believes it's initialized.
     * XXX     Fatal Python error: Interpreter not initialized (version mismatch?)
     * XXX is easy to provoke that way.  I've also seen, e.g.,
     * XXX     Exception exceptions.ImportError: 'No module named sha'
     * XXX         in <function callback at 0x008F5718> ignored
     * XXX but I'm unclear on exactly how that one happens.  In any case,
     * XXX I haven't seen a real-life report of either of these.
     */
    _PyGC_CollectIfEnabled();
#ifdef COUNT_ALLOCS
    /* With COUNT_ALLOCS, it helps to run GC multiple times:
       each collection might release some types from the type
       list, so they become garbage. */
    while (_PyGC_CollectIfEnabled() > 0)
        /* nothing */;
#endif
    /* Destroy all modules */
    PyImport_Cleanup();

    /* Flush sys.stdout and sys.stderr (again, in case more was printed) */
    if (flush_std_files() < 0) {
        status = -1;
    }

    /* Collect final garbage.  This disposes of cycles created by
     * class definitions, for example.
     * XXX This is disabled because it caused too many problems.  If
     * XXX a __del__ or weakref callback triggers here, Python code has
     * XXX a hard time running, because even the sys module has been
     * XXX cleared out (sys.stdout is gone, sys.excepthook is gone, etc).
     * XXX One symptom is a sequence of information-free messages
     * XXX coming from threads (if a __del__ or callback is invoked,
     * XXX other threads can execute too, and any exception they encounter
     * XXX triggers a comedy of errors as subsystem after subsystem
     * XXX fails to find what it *expects* to find in sys to help report
     * XXX the exception and consequent unexpected failures).  I've also
     * XXX seen segfaults then, after adding print statements to the
     * XXX Python code getting called.
     */
#if 0
    _PyGC_CollectIfEnabled();
#endif

    /* Disable tracemalloc after all Python objects have been destroyed,
       so it is possible to use tracemalloc in objects destructor. */
    _PyTraceMalloc_Fini();

    /* Destroy the database used by _PyImport_{Fixup,Find}Extension */
    _PyImport_Fini();

    /* Cleanup typeobject.c's internal caches. */
    _PyType_Fini();

    /* unload faulthandler module */
    _PyFaulthandler_Fini();

    /* Debugging stuff */
#ifdef COUNT_ALLOCS
    dump_counts(stderr);
#endif
    /* dump hash stats */
    _PyHash_Fini();

    _PY_DEBUG_PRINT_TOTAL_REFS();

#ifdef Py_TRACE_REFS
    /* Display all objects still alive -- this can invoke arbitrary
     * __repr__ overrides, so requires a mostly-intact interpreter.
     * Alas, a lot of stuff may still be alive now that will be cleaned
     * up later.
     */
    if (Py_GETENV("PYTHONDUMPREFS"))
        _Py_PrintReferences(stderr);
#endif /* Py_TRACE_REFS */

    /* Clear interpreter state and all thread states. */
    PyInterpreterState_Clear(interp);

    /* Now we decref the exception classes.  After this point nothing
       can raise an exception.  That's okay, because each Fini() method
       below has been checked to make sure no exceptions are ever
       raised.
    */

    _PyExc_Fini();

    /* Sundry finalizers */
    PyMethod_Fini();
    PyFrame_Fini();
    PyCFunction_Fini();
    PyTuple_Fini();
    PyList_Fini();
    PySet_Fini();
    PyBytes_Fini();
    PyByteArray_Fini();
    PyLong_Fini();
    PyFloat_Fini();
    PyDict_Fini();
    PySlice_Fini();
    _PyGC_Fini();
    _Py_HashRandomization_Fini();
    _PyArg_Fini();
    PyAsyncGen_Fini();

    /* Cleanup Unicode implementation */
    _PyUnicode_Fini();

    /* reset file system default encoding */
    if (!Py_HasFileSystemDefaultEncoding && Py_FileSystemDefaultEncoding) {
        PyMem_RawFree((char*)Py_FileSystemDefaultEncoding);
        Py_FileSystemDefaultEncoding = NULL;
    }

    /* XXX Still allocated:
       - various static ad-hoc pointers to interned strings
       - int and float free list blocks
       - whatever various modules and libraries allocate
    */

    PyGrammar_RemoveAccelerators(&_PyParser_Grammar);

    /* Cleanup auto-thread-state */
#ifdef WITH_THREAD
    _PyGILState_Fini();
#endif /* WITH_THREAD */

    /* Delete current thread. After this, many C API calls become crashy. */
    PyThreadState_Swap(NULL);

    PyInterpreterState_Delete(interp);

#ifdef Py_TRACE_REFS
    /* Display addresses (& refcnts) of all objects still alive.
     * An address can be used to find the repr of the object, printed
     * above by _Py_PrintReferences.
     */
    if (Py_GETENV("PYTHONDUMPREFS"))
        _Py_PrintReferenceAddresses(stderr);
#endif /* Py_TRACE_REFS */
#ifdef WITH_PYMALLOC
    if (_PyMem_PymallocEnabled()) {
        char *opt = Py_GETENV("PYTHONMALLOCSTATS");
        if (opt != NULL && *opt != '\0')
            _PyObject_DebugMallocStats(stderr);
    }
#endif

    call_ll_exitfuncs();
    return status;
}

void
Py_Finalize(void)
{
    Py_FinalizeEx();
}

/* Create and initialize a new interpreter and thread, and return the
   new thread.  This requires that Py_Initialize() has been called
   first.

   Unsuccessful initialization yields a NULL pointer.  Note that *no*
   exception information is available even in this case -- the
   exception information is held in the thread, and there is no
   thread.

   Locking: as above.

*/

PyThreadState *
Py_NewInterpreter(void)
{
    PyInterpreterState *interp;
    PyThreadState *tstate, *save_tstate;
    PyObject *bimod, *sysmod;

    if (!_Py_Initialized)
        Py_FatalError("Py_NewInterpreter: call Py_Initialize first");

#ifdef WITH_THREAD
    /* Issue #10915, #15751: The GIL API doesn't work with multiple
       interpreters: disable PyGILState_Check(). */
    _PyGILState_check_enabled = 0;
#endif

    interp = PyInterpreterState_New();
    if (interp == NULL)
        return NULL;

    tstate = PyThreadState_New(interp);
    if (tstate == NULL) {
        PyInterpreterState_Delete(interp);
        return NULL;
    }

    save_tstate = PyThreadState_Swap(tstate);

    /* Copy the current interpreter config into the new interpreter */
    if (save_tstate != NULL) {
        interp->core_config = save_tstate->interp->core_config;
        interp->config = save_tstate->interp->config;
    } else {
        /* No current thread state, copy from the main interpreter */
        PyInterpreterState *main_interp = PyInterpreterState_Main();
        interp->core_config = main_interp->core_config;
        interp->config = main_interp->config;
    }

    /* XXX The following is lax in error checking */

    interp->modules = PyDict_New();

    bimod = _PyImport_FindBuiltin("builtins");
    if (bimod != NULL) {
        interp->builtins = PyModule_GetDict(bimod);
        if (interp->builtins == NULL)
            goto handle_error;
        Py_INCREF(interp->builtins);
    }

    /* initialize builtin exceptions */
    _PyExc_Init(bimod);

    sysmod = _PyImport_FindBuiltin("sys");
    if (bimod != NULL && sysmod != NULL) {
        PyObject *pstderr;

        interp->sysdict = PyModule_GetDict(sysmod);
        if (interp->sysdict == NULL)
            goto handle_error;
        Py_INCREF(interp->sysdict);
        _PySys_EndInit(interp->sysdict);
        PySys_SetPath(Py_GetPath());
        PyDict_SetItemString(interp->sysdict, "modules",
                             interp->modules);
        /* Set up a preliminary stderr printer until we have enough
           infrastructure for the io module in place. */
        pstderr = PyFile_NewStdPrinter(fileno(stderr));
        if (pstderr == NULL)
            Py_FatalError("Py_NewInterpreter: can't set preliminary stderr");
        _PySys_SetObjectId(&PyId_stderr, pstderr);
        PySys_SetObject("__stderr__", pstderr);
        Py_DECREF(pstderr);

        _PyImportHooks_Init();

        initimport(interp, sysmod);
        initexternalimport(interp);

        if (initfsencoding(interp) < 0)
            goto handle_error;

        if (initstdio() < 0)
            Py_FatalError(
                "Py_NewInterpreter: can't initialize sys standard streams");
        initmain(interp);
        if (!Py_NoSiteFlag)
            initsite();
    }

    if (!PyErr_Occurred())
        return tstate;

handle_error:
    /* Oops, it didn't work.  Undo it all. */

    PyErr_PrintEx(0);
    PyThreadState_Clear(tstate);
    PyThreadState_Swap(save_tstate);
    PyThreadState_Delete(tstate);
    PyInterpreterState_Delete(interp);

    return NULL;
}

/* Delete an interpreter and its last thread.  This requires that the
   given thread state is current, that the thread has no remaining
   frames, and that it is its interpreter's only remaining thread.
   It is a fatal error to violate these constraints.

   (Py_FinalizeEx() doesn't have these constraints -- it zaps
   everything, regardless.)

   Locking: as above.

*/

void
Py_EndInterpreter(PyThreadState *tstate)
{
    PyInterpreterState *interp = tstate->interp;

    if (tstate != PyThreadState_GET())
        Py_FatalError("Py_EndInterpreter: thread is not current");
    if (tstate->frame != NULL)
        Py_FatalError("Py_EndInterpreter: thread still has a frame");

    wait_for_thread_shutdown();

    if (tstate != interp->tstate_head || tstate->next != NULL)
        Py_FatalError("Py_EndInterpreter: not the last thread");

    PyImport_Cleanup();
    PyInterpreterState_Clear(interp);
    PyThreadState_Swap(NULL);
    PyInterpreterState_Delete(interp);
}

#ifdef MS_WINDOWS
static wchar_t *progname = L"python";
#else
static wchar_t *progname = L"python3";
#endif

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

static wchar_t *default_home = NULL;
static wchar_t env_home[MAXPATHLEN+1];

void
Py_SetPythonHome(wchar_t *home)
{
    default_home = home;
}

wchar_t *
Py_GetPythonHome(void)
{
    wchar_t *home = default_home;
    if (home == NULL && !Py_IgnoreEnvironmentFlag) {
        char* chome = Py_GETENV("PYTHONHOME");
        if (chome) {
            size_t size = Py_ARRAY_LENGTH(env_home);
            size_t r = mbstowcs(env_home, chome, size);
            if (r != (size_t)-1 && r < size)
                home = env_home;
        }

    }
    return home;
}

/* Create __main__ module */

static void
initmain(PyInterpreterState *interp)
{
    PyObject *m, *d, *loader, *ann_dict;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        Py_FatalError("can't create __main__ module");
    d = PyModule_GetDict(m);
    ann_dict = PyDict_New();
    if ((ann_dict == NULL) ||
        (PyDict_SetItemString(d, "__annotations__", ann_dict) < 0)) {
        Py_FatalError("Failed to initialize __main__.__annotations__");
    }
    Py_DECREF(ann_dict);
    if (PyDict_GetItemString(d, "__builtins__") == NULL) {
        PyObject *bimod = PyImport_ImportModule("builtins");
        if (bimod == NULL) {
            Py_FatalError("Failed to retrieve builtins module");
        }
        if (PyDict_SetItemString(d, "__builtins__", bimod) < 0) {
            Py_FatalError("Failed to initialize __main__.__builtins__");
        }
        Py_DECREF(bimod);
    }
    /* Main is a little special - imp.is_builtin("__main__") will return
     * False, but BuiltinImporter is still the most appropriate initial
     * setting for its __loader__ attribute. A more suitable value will
     * be set if __main__ gets further initialized later in the startup
     * process.
     */
    loader = PyDict_GetItemString(d, "__loader__");
    if (loader == NULL || loader == Py_None) {
        PyObject *loader = PyObject_GetAttrString(interp->importlib,
                                                  "BuiltinImporter");
        if (loader == NULL) {
            Py_FatalError("Failed to retrieve BuiltinImporter");
        }
        if (PyDict_SetItemString(d, "__loader__", loader) < 0) {
            Py_FatalError("Failed to initialize __main__.__loader__");
        }
        Py_DECREF(loader);
    }
}

static int
initfsencoding(PyInterpreterState *interp)
{
    PyObject *codec;

#ifdef MS_WINDOWS
    if (Py_LegacyWindowsFSEncodingFlag)
    {
        Py_FileSystemDefaultEncoding = "mbcs";
        Py_FileSystemDefaultEncodeErrors = "replace";
    }
    else
    {
        Py_FileSystemDefaultEncoding = "utf-8";
        Py_FileSystemDefaultEncodeErrors = "surrogatepass";
    }
#else
    if (Py_FileSystemDefaultEncoding == NULL)
    {
        Py_FileSystemDefaultEncoding = get_locale_encoding();
        if (Py_FileSystemDefaultEncoding == NULL)
            Py_FatalError("Py_Initialize: Unable to get the locale encoding");

        Py_HasFileSystemDefaultEncoding = 0;
        interp->fscodec_initialized = 1;
        return 0;
    }
#endif

    /* the encoding is mbcs, utf-8 or ascii */
    codec = _PyCodec_Lookup(Py_FileSystemDefaultEncoding);
    if (!codec) {
        /* Such error can only occurs in critical situations: no more
         * memory, import a module of the standard library failed,
         * etc. */
        return -1;
    }
    Py_DECREF(codec);
    interp->fscodec_initialized = 1;
    return 0;
}

/* Import the site module (not into __main__ though) */

static void
initsite(void)
{
    PyObject *m;
    m = PyImport_ImportModule("site");
    if (m == NULL) {
        fprintf(stderr, "Failed to import the site module\n");
        PyErr_Print();
        Py_Finalize();
        exit(1);
    }
    else {
        Py_DECREF(m);
    }
}

/* Check if a file descriptor is valid or not.
   Return 0 if the file descriptor is invalid, return non-zero otherwise. */
static int
is_valid_fd(int fd)
{
#ifdef __APPLE__
    /* bpo-30225: On macOS Tiger, when stdout is redirected to a pipe
       and the other side of the pipe is closed, dup(1) succeed, whereas
       fstat(1, &st) fails with EBADF. Prefer fstat() over dup() to detect
       such error. */
    struct stat st;
    return (fstat(fd, &st) == 0);
#else
    int fd2;
    if (fd < 0)
        return 0;
    _Py_BEGIN_SUPPRESS_IPH
    /* Prefer dup() over fstat(). fstat() can require input/output whereas
       dup() doesn't, there is a low risk of EMFILE/ENFILE at Python
       startup. */
    fd2 = dup(fd);
    if (fd2 >= 0)
        close(fd2);
    _Py_END_SUPPRESS_IPH
    return fd2 >= 0;
#endif
}

/* returns Py_None if the fd is not valid */
static PyObject*
create_stdio(PyObject* io,
    int fd, int write_mode, const char* name,
    const char* encoding, const char* errors)
{
    PyObject *buf = NULL, *stream = NULL, *text = NULL, *raw = NULL, *res;
    const char* mode;
    const char* newline;
    PyObject *line_buffering;
    int buffering, isatty;
    _Py_IDENTIFIER(open);
    _Py_IDENTIFIER(isatty);
    _Py_IDENTIFIER(TextIOWrapper);
    _Py_IDENTIFIER(mode);

    if (!is_valid_fd(fd))
        Py_RETURN_NONE;

    /* stdin is always opened in buffered mode, first because it shouldn't
       make a difference in common use cases, second because TextIOWrapper
       depends on the presence of a read1() method which only exists on
       buffered streams.
    */
    if (Py_UnbufferedStdioFlag && write_mode)
        buffering = 0;
    else
        buffering = -1;
    if (write_mode)
        mode = "wb";
    else
        mode = "rb";
    buf = _PyObject_CallMethodId(io, &PyId_open, "isiOOOi",
                                 fd, mode, buffering,
                                 Py_None, Py_None, /* encoding, errors */
                                 Py_None, 0); /* newline, closefd */
    if (buf == NULL)
        goto error;

    if (buffering) {
        _Py_IDENTIFIER(raw);
        raw = _PyObject_GetAttrId(buf, &PyId_raw);
        if (raw == NULL)
            goto error;
    }
    else {
        raw = buf;
        Py_INCREF(raw);
    }

#ifdef MS_WINDOWS
    /* Windows console IO is always UTF-8 encoded */
    if (PyWindowsConsoleIO_Check(raw))
        encoding = "utf-8";
#endif

    text = PyUnicode_FromString(name);
    if (text == NULL || _PyObject_SetAttrId(raw, &PyId_name, text) < 0)
        goto error;
    res = _PyObject_CallMethodId(raw, &PyId_isatty, NULL);
    if (res == NULL)
        goto error;
    isatty = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (isatty == -1)
        goto error;
    if (isatty || Py_UnbufferedStdioFlag)
        line_buffering = Py_True;
    else
        line_buffering = Py_False;

    Py_CLEAR(raw);
    Py_CLEAR(text);

#ifdef MS_WINDOWS
    /* sys.stdin: enable universal newline mode, translate "\r\n" and "\r"
       newlines to "\n".
       sys.stdout and sys.stderr: translate "\n" to "\r\n". */
    newline = NULL;
#else
    /* sys.stdin: split lines at "\n".
       sys.stdout and sys.stderr: don't translate newlines (use "\n"). */
    newline = "\n";
#endif

    stream = _PyObject_CallMethodId(io, &PyId_TextIOWrapper, "OsssO",
                                    buf, encoding, errors,
                                    newline, line_buffering);
    Py_CLEAR(buf);
    if (stream == NULL)
        goto error;

    if (write_mode)
        mode = "w";
    else
        mode = "r";
    text = PyUnicode_FromString(mode);
    if (!text || _PyObject_SetAttrId(stream, &PyId_mode, text) < 0)
        goto error;
    Py_CLEAR(text);
    return stream;

error:
    Py_XDECREF(buf);
    Py_XDECREF(stream);
    Py_XDECREF(text);
    Py_XDECREF(raw);

    if (PyErr_ExceptionMatches(PyExc_OSError) && !is_valid_fd(fd)) {
        /* Issue #24891: the file descriptor was closed after the first
           is_valid_fd() check was called. Ignore the OSError and set the
           stream to None. */
        PyErr_Clear();
        Py_RETURN_NONE;
    }
    return NULL;
}

/* Initialize sys.stdin, stdout, stderr and builtins.open */
static int
initstdio(void)
{
    PyObject *iomod = NULL, *wrapper;
    PyObject *bimod = NULL;
    PyObject *m;
    PyObject *std = NULL;
    int status = 0, fd;
    PyObject * encoding_attr;
    char *pythonioencoding = NULL, *encoding, *errors;

    /* Hack to avoid a nasty recursion issue when Python is invoked
       in verbose mode: pre-import the Latin-1 and UTF-8 codecs */
    if ((m = PyImport_ImportModule("encodings.utf_8")) == NULL) {
        goto error;
    }
    Py_DECREF(m);

    if (!(m = PyImport_ImportModule("encodings.latin_1"))) {
        goto error;
    }
    Py_DECREF(m);

    if (!(bimod = PyImport_ImportModule("builtins"))) {
        goto error;
    }

    if (!(iomod = PyImport_ImportModule("io"))) {
        goto error;
    }
    if (!(wrapper = PyObject_GetAttrString(iomod, "OpenWrapper"))) {
        goto error;
    }

    /* Set builtins.open */
    if (PyObject_SetAttrString(bimod, "open", wrapper) == -1) {
        Py_DECREF(wrapper);
        goto error;
    }
    Py_DECREF(wrapper);

    encoding = _Py_StandardStreamEncoding;
    errors = _Py_StandardStreamErrors;
    if (!encoding || !errors) {
        pythonioencoding = Py_GETENV("PYTHONIOENCODING");
        if (pythonioencoding) {
            char *err;
            pythonioencoding = _PyMem_Strdup(pythonioencoding);
            if (pythonioencoding == NULL) {
                PyErr_NoMemory();
                goto error;
            }
            err = strchr(pythonioencoding, ':');
            if (err) {
                *err = '\0';
                err++;
                if (*err && !errors) {
                    errors = err;
                }
            }
            if (*pythonioencoding && !encoding) {
                encoding = pythonioencoding;
            }
        }
        if (!errors && !(pythonioencoding && *pythonioencoding)) {
            /* Choose the default error handler based on the current locale */
            errors = get_default_standard_stream_error_handler();
        }
    }

    /* Set sys.stdin */
    fd = fileno(stdin);
    /* Under some conditions stdin, stdout and stderr may not be connected
     * and fileno() may point to an invalid file descriptor. For example
     * GUI apps don't have valid standard streams by default.
     */
    std = create_stdio(iomod, fd, 0, "<stdin>", encoding, errors);
    if (std == NULL)
        goto error;
    PySys_SetObject("__stdin__", std);
    _PySys_SetObjectId(&PyId_stdin, std);
    Py_DECREF(std);

    /* Set sys.stdout */
    fd = fileno(stdout);
    std = create_stdio(iomod, fd, 1, "<stdout>", encoding, errors);
    if (std == NULL)
        goto error;
    PySys_SetObject("__stdout__", std);
    _PySys_SetObjectId(&PyId_stdout, std);
    Py_DECREF(std);

#if 1 /* Disable this if you have trouble debugging bootstrap stuff */
    /* Set sys.stderr, replaces the preliminary stderr */
    fd = fileno(stderr);
    std = create_stdio(iomod, fd, 1, "<stderr>", encoding, "backslashreplace");
    if (std == NULL)
        goto error;

    /* Same as hack above, pre-import stderr's codec to avoid recursion
       when import.c tries to write to stderr in verbose mode. */
    encoding_attr = PyObject_GetAttrString(std, "encoding");
    if (encoding_attr != NULL) {
        const char *std_encoding = PyUnicode_AsUTF8(encoding_attr);
        if (std_encoding != NULL) {
            PyObject *codec_info = _PyCodec_Lookup(std_encoding);
            Py_XDECREF(codec_info);
        }
        Py_DECREF(encoding_attr);
    }
    PyErr_Clear();  /* Not a fatal error if codec isn't available */

    if (PySys_SetObject("__stderr__", std) < 0) {
        Py_DECREF(std);
        goto error;
    }
    if (_PySys_SetObjectId(&PyId_stderr, std) < 0) {
        Py_DECREF(std);
        goto error;
    }
    Py_DECREF(std);
#endif

    if (0) {
  error:
        status = -1;
    }

    /* We won't need them anymore. */
    if (_Py_StandardStreamEncoding) {
        PyMem_RawFree(_Py_StandardStreamEncoding);
        _Py_StandardStreamEncoding = NULL;
    }
    if (_Py_StandardStreamErrors) {
        PyMem_RawFree(_Py_StandardStreamErrors);
        _Py_StandardStreamErrors = NULL;
    }
    PyMem_Free(pythonioencoding);
    Py_XDECREF(bimod);
    Py_XDECREF(iomod);
    return status;
}


static void
_Py_FatalError_DumpTracebacks(int fd)
{
    fputc('\n', stderr);
    fflush(stderr);

    /* display the current Python stack */
    _Py_DumpTracebackThreads(fd, NULL, NULL);
}

/* Print the current exception (if an exception is set) with its traceback,
   or display the current Python stack.

   Don't call PyErr_PrintEx() and the except hook, because Py_FatalError() is
   called on catastrophic cases.

   Return 1 if the traceback was displayed, 0 otherwise. */

static int
_Py_FatalError_PrintExc(int fd)
{
    PyObject *ferr, *res;
    PyObject *exception, *v, *tb;
    int has_tb;

    if (PyThreadState_GET() == NULL) {
        /* The GIL is released: trying to acquire it is likely to deadlock,
           just give up. */
        return 0;
    }

    PyErr_Fetch(&exception, &v, &tb);
    if (exception == NULL) {
        /* No current exception */
        return 0;
    }

    ferr = _PySys_GetObjectId(&PyId_stderr);
    if (ferr == NULL || ferr == Py_None) {
        /* sys.stderr is not set yet or set to None,
           no need to try to display the exception */
        return 0;
    }

    PyErr_NormalizeException(&exception, &v, &tb);
    if (tb == NULL) {
        tb = Py_None;
        Py_INCREF(tb);
    }
    PyException_SetTraceback(v, tb);
    if (exception == NULL) {
        /* PyErr_NormalizeException() failed */
        return 0;
    }

    has_tb = (tb != Py_None);
    PyErr_Display(exception, v, tb);
    Py_XDECREF(exception);
    Py_XDECREF(v);
    Py_XDECREF(tb);

    /* sys.stderr may be buffered: call sys.stderr.flush() */
    res = _PyObject_CallMethodId(ferr, &PyId_flush, NULL);
    if (res == NULL)
        PyErr_Clear();
    else
        Py_DECREF(res);

    return has_tb;
}

/* Print fatal error message and abort */

void
Py_FatalError(const char *msg)
{
    const int fd = fileno(stderr);
    static int reentrant = 0;
#ifdef MS_WINDOWS
    size_t len;
    WCHAR* buffer;
    size_t i;
#endif

    if (reentrant) {
        /* Py_FatalError() caused a second fatal error.
           Example: flush_std_files() raises a recursion error. */
        goto exit;
    }
    reentrant = 1;

    fprintf(stderr, "Fatal Python error: %s\n", msg);
    fflush(stderr); /* it helps in Windows debug build */

    /* Print the exception (if an exception is set) with its traceback,
     * or display the current Python stack. */
    if (!_Py_FatalError_PrintExc(fd))
        _Py_FatalError_DumpTracebacks(fd);

    /* The main purpose of faulthandler is to display the traceback. We already
     * did our best to display it. So faulthandler can now be disabled.
     * (Don't trigger it on abort().) */
    _PyFaulthandler_Fini();

    /* Check if the current Python thread hold the GIL */
    if (PyThreadState_GET() != NULL) {
        /* Flush sys.stdout and sys.stderr */
        flush_std_files();
    }

#ifdef MS_WINDOWS
    len = strlen(msg);

    /* Convert the message to wchar_t. This uses a simple one-to-one
    conversion, assuming that the this error message actually uses ASCII
    only. If this ceases to be true, we will have to convert. */
    buffer = alloca( (len+1) * (sizeof *buffer));
    for( i=0; i<=len; ++i)
        buffer[i] = msg[i];
    OutputDebugStringW(L"Fatal Python error: ");
    OutputDebugStringW(buffer);
    OutputDebugStringW(L"\n");
#endif /* MS_WINDOWS */

exit:
#if defined(MS_WINDOWS) && defined(_DEBUG)
    DebugBreak();
#endif
    abort();
}

/* Clean up and exit */

#ifdef WITH_THREAD
#  include "pythread.h"
#endif

static void (*pyexitfunc)(void) = NULL;
/* For the atexit module. */
void _Py_PyAtExit(void (*func)(void))
{
    pyexitfunc = func;
}

static void
call_py_exitfuncs(void)
{
    if (pyexitfunc == NULL)
        return;

    (*pyexitfunc)();
    PyErr_Clear();
}

/* Wait until threading._shutdown completes, provided
   the threading module was imported in the first place.
   The shutdown routine will wait until all non-daemon
   "threading" threads have completed. */
static void
wait_for_thread_shutdown(void)
{
#ifdef WITH_THREAD
    _Py_IDENTIFIER(_shutdown);
    PyObject *result;
    PyThreadState *tstate = PyThreadState_GET();
    PyObject *threading = PyMapping_GetItemString(tstate->interp->modules,
                                                  "threading");
    if (threading == NULL) {
        /* threading not imported */
        PyErr_Clear();
        return;
    }
    result = _PyObject_CallMethodId(threading, &PyId__shutdown, NULL);
    if (result == NULL) {
        PyErr_WriteUnraisable(threading);
    }
    else {
        Py_DECREF(result);
    }
    Py_DECREF(threading);
#endif
}

#define NEXITFUNCS 32
static void (*exitfuncs[NEXITFUNCS])(void);
static int nexitfuncs = 0;

int Py_AtExit(void (*func)(void))
{
    if (nexitfuncs >= NEXITFUNCS)
        return -1;
    exitfuncs[nexitfuncs++] = func;
    return 0;
}

static void
call_ll_exitfuncs(void)
{
    while (nexitfuncs > 0)
        (*exitfuncs[--nexitfuncs])();

    fflush(stdout);
    fflush(stderr);
}

void
Py_Exit(int sts)
{
    if (Py_FinalizeEx() < 0) {
        sts = 120;
    }

    exit(sts);
}

static void
initsigs(void)
{
#ifdef SIGPIPE
    PyOS_setsig(SIGPIPE, SIG_IGN);
#endif
#ifdef SIGXFZ
    PyOS_setsig(SIGXFZ, SIG_IGN);
#endif
#ifdef SIGXFSZ
    PyOS_setsig(SIGXFSZ, SIG_IGN);
#endif
    PyOS_InitInterrupts(); /* May imply initsignal() */
    if (PyErr_Occurred()) {
        Py_FatalError("Py_Initialize: can't import signal");
    }
}


/* Restore signals that the interpreter has called SIG_IGN on to SIG_DFL.
 *
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 */
void
_Py_RestoreSignals(void)
{
#ifdef SIGPIPE
    PyOS_setsig(SIGPIPE, SIG_DFL);
#endif
#ifdef SIGXFZ
    PyOS_setsig(SIGXFZ, SIG_DFL);
#endif
#ifdef SIGXFSZ
    PyOS_setsig(SIGXFSZ, SIG_DFL);
#endif
}


/*
 * The file descriptor fd is considered ``interactive'' if either
 *   a) isatty(fd) is TRUE, or
 *   b) the -i flag was given, and the filename associated with
 *      the descriptor is NULL or "<stdin>" or "???".
 */
int
Py_FdIsInteractive(FILE *fp, const char *filename)
{
    if (isatty((int)fileno(fp)))
        return 1;
    if (!Py_InteractiveFlag)
        return 0;
    return (filename == NULL) ||
           (strcmp(filename, "<stdin>") == 0) ||
           (strcmp(filename, "???") == 0);
}


/* Wrappers around sigaction() or signal(). */

PyOS_sighandler_t
PyOS_getsig(int sig)
{
#ifdef HAVE_SIGACTION
    struct sigaction context;
    if (sigaction(sig, NULL, &context) == -1)
        return SIG_ERR;
    return context.sa_handler;
#else
    PyOS_sighandler_t handler;
/* Special signal handling for the secure CRT in Visual Studio 2005 */
#if defined(_MSC_VER) && _MSC_VER >= 1400
    switch (sig) {
    /* Only these signals are valid */
    case SIGINT:
    case SIGILL:
    case SIGFPE:
    case SIGSEGV:
    case SIGTERM:
    case SIGBREAK:
    case SIGABRT:
        break;
    /* Don't call signal() with other values or it will assert */
    default:
        return SIG_ERR;
    }
#endif /* _MSC_VER && _MSC_VER >= 1400 */
    handler = signal(sig, SIG_IGN);
    if (handler != SIG_ERR)
        signal(sig, handler);
    return handler;
#endif
}

/*
 * All of the code in this function must only use async-signal-safe functions,
 * listed at `man 7 signal` or
 * http://www.opengroup.org/onlinepubs/009695399/functions/xsh_chap02_04.html.
 */
PyOS_sighandler_t
PyOS_setsig(int sig, PyOS_sighandler_t handler)
{
#ifdef HAVE_SIGACTION
    /* Some code in Modules/signalmodule.c depends on sigaction() being
     * used here if HAVE_SIGACTION is defined.  Fix that if this code
     * changes to invalidate that assumption.
     */
    struct sigaction context, ocontext;
    context.sa_handler = handler;
    sigemptyset(&context.sa_mask);
    context.sa_flags = 0;
    if (sigaction(sig, &context, &ocontext) == -1)
        return SIG_ERR;
    return ocontext.sa_handler;
#else
    PyOS_sighandler_t oldhandler;
    oldhandler = signal(sig, handler);
#ifdef HAVE_SIGINTERRUPT
    siginterrupt(sig, 1);
#endif
    return oldhandler;
#endif
}

#ifdef __cplusplus
}
#endif
