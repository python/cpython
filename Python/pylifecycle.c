/* Python interpreter top-level routines, including init/exit */

#include "Python.h"

#include "Python-ast.h"
#undef Yield /* undefine macro conflicting with winbase.h */
#include "internal/context.h"
#include "internal/hamt.h"
#include "internal/pystate.h"
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
_Py_IDENTIFIER(threading);

#ifdef __cplusplus
extern "C" {
#endif

extern grammar _PyParser_Grammar; /* From graminit.c */

/* Forward */
static _PyInitError add_main_module(PyInterpreterState *interp);
static _PyInitError initfsencoding(PyInterpreterState *interp);
static _PyInitError initsite(void);
static _PyInitError init_sys_streams(PyInterpreterState *interp);
static _PyInitError initsigs(void);
static void call_py_exitfuncs(PyInterpreterState *);
static void wait_for_thread_shutdown(void);
static void call_ll_exitfuncs(void);
extern int _PyUnicode_Init(void);
extern int _PyStructSequence_Init(void);
extern void _PyUnicode_Fini(void);
extern int _PyLong_Init(void);
extern void PyLong_Fini(void);
extern _PyInitError _PyFaulthandler_Init(int enable);
extern void _PyFaulthandler_Fini(void);
extern void _PyHash_Fini(void);
extern int _PyTraceMalloc_Init(int enable);
extern int _PyTraceMalloc_Fini(void);
extern void _Py_ReadyTypes(void);

extern void _PyGILState_Init(PyInterpreterState *, PyThreadState *);
extern void _PyGILState_Fini(void);

_PyRuntimeState _PyRuntime = _PyRuntimeState_INIT;

_PyInitError
_PyRuntime_Initialize(void)
{
    /* XXX We only initialize once in the process, which aligns with
       the static initialization of the former globals now found in
       _PyRuntime.  However, _PyRuntime *should* be initialized with
       every Py_Initialize() call, but doing so breaks the runtime.
       This is because the runtime state is not properly finalized
       currently. */
    static int initialized = 0;
    if (initialized) {
        return _Py_INIT_OK();
    }
    initialized = 1;

    return _PyRuntimeState_Init(&_PyRuntime);
}

void
_PyRuntime_Finalize(void)
{
    _PyRuntimeState_Fini(&_PyRuntime);
}

int
_Py_IsFinalizing(void)
{
    return _PyRuntime.finalizing != NULL;
}

/* Global configuration variable declarations are in pydebug.h */
/* XXX (ncoghlan): move those declarations to pylifecycle.h? */
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

int
_Py_IsCoreInitialized(void)
{
    return _PyRuntime.core_initialized;
}

int
Py_IsInitialized(void)
{
    return _PyRuntime.initialized;
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


/* Global initializations.  Can be undone by Py_FinalizeEx().  Don't
   call this twice without an intervening Py_FinalizeEx() call.  When
   initializations fail, a fatal error is issued and the function does
   not return.  On return, the first thread and interpreter state have
   been created.

   Locking: you must hold the interpreter lock while calling this.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/

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
#if defined(HAVE_LANGINFO_H) && defined(CODESET)
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

static _PyInitError
initimport(PyInterpreterState *interp, PyObject *sysmod)
{
    PyObject *importlib;
    PyObject *impmod;
    PyObject *value;
    _PyInitError err;

    /* Import _importlib through its frozen version, _frozen_importlib. */
    if (PyImport_ImportFrozenModule("_frozen_importlib") <= 0) {
        return _Py_INIT_ERR("can't import _frozen_importlib");
    }
    else if (Py_VerboseFlag) {
        PySys_FormatStderr("import _frozen_importlib # frozen\n");
    }
    importlib = PyImport_AddModule("_frozen_importlib");
    if (importlib == NULL) {
        return _Py_INIT_ERR("couldn't get _frozen_importlib from sys.modules");
    }
    interp->importlib = importlib;
    Py_INCREF(interp->importlib);

    interp->import_func = PyDict_GetItemString(interp->builtins, "__import__");
    if (interp->import_func == NULL)
        return _Py_INIT_ERR("__import__ not found");
    Py_INCREF(interp->import_func);

    /* Import the _imp module */
    impmod = PyInit__imp();
    if (impmod == NULL) {
        return _Py_INIT_ERR("can't import _imp");
    }
    else if (Py_VerboseFlag) {
        PySys_FormatStderr("import _imp # builtin\n");
    }
    if (_PyImport_SetModuleString("_imp", impmod) < 0) {
        return _Py_INIT_ERR("can't save _imp to sys.modules");
    }

    /* Install importlib as the implementation of import */
    value = PyObject_CallMethod(importlib, "_install", "OO", sysmod, impmod);
    if (value == NULL) {
        PyErr_Print();
        return _Py_INIT_ERR("importlib install failed");
    }
    Py_DECREF(value);
    Py_DECREF(impmod);

    err = _PyImportZip_Init();
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    return _Py_INIT_OK();
}

static _PyInitError
initexternalimport(PyInterpreterState *interp)
{
    PyObject *value;
    value = PyObject_CallMethod(interp->importlib,
                                "_install_external_importers", "");
    if (value == NULL) {
        PyErr_Print();
        return _Py_INIT_ERR("external importer setup failed");
    }
    Py_DECREF(value);
    return _Py_INIT_OK();
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

static void
_emit_stderr_warning_for_legacy_locale(const _PyCoreConfig *core_config)
{
    if (core_config->coerce_c_locale_warn) {
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

static const char *
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
static const char C_LOCALE_COERCION_WARNING[] =
    "Python detected LC_CTYPE=C: LC_CTYPE coerced to %.20s (set another locale "
    "or PYTHONCOERCECLOCALE=0 to disable this locale coercion behavior).\n";

static void
_coerce_default_locale_settings(const _PyCoreConfig *config, const _LocaleCoercionTarget *target)
{
    const char *newloc = target->locale_name;

    /* Reset locale back to currently configured defaults */
    _Py_SetLocaleFromEnv(LC_ALL);

    /* Set the relevant locale environment variable */
    if (setenv("LC_CTYPE", newloc, 1)) {
        fprintf(stderr,
                "Error setting LC_CTYPE, skipping C locale coercion\n");
        return;
    }
    if (config->coerce_c_locale_warn) {
        fprintf(stderr, C_LOCALE_COERCION_WARNING, newloc);
    }

    /* Reconfigure with the overridden environment variables */
    _Py_SetLocaleFromEnv(LC_ALL);
}
#endif

void
_Py_CoerceLegacyLocale(const _PyCoreConfig *config)
{
#ifdef PY_COERCE_C_LOCALE
    const char *locale_override = getenv("LC_ALL");
    if (locale_override == NULL || *locale_override == '\0') {
        /* LC_ALL is also not set (or is set to an empty string) */
        const _LocaleCoercionTarget *target = NULL;
        for (target = _TARGET_LOCALES; target->locale_name; target++) {
            const char *new_locale = setlocale(LC_CTYPE,
                                               target->locale_name);
            if (new_locale != NULL) {
#if !defined(__APPLE__) && !defined(__ANDROID__) && \
defined(HAVE_LANGINFO_H) && defined(CODESET)
                /* Also ensure that nl_langinfo works in this locale */
                char *codeset = nl_langinfo(CODESET);
                if (!codeset || *codeset == '\0') {
                    /* CODESET is not set or empty, so skip coercion */
                    new_locale = NULL;
                    _Py_SetLocaleFromEnv(LC_CTYPE);
                    continue;
                }
#endif
                /* Successfully configured locale, so make it the default */
                _coerce_default_locale_settings(config, target);
                return;
            }
        }
    }
    /* No C locale warning here, as Py_Initialize will emit one later */
#endif
}

/* _Py_SetLocaleFromEnv() is a wrapper around setlocale(category, "") to
 * isolate the idiosyncrasies of different libc implementations. It reads the
 * appropriate environment variable and uses its value to select the locale for
 * 'category'. */
char *
_Py_SetLocaleFromEnv(int category)
{
#ifdef __ANDROID__
    const char *locale;
    const char **pvar;
#ifdef PY_COERCE_C_LOCALE
    const char *coerce_c_locale;
#endif
    const char *utf8_locale = "C.UTF-8";
    const char *env_var_set[] = {
        "LC_ALL",
        "LC_CTYPE",
        "LANG",
        NULL,
    };

    /* Android setlocale(category, "") doesn't check the environment variables
     * and incorrectly sets the "C" locale at API 24 and older APIs. We only
     * check the environment variables listed in env_var_set. */
    for (pvar=env_var_set; *pvar; pvar++) {
        locale = getenv(*pvar);
        if (locale != NULL && *locale != '\0') {
            if (strcmp(locale, utf8_locale) == 0 ||
                    strcmp(locale, "en_US.UTF-8") == 0) {
                return setlocale(category, utf8_locale);
            }
            return setlocale(category, "C");
        }
    }

    /* Android uses UTF-8, so explicitly set the locale to C.UTF-8 if none of
     * LC_ALL, LC_CTYPE, or LANG is set to a non-empty string.
     * Quote from POSIX section "8.2 Internationalization Variables":
     * "4. If the LANG environment variable is not set or is set to the empty
     * string, the implementation-defined default locale shall be used." */

#ifdef PY_COERCE_C_LOCALE
    coerce_c_locale = getenv("PYTHONCOERCECLOCALE");
    if (coerce_c_locale == NULL || strcmp(coerce_c_locale, "0") != 0) {
        /* Some other ported code may check the environment variables (e.g. in
         * extension modules), so we make sure that they match the locale
         * configuration */
        if (setenv("LC_CTYPE", utf8_locale, 1)) {
            fprintf(stderr, "Warning: failed setting the LC_CTYPE "
                            "environment variable to %s\n", utf8_locale);
        }
    }
#endif
    return setlocale(category, utf8_locale);
#else /* __ANDROID__ */
    return setlocale(category, "");
#endif /* __ANDROID__ */
}


/* Global initializations.  Can be undone by Py_Finalize().  Don't
   call this twice without an intervening Py_Finalize() call.

   Every call to _Py_InitializeCore, Py_Initialize or Py_InitializeEx
   must have a corresponding call to Py_Finalize.

   Locking: you must hold the interpreter lock while calling these APIs.
   (If the lock has not yet been initialized, that's equivalent to
   having the lock, but you cannot use multiple threads.)

*/

static _PyInitError
_Py_Initialize_ReconfigureCore(PyInterpreterState *interp,
                               const _PyCoreConfig *core_config)
{
    if (core_config->allocator != NULL) {
        const char *allocator = _PyMem_GetAllocatorsName();
        if (allocator == NULL || strcmp(core_config->allocator, allocator) != 0) {
            return _Py_INIT_USER_ERR("cannot modify memory allocator "
                                     "after first Py_Initialize()");
        }
    }

    _PyCoreConfig_SetGlobalConfig(core_config);

    if (_PyCoreConfig_Copy(&interp->core_config, core_config) < 0) {
        return _Py_INIT_ERR("failed to copy core config");
    }
    core_config = &interp->core_config;

    if (core_config->_install_importlib) {
        _PyInitError err = _PyCoreConfig_SetPathConfig(core_config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    return _Py_INIT_OK();
}


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
 *
 * The caller is responsible to call _PyCoreConfig_Read().
 */

static _PyInitError
_Py_InitializeCore_impl(PyInterpreterState **interp_p,
                        const _PyCoreConfig *core_config)
{
    PyInterpreterState *interp;
    _PyInitError err;

    /* bpo-34008: For backward compatibility reasons, calling Py_Main() after
       Py_Initialize() ignores the new configuration. */
    if (_PyRuntime.core_initialized) {
        PyThreadState *tstate = PyThreadState_GET();
        if (!tstate) {
            return _Py_INIT_ERR("failed to read thread state");
        }

        interp = tstate->interp;
        if (interp == NULL) {
            return _Py_INIT_ERR("can't make main interpreter");
        }
        *interp_p = interp;

        return _Py_Initialize_ReconfigureCore(interp, core_config);
    }

    if (_PyRuntime.initialized) {
        return _Py_INIT_ERR("main interpreter already initialized");
    }

    _PyCoreConfig_SetGlobalConfig(core_config);

    err = _PyRuntime_Initialize();
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    if (core_config->allocator != NULL) {
        if (_PyMem_SetupAllocators(core_config->allocator) < 0) {
            return _Py_INIT_USER_ERR("Unknown PYTHONMALLOC allocator");
        }
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
    _PyRuntime.finalizing = NULL;

#ifndef MS_WINDOWS
    /* Set up the LC_CTYPE locale, so we can obtain
       the locale's charset without having to switch
       locales. */
    _Py_SetLocaleFromEnv(LC_CTYPE);
    _emit_stderr_warning_for_legacy_locale(core_config);
#endif

    err = _Py_HashRandomization_Init(core_config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    err = _PyInterpreterState_Enable(&_PyRuntime);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    interp = PyInterpreterState_New();
    if (interp == NULL) {
        return _Py_INIT_ERR("can't make main interpreter");
    }
    *interp_p = interp;

    if (_PyCoreConfig_Copy(&interp->core_config, core_config) < 0) {
        return _Py_INIT_ERR("failed to copy core config");
    }
    core_config = &interp->core_config;

    PyThreadState *tstate = PyThreadState_New(interp);
    if (tstate == NULL)
        return _Py_INIT_ERR("can't make first thread");
    (void) PyThreadState_Swap(tstate);

    /* We can't call _PyEval_FiniThreads() in Py_FinalizeEx because
       destroying the GIL might fail when it is being referenced from
       another running thread (see issue #9901).
       Instead we destroy the previously created GIL here, which ensures
       that we can call Py_Initialize / Py_FinalizeEx multiple times. */
    _PyEval_FiniThreads();

    /* Auto-thread-state API */
    _PyGILState_Init(interp, tstate);

    /* Create the GIL */
    PyEval_InitThreads();

    _Py_ReadyTypes();

    if (!_PyLong_Init())
        return _Py_INIT_ERR("can't init longs");

    if (!PyByteArray_Init())
        return _Py_INIT_ERR("can't init bytearray");

    if (!_PyFloat_Init())
        return _Py_INIT_ERR("can't init float");

    PyObject *modules = PyDict_New();
    if (modules == NULL)
        return _Py_INIT_ERR("can't make modules dictionary");
    interp->modules = modules;

    PyObject *sysmod;
    err = _PySys_BeginInit(&sysmod);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    interp->sysdict = PyModule_GetDict(sysmod);
    if (interp->sysdict == NULL) {
        return _Py_INIT_ERR("can't initialize sys dict");
    }

    Py_INCREF(interp->sysdict);
    PyDict_SetItemString(interp->sysdict, "modules", modules);
    _PyImport_FixupBuiltin(sysmod, "sys", modules);

    /* Init Unicode implementation; relies on the codec registry */
    if (_PyUnicode_Init() < 0)
        return _Py_INIT_ERR("can't initialize unicode");

    if (_PyStructSequence_Init() < 0)
        return _Py_INIT_ERR("can't initialize structseq");

    PyObject *bimod = _PyBuiltin_Init();
    if (bimod == NULL)
        return _Py_INIT_ERR("can't initialize builtins modules");
    _PyImport_FixupBuiltin(bimod, "builtins", modules);
    interp->builtins = PyModule_GetDict(bimod);
    if (interp->builtins == NULL)
        return _Py_INIT_ERR("can't initialize builtins dict");
    Py_INCREF(interp->builtins);

    /* initialize builtin exceptions */
    _PyExc_Init(bimod);

    /* Set up a preliminary stderr printer until we have enough
       infrastructure for the io module in place. */
    PyObject *pstderr = PyFile_NewStdPrinter(fileno(stderr));
    if (pstderr == NULL)
        return _Py_INIT_ERR("can't set preliminary stderr");
    _PySys_SetObjectId(&PyId_stderr, pstderr);
    PySys_SetObject("__stderr__", pstderr);
    Py_DECREF(pstderr);

    err = _PyImport_Init(interp);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    err = _PyImportHooks_Init();
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* Initialize _warnings. */
    if (_PyWarnings_Init() == NULL) {
        return _Py_INIT_ERR("can't initialize warnings");
    }

    if (!_PyContext_Init())
        return _Py_INIT_ERR("can't init context");

    if (core_config->_install_importlib) {
        err = _PyCoreConfig_SetPathConfig(core_config);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    /* This call sets up builtin and frozen import support */
    if (core_config->_install_importlib) {
        err = initimport(interp, sysmod);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    /* Only when we get here is the runtime core fully initialized */
    _PyRuntime.core_initialized = 1;
    return _Py_INIT_OK();
}

_PyInitError
_Py_InitializeCore(PyInterpreterState **interp_p,
                   const _PyCoreConfig *src_config)
{
    assert(src_config != NULL);


    PyMemAllocatorEx old_alloc;
    _PyInitError err;

    /* Copy the configuration, since _PyCoreConfig_Read() modifies it
       (and the input configuration is read only). */
    _PyCoreConfig config = _PyCoreConfig_INIT;

    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    if (_PyCoreConfig_Copy(&config, src_config) >= 0) {
        err = _PyCoreConfig_Read(&config);
    }
    else {
        err = _Py_INIT_ERR("failed to copy core config");
    }
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    if (_Py_INIT_FAILED(err)) {
        goto done;
    }

    err = _Py_InitializeCore_impl(interp_p, &config);

done:
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, &old_alloc);
    _PyCoreConfig_Clear(&config);
    PyMem_SetAllocator(PYMEM_DOMAIN_RAW, &old_alloc);

    return err;
}

/* Py_Initialize() has already been called: update the main interpreter
   configuration. Example of bpo-34008: Py_Main() called after
   Py_Initialize(). */
static _PyInitError
_Py_ReconfigureMainInterpreter(PyInterpreterState *interp,
                               const _PyMainInterpreterConfig *config)
{
    if (config->argv != NULL) {
        int res = PyDict_SetItemString(interp->sysdict, "argv", config->argv);
        if (res < 0) {
            return _Py_INIT_ERR("fail to set sys.argv");
        }
    }
    return _Py_INIT_OK();
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
_PyInitError
_Py_InitializeMainInterpreter(PyInterpreterState *interp,
                              const _PyMainInterpreterConfig *config)
{
    if (!_PyRuntime.core_initialized) {
        return _Py_INIT_ERR("runtime core not initialized");
    }

    /* Configure the main interpreter */
    if (_PyMainInterpreterConfig_Copy(&interp->config, config) < 0) {
        return _Py_INIT_ERR("failed to copy main interpreter config");
    }
    config = &interp->config;
    _PyCoreConfig *core_config = &interp->core_config;

    if (_PyRuntime.initialized) {
        return _Py_ReconfigureMainInterpreter(interp, config);
    }

    if (!core_config->_install_importlib) {
        /* Special mode for freeze_importlib: run with no import system
         *
         * This means anything which needs support from extension modules
         * or pure Python code in the standard library won't work.
         */
        _PyRuntime.initialized = 1;
        return _Py_INIT_OK();
    }

    if (_PyTime_Init() < 0) {
        return _Py_INIT_ERR("can't initialize time");
    }

    if (_PySys_EndInit(interp->sysdict, &interp->config) < 0) {
        return _Py_INIT_ERR("can't finish initializing sys");
    }

    _PyInitError err = initexternalimport(interp);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* initialize the faulthandler module */
    err = _PyFaulthandler_Init(core_config->faulthandler);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    err = initfsencoding(interp);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    if (interp->config.install_signal_handlers) {
        err = initsigs(); /* Signal handling stuff, including initintr() */
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }

    if (_PyTraceMalloc_Init(core_config->tracemalloc) < 0) {
        return _Py_INIT_ERR("can't initialize tracemalloc");
    }

    err = add_main_module(interp);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    err = init_sys_streams(interp);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }

    /* Initialize warnings. */
    if (interp->config.warnoptions != NULL &&
        PyList_Size(interp->config.warnoptions) > 0)
    {
        PyObject *warnings_module = PyImport_ImportModule("warnings");
        if (warnings_module == NULL) {
            fprintf(stderr, "'import warnings' failed; traceback:\n");
            PyErr_Print();
        }
        Py_XDECREF(warnings_module);
    }

    _PyRuntime.initialized = 1;

    if (core_config->site_import) {
        err = initsite(); /* Module site */
        if (_Py_INIT_FAILED(err)) {
            return err;
        }
    }
    return _Py_INIT_OK();
}

#undef _INIT_DEBUG_PRINT

_PyInitError
_Py_InitializeFromConfig(const _PyCoreConfig *config)
{
    PyInterpreterState *interp;
    _PyInitError err;
    err = _Py_InitializeCore(&interp, config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }
    config = &interp->core_config;

    _PyMainInterpreterConfig main_config = _PyMainInterpreterConfig_INIT;
    err = _PyMainInterpreterConfig_Read(&main_config, config);
    if (!_Py_INIT_FAILED(err)) {
        err = _Py_InitializeMainInterpreter(interp, &main_config);
    }
    _PyMainInterpreterConfig_Clear(&main_config);
    if (_Py_INIT_FAILED(err)) {
        return err;
    }
    return _Py_INIT_OK();
}


void
Py_InitializeEx(int install_sigs)
{
    if (_PyRuntime.initialized) {
        /* bpo-33932: Calling Py_Initialize() twice does nothing. */
        return;
    }

    _PyInitError err;
    _PyCoreConfig config = _PyCoreConfig_INIT;
    config.install_signal_handlers = install_sigs;

    err = _Py_InitializeFromConfig(&config);
    _PyCoreConfig_Clear(&config);

    if (_Py_INIT_FAILED(err)) {
        _Py_FatalInitError(err);
    }
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

    if (!_PyRuntime.initialized)
        return status;

    wait_for_thread_shutdown();

    /* Get current thread state and interpreter pointer */
    tstate = PyThreadState_GET();
    interp = tstate->interp;

    /* The interpreter is still entirely intact at this point, and the
     * exit funcs may be relying on that.  In particular, if some thread
     * or exit func is still waiting to do an import, the import machinery
     * expects Py_IsInitialized() to return true.  So don't say the
     * interpreter is uninitialized until after the exit funcs have run.
     * Note that Threading.py uses an exit func to do a join on all the
     * threads created thru it, so this also protects pending imports in
     * the threads created via Threading.
     */

    call_py_exitfuncs(interp);

    /* Copy the core config, PyInterpreterState_Delete() free
       the core config memory */
#ifdef Py_REF_DEBUG
    int show_ref_count = interp->core_config.show_ref_count;
#endif
#ifdef Py_TRACE_REFS
    int dump_refs = interp->core_config.dump_refs;
#endif
#ifdef WITH_PYMALLOC
    int malloc_stats = interp->core_config.malloc_stats;
#endif

    /* Remaining threads (e.g. daemon threads) will automatically exit
       after taking the GIL (in PyEval_RestoreThread()). */
    _PyRuntime.finalizing = tstate;
    _PyRuntime.initialized = 0;
    _PyRuntime.core_initialized = 0;

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

#ifdef Py_REF_DEBUG
    if (show_ref_count) {
        _PyDebug_PrintTotalRefs();
    }
#endif

#ifdef Py_TRACE_REFS
    /* Display all objects still alive -- this can invoke arbitrary
     * __repr__ overrides, so requires a mostly-intact interpreter.
     * Alas, a lot of stuff may still be alive now that will be cleaned
     * up later.
     */
    if (dump_refs) {
        _Py_PrintReferences(stderr);
    }
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
    _PyContext_Fini();

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
    _PyGILState_Fini();

    /* Delete current thread. After this, many C API calls become crashy. */
    PyThreadState_Swap(NULL);

    PyInterpreterState_Delete(interp);

#ifdef Py_TRACE_REFS
    /* Display addresses (& refcnts) of all objects still alive.
     * An address can be used to find the repr of the object, printed
     * above by _Py_PrintReferences.
     */
    if (dump_refs) {
        _Py_PrintReferenceAddresses(stderr);
    }
#endif /* Py_TRACE_REFS */
#ifdef WITH_PYMALLOC
    if (malloc_stats) {
        _PyObject_DebugMallocStats(stderr);
    }
#endif

    call_ll_exitfuncs();

    _PyRuntime_Finalize();
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

static _PyInitError
new_interpreter(PyThreadState **tstate_p)
{
    PyInterpreterState *interp;
    PyThreadState *tstate, *save_tstate;
    PyObject *bimod, *sysmod;
    _PyInitError err;

    if (!_PyRuntime.initialized) {
        return _Py_INIT_ERR("Py_Initialize must be called first");
    }

    /* Issue #10915, #15751: The GIL API doesn't work with multiple
       interpreters: disable PyGILState_Check(). */
    _PyGILState_check_enabled = 0;

    interp = PyInterpreterState_New();
    if (interp == NULL) {
        *tstate_p = NULL;
        return _Py_INIT_OK();
    }

    tstate = PyThreadState_New(interp);
    if (tstate == NULL) {
        PyInterpreterState_Delete(interp);
        *tstate_p = NULL;
        return _Py_INIT_OK();
    }

    save_tstate = PyThreadState_Swap(tstate);

    /* Copy the current interpreter config into the new interpreter */
    _PyCoreConfig *core_config;
    _PyMainInterpreterConfig *config;
    if (save_tstate != NULL) {
        core_config = &save_tstate->interp->core_config;
        config = &save_tstate->interp->config;
    } else {
        /* No current thread state, copy from the main interpreter */
        PyInterpreterState *main_interp = PyInterpreterState_Main();
        core_config = &main_interp->core_config;
        config = &main_interp->config;
    }

    if (_PyCoreConfig_Copy(&interp->core_config, core_config) < 0) {
        return _Py_INIT_ERR("failed to copy core config");
    }
    core_config = &interp->core_config;
    if (_PyMainInterpreterConfig_Copy(&interp->config, config) < 0) {
        return _Py_INIT_ERR("failed to copy main interpreter config");
    }

    /* XXX The following is lax in error checking */
    PyObject *modules = PyDict_New();
    if (modules == NULL) {
        return _Py_INIT_ERR("can't make modules dictionary");
    }
    interp->modules = modules;

    sysmod = _PyImport_FindBuiltin("sys", modules);
    if (sysmod != NULL) {
        interp->sysdict = PyModule_GetDict(sysmod);
        if (interp->sysdict == NULL)
            goto handle_error;
        Py_INCREF(interp->sysdict);
        PyDict_SetItemString(interp->sysdict, "modules", modules);
        _PySys_EndInit(interp->sysdict, &interp->config);
    }

    bimod = _PyImport_FindBuiltin("builtins", modules);
    if (bimod != NULL) {
        interp->builtins = PyModule_GetDict(bimod);
        if (interp->builtins == NULL)
            goto handle_error;
        Py_INCREF(interp->builtins);
    }

    /* initialize builtin exceptions */
    _PyExc_Init(bimod);

    if (bimod != NULL && sysmod != NULL) {
        PyObject *pstderr;

        /* Set up a preliminary stderr printer until we have enough
           infrastructure for the io module in place. */
        pstderr = PyFile_NewStdPrinter(fileno(stderr));
        if (pstderr == NULL) {
            return _Py_INIT_ERR("can't set preliminary stderr");
        }
        _PySys_SetObjectId(&PyId_stderr, pstderr);
        PySys_SetObject("__stderr__", pstderr);
        Py_DECREF(pstderr);

        err = _PyImportHooks_Init();
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        err = initimport(interp, sysmod);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        err = initexternalimport(interp);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        err = initfsencoding(interp);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        err = init_sys_streams(interp);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        err = add_main_module(interp);
        if (_Py_INIT_FAILED(err)) {
            return err;
        }

        if (core_config->site_import) {
            err = initsite();
            if (_Py_INIT_FAILED(err)) {
                return err;
            }
        }
    }

    if (PyErr_Occurred()) {
        goto handle_error;
    }

    *tstate_p = tstate;
    return _Py_INIT_OK();

handle_error:
    /* Oops, it didn't work.  Undo it all. */

    PyErr_PrintEx(0);
    PyThreadState_Clear(tstate);
    PyThreadState_Swap(save_tstate);
    PyThreadState_Delete(tstate);
    PyInterpreterState_Delete(interp);

    *tstate_p = NULL;
    return _Py_INIT_OK();
}

PyThreadState *
Py_NewInterpreter(void)
{
    PyThreadState *tstate;
    _PyInitError err = new_interpreter(&tstate);
    if (_Py_INIT_FAILED(err)) {
        _Py_FatalInitError(err);
    }
    return tstate;

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

    call_py_exitfuncs(interp);

    if (tstate != interp->tstate_head || tstate->next != NULL)
        Py_FatalError("Py_EndInterpreter: not the last thread");

    PyImport_Cleanup();
    PyInterpreterState_Clear(interp);
    PyThreadState_Swap(NULL);
    PyInterpreterState_Delete(interp);
}

/* Add the __main__ module */

static _PyInitError
add_main_module(PyInterpreterState *interp)
{
    PyObject *m, *d, *loader, *ann_dict;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return _Py_INIT_ERR("can't create __main__ module");

    d = PyModule_GetDict(m);
    ann_dict = PyDict_New();
    if ((ann_dict == NULL) ||
        (PyDict_SetItemString(d, "__annotations__", ann_dict) < 0)) {
        return _Py_INIT_ERR("Failed to initialize __main__.__annotations__");
    }
    Py_DECREF(ann_dict);

    if (PyDict_GetItemString(d, "__builtins__") == NULL) {
        PyObject *bimod = PyImport_ImportModule("builtins");
        if (bimod == NULL) {
            return _Py_INIT_ERR("Failed to retrieve builtins module");
        }
        if (PyDict_SetItemString(d, "__builtins__", bimod) < 0) {
            return _Py_INIT_ERR("Failed to initialize __main__.__builtins__");
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
            return _Py_INIT_ERR("Failed to retrieve BuiltinImporter");
        }
        if (PyDict_SetItemString(d, "__loader__", loader) < 0) {
            return _Py_INIT_ERR("Failed to initialize __main__.__loader__");
        }
        Py_DECREF(loader);
    }
    return _Py_INIT_OK();
}

static _PyInitError
initfsencoding(PyInterpreterState *interp)
{
    PyObject *codec;

#ifdef MS_WINDOWS
    if (Py_LegacyWindowsFSEncodingFlag) {
        Py_FileSystemDefaultEncoding = "mbcs";
        Py_FileSystemDefaultEncodeErrors = "replace";
    }
    else {
        Py_FileSystemDefaultEncoding = "utf-8";
        Py_FileSystemDefaultEncodeErrors = "surrogatepass";
    }
#else
    if (Py_FileSystemDefaultEncoding == NULL &&
        interp->core_config.utf8_mode)
    {
        Py_FileSystemDefaultEncoding = "utf-8";
        Py_HasFileSystemDefaultEncoding = 1;
    }
    else if (Py_FileSystemDefaultEncoding == NULL) {
        Py_FileSystemDefaultEncoding = get_locale_encoding();
        if (Py_FileSystemDefaultEncoding == NULL) {
            return _Py_INIT_ERR("Unable to get the locale encoding");
        }

        Py_HasFileSystemDefaultEncoding = 0;
        interp->fscodec_initialized = 1;
        return _Py_INIT_OK();
    }
#endif

    /* the encoding is mbcs, utf-8 or ascii */
    codec = _PyCodec_Lookup(Py_FileSystemDefaultEncoding);
    if (!codec) {
        /* Such error can only occurs in critical situations: no more
         * memory, import a module of the standard library failed,
         * etc. */
        return _Py_INIT_ERR("unable to load the file system codec");
    }
    Py_DECREF(codec);
    interp->fscodec_initialized = 1;
    return _Py_INIT_OK();
}

/* Import the site module (not into __main__ though) */

static _PyInitError
initsite(void)
{
    PyObject *m;
    m = PyImport_ImportModule("site");
    if (m == NULL) {
        return _Py_INIT_USER_ERR("Failed to import the site module");
    }
    Py_DECREF(m);
    return _Py_INIT_OK();
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
    PyObject *line_buffering, *write_through;
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
    if (Py_UnbufferedStdioFlag)
        write_through = Py_True;
    else
        write_through = Py_False;
    if (isatty && !Py_UnbufferedStdioFlag)
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

    stream = _PyObject_CallMethodId(io, &PyId_TextIOWrapper, "OsssOO",
                                    buf, encoding, errors,
                                    newline, line_buffering, write_through);
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
static _PyInitError
init_sys_streams(PyInterpreterState *interp)
{
    PyObject *iomod = NULL, *wrapper;
    PyObject *bimod = NULL;
    PyObject *m;
    PyObject *std = NULL;
    int fd;
    PyObject * encoding_attr;
    char *pythonioencoding = NULL;
    const char *encoding, *errors;
    _PyInitError res = _Py_INIT_OK();

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
        char *opt = Py_GETENV("PYTHONIOENCODING");
        if (opt && opt[0] != '\0') {
            char *err;
            pythonioencoding = _PyMem_Strdup(opt);
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
        else if (interp->core_config.utf8_mode) {
            encoding = "utf-8";
            errors = "surrogateescape";
        }

        if (!errors && !pythonioencoding) {
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

    goto done;

error:
    res = _Py_INIT_ERR("can't initialize sys standard streams");

    /* Use the same allocator than Py_SetStandardStreamEncoding() */
    PyMemAllocatorEx old_alloc;
done:
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

    PyMem_Free(pythonioencoding);
    Py_XDECREF(bimod);
    Py_XDECREF(iomod);
    return res;
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

#ifdef MS_WINDOWS
static void
fatal_output_debug(const char *msg)
{
    /* buffer of 256 bytes allocated on the stack */
    WCHAR buffer[256 / sizeof(WCHAR)];
    size_t buflen = Py_ARRAY_LENGTH(buffer) - 1;
    size_t msglen;

    OutputDebugStringW(L"Fatal Python error: ");

    msglen = strlen(msg);
    while (msglen) {
        size_t i;

        if (buflen > msglen) {
            buflen = msglen;
        }

        /* Convert the message to wchar_t. This uses a simple one-to-one
           conversion, assuming that the this error message actually uses
           ASCII only. If this ceases to be true, we will have to convert. */
        for (i=0; i < buflen; ++i) {
            buffer[i] = msg[i];
        }
        buffer[i] = L'\0';
        OutputDebugStringW(buffer);

        msg += buflen;
        msglen -= buflen;
    }
    OutputDebugStringW(L"\n");
}
#endif

static void _Py_NO_RETURN
fatal_error(const char *prefix, const char *msg, int status)
{
    const int fd = fileno(stderr);
    static int reentrant = 0;

    if (reentrant) {
        /* Py_FatalError() caused a second fatal error.
           Example: flush_std_files() raises a recursion error. */
        goto exit;
    }
    reentrant = 1;

    fprintf(stderr, "Fatal Python error: ");
    if (prefix) {
        fputs(prefix, stderr);
        fputs(": ", stderr);
    }
    if (msg) {
        fputs(msg, stderr);
    }
    else {
        fprintf(stderr, "<message not set>");
    }
    fputs("\n", stderr);
    fflush(stderr); /* it helps in Windows debug build */

    /* Print the exception (if an exception is set) with its traceback,
     * or display the current Python stack. */
    if (!_Py_FatalError_PrintExc(fd)) {
        _Py_FatalError_DumpTracebacks(fd);
    }

    /* The main purpose of faulthandler is to display the traceback.
       This function already did its best to display a traceback.
       Disable faulthandler to prevent writing a second traceback
       on abort(). */
    _PyFaulthandler_Fini();

    /* Check if the current Python thread hold the GIL */
    if (PyThreadState_GET() != NULL) {
        /* Flush sys.stdout and sys.stderr */
        flush_std_files();
    }

#ifdef MS_WINDOWS
    fatal_output_debug(msg);
#endif /* MS_WINDOWS */

exit:
    if (status < 0) {
#if defined(MS_WINDOWS) && defined(_DEBUG)
        DebugBreak();
#endif
        abort();
    }
    else {
        exit(status);
    }
}

void _Py_NO_RETURN
Py_FatalError(const char *msg)
{
    fatal_error(NULL, msg, -1);
}

void _Py_NO_RETURN
_Py_FatalInitError(_PyInitError err)
{
    /* On "user" error: exit with status 1.
       For all other errors, call abort(). */
    int status = err.user_err ? 1 : -1;
    fatal_error(err.prefix, err.msg, status);
}

/* Clean up and exit */

#  include "pythread.h"

/* For the atexit module. */
void _Py_PyAtExit(void (*func)(PyObject *), PyObject *module)
{
    PyThreadState *ts;
    PyInterpreterState *is;

    ts = PyThreadState_GET();
    is = ts->interp;

    /* Guard against API misuse (see bpo-17852) */
    assert(is->pyexitfunc == NULL || is->pyexitfunc == func);

    is->pyexitfunc = func;
    is->pyexitmodule = module;
}

static void
call_py_exitfuncs(PyInterpreterState *istate)
{
    if (istate->pyexitfunc == NULL)
        return;

    (*istate->pyexitfunc)(istate->pyexitmodule);
    PyErr_Clear();
}

/* Wait until threading._shutdown completes, provided
   the threading module was imported in the first place.
   The shutdown routine will wait until all non-daemon
   "threading" threads have completed. */
static void
wait_for_thread_shutdown(void)
{
    _Py_IDENTIFIER(_shutdown);
    PyObject *result;
    PyObject *threading = _PyImport_GetModuleId(&PyId_threading);
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
}

#define NEXITFUNCS 32
int Py_AtExit(void (*func)(void))
{
    if (_PyRuntime.nexitfuncs >= NEXITFUNCS)
        return -1;
    _PyRuntime.exitfuncs[_PyRuntime.nexitfuncs++] = func;
    return 0;
}

static void
call_ll_exitfuncs(void)
{
    while (_PyRuntime.nexitfuncs > 0)
        (*_PyRuntime.exitfuncs[--_PyRuntime.nexitfuncs])();

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

static _PyInitError
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
        return _Py_INIT_ERR("can't import signal");
    }
    return _Py_INIT_OK();
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
