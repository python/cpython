#include <Python.h>
#include "internal/import.h"
#include "pythread.h"
#include <inttypes.h>
#include <stdio.h>
#include <wchar.h>

/*********************************************************
 * Embedded interpreter tests that need a custom exe
 *
 * Executed via 'EmbeddingTests' in Lib/test/test_capi.py
 *********************************************************/

static void _testembed_Py_Initialize(void)
{
    /* HACK: the "./" at front avoids a search along the PATH in
       Modules/getpath.c */
    Py_SetProgramName(L"./_testembed");
    Py_Initialize();
}


/*****************************************************
 * Test repeated initialisation and subinterpreters
 *****************************************************/

static void print_subinterp(void)
{
    /* Output information about the interpreter in the format
       expected in Lib/test/test_capi.py (test_subinterps). */
    PyThreadState *ts = PyThreadState_Get();
    PyInterpreterState *interp = ts->interp;
    int64_t id = PyInterpreterState_GetID(interp);
    printf("interp %" PRId64 " <0x%" PRIXPTR ">, thread state <0x%" PRIXPTR ">: ",
            id, (uintptr_t)interp, (uintptr_t)ts);
    fflush(stdout);
    PyRun_SimpleString(
        "import sys;"
        "print('id(modules) =', id(sys.modules));"
        "sys.stdout.flush()"
    );
}

static int test_repeated_init_and_subinterpreters(void)
{
    PyThreadState *mainstate, *substate;
    PyGILState_STATE gilstate;
    int i, j;

    for (i=0; i<15; i++) {
        printf("--- Pass %d ---\n", i);
        _testembed_Py_Initialize();
        mainstate = PyThreadState_Get();

        PyEval_InitThreads();
        PyEval_ReleaseThread(mainstate);

        gilstate = PyGILState_Ensure();
        print_subinterp();
        PyThreadState_Swap(NULL);

        for (j=0; j<3; j++) {
            substate = Py_NewInterpreter();
            print_subinterp();
            Py_EndInterpreter(substate);
        }

        PyThreadState_Swap(mainstate);
        print_subinterp();
        PyGILState_Release(gilstate);

        PyEval_RestoreThread(mainstate);
        Py_Finalize();
    }
    return 0;
}

/*****************************************************
 * Test forcing a particular IO encoding
 *****************************************************/

static void check_stdio_details(const char *encoding, const char * errors)
{
    /* Output info for the test case to check */
    if (encoding) {
        printf("Expected encoding: %s\n", encoding);
    } else {
        printf("Expected encoding: default\n");
    }
    if (errors) {
        printf("Expected errors: %s\n", errors);
    } else {
        printf("Expected errors: default\n");
    }
    fflush(stdout);
    /* Force the given IO encoding */
    Py_SetStandardStreamEncoding(encoding, errors);
    _testembed_Py_Initialize();
    PyRun_SimpleString(
        "import sys;"
        "print('stdin: {0.encoding}:{0.errors}'.format(sys.stdin));"
        "print('stdout: {0.encoding}:{0.errors}'.format(sys.stdout));"
        "print('stderr: {0.encoding}:{0.errors}'.format(sys.stderr));"
        "sys.stdout.flush()"
    );
    Py_Finalize();
}

static int test_forced_io_encoding(void)
{
    /* Check various combinations */
    printf("--- Use defaults ---\n");
    check_stdio_details(NULL, NULL);
    printf("--- Set errors only ---\n");
    check_stdio_details(NULL, "ignore");
    printf("--- Set encoding only ---\n");
    check_stdio_details("latin-1", NULL);
    printf("--- Set encoding and errors ---\n");
    check_stdio_details("latin-1", "replace");

    /* Check calling after initialization fails */
    Py_Initialize();

    if (Py_SetStandardStreamEncoding(NULL, NULL) == 0) {
        printf("Unexpected success calling Py_SetStandardStreamEncoding");
    }
    Py_Finalize();
    return 0;
}

/*********************************************************
 * Test parts of the C-API that work before initialization
 *********************************************************/

/* The pre-initialization tests tend to break by segfaulting, so explicitly
 * flushed progress messages make the broken API easier to find when they fail.
 */
#define _Py_EMBED_PREINIT_CHECK(msg) \
    do {printf(msg); fflush(stdout);} while (0);

static int test_pre_initialization_api(void)
{
    /* Leading "./" ensures getpath.c can still find the standard library */
    _Py_EMBED_PREINIT_CHECK("Checking Py_DecodeLocale\n");
    wchar_t *program = Py_DecodeLocale("./spam", NULL);
    if (program == NULL) {
        fprintf(stderr, "Fatal error: cannot decode program name\n");
        return 1;
    }
    _Py_EMBED_PREINIT_CHECK("Checking Py_SetProgramName\n");
    Py_SetProgramName(program);

    _Py_EMBED_PREINIT_CHECK("Initializing interpreter\n");
    Py_Initialize();
    _Py_EMBED_PREINIT_CHECK("Check sys module contents\n");
    PyRun_SimpleString("import sys; "
                       "print('sys.executable:', sys.executable)");
    _Py_EMBED_PREINIT_CHECK("Finalizing interpreter\n");
    Py_Finalize();

    _Py_EMBED_PREINIT_CHECK("Freeing memory allocated by Py_DecodeLocale\n");
    PyMem_RawFree(program);
    return 0;
}


/* bpo-33042: Ensure embedding apps can predefine sys module options */
static int test_pre_initialization_sys_options(void)
{
    /* We allocate a couple of the options dynamically, and then delete
     * them before calling Py_Initialize. This ensures the interpreter isn't
     * relying on the caller to keep the passed in strings alive.
     */
    const wchar_t *static_warnoption = L"once";
    const wchar_t *static_xoption = L"also_not_an_option=2";
    size_t warnoption_len = wcslen(static_warnoption);
    size_t xoption_len = wcslen(static_xoption);
    wchar_t *dynamic_once_warnoption = \
             (wchar_t *) calloc(warnoption_len+1, sizeof(wchar_t));
    wchar_t *dynamic_xoption = \
             (wchar_t *) calloc(xoption_len+1, sizeof(wchar_t));
    wcsncpy(dynamic_once_warnoption, static_warnoption, warnoption_len+1);
    wcsncpy(dynamic_xoption, static_xoption, xoption_len+1);

    _Py_EMBED_PREINIT_CHECK("Checking PySys_AddWarnOption\n");
    PySys_AddWarnOption(L"default");
    _Py_EMBED_PREINIT_CHECK("Checking PySys_ResetWarnOptions\n");
    PySys_ResetWarnOptions();
    _Py_EMBED_PREINIT_CHECK("Checking PySys_AddWarnOption linked list\n");
    PySys_AddWarnOption(dynamic_once_warnoption);
    PySys_AddWarnOption(L"module");
    PySys_AddWarnOption(L"default");
    _Py_EMBED_PREINIT_CHECK("Checking PySys_AddXOption\n");
    PySys_AddXOption(L"not_an_option=1");
    PySys_AddXOption(dynamic_xoption);

    /* Delete the dynamic options early */
    free(dynamic_once_warnoption);
    dynamic_once_warnoption = NULL;
    free(dynamic_xoption);
    dynamic_xoption = NULL;

    _Py_EMBED_PREINIT_CHECK("Initializing interpreter\n");
    _testembed_Py_Initialize();
    _Py_EMBED_PREINIT_CHECK("Check sys module contents\n");
    PyRun_SimpleString("import sys; "
                       "print('sys.warnoptions:', sys.warnoptions); "
                       "print('sys._xoptions:', sys._xoptions); "
                       "warnings = sys.modules['warnings']; "
                       "latest_filters = [f[0] for f in warnings.filters[:3]]; "
                       "print('warnings.filters[:3]:', latest_filters)");
    _Py_EMBED_PREINIT_CHECK("Finalizing interpreter\n");
    Py_Finalize();

    return 0;
}


/* bpo-20891: Avoid race condition when initialising the GIL */
static void bpo20891_thread(void *lockp)
{
    PyThread_type_lock lock = *((PyThread_type_lock*)lockp);

    PyGILState_STATE state = PyGILState_Ensure();
    if (!PyGILState_Check()) {
        fprintf(stderr, "PyGILState_Check failed!");
        abort();
    }

    PyGILState_Release(state);

    PyThread_release_lock(lock);

    PyThread_exit_thread();
}

static int test_bpo20891(void)
{
    /* bpo-20891: Calling PyGILState_Ensure in a non-Python thread before
       calling PyEval_InitThreads() must not crash. PyGILState_Ensure() must
       call PyEval_InitThreads() for us in this case. */
    PyThread_type_lock lock = PyThread_allocate_lock();
    if (!lock) {
        fprintf(stderr, "PyThread_allocate_lock failed!");
        return 1;
    }

    _testembed_Py_Initialize();

    unsigned long thrd = PyThread_start_new_thread(bpo20891_thread, &lock);
    if (thrd == PYTHREAD_INVALID_THREAD_ID) {
        fprintf(stderr, "PyThread_start_new_thread failed!");
        return 1;
    }
    PyThread_acquire_lock(lock, WAIT_LOCK);

    Py_BEGIN_ALLOW_THREADS
    /* wait until the thread exit */
    PyThread_acquire_lock(lock, WAIT_LOCK);
    Py_END_ALLOW_THREADS

    PyThread_free_lock(lock);

    return 0;
}

static int test_initialize_twice(void)
{
    _testembed_Py_Initialize();

    /* bpo-33932: Calling Py_Initialize() twice should do nothing
     * (and not crash!). */
    Py_Initialize();

    Py_Finalize();

    return 0;
}

static int test_initialize_pymain(void)
{
    wchar_t *argv[] = {L"PYTHON", L"-c",
                       L"import sys; print(f'Py_Main() after Py_Initialize: sys.argv={sys.argv}')",
                       L"arg2"};
    _testembed_Py_Initialize();

    /* bpo-34008: Calling Py_Main() after Py_Initialize() must not crash */
    Py_Main(Py_ARRAY_LENGTH(argv), argv);

    Py_Finalize();

    return 0;
}


static int
dump_config_impl(void)
{
    PyObject *config = NULL;
    PyObject *dict = NULL;

    config = PyDict_New();
    if (config == NULL) {
        goto error;
    }

    /* global config */
    dict = _Py_GetGlobalVariablesAsDict();
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(config, "global_config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    /* core config */
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    const _PyCoreConfig *core_config = &interp->core_config;
    dict = _PyCoreConfig_AsDict(core_config);
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(config, "core_config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    /* main config */
    const _PyMainInterpreterConfig *main_config = &interp->config;
    dict = _PyMainInterpreterConfig_AsDict(main_config);
    if (dict == NULL) {
        goto error;
    }
    if (PyDict_SetItemString(config, "main_config", dict) < 0) {
        goto error;
    }
    Py_CLEAR(dict);

    PyObject *json = PyImport_ImportModule("json");
    PyObject *res = PyObject_CallMethod(json, "dumps", "O", config);
    Py_DECREF(json);
    Py_CLEAR(config);
    if (res == NULL) {
        goto error;
    }

    PySys_FormatStdout("%S\n", res);
    Py_DECREF(res);

    return 0;

error:
    Py_XDECREF(config);
    Py_XDECREF(dict);
    return -1;
}


static void
dump_config(void)
{
    if (dump_config_impl() < 0) {
        fprintf(stderr, "failed to dump the configuration:\n");
        PyErr_Print();
    }
}


static int test_init_default_config(void)
{
    _testembed_Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_global_config(void)
{
    /* FIXME: test Py_IgnoreEnvironmentFlag */

    putenv("PYTHONUTF8=0");
    Py_UTF8Mode = 1;

    /* Test initialization from global configuration variables (Py_xxx) */
    Py_SetProgramName(L"./globalvar");

    /* Py_IsolatedFlag is not tested */
    Py_NoSiteFlag = 1;
    Py_BytesWarningFlag = 1;

    putenv("PYTHONINSPECT=");
    Py_InspectFlag = 1;

    putenv("PYTHONOPTIMIZE=0");
    Py_InteractiveFlag = 1;

    putenv("PYTHONDEBUG=0");
    Py_OptimizeFlag = 2;

    /* Py_DebugFlag is not tested */

    putenv("PYTHONDONTWRITEBYTECODE=");
    Py_DontWriteBytecodeFlag = 1;

    putenv("PYTHONVERBOSE=0");
    Py_VerboseFlag = 1;

    Py_QuietFlag = 1;
    Py_NoUserSiteDirectory = 1;

    putenv("PYTHONUNBUFFERED=");
    Py_UnbufferedStdioFlag = 1;

    Py_FrozenFlag = 1;

    /* FIXME: test Py_LegacyWindowsFSEncodingFlag */
    /* FIXME: test Py_LegacyWindowsStdioFlag */

    Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_from_config(void)
{
    /* Test _Py_InitializeFromConfig() */
    _PyCoreConfig config = _PyCoreConfig_INIT;
    config.install_signal_handlers = 0;

    /* FIXME: test ignore_environment */

    putenv("PYTHONHASHSEED=42");
    config.use_hash_seed = 1;
    config.hash_seed = 123;

    putenv("PYTHONMALLOC=malloc");
    config.allocator = "malloc_debug";

    /* dev_mode=1 is tested in test_init_dev_mode() */

    putenv("PYTHONFAULTHANDLER=");
    config.faulthandler = 1;

    putenv("PYTHONTRACEMALLOC=0");
    config.tracemalloc = 2;

    putenv("PYTHONPROFILEIMPORTTIME=0");
    config.import_time = 1;

    config.show_ref_count = 1;
    config.show_alloc_count = 1;
    /* FIXME: test dump_refs: bpo-34223 */

    putenv("PYTHONMALLOCSTATS=0");
    config.malloc_stats = 1;

    /* FIXME: test coerce_c_locale and coerce_c_locale_warn */

    putenv("PYTHONUTF8=0");
    Py_UTF8Mode = 0;
    config.utf8_mode = 1;

    Py_SetProgramName(L"./globalvar");
    config.program_name = L"./conf_program_name";

    static wchar_t* argv[2] = {
        L"-c",
        L"pass",
    };
    config.argc = Py_ARRAY_LENGTH(argv);
    config.argv = argv;

    config.program = L"conf_program";

    static wchar_t* xoptions[3] = {
        L"core_xoption1=3",
        L"core_xoption2=",
        L"core_xoption3",
    };
    config.nxoption = Py_ARRAY_LENGTH(xoptions);
    config.xoptions = xoptions;

    static wchar_t* warnoptions[2] = {
        L"default",
        L"error::ResourceWarning",
    };
    config.nwarnoption = Py_ARRAY_LENGTH(warnoptions);
    config.warnoptions = warnoptions;

    /* FIXME: test module_search_path_env */
    /* FIXME: test home */
    /* FIXME: test path config: module_search_path .. dll_path */

    _PyInitError err = _Py_InitializeFromConfig(&config);
    /* Don't call _PyCoreConfig_Clear() since all strings are static */
    if (_Py_INIT_FAILED(err)) {
        _Py_FatalInitError(err);
    }
    dump_config();
    Py_Finalize();
    return 0;
}


static void test_init_env_putenvs(void)
{
    putenv("PYTHONHASHSEED=42");
    putenv("PYTHONMALLOC=malloc_debug");
    putenv("PYTHONTRACEMALLOC=2");
    putenv("PYTHONPROFILEIMPORTTIME=1");
    putenv("PYTHONMALLOCSTATS=1");
    putenv("PYTHONUTF8=1");
    putenv("PYTHONVERBOSE=1");
    putenv("PYTHONINSPECT=1");
    putenv("PYTHONOPTIMIZE=2");
    putenv("PYTHONDONTWRITEBYTECODE=1");
    putenv("PYTHONUNBUFFERED=1");
    putenv("PYTHONNOUSERSITE=1");
    putenv("PYTHONFAULTHANDLER=1");
    putenv("PYTHONDEVMODE=1");
    /* FIXME: test PYTHONWARNINGS */
    /* FIXME: test PYTHONEXECUTABLE */
    /* FIXME: test PYTHONHOME */
    /* FIXME: test PYTHONDEBUG */
    /* FIXME: test PYTHONDUMPREFS */
    /* FIXME: test PYTHONCOERCECLOCALE */
    /* FIXME: test PYTHONPATH */
}


static int test_init_env(void)
{
    /* Test initialization from environment variables */
    Py_IgnoreEnvironmentFlag = 0;
    test_init_env_putenvs();
    _testembed_Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_isolated(void)
{
    /* Test _PyCoreConfig.isolated=1 */
    _PyCoreConfig config = _PyCoreConfig_INIT;

    /* Set coerce_c_locale and utf8_mode to not depend on the locale */
    config.coerce_c_locale = 0;
    config.utf8_mode = 0;
    /* Use path starting with "./" avoids a search along the PATH */
    config.program_name = L"./_testembed";

    Py_IsolatedFlag = 1;

    test_init_env_putenvs();
    _PyInitError err = _Py_InitializeFromConfig(&config);
    if (_Py_INIT_FAILED(err)) {
        _Py_FatalInitError(err);
    }
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_dev_mode(void)
{
    _PyCoreConfig config = _PyCoreConfig_INIT;
    putenv("PYTHONFAULTHANDLER=");
    putenv("PYTHONMALLOC=");
    config.dev_mode = 1;
    config.program_name = L"./_testembed";
    _PyInitError err = _Py_InitializeFromConfig(&config);
    if (_Py_INIT_FAILED(err)) {
        _Py_FatalInitError(err);
    }
    dump_config();
    Py_Finalize();
    return 0;
}


/* *********************************************************
 * List of test cases and the function that implements it.
 *
 * Names are compared case-sensitively with the first
 * argument. If no match is found, or no first argument was
 * provided, the names of all test cases are printed and
 * the exit code will be -1.
 *
 * The int returned from test functions is used as the exit
 * code, and test_capi treats all non-zero exit codes as a
 * failed test.
 *********************************************************/
struct TestCase
{
    const char *name;
    int (*func)(void);
};

static struct TestCase TestCases[] = {
    { "forced_io_encoding", test_forced_io_encoding },
    { "repeated_init_and_subinterpreters", test_repeated_init_and_subinterpreters },
    { "pre_initialization_api", test_pre_initialization_api },
    { "pre_initialization_sys_options", test_pre_initialization_sys_options },
    { "bpo20891", test_bpo20891 },
    { "initialize_twice", test_initialize_twice },
    { "initialize_pymain", test_initialize_pymain },
    { "init_default_config", test_init_default_config },
    { "init_global_config", test_init_global_config },
    { "init_from_config", test_init_from_config },
    { "init_env", test_init_env },
    { "init_dev_mode", test_init_dev_mode },
    { "init_isolated", test_init_isolated },
    { NULL, NULL }
};

int main(int argc, char *argv[])
{
    if (argc > 1) {
        for (struct TestCase *tc = TestCases; tc && tc->name; tc++) {
            if (strcmp(argv[1], tc->name) == 0)
                return (*tc->func)();
        }
    }

    /* No match found, or no test name provided, so display usage */
    printf("Python " PY_VERSION " _testembed executable for embedded interpreter tests\n"
           "Normally executed via 'EmbeddingTests' in Lib/test/test_embed.py\n\n"
           "Usage: %s TESTNAME\n\nAll available tests:\n", argv[0]);
    for (struct TestCase *tc = TestCases; tc && tc->name; tc++) {
        printf("  %s\n", tc->name);
    }

    /* Non-zero exit code will cause test_embed.py tests to fail.
       This is intentional. */
    return -1;
}
