
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

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef MS_WINDOWS
#include "malloc.h" /* for alloca */
#endif

#ifdef HAVE_LANGINFO_H
#include <locale.h>
#include <langinfo.h>
#endif

#ifdef MS_WINDOWS
#undef BYTE
#include "windows.h"
#define PATH_MAX MAXPATHLEN
#endif

#ifdef __gnu_hurd__
#define PATH_MAX MAXPATHLEN
#endif

_Py_IDENTIFIER(builtins);
_Py_IDENTIFIER(excepthook);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(last_traceback);
_Py_IDENTIFIER(last_type);
_Py_IDENTIFIER(last_value);
_Py_IDENTIFIER(name);
_Py_IDENTIFIER(ps1);
_Py_IDENTIFIER(ps2);
_Py_IDENTIFIER(stdin);
_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(stderr);
_Py_static_string(PyId_string, "<string>");

#ifdef Py_REF_DEBUG
static
void _print_total_refs(void) {
    PyObject *xoptions, *value;
    _Py_IDENTIFIER(showrefcount);

    xoptions = PySys_GetXOptions();
    if (xoptions == NULL)
        return;
    value = _PyDict_GetItemId(xoptions, &PyId_showrefcount);
    if (value == Py_True)
        fprintf(stderr,
                "[%" PY_FORMAT_SIZE_T "d refs, "
                "%" PY_FORMAT_SIZE_T "d blocks]\n",
                _Py_GetRefTotal(), _Py_GetAllocatedBlocks());
}
#endif

#ifndef Py_REF_DEBUG
#define PRINT_TOTAL_REFS()
#else /* Py_REF_DEBUG */
#define PRINT_TOTAL_REFS() _print_total_refs()
#endif

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
static void flush_io(void);
static PyObject *run_mod(mod_ty, PyObject *, PyObject *, PyObject *,
                          PyCompilerFlags *, PyArena *);
static PyObject *run_pyc_file(FILE *, const char *, PyObject *, PyObject *,
                              PyCompilerFlags *);
static void err_input(perrdetail *);
static void err_free(perrdetail *);
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

#ifdef WITH_THREAD
extern void _PyGILState_Init(PyInterpreterState *, PyThreadState *);
extern void _PyGILState_Fini(void);
#endif /* WITH_THREAD */

int Py_DebugFlag; /* Needed by parser.c */
int Py_VerboseFlag; /* Needed by import.c */
int Py_QuietFlag; /* Needed by sysmodule.c */
int Py_InteractiveFlag; /* Needed by Py_FdIsInteractive() below */
int Py_InspectFlag; /* Needed to determine whether to exit at SystemExit */
int Py_NoSiteFlag; /* Suppress 'import site' */
int Py_BytesWarningFlag; /* Warn on str(bytes) and str(buffer) */
int Py_DontWriteBytecodeFlag; /* Suppress writing bytecode files (*.py[co]) */
int Py_UseClassExceptionsFlag = 1; /* Needed by bltinmodule.c: deprecated */
int Py_FrozenFlag; /* Needed by getpath.c */
int Py_IgnoreEnvironmentFlag; /* e.g. PYTHONPATH, PYTHONHOME */
int Py_NoUserSiteDirectory = 0; /* for -s and site.py */
int Py_UnbufferedStdioFlag = 0; /* Unbuffered binary std{in,out,err} */
int Py_HashRandomizationFlag = 0; /* for -R and PYTHONHASHSEED */
int Py_IsolatedFlag = 0; /* for -I, isolate from user's env */

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

static int initialized = 0;

/* API to access the initialized flag -- useful for esoteric use */

int
Py_IsInitialized(void)
{
    return initialized;
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
    return 0;
}

/* Global initializations.  Can be undone by Py_Finalize().  Don't
   call this twice without an intervening Py_Finalize() call.  When
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
    char *name_utf8, *name_str;
    PyObject *codec, *name = NULL;

    codec = _PyCodec_Lookup(encoding);
    if (!codec)
        goto error;

    name = _PyObject_GetAttrId(codec, &PyId_name);
    Py_CLEAR(codec);
    if (!name)
        goto error;

    name_utf8 = _PyUnicode_AsString(name);
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
#else
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
#endif
}

static void
import_init(PyInterpreterState *interp, PyObject *sysmod)
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

    /* Install _importlib as __import__ */
    impmod = PyInit_imp();
    if (impmod == NULL) {
        Py_FatalError("Py_Initialize: can't import imp");
    }
    else if (Py_VerboseFlag) {
        PySys_FormatStderr("import imp # builtin\n");
    }
    sys_modules = PyImport_GetModuleDict();
    if (Py_VerboseFlag) {
        PySys_FormatStderr("import sys # builtin\n");
    }
    if (PyDict_SetItemString(sys_modules, "_imp", impmod) < 0) {
        Py_FatalError("Py_Initialize: can't save _imp to sys.modules");
    }

    value = PyObject_CallMethod(importlib, "_install", "OO", sysmod, impmod);
    if (value == NULL) {
        PyErr_Print();
        Py_FatalError("Py_Initialize: importlib install failed");
    }
    Py_DECREF(value);
    Py_DECREF(impmod);

    _PyImportZip_Init();
}


void
_Py_InitializeEx_Private(int install_sigs, int install_importlib)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;
    PyObject *bimod, *sysmod, *pstderr;
    char *p;
    extern void _Py_ReadyTypes(void);

    if (initialized)
        return;
    initialized = 1;
    _Py_Finalizing = NULL;

#if defined(HAVE_LANGINFO_H) && defined(HAVE_SETLOCALE)
    /* Set up the LC_CTYPE locale, so we can obtain
       the locale's charset without having to switch
       locales. */
    setlocale(LC_CTYPE, "");
#endif

    if ((p = Py_GETENV("PYTHONDEBUG")) && *p != '\0')
        Py_DebugFlag = add_flag(Py_DebugFlag, p);
    if ((p = Py_GETENV("PYTHONVERBOSE")) && *p != '\0')
        Py_VerboseFlag = add_flag(Py_VerboseFlag, p);
    if ((p = Py_GETENV("PYTHONOPTIMIZE")) && *p != '\0')
        Py_OptimizeFlag = add_flag(Py_OptimizeFlag, p);
    if ((p = Py_GETENV("PYTHONDONTWRITEBYTECODE")) && *p != '\0')
        Py_DontWriteBytecodeFlag = add_flag(Py_DontWriteBytecodeFlag, p);
    /* The variable is only tested for existence here; _PyRandom_Init will
       check its value further. */
    if ((p = Py_GETENV("PYTHONHASHSEED")) && *p != '\0')
        Py_HashRandomizationFlag = add_flag(Py_HashRandomizationFlag, p);

    _PyRandom_Init();

    interp = PyInterpreterState_New();
    if (interp == NULL)
        Py_FatalError("Py_Initialize: can't make first interpreter");

    tstate = PyThreadState_New(interp);
    if (tstate == NULL)
        Py_FatalError("Py_Initialize: can't make first thread");
    (void) PyThreadState_Swap(tstate);

#ifdef WITH_THREAD
    /* We can't call _PyEval_FiniThreads() in Py_Finalize because
       destroying the GIL might fail when it is being referenced from
       another running thread (see issue #9901).
       Instead we destroy the previously created GIL here, which ensures
       that we can call Py_Initialize / Py_Finalize multiple times. */
    _PyEval_FiniThreads();

    /* Auto-thread-state API */
    _PyGILState_Init(interp, tstate);
#endif /* WITH_THREAD */

    _Py_ReadyTypes();

    if (!_PyFrame_Init())
        Py_FatalError("Py_Initialize: can't init frames");

    if (!_PyLong_Init())
        Py_FatalError("Py_Initialize: can't init longs");

    if (!PyByteArray_Init())
        Py_FatalError("Py_Initialize: can't init bytearray");

    if (!_PyFloat_Init())
        Py_FatalError("Py_Initialize: can't init float");

    interp->modules = PyDict_New();
    if (interp->modules == NULL)
        Py_FatalError("Py_Initialize: can't make modules dictionary");

    /* Init Unicode implementation; relies on the codec registry */
    if (_PyUnicode_Init() < 0)
        Py_FatalError("Py_Initialize: can't initialize unicode");
    if (_PyStructSequence_Init() < 0)
        Py_FatalError("Py_Initialize: can't initialize structseq");

    bimod = _PyBuiltin_Init();
    if (bimod == NULL)
        Py_FatalError("Py_Initialize: can't initialize builtins modules");
    _PyImport_FixupBuiltin(bimod, "builtins");
    interp->builtins = PyModule_GetDict(bimod);
    if (interp->builtins == NULL)
        Py_FatalError("Py_Initialize: can't initialize builtins dict");
    Py_INCREF(interp->builtins);

    /* initialize builtin exceptions */
    _PyExc_Init(bimod);

    sysmod = _PySys_Init();
    if (sysmod == NULL)
        Py_FatalError("Py_Initialize: can't initialize sys");
    interp->sysdict = PyModule_GetDict(sysmod);
    if (interp->sysdict == NULL)
        Py_FatalError("Py_Initialize: can't initialize sys dict");
    Py_INCREF(interp->sysdict);
    _PyImport_FixupBuiltin(sysmod, "sys");
    PySys_SetPath(Py_GetPath());
    PyDict_SetItemString(interp->sysdict, "modules",
                         interp->modules);

    /* Set up a preliminary stderr printer until we have enough
       infrastructure for the io module in place. */
    pstderr = PyFile_NewStdPrinter(fileno(stderr));
    if (pstderr == NULL)
        Py_FatalError("Py_Initialize: can't set preliminary stderr");
    _PySys_SetObjectId(&PyId_stderr, pstderr);
    PySys_SetObject("__stderr__", pstderr);
    Py_DECREF(pstderr);

    _PyImport_Init();

    _PyImportHooks_Init();

    /* Initialize _warnings. */
    _PyWarnings_Init();

    if (!install_importlib)
        return;

    import_init(interp, sysmod);

    /* initialize the faulthandler module */
    if (_PyFaulthandler_Init())
        Py_FatalError("Py_Initialize: can't initialize faulthandler");

    _PyTime_Init();

    if (initfsencoding(interp) < 0)
        Py_FatalError("Py_Initialize: unable to load the file system codec");

    if (install_sigs)
        initsigs(); /* Signal handling stuff, including initintr() */

    if (_PyTraceMalloc_Init() < 0)
        Py_FatalError("Py_Initialize: can't initialize tracemalloc");

    initmain(interp); /* Module __main__ */
    if (initstdio() < 0)
        Py_FatalError(
            "Py_Initialize: can't initialize sys standard streams");

    /* Initialize warnings. */
    if (PySys_HasWarnOptions()) {
        PyObject *warnings_module = PyImport_ImportModule("warnings");
        if (warnings_module == NULL) {
            fprintf(stderr, "'import warnings' failed; traceback:\n");
            PyErr_Print();
        }
        Py_XDECREF(warnings_module);
    }

    if (!Py_NoSiteFlag)
        initsite(); /* Module site */
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

static void
flush_std_files(void)
{
    PyObject *fout = _PySys_GetObjectId(&PyId_stdout);
    PyObject *ferr = _PySys_GetObjectId(&PyId_stderr);
    PyObject *tmp;

    if (fout != NULL && fout != Py_None && !file_is_closed(fout)) {
        tmp = _PyObject_CallMethodId(fout, &PyId_flush, "");
        if (tmp == NULL)
            PyErr_WriteUnraisable(fout);
        else
            Py_DECREF(tmp);
    }

    if (ferr != NULL && ferr != Py_None && !file_is_closed(ferr)) {
        tmp = _PyObject_CallMethodId(ferr, &PyId_flush, "");
        if (tmp == NULL)
            PyErr_Clear();
        else
            Py_DECREF(tmp);
    }
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

void
Py_Finalize(void)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;

    if (!initialized)
        return;

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
    initialized = 0;

    /* Flush stdout+stderr */
    flush_std_files();

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
    PyGC_Collect();
#ifdef COUNT_ALLOCS
    /* With COUNT_ALLOCS, it helps to run GC multiple times:
       each collection might release some types from the type
       list, so they become garbage. */
    while (PyGC_Collect() > 0)
        /* nothing */;
#endif
    /* Destroy all modules */
    PyImport_Cleanup();

    /* Flush stdout+stderr (again, in case more was printed) */
    flush_std_files();

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
    PyGC_Collect();
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
    dump_counts(stdout);
#endif
    /* dump hash stats */
    _PyHash_Fini();

    PRINT_TOTAL_REFS();

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
    _PyRandom_Fini();

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
#ifdef PYMALLOC_DEBUG
    if (Py_GETENV("PYTHONMALLOCSTATS"))
        _PyObject_DebugMallocStats(stderr);
#endif

    call_ll_exitfuncs();
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

    if (!initialized)
        Py_FatalError("Py_NewInterpreter: call Py_Initialize first");

    interp = PyInterpreterState_New();
    if (interp == NULL)
        return NULL;

    tstate = PyThreadState_New(interp);
    if (tstate == NULL) {
        PyInterpreterState_Delete(interp);
        return NULL;
    }

    save_tstate = PyThreadState_Swap(tstate);

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
        PySys_SetPath(Py_GetPath());
        PyDict_SetItemString(interp->sysdict, "modules",
                             interp->modules);
        /* Set up a preliminary stderr printer until we have enough
           infrastructure for the io module in place. */
        pstderr = PyFile_NewStdPrinter(fileno(stderr));
        if (pstderr == NULL)
            Py_FatalError("Py_Initialize: can't set preliminary stderr");
        _PySys_SetObjectId(&PyId_stderr, pstderr);
        PySys_SetObject("__stderr__", pstderr);
        Py_DECREF(pstderr);

        _PyImportHooks_Init();

        import_init(interp, sysmod);

        if (initfsencoding(interp) < 0)
            goto handle_error;

        if (initstdio() < 0)
            Py_FatalError(
            "Py_Initialize: can't initialize sys standard streams");
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

   (Py_Finalize() doesn't have these constraints -- it zaps
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
    PyObject *m, *d, *loader;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        Py_FatalError("can't create __main__ module");
    d = PyModule_GetDict(m);
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

    if (Py_FileSystemDefaultEncoding == NULL)
    {
        Py_FileSystemDefaultEncoding = get_locale_encoding();
        if (Py_FileSystemDefaultEncoding == NULL)
            Py_FatalError("Py_Initialize: Unable to get the locale encoding");

        Py_HasFileSystemDefaultEncoding = 0;
        interp->fscodec_initialized = 1;
        return 0;
    }

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

static PyObject*
create_stdio(PyObject* io,
    int fd, int write_mode, char* name,
    char* encoding, char* errors)
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
                                 Py_None, Py_None, Py_None, 0);
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

    text = PyUnicode_FromString(name);
    if (text == NULL || _PyObject_SetAttrId(raw, &PyId_name, text) < 0)
        goto error;
    res = _PyObject_CallMethodId(raw, &PyId_isatty, "");
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
    return NULL;
}

static int
is_valid_fd(int fd)
{
    int dummy_fd;
    if (fd < 0 || !_PyVerify_fd(fd))
        return 0;
    dummy_fd = dup(fd);
    if (dummy_fd < 0)
        return 0;
    close(dummy_fd);
    return 1;
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
    }

    /* Set sys.stdin */
    fd = fileno(stdin);
    /* Under some conditions stdin, stdout and stderr may not be connected
     * and fileno() may point to an invalid file descriptor. For example
     * GUI apps don't have valid standard streams by default.
     */
    if (!is_valid_fd(fd)) {
        std = Py_None;
        Py_INCREF(std);
    }
    else {
        std = create_stdio(iomod, fd, 0, "<stdin>", encoding, errors);
        if (std == NULL)
            goto error;
    } /* if (fd < 0) */
    PySys_SetObject("__stdin__", std);
    _PySys_SetObjectId(&PyId_stdin, std);
    Py_DECREF(std);

    /* Set sys.stdout */
    fd = fileno(stdout);
    if (!is_valid_fd(fd)) {
        std = Py_None;
        Py_INCREF(std);
    }
    else {
        std = create_stdio(iomod, fd, 1, "<stdout>", encoding, errors);
        if (std == NULL)
            goto error;
    } /* if (fd < 0) */
    PySys_SetObject("__stdout__", std);
    _PySys_SetObjectId(&PyId_stdout, std);
    Py_DECREF(std);

#if 1 /* Disable this if you have trouble debugging bootstrap stuff */
    /* Set sys.stderr, replaces the preliminary stderr */
    fd = fileno(stderr);
    if (!is_valid_fd(fd)) {
        std = Py_None;
        Py_INCREF(std);
    }
    else {
        std = create_stdio(iomod, fd, 1, "<stderr>", encoding, "backslashreplace");
        if (std == NULL)
            goto error;
    } /* if (fd < 0) */

    /* Same as hack above, pre-import stderr's codec to avoid recursion
       when import.c tries to write to stderr in verbose mode. */
    encoding_attr = PyObject_GetAttrString(std, "encoding");
    if (encoding_attr != NULL) {
        const char * std_encoding;
        std_encoding = _PyUnicode_AsString(encoding_attr);
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

/* Parse input from a file and execute it */

int
PyRun_AnyFileExFlags(FILE *fp, const char *filename, int closeit,
                     PyCompilerFlags *flags)
{
    if (filename == NULL)
        filename = "???";
    if (Py_FdIsInteractive(fp, filename)) {
        int err = PyRun_InteractiveLoopFlags(fp, filename, flags);
        if (closeit)
            fclose(fp);
        return err;
    }
    else
        return PyRun_SimpleFileExFlags(fp, filename, closeit, flags);
}

int
PyRun_InteractiveLoopFlags(FILE *fp, const char *filename_str, PyCompilerFlags *flags)
{
    PyObject *filename, *v;
    int ret, err;
    PyCompilerFlags local_flags;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL) {
        PyErr_Print();
        return -1;
    }

    if (flags == NULL) {
        flags = &local_flags;
        local_flags.cf_flags = 0;
    }
    v = _PySys_GetObjectId(&PyId_ps1);
    if (v == NULL) {
        _PySys_SetObjectId(&PyId_ps1, v = PyUnicode_FromString(">>> "));
        Py_XDECREF(v);
    }
    v = _PySys_GetObjectId(&PyId_ps2);
    if (v == NULL) {
        _PySys_SetObjectId(&PyId_ps2, v = PyUnicode_FromString("... "));
        Py_XDECREF(v);
    }
    err = -1;
    for (;;) {
        ret = PyRun_InteractiveOneObject(fp, filename, flags);
        PRINT_TOTAL_REFS();
        if (ret == E_EOF) {
            err = 0;
            break;
        }
        /*
        if (ret == E_NOMEM)
            break;
        */
    }
    Py_DECREF(filename);
    return err;
}

/* compute parser flags based on compiler flags */
static int PARSER_FLAGS(PyCompilerFlags *flags)
{
    int parser_flags = 0;
    if (!flags)
        return 0;
    if (flags->cf_flags & PyCF_DONT_IMPLY_DEDENT)
        parser_flags |= PyPARSE_DONT_IMPLY_DEDENT;
    if (flags->cf_flags & PyCF_IGNORE_COOKIE)
        parser_flags |= PyPARSE_IGNORE_COOKIE;
    if (flags->cf_flags & CO_FUTURE_BARRY_AS_BDFL)
        parser_flags |= PyPARSE_BARRY_AS_BDFL;
    return parser_flags;
}

#if 0
/* Keep an example of flags with future keyword support. */
#define PARSER_FLAGS(flags) \
    ((flags) ? ((((flags)->cf_flags & PyCF_DONT_IMPLY_DEDENT) ? \
                  PyPARSE_DONT_IMPLY_DEDENT : 0) \
                | ((flags)->cf_flags & CO_FUTURE_WITH_STATEMENT ? \
                   PyPARSE_WITH_IS_KEYWORD : 0)) : 0)
#endif

int
PyRun_InteractiveOneObject(FILE *fp, PyObject *filename, PyCompilerFlags *flags)
{
    PyObject *m, *d, *v, *w, *oenc = NULL, *mod_name;
    mod_ty mod;
    PyArena *arena;
    char *ps1 = "", *ps2 = "", *enc = NULL;
    int errcode = 0;
    _Py_IDENTIFIER(encoding);
    _Py_IDENTIFIER(__main__);

    mod_name = _PyUnicode_FromId(&PyId___main__); /* borrowed */
    if (mod_name == NULL) {
        PyErr_Print();
        return -1;
    }

    if (fp == stdin) {
        /* Fetch encoding from sys.stdin if possible. */
        v = _PySys_GetObjectId(&PyId_stdin);
        if (v && v != Py_None) {
            oenc = _PyObject_GetAttrId(v, &PyId_encoding);
            if (oenc)
                enc = _PyUnicode_AsString(oenc);
            if (!enc)
                PyErr_Clear();
        }
    }
    v = _PySys_GetObjectId(&PyId_ps1);
    if (v != NULL) {
        v = PyObject_Str(v);
        if (v == NULL)
            PyErr_Clear();
        else if (PyUnicode_Check(v)) {
            ps1 = _PyUnicode_AsString(v);
            if (ps1 == NULL) {
                PyErr_Clear();
                ps1 = "";
            }
        }
    }
    w = _PySys_GetObjectId(&PyId_ps2);
    if (w != NULL) {
        w = PyObject_Str(w);
        if (w == NULL)
            PyErr_Clear();
        else if (PyUnicode_Check(w)) {
            ps2 = _PyUnicode_AsString(w);
            if (ps2 == NULL) {
                PyErr_Clear();
                ps2 = "";
            }
        }
    }
    arena = PyArena_New();
    if (arena == NULL) {
        Py_XDECREF(v);
        Py_XDECREF(w);
        Py_XDECREF(oenc);
        return -1;
    }
    mod = PyParser_ASTFromFileObject(fp, filename, enc,
                                     Py_single_input, ps1, ps2,
                                     flags, &errcode, arena);
    Py_XDECREF(v);
    Py_XDECREF(w);
    Py_XDECREF(oenc);
    if (mod == NULL) {
        PyArena_Free(arena);
        if (errcode == E_EOF) {
            PyErr_Clear();
            return E_EOF;
        }
        PyErr_Print();
        return -1;
    }
    m = PyImport_AddModuleObject(mod_name);
    if (m == NULL) {
        PyArena_Free(arena);
        return -1;
    }
    d = PyModule_GetDict(m);
    v = run_mod(mod, filename, d, d, flags, arena);
    PyArena_Free(arena);
    if (v == NULL) {
        PyErr_Print();
        flush_io();
        return -1;
    }
    Py_DECREF(v);
    flush_io();
    return 0;
}

int
PyRun_InteractiveOneFlags(FILE *fp, const char *filename_str, PyCompilerFlags *flags)
{
    PyObject *filename;
    int res;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL) {
        PyErr_Print();
        return -1;
    }
    res = PyRun_InteractiveOneObject(fp, filename, flags);
    Py_DECREF(filename);
    return res;
}


/* Check whether a file maybe a pyc file: Look at the extension,
   the file type, and, if we may close it, at the first few bytes. */

static int
maybe_pyc_file(FILE *fp, const char* filename, const char* ext, int closeit)
{
    if (strcmp(ext, ".pyc") == 0 || strcmp(ext, ".pyo") == 0)
        return 1;

    /* Only look into the file if we are allowed to close it, since
       it then should also be seekable. */
    if (closeit) {
        /* Read only two bytes of the magic. If the file was opened in
           text mode, the bytes 3 and 4 of the magic (\r\n) might not
           be read as they are on disk. */
        unsigned int halfmagic = PyImport_GetMagicNumber() & 0xFFFF;
        unsigned char buf[2];
        /* Mess:  In case of -x, the stream is NOT at its start now,
           and ungetc() was used to push back the first newline,
           which makes the current stream position formally undefined,
           and a x-platform nightmare.
           Unfortunately, we have no direct way to know whether -x
           was specified.  So we use a terrible hack:  if the current
           stream position is not 0, we assume -x was specified, and
           give up.  Bug 132850 on SourceForge spells out the
           hopelessness of trying anything else (fseek and ftell
           don't work predictably x-platform for text-mode files).
        */
        int ispyc = 0;
        if (ftell(fp) == 0) {
            if (fread(buf, 1, 2, fp) == 2 &&
                ((unsigned int)buf[1]<<8 | buf[0]) == halfmagic)
                ispyc = 1;
            rewind(fp);
        }
        return ispyc;
    }
    return 0;
}

static int
set_main_loader(PyObject *d, const char *filename, const char *loader_name)
{
    PyInterpreterState *interp;
    PyThreadState *tstate;
    PyObject *filename_obj, *loader_type, *loader;
    int result = 0;

    filename_obj = PyUnicode_DecodeFSDefault(filename);
    if (filename_obj == NULL)
        return -1;
    /* Get current thread state and interpreter pointer */
    tstate = PyThreadState_GET();
    interp = tstate->interp;
    loader_type = PyObject_GetAttrString(interp->importlib, loader_name);
    if (loader_type == NULL) {
        Py_DECREF(filename_obj);
        return -1;
    }
    loader = PyObject_CallFunction(loader_type, "sN", "__main__", filename_obj);
    Py_DECREF(loader_type);
    if (loader == NULL) {
        return -1;
    }
    if (PyDict_SetItemString(d, "__loader__", loader) < 0) {
        result = -1;
    }
    Py_DECREF(loader);
    return result;
}

int
PyRun_SimpleFileExFlags(FILE *fp, const char *filename, int closeit,
                        PyCompilerFlags *flags)
{
    PyObject *m, *d, *v;
    const char *ext;
    int set_file_name = 0, ret = -1;
    size_t len;

    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return -1;
    Py_INCREF(m);
    d = PyModule_GetDict(m);
    if (PyDict_GetItemString(d, "__file__") == NULL) {
        PyObject *f;
        f = PyUnicode_DecodeFSDefault(filename);
        if (f == NULL)
            goto done;
        if (PyDict_SetItemString(d, "__file__", f) < 0) {
            Py_DECREF(f);
            goto done;
        }
        if (PyDict_SetItemString(d, "__cached__", Py_None) < 0) {
            Py_DECREF(f);
            goto done;
        }
        set_file_name = 1;
        Py_DECREF(f);
    }
    len = strlen(filename);
    ext = filename + len - (len > 4 ? 4 : 0);
    if (maybe_pyc_file(fp, filename, ext, closeit)) {
        FILE *pyc_fp;
        /* Try to run a pyc file. First, re-open in binary */
        if (closeit)
            fclose(fp);
        if ((pyc_fp = _Py_fopen(filename, "rb")) == NULL) {
            fprintf(stderr, "python: Can't reopen .pyc file\n");
            goto done;
        }
        /* Turn on optimization if a .pyo file is given */
        if (strcmp(ext, ".pyo") == 0)
            Py_OptimizeFlag = 1;

        if (set_main_loader(d, filename, "SourcelessFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            fclose(pyc_fp);
            goto done;
        }
        v = run_pyc_file(pyc_fp, filename, d, d, flags);
        fclose(pyc_fp);
    } else {
        /* When running from stdin, leave __main__.__loader__ alone */
        if (strcmp(filename, "<stdin>") != 0 &&
            set_main_loader(d, filename, "SourceFileLoader") < 0) {
            fprintf(stderr, "python: failed to set __main__.__loader__\n");
            ret = -1;
            goto done;
        }
        v = PyRun_FileExFlags(fp, filename, Py_file_input, d, d,
                              closeit, flags);
    }
    flush_io();
    if (v == NULL) {
        PyErr_Print();
        goto done;
    }
    Py_DECREF(v);
    ret = 0;
  done:
    if (set_file_name && PyDict_DelItemString(d, "__file__"))
        PyErr_Clear();
    Py_DECREF(m);
    return ret;
}

int
PyRun_SimpleStringFlags(const char *command, PyCompilerFlags *flags)
{
    PyObject *m, *d, *v;
    m = PyImport_AddModule("__main__");
    if (m == NULL)
        return -1;
    d = PyModule_GetDict(m);
    v = PyRun_StringFlags(command, Py_file_input, d, d, flags);
    if (v == NULL) {
        PyErr_Print();
        return -1;
    }
    Py_DECREF(v);
    return 0;
}

static int
parse_syntax_error(PyObject *err, PyObject **message, PyObject **filename,
                   int *lineno, int *offset, PyObject **text)
{
    long hold;
    PyObject *v;
    _Py_IDENTIFIER(msg);
    _Py_IDENTIFIER(filename);
    _Py_IDENTIFIER(lineno);
    _Py_IDENTIFIER(offset);
    _Py_IDENTIFIER(text);

    *message = NULL;
    *filename = NULL;

    /* new style errors.  `err' is an instance */
    *message = _PyObject_GetAttrId(err, &PyId_msg);
    if (!*message)
        goto finally;

    v = _PyObject_GetAttrId(err, &PyId_filename);
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *filename = _PyUnicode_FromId(&PyId_string);
        if (*filename == NULL)
            goto finally;
        Py_INCREF(*filename);
    }
    else {
        *filename = v;
    }

    v = _PyObject_GetAttrId(err, &PyId_lineno);
    if (!v)
        goto finally;
    hold = PyLong_AsLong(v);
    Py_DECREF(v);
    if (hold < 0 && PyErr_Occurred())
        goto finally;
    *lineno = (int)hold;

    v = _PyObject_GetAttrId(err, &PyId_offset);
    if (!v)
        goto finally;
    if (v == Py_None) {
        *offset = -1;
        Py_DECREF(v);
    } else {
        hold = PyLong_AsLong(v);
        Py_DECREF(v);
        if (hold < 0 && PyErr_Occurred())
            goto finally;
        *offset = (int)hold;
    }

    v = _PyObject_GetAttrId(err, &PyId_text);
    if (!v)
        goto finally;
    if (v == Py_None) {
        Py_DECREF(v);
        *text = NULL;
    }
    else {
        *text = v;
    }
    return 1;

finally:
    Py_XDECREF(*message);
    Py_XDECREF(*filename);
    return 0;
}

void
PyErr_Print(void)
{
    PyErr_PrintEx(1);
}

static void
print_error_text(PyObject *f, int offset, PyObject *text_obj)
{
    char *text;
    char *nl;

    text = _PyUnicode_AsString(text_obj);
    if (text == NULL)
        return;

    if (offset >= 0) {
        if (offset > 0 && offset == strlen(text) && text[offset - 1] == '\n')
            offset--;
        for (;;) {
            nl = strchr(text, '\n');
            if (nl == NULL || nl-text >= offset)
                break;
            offset -= (int)(nl+1-text);
            text = nl+1;
        }
        while (*text == ' ' || *text == '\t') {
            text++;
            offset--;
        }
    }
    PyFile_WriteString("    ", f);
    PyFile_WriteString(text, f);
    if (*text == '\0' || text[strlen(text)-1] != '\n')
        PyFile_WriteString("\n", f);
    if (offset == -1)
        return;
    PyFile_WriteString("    ", f);
    while (--offset > 0)
        PyFile_WriteString(" ", f);
    PyFile_WriteString("^\n", f);
}

static void
handle_system_exit(void)
{
    PyObject *exception, *value, *tb;
    int exitcode = 0;

    if (Py_InspectFlag)
        /* Don't exit if -i flag was given. This flag is set to 0
         * when entering interactive mode for inspecting. */
        return;

    PyErr_Fetch(&exception, &value, &tb);
    fflush(stdout);
    if (value == NULL || value == Py_None)
        goto done;
    if (PyExceptionInstance_Check(value)) {
        /* The error code should be in the `code' attribute. */
        _Py_IDENTIFIER(code);
        PyObject *code = _PyObject_GetAttrId(value, &PyId_code);
        if (code) {
            Py_DECREF(value);
            value = code;
            if (value == Py_None)
                goto done;
        }
        /* If we failed to dig out the 'code' attribute,
           just let the else clause below print the error. */
    }
    if (PyLong_Check(value))
        exitcode = (int)PyLong_AsLong(value);
    else {
        PyObject *sys_stderr = _PySys_GetObjectId(&PyId_stderr);
        /* We clear the exception here to avoid triggering the assertion
         * in PyObject_Str that ensures it won't silently lose exception
         * details.
         */
        PyErr_Clear();
        if (sys_stderr != NULL && sys_stderr != Py_None) {
            PyFile_WriteObject(value, sys_stderr, Py_PRINT_RAW);
        } else {
            PyObject_Print(value, stderr, Py_PRINT_RAW);
            fflush(stderr);
        }
        PySys_WriteStderr("\n");
        exitcode = 1;
    }
 done:
    /* Restore and clear the exception info, in order to properly decref
     * the exception, value, and traceback.      If we just exit instead,
     * these leak, which confuses PYTHONDUMPREFS output, and may prevent
     * some finalizers from running.
     */
    PyErr_Restore(exception, value, tb);
    PyErr_Clear();
    Py_Exit(exitcode);
    /* NOTREACHED */
}

void
PyErr_PrintEx(int set_sys_last_vars)
{
    PyObject *exception, *v, *tb, *hook;

    if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
        handle_system_exit();
    }
    PyErr_Fetch(&exception, &v, &tb);
    if (exception == NULL)
        return;
    PyErr_NormalizeException(&exception, &v, &tb);
    if (tb == NULL) {
        tb = Py_None;
        Py_INCREF(tb);
    }
    PyException_SetTraceback(v, tb);
    if (exception == NULL)
        return;
    /* Now we know v != NULL too */
    if (set_sys_last_vars) {
        _PySys_SetObjectId(&PyId_last_type, exception);
        _PySys_SetObjectId(&PyId_last_value, v);
        _PySys_SetObjectId(&PyId_last_traceback, tb);
    }
    hook = _PySys_GetObjectId(&PyId_excepthook);
    if (hook) {
        PyObject *args = PyTuple_Pack(3, exception, v, tb);
        PyObject *result = PyEval_CallObject(hook, args);
        if (result == NULL) {
            PyObject *exception2, *v2, *tb2;
            if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
                handle_system_exit();
            }
            PyErr_Fetch(&exception2, &v2, &tb2);
            PyErr_NormalizeException(&exception2, &v2, &tb2);
            /* It should not be possible for exception2 or v2
               to be NULL. However PyErr_Display() can't
               tolerate NULLs, so just be safe. */
            if (exception2 == NULL) {
                exception2 = Py_None;
                Py_INCREF(exception2);
            }
            if (v2 == NULL) {
                v2 = Py_None;
                Py_INCREF(v2);
            }
            fflush(stdout);
            PySys_WriteStderr("Error in sys.excepthook:\n");
            PyErr_Display(exception2, v2, tb2);
            PySys_WriteStderr("\nOriginal exception was:\n");
            PyErr_Display(exception, v, tb);
            Py_DECREF(exception2);
            Py_DECREF(v2);
            Py_XDECREF(tb2);
        }
        Py_XDECREF(result);
        Py_XDECREF(args);
    } else {
        PySys_WriteStderr("sys.excepthook is missing\n");
        PyErr_Display(exception, v, tb);
    }
    Py_XDECREF(exception);
    Py_XDECREF(v);
    Py_XDECREF(tb);
}

static void
print_exception(PyObject *f, PyObject *value)
{
    int err = 0;
    PyObject *type, *tb;
    _Py_IDENTIFIER(print_file_and_line);

    if (!PyExceptionInstance_Check(value)) {
        err = PyFile_WriteString("TypeError: print_exception(): Exception expected for value, ", f);
        err += PyFile_WriteString(Py_TYPE(value)->tp_name, f);
        err += PyFile_WriteString(" found\n", f);
        if (err)
            PyErr_Clear();
        return;
    }

    Py_INCREF(value);
    fflush(stdout);
    type = (PyObject *) Py_TYPE(value);
    tb = PyException_GetTraceback(value);
    if (tb && tb != Py_None)
        err = PyTraceBack_Print(tb, f);
    if (err == 0 &&
        _PyObject_HasAttrId(value, &PyId_print_file_and_line))
    {
        PyObject *message, *filename, *text;
        int lineno, offset;
        if (!parse_syntax_error(value, &message, &filename,
                                &lineno, &offset, &text))
            PyErr_Clear();
        else {
            PyObject *line;

            Py_DECREF(value);
            value = message;

            line = PyUnicode_FromFormat("  File \"%U\", line %d\n",
                                          filename, lineno);
            Py_DECREF(filename);
            if (line != NULL) {
                PyFile_WriteObject(line, f, Py_PRINT_RAW);
                Py_DECREF(line);
            }

            if (text != NULL) {
                print_error_text(f, offset, text);
                Py_DECREF(text);
            }

            /* Can't be bothered to check all those
               PyFile_WriteString() calls */
            if (PyErr_Occurred())
                err = -1;
        }
    }
    if (err) {
        /* Don't do anything else */
    }
    else {
        PyObject* moduleName;
        char* className;
        _Py_IDENTIFIER(__module__);
        assert(PyExceptionClass_Check(type));
        className = PyExceptionClass_Name(type);
        if (className != NULL) {
            char *dot = strrchr(className, '.');
            if (dot != NULL)
                className = dot+1;
        }

        moduleName = _PyObject_GetAttrId(type, &PyId___module__);
        if (moduleName == NULL || !PyUnicode_Check(moduleName))
        {
            Py_XDECREF(moduleName);
            err = PyFile_WriteString("<unknown>", f);
        }
        else {
            if (_PyUnicode_CompareWithId(moduleName, &PyId_builtins) != 0)
            {
                err = PyFile_WriteObject(moduleName, f, Py_PRINT_RAW);
                err += PyFile_WriteString(".", f);
            }
            Py_DECREF(moduleName);
        }
        if (err == 0) {
            if (className == NULL)
                      err = PyFile_WriteString("<unknown>", f);
            else
                      err = PyFile_WriteString(className, f);
        }
    }
    if (err == 0 && (value != Py_None)) {
        PyObject *s = PyObject_Str(value);
        /* only print colon if the str() of the
           object is not the empty string
        */
        if (s == NULL)
            err = -1;
        else if (!PyUnicode_Check(s) ||
            PyUnicode_GetLength(s) != 0)
            err = PyFile_WriteString(": ", f);
        if (err == 0)
          err = PyFile_WriteObject(s, f, Py_PRINT_RAW);
        Py_XDECREF(s);
    }
    /* try to write a newline in any case */
    err += PyFile_WriteString("\n", f);
    Py_XDECREF(tb);
    Py_DECREF(value);
    /* If an error happened here, don't show it.
       XXX This is wrong, but too many callers rely on this behavior. */
    if (err != 0)
        PyErr_Clear();
}

static const char *cause_message =
    "\nThe above exception was the direct cause "
    "of the following exception:\n\n";

static const char *context_message =
    "\nDuring handling of the above exception, "
    "another exception occurred:\n\n";

static void
print_exception_recursive(PyObject *f, PyObject *value, PyObject *seen)
{
    int err = 0, res;
    PyObject *cause, *context;

    if (seen != NULL) {
        /* Exception chaining */
        if (PySet_Add(seen, value) == -1)
            PyErr_Clear();
        else if (PyExceptionInstance_Check(value)) {
            cause = PyException_GetCause(value);
            context = PyException_GetContext(value);
            if (cause) {
                res = PySet_Contains(seen, cause);
                if (res == -1)
                    PyErr_Clear();
                if (res == 0) {
                    print_exception_recursive(
                        f, cause, seen);
                    err |= PyFile_WriteString(
                        cause_message, f);
                }
            }
            else if (context &&
                !((PyBaseExceptionObject *)value)->suppress_context) {
                res = PySet_Contains(seen, context);
                if (res == -1)
                    PyErr_Clear();
                if (res == 0) {
                    print_exception_recursive(
                        f, context, seen);
                    err |= PyFile_WriteString(
                        context_message, f);
                }
            }
            Py_XDECREF(context);
            Py_XDECREF(cause);
        }
    }
    print_exception(f, value);
    if (err != 0)
        PyErr_Clear();
}

void
PyErr_Display(PyObject *exception, PyObject *value, PyObject *tb)
{
    PyObject *seen;
    PyObject *f = _PySys_GetObjectId(&PyId_stderr);
    if (PyExceptionInstance_Check(value)
        && tb != NULL && PyTraceBack_Check(tb)) {
        /* Put the traceback on the exception, otherwise it won't get
           displayed.  See issue #18776. */
        PyObject *cur_tb = PyException_GetTraceback(value);
        if (cur_tb == NULL)
            PyException_SetTraceback(value, tb);
        else
            Py_DECREF(cur_tb);
    }
    if (f == Py_None) {
        /* pass */
    }
    else if (f == NULL) {
        _PyObject_Dump(value);
        fprintf(stderr, "lost sys.stderr\n");
    }
    else {
        /* We choose to ignore seen being possibly NULL, and report
           at least the main exception (it could be a MemoryError).
        */
        seen = PySet_New(NULL);
        if (seen == NULL)
            PyErr_Clear();
        print_exception_recursive(f, value, seen);
        Py_XDECREF(seen);
    }
}

PyObject *
PyRun_StringFlags(const char *str, int start, PyObject *globals,
                  PyObject *locals, PyCompilerFlags *flags)
{
    PyObject *ret = NULL;
    mod_ty mod;
    PyArena *arena;
    PyObject *filename;

    filename = _PyUnicode_FromId(&PyId_string); /* borrowed */
    if (filename == NULL)
        return NULL;

    arena = PyArena_New();
    if (arena == NULL)
        return NULL;

    mod = PyParser_ASTFromStringObject(str, filename, start, flags, arena);
    if (mod != NULL)
        ret = run_mod(mod, filename, globals, locals, flags, arena);
    PyArena_Free(arena);
    return ret;
}

PyObject *
PyRun_FileExFlags(FILE *fp, const char *filename_str, int start, PyObject *globals,
                  PyObject *locals, int closeit, PyCompilerFlags *flags)
{
    PyObject *ret = NULL;
    mod_ty mod;
    PyArena *arena = NULL;
    PyObject *filename;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        goto exit;

    arena = PyArena_New();
    if (arena == NULL)
        goto exit;

    mod = PyParser_ASTFromFileObject(fp, filename, NULL, start, 0, 0,
                                     flags, NULL, arena);
    if (closeit)
        fclose(fp);
    if (mod == NULL) {
        goto exit;
    }
    ret = run_mod(mod, filename, globals, locals, flags, arena);

exit:
    Py_XDECREF(filename);
    if (arena != NULL)
        PyArena_Free(arena);
    return ret;
}

static void
flush_io(void)
{
    PyObject *f, *r;
    PyObject *type, *value, *traceback;

    /* Save the current exception */
    PyErr_Fetch(&type, &value, &traceback);

    f = _PySys_GetObjectId(&PyId_stderr);
    if (f != NULL) {
        r = _PyObject_CallMethodId(f, &PyId_flush, "");
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }
    f = _PySys_GetObjectId(&PyId_stdout);
    if (f != NULL) {
        r = _PyObject_CallMethodId(f, &PyId_flush, "");
        if (r)
            Py_DECREF(r);
        else
            PyErr_Clear();
    }

    PyErr_Restore(type, value, traceback);
}

static PyObject *
run_mod(mod_ty mod, PyObject *filename, PyObject *globals, PyObject *locals,
            PyCompilerFlags *flags, PyArena *arena)
{
    PyCodeObject *co;
    PyObject *v;
    co = PyAST_CompileObject(mod, filename, flags, -1, arena);
    if (co == NULL)
        return NULL;
    v = PyEval_EvalCode((PyObject*)co, globals, locals);
    Py_DECREF(co);
    return v;
}

static PyObject *
run_pyc_file(FILE *fp, const char *filename, PyObject *globals,
             PyObject *locals, PyCompilerFlags *flags)
{
    PyCodeObject *co;
    PyObject *v;
    long magic;
    long PyImport_GetMagicNumber(void);

    magic = PyMarshal_ReadLongFromFile(fp);
    if (magic != PyImport_GetMagicNumber()) {
        PyErr_SetString(PyExc_RuntimeError,
                   "Bad magic number in .pyc file");
        return NULL;
    }
    /* Skip mtime and size */
    (void) PyMarshal_ReadLongFromFile(fp);
    (void) PyMarshal_ReadLongFromFile(fp);
    v = PyMarshal_ReadLastObjectFromFile(fp);
    if (v == NULL || !PyCode_Check(v)) {
        Py_XDECREF(v);
        PyErr_SetString(PyExc_RuntimeError,
                   "Bad code object in .pyc file");
        return NULL;
    }
    co = (PyCodeObject *)v;
    v = PyEval_EvalCode((PyObject*)co, globals, locals);
    if (v && flags)
        flags->cf_flags |= (co->co_flags & PyCF_MASK);
    Py_DECREF(co);
    return v;
}

PyObject *
Py_CompileStringObject(const char *str, PyObject *filename, int start,
                       PyCompilerFlags *flags, int optimize)
{
    PyCodeObject *co;
    mod_ty mod;
    PyArena *arena = PyArena_New();
    if (arena == NULL)
        return NULL;

    mod = PyParser_ASTFromStringObject(str, filename, start, flags, arena);
    if (mod == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    if (flags && (flags->cf_flags & PyCF_ONLY_AST)) {
        PyObject *result = PyAST_mod2obj(mod);
        PyArena_Free(arena);
        return result;
    }
    co = PyAST_CompileObject(mod, filename, flags, optimize, arena);
    PyArena_Free(arena);
    return (PyObject *)co;
}

PyObject *
Py_CompileStringExFlags(const char *str, const char *filename_str, int start,
                        PyCompilerFlags *flags, int optimize)
{
    PyObject *filename, *co;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    co = Py_CompileStringObject(str, filename, start, flags, optimize);
    Py_DECREF(filename);
    return co;
}

/* For use in Py_LIMITED_API */
#undef Py_CompileString
PyObject *
PyCompileString(const char *str, const char *filename, int start)
{
    return Py_CompileStringFlags(str, filename, start, NULL);
}

struct symtable *
Py_SymtableStringObject(const char *str, PyObject *filename, int start)
{
    struct symtable *st;
    mod_ty mod;
    PyCompilerFlags flags;
    PyArena *arena;

    arena = PyArena_New();
    if (arena == NULL)
        return NULL;

    flags.cf_flags = 0;
    mod = PyParser_ASTFromStringObject(str, filename, start, &flags, arena);
    if (mod == NULL) {
        PyArena_Free(arena);
        return NULL;
    }
    st = PySymtable_BuildObject(mod, filename, 0);
    PyArena_Free(arena);
    return st;
}

struct symtable *
Py_SymtableString(const char *str, const char *filename_str, int start)
{
    PyObject *filename;
    struct symtable *st;

    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    st = Py_SymtableStringObject(str, filename, start);
    Py_DECREF(filename);
    return st;
}

/* Preferred access to parser is through AST. */
mod_ty
PyParser_ASTFromStringObject(const char *s, PyObject *filename, int start,
                             PyCompilerFlags *flags, PyArena *arena)
{
    mod_ty mod;
    PyCompilerFlags localflags;
    perrdetail err;
    int iflags = PARSER_FLAGS(flags);

    node *n = PyParser_ParseStringObject(s, filename,
                                         &_PyParser_Grammar, start, &err,
                                         &iflags);
    if (flags == NULL) {
        localflags.cf_flags = 0;
        flags = &localflags;
    }
    if (n) {
        flags->cf_flags |= iflags & PyCF_MASK;
        mod = PyAST_FromNodeObject(n, flags, filename, arena);
        PyNode_Free(n);
    }
    else {
        err_input(&err);
        mod = NULL;
    }
    err_free(&err);
    return mod;
}

mod_ty
PyParser_ASTFromString(const char *s, const char *filename_str, int start,
                       PyCompilerFlags *flags, PyArena *arena)
{
    PyObject *filename;
    mod_ty mod;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    mod = PyParser_ASTFromStringObject(s, filename, start, flags, arena);
    Py_DECREF(filename);
    return mod;
}

mod_ty
PyParser_ASTFromFileObject(FILE *fp, PyObject *filename, const char* enc,
                           int start, char *ps1,
                           char *ps2, PyCompilerFlags *flags, int *errcode,
                           PyArena *arena)
{
    mod_ty mod;
    PyCompilerFlags localflags;
    perrdetail err;
    int iflags = PARSER_FLAGS(flags);

    node *n = PyParser_ParseFileObject(fp, filename, enc,
                                       &_PyParser_Grammar,
                                       start, ps1, ps2, &err, &iflags);
    if (flags == NULL) {
        localflags.cf_flags = 0;
        flags = &localflags;
    }
    if (n) {
        flags->cf_flags |= iflags & PyCF_MASK;
        mod = PyAST_FromNodeObject(n, flags, filename, arena);
        PyNode_Free(n);
    }
    else {
        err_input(&err);
        if (errcode)
            *errcode = err.error;
        mod = NULL;
    }
    err_free(&err);
    return mod;
}

mod_ty
PyParser_ASTFromFile(FILE *fp, const char *filename_str, const char* enc,
                     int start, char *ps1,
                     char *ps2, PyCompilerFlags *flags, int *errcode,
                     PyArena *arena)
{
    mod_ty mod;
    PyObject *filename;
    filename = PyUnicode_DecodeFSDefault(filename_str);
    if (filename == NULL)
        return NULL;
    mod = PyParser_ASTFromFileObject(fp, filename, enc, start, ps1, ps2,
                                     flags, errcode, arena);
    Py_DECREF(filename);
    return mod;
}

/* Simplified interface to parsefile -- return node or set exception */

node *
PyParser_SimpleParseFileFlags(FILE *fp, const char *filename, int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseFileFlags(fp, filename, NULL,
                                      &_PyParser_Grammar,
                                      start, NULL, NULL, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);

    return n;
}

/* Simplified interface to parsestring -- return node or set exception */

node *
PyParser_SimpleParseStringFlags(const char *str, int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseStringFlags(str, &_PyParser_Grammar,
                                        start, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);
    return n;
}

node *
PyParser_SimpleParseStringFlagsFilename(const char *str, const char *filename,
                                        int start, int flags)
{
    perrdetail err;
    node *n = PyParser_ParseStringFlagsFilename(str, filename,
                            &_PyParser_Grammar, start, &err, flags);
    if (n == NULL)
        err_input(&err);
    err_free(&err);
    return n;
}

node *
PyParser_SimpleParseStringFilename(const char *str, const char *filename, int start)
{
    return PyParser_SimpleParseStringFlagsFilename(str, filename, start, 0);
}

/* May want to move a more generalized form of this to parsetok.c or
   even parser modules. */

void
PyParser_ClearError(perrdetail *err)
{
    err_free(err);
}

void
PyParser_SetError(perrdetail *err)
{
    err_input(err);
}

static void
err_free(perrdetail *err)
{
    Py_CLEAR(err->filename);
}

/* Set the error appropriate to the given input error code (see errcode.h) */

static void
err_input(perrdetail *err)
{
    PyObject *v, *w, *errtype, *errtext;
    PyObject *msg_obj = NULL;
    char *msg = NULL;
    int offset = err->offset;

    errtype = PyExc_SyntaxError;
    switch (err->error) {
    case E_ERROR:
        return;
    case E_SYNTAX:
        errtype = PyExc_IndentationError;
        if (err->expected == INDENT)
            msg = "expected an indented block";
        else if (err->token == INDENT)
            msg = "unexpected indent";
        else if (err->token == DEDENT)
            msg = "unexpected unindent";
        else {
            errtype = PyExc_SyntaxError;
            msg = "invalid syntax";
        }
        break;
    case E_TOKEN:
        msg = "invalid token";
        break;
    case E_EOFS:
        msg = "EOF while scanning triple-quoted string literal";
        break;
    case E_EOLS:
        msg = "EOL while scanning string literal";
        break;
    case E_INTR:
        if (!PyErr_Occurred())
            PyErr_SetNone(PyExc_KeyboardInterrupt);
        goto cleanup;
    case E_NOMEM:
        PyErr_NoMemory();
        goto cleanup;
    case E_EOF:
        msg = "unexpected EOF while parsing";
        break;
    case E_TABSPACE:
        errtype = PyExc_TabError;
        msg = "inconsistent use of tabs and spaces in indentation";
        break;
    case E_OVERFLOW:
        msg = "expression too long";
        break;
    case E_DEDENT:
        errtype = PyExc_IndentationError;
        msg = "unindent does not match any outer indentation level";
        break;
    case E_TOODEEP:
        errtype = PyExc_IndentationError;
        msg = "too many levels of indentation";
        break;
    case E_DECODE: {
        PyObject *type, *value, *tb;
        PyErr_Fetch(&type, &value, &tb);
        msg = "unknown decode error";
        if (value != NULL)
            msg_obj = PyObject_Str(value);
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(tb);
        break;
    }
    case E_LINECONT:
        msg = "unexpected character after line continuation character";
        break;

    case E_IDENTIFIER:
        msg = "invalid character in identifier";
        break;
    case E_BADSINGLE:
        msg = "multiple statements found while compiling a single statement";
        break;
    default:
        fprintf(stderr, "error=%d\n", err->error);
        msg = "unknown parsing error";
        break;
    }
    /* err->text may not be UTF-8 in case of decoding errors.
       Explicitly convert to an object. */
    if (!err->text) {
        errtext = Py_None;
        Py_INCREF(Py_None);
    } else {
        errtext = PyUnicode_DecodeUTF8(err->text, err->offset,
                                       "replace");
        if (errtext != NULL) {
            Py_ssize_t len = strlen(err->text);
            offset = (int)PyUnicode_GET_LENGTH(errtext);
            if (len != err->offset) {
                Py_DECREF(errtext);
                errtext = PyUnicode_DecodeUTF8(err->text, len,
                                               "replace");
            }
        }
    }
    v = Py_BuildValue("(OiiN)", err->filename,
                      err->lineno, offset, errtext);
    if (v != NULL) {
        if (msg_obj)
            w = Py_BuildValue("(OO)", msg_obj, v);
        else
            w = Py_BuildValue("(sO)", msg, v);
    } else
        w = NULL;
    Py_XDECREF(v);
    PyErr_SetObject(errtype, w);
    Py_XDECREF(w);
cleanup:
    Py_XDECREF(msg_obj);
    if (err->text != NULL) {
        PyObject_FREE(err->text);
        err->text = NULL;
    }
}

/* Print fatal error message and abort */

void
Py_FatalError(const char *msg)
{
    const int fd = fileno(stderr);
    PyThreadState *tstate;

    fprintf(stderr, "Fatal Python error: %s\n", msg);
    fflush(stderr); /* it helps in Windows debug build */
    if (PyErr_Occurred()) {
        PyErr_PrintEx(0);
    }
    else {
        tstate = _Py_atomic_load_relaxed(&_PyThreadState_Current);
        if (tstate != NULL) {
            fputc('\n', stderr);
            fflush(stderr);
            _Py_DumpTracebackThreads(fd, tstate->interp, tstate);
        }
        _PyFaulthandler_Fini();
    }

#ifdef MS_WINDOWS
    {
        size_t len = strlen(msg);
        WCHAR* buffer;
        size_t i;

        /* Convert the message to wchar_t. This uses a simple one-to-one
        conversion, assuming that the this error message actually uses ASCII
        only. If this ceases to be true, we will have to convert. */
        buffer = alloca( (len+1) * (sizeof *buffer));
        for( i=0; i<=len; ++i)
            buffer[i] = msg[i];
        OutputDebugStringW(L"Fatal Python error: ");
        OutputDebugStringW(buffer);
        OutputDebugStringW(L"\n");
    }
#ifdef _DEBUG
    DebugBreak();
#endif
#endif /* MS_WINDOWS */
    abort();
}

/* Clean up and exit */

#ifdef WITH_THREAD
#include "pythread.h"
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
    result = _PyObject_CallMethodId(threading, &PyId__shutdown, "");
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
    Py_Finalize();

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


#if defined(USE_STACKCHECK)
#if defined(WIN32) && defined(_MSC_VER)

/* Stack checking for Microsoft C */

#include <malloc.h>
#include <excpt.h>

/*
 * Return non-zero when we run out of memory on the stack; zero otherwise.
 */
int
PyOS_CheckStack(void)
{
    __try {
        /* alloca throws a stack overflow exception if there's
           not enough space left on the stack */
        alloca(PYOS_STACK_MARGIN * sizeof(void*));
        return 0;
    } __except (GetExceptionCode() == STATUS_STACK_OVERFLOW ?
                    EXCEPTION_EXECUTE_HANDLER :
            EXCEPTION_CONTINUE_SEARCH) {
        int errcode = _resetstkoflw();
        if (errcode == 0)
        {
            Py_FatalError("Could not reset the stack!");
        }
    }
    return 1;
}

#endif /* WIN32 && _MSC_VER */

/* Alternate implementations can be added here... */

#endif /* USE_STACKCHECK */


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

/* Deprecated C API functions still provided for binary compatiblity */

#undef PyParser_SimpleParseFile
PyAPI_FUNC(node *)
PyParser_SimpleParseFile(FILE *fp, const char *filename, int start)
{
    return PyParser_SimpleParseFileFlags(fp, filename, start, 0);
}

#undef PyParser_SimpleParseString
PyAPI_FUNC(node *)
PyParser_SimpleParseString(const char *str, int start)
{
    return PyParser_SimpleParseStringFlags(str, start, 0);
}

#undef PyRun_AnyFile
PyAPI_FUNC(int)
PyRun_AnyFile(FILE *fp, const char *name)
{
    return PyRun_AnyFileExFlags(fp, name, 0, NULL);
}

#undef PyRun_AnyFileEx
PyAPI_FUNC(int)
PyRun_AnyFileEx(FILE *fp, const char *name, int closeit)
{
    return PyRun_AnyFileExFlags(fp, name, closeit, NULL);
}

#undef PyRun_AnyFileFlags
PyAPI_FUNC(int)
PyRun_AnyFileFlags(FILE *fp, const char *name, PyCompilerFlags *flags)
{
    return PyRun_AnyFileExFlags(fp, name, 0, flags);
}

#undef PyRun_File
PyAPI_FUNC(PyObject *)
PyRun_File(FILE *fp, const char *p, int s, PyObject *g, PyObject *l)
{
    return PyRun_FileExFlags(fp, p, s, g, l, 0, NULL);
}

#undef PyRun_FileEx
PyAPI_FUNC(PyObject *)
PyRun_FileEx(FILE *fp, const char *p, int s, PyObject *g, PyObject *l, int c)
{
    return PyRun_FileExFlags(fp, p, s, g, l, c, NULL);
}

#undef PyRun_FileFlags
PyAPI_FUNC(PyObject *)
PyRun_FileFlags(FILE *fp, const char *p, int s, PyObject *g, PyObject *l,
                PyCompilerFlags *flags)
{
    return PyRun_FileExFlags(fp, p, s, g, l, 0, flags);
}

#undef PyRun_SimpleFile
PyAPI_FUNC(int)
PyRun_SimpleFile(FILE *f, const char *p)
{
    return PyRun_SimpleFileExFlags(f, p, 0, NULL);
}

#undef PyRun_SimpleFileEx
PyAPI_FUNC(int)
PyRun_SimpleFileEx(FILE *f, const char *p, int c)
{
    return PyRun_SimpleFileExFlags(f, p, c, NULL);
}


#undef PyRun_String
PyAPI_FUNC(PyObject *)
PyRun_String(const char *str, int s, PyObject *g, PyObject *l)
{
    return PyRun_StringFlags(str, s, g, l, NULL);
}

#undef PyRun_SimpleString
PyAPI_FUNC(int)
PyRun_SimpleString(const char *s)
{
    return PyRun_SimpleStringFlags(s, NULL);
}

#undef Py_CompileString
PyAPI_FUNC(PyObject *)
Py_CompileString(const char *str, const char *p, int s)
{
    return Py_CompileStringExFlags(str, p, s, NULL, -1);
}

#undef Py_CompileStringFlags
PyAPI_FUNC(PyObject *)
Py_CompileStringFlags(const char *str, const char *p, int s,
                      PyCompilerFlags *flags)
{
    return Py_CompileStringExFlags(str, p, s, flags, -1);
}

#undef PyRun_InteractiveOne
PyAPI_FUNC(int)
PyRun_InteractiveOne(FILE *f, const char *p)
{
    return PyRun_InteractiveOneFlags(f, p, NULL);
}

#undef PyRun_InteractiveLoop
PyAPI_FUNC(int)
PyRun_InteractiveLoop(FILE *f, const char *p)
{
    return PyRun_InteractiveLoopFlags(f, p, NULL);
}

#ifdef __cplusplus
}
#endif
