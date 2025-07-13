#ifndef Py_BUILD_CORE_MODULE
#  define Py_BUILD_CORE_MODULE
#endif

/* Always enable assertion (even in release mode) */
#undef NDEBUG

#include <Python.h>
#include "pycore_initconfig.h"    // _PyConfig_InitCompatConfig()
#include "pycore_runtime.h"       // _PyRuntime
#include "pycore_lock.h"          // PyEvent
#include "pycore_pythread.h"      // PyThread_start_joinable_thread()
#include "pycore_import.h"        // _PyImport_FrozenBootstrap
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>               // putenv()
#include <wchar.h>

// These functions were removed from Python 3.13 API but are still exported
// for the stable ABI. We want to test them in this program.
extern void PySys_AddWarnOption(const wchar_t *s);
extern void PySys_AddXOption(const wchar_t *s);
extern void Py_SetPath(const wchar_t *path);


int main_argc;
char **main_argv;

/*********************************************************
 * Embedded interpreter tests that need a custom exe
 *
 * Executed via Lib/test/test_embed.py
 *********************************************************/

// Use to display the usage
#define PROGRAM "test_embed"

/* Use path starting with "./" avoids a search along the PATH */
#define PROGRAM_NAME L"./_testembed"
#define PROGRAM_NAME_UTF8 "./_testembed"

#define INIT_LOOPS 4

// Ignore Py_DEPRECATED() compiler warnings: deprecated functions are
// tested on purpose here.
_Py_COMP_DIAG_PUSH
_Py_COMP_DIAG_IGNORE_DEPR_DECLS


static void error(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n", msg);
    fflush(stderr);
}


static void config_set_string(PyConfig *config, wchar_t **config_str, const wchar_t *str)
{
    PyStatus status = PyConfig_SetString(config, config_str, str);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(config);
        Py_ExitStatusException(status);
    }
}


static void config_set_program_name(PyConfig *config)
{
    const wchar_t *program_name = PROGRAM_NAME;
    config_set_string(config, &config->program_name, program_name);
}


static void init_from_config_clear(PyConfig *config)
{
    PyStatus status = Py_InitializeFromConfig(config);
    PyConfig_Clear(config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }
}


static void _testembed_initialize(void)
{
    PyConfig config;
    _PyConfig_InitCompatConfig(&config);
    config_set_program_name(&config);
    init_from_config_clear(&config);
}


static int test_import_in_subinterpreters(void)
{
    _testembed_initialize();
    PyThreadState_Swap(Py_NewInterpreter());
    return PyRun_SimpleString("import readline"); // gh-124160
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

    for (int i=1; i <= INIT_LOOPS; i++) {
        printf("--- Pass %d ---\n", i);
        _testembed_initialize();
        mainstate = PyThreadState_Get();

        PyEval_ReleaseThread(mainstate);

        gilstate = PyGILState_Ensure();
        print_subinterp();
        PyThreadState_Swap(NULL);

        for (int j=0; j<3; j++) {
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

#define EMBEDDED_EXT_NAME "embedded_ext"

static PyModuleDef embedded_ext = {
    PyModuleDef_HEAD_INIT,
    .m_name = EMBEDDED_EXT_NAME,
    .m_size = 0,
};

static PyObject*
PyInit_embedded_ext(void)
{
    return PyModule_Create(&embedded_ext);
}

/****************************************************************************
 * Call Py_Initialize()/Py_Finalize() multiple times and execute Python code
 ***************************************************************************/

// Used by bpo-46417 to test that structseq types used by the sys module are
// cleared properly and initialized again properly when Python is finalized
// multiple times.
static int test_repeated_init_exec(void)
{
    if (main_argc < 3) {
        fprintf(stderr,
                "usage: %s test_repeated_init_exec CODE ...\n", PROGRAM);
        exit(1);
    }
    const char *code = main_argv[2];
    int loops = main_argc > 3
        ? main_argc - 2
        : INIT_LOOPS;

    for (int i=0; i < loops; i++) {
        fprintf(stderr, "--- Loop #%d ---\n", i+1);
        fflush(stderr);

        if (main_argc > 3) {
            code = main_argv[i+2];
        }

        _testembed_initialize();
        int err = PyRun_SimpleString(code);
        Py_Finalize();
        if (err) {
            return 1;
        }
    }
    return 0;
}

/****************************************************************************
 * Test the Py_Initialize(Ex) convenience/compatibility wrappers
 ***************************************************************************/
// This is here to help ensure there are no wrapper resource leaks (gh-96853)
static int test_repeated_simple_init(void)
{
    for (int i=1; i <= INIT_LOOPS; i++) {
        fprintf(stderr, "--- Loop #%d ---\n", i);
        fflush(stderr);

        _testembed_initialize();
        Py_Finalize();
        printf("Finalized\n"); // Give test_embed some output to check
    }
    return 0;
}


/*****************************************************
 * Test forcing a particular IO encoding
 *****************************************************/

static void check_stdio_details(const wchar_t *encoding, const wchar_t *errors)
{
    /* Output info for the test case to check */
    if (encoding) {
        printf("Expected encoding: %ls\n", encoding);
    } else {
        printf("Expected encoding: default\n");
    }
    if (errors) {
        printf("Expected errors: %ls\n", errors);
    } else {
        printf("Expected errors: default\n");
    }
    fflush(stdout);

    PyConfig config;
    _PyConfig_InitCompatConfig(&config);
    /* Force the given IO encoding */
    if (encoding) {
        config_set_string(&config, &config.stdio_encoding, encoding);
    }
    if (errors) {
        config_set_string(&config, &config.stdio_errors, errors);
    }
#ifdef MS_WINDOWS
    // gh-106659: On Windows, don't use _io._WindowsConsoleIO which always
    // announce UTF-8 for sys.stdin.encoding.
    config.legacy_windows_stdio = 1;
#endif
    config_set_program_name(&config);
    init_from_config_clear(&config);


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
    check_stdio_details(NULL, L"ignore");
    printf("--- Set encoding only ---\n");
    check_stdio_details(L"iso8859-1", NULL);
    printf("--- Set encoding and errors ---\n");
    check_stdio_details(L"iso8859-1", L"replace");
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
    /* the test doesn't support custom memory allocators */
    putenv("PYTHONMALLOC=");

    _Py_EMBED_PREINIT_CHECK("Initializing interpreter\n");
    _testembed_initialize();

    _Py_EMBED_PREINIT_CHECK("Checking Py_IsInitialized post-initialization\n");
    if (!Py_IsInitialized()) {
        fprintf(stderr, "Fatal error: not initialized after initialization!\n");
        return 1;
    }

    _Py_EMBED_PREINIT_CHECK("Check sys module contents\n");
    PyRun_SimpleString(
        "import sys; "
        "print('sys.executable:', sys.executable); "
        "sys.stdout.flush(); "
    );
    _Py_EMBED_PREINIT_CHECK("Finalizing interpreter\n");
    Py_Finalize();

    _Py_EMBED_PREINIT_CHECK("Checking !Py_IsInitialized post-finalization\n");
    if (Py_IsInitialized()) {
        fprintf(stderr, "Fatal error: still initialized after finalization!\n");
        return 1;
    }
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
    _testembed_initialize();
    _Py_EMBED_PREINIT_CHECK("Check sys module contents\n");
    PyRun_SimpleString(
        "import sys; "
        "print('sys.warnoptions:', sys.warnoptions); "
        "print('sys._xoptions:', sys._xoptions); "
        "warnings = sys.modules['warnings']; "
        "latest_filters = [f[0] for f in warnings.filters[:3]]; "
        "print('warnings.filters[:3]:', latest_filters); "
        "sys.stdout.flush(); "
    );
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
        error("PyGILState_Check failed!");
        abort();
    }

    PyGILState_Release(state);

    PyThread_release_lock(lock);
}

static int test_bpo20891(void)
{
    /* the test doesn't support custom memory allocators */
    putenv("PYTHONMALLOC=");

    /* bpo-20891: Calling PyGILState_Ensure in a non-Python thread must not
       crash. */
    PyThread_type_lock lock = PyThread_allocate_lock();
    if (!lock) {
        error("PyThread_allocate_lock failed!");
        return 1;
    }

    _testembed_initialize();

    unsigned long thrd = PyThread_start_new_thread(bpo20891_thread, &lock);
    if (thrd == PYTHREAD_INVALID_THREAD_ID) {
        error("PyThread_start_new_thread failed!");
        return 1;
    }
    PyThread_acquire_lock(lock, WAIT_LOCK);

    Py_BEGIN_ALLOW_THREADS
    /* wait until the thread exit */
    PyThread_acquire_lock(lock, WAIT_LOCK);
    Py_END_ALLOW_THREADS

    PyThread_free_lock(lock);

    Py_Finalize();

    return 0;
}

static int test_initialize_twice(void)
{
    _testembed_initialize();

    /* bpo-33932: Calling Py_Initialize() twice should do nothing
     * (and not crash!). */
    Py_Initialize();

    Py_Finalize();

    return 0;
}

static int test_initialize_pymain(void)
{
    wchar_t *argv[] = {L"PYTHON", L"-c",
                       (L"import sys; "
                        L"print(f'Py_Main() after Py_Initialize: "
                        L"sys.argv={sys.argv}')"),
                       L"arg2"};
    _testembed_initialize();

    /* bpo-34008: Calling Py_Main() after Py_Initialize() must not crash */
    Py_Main(Py_ARRAY_LENGTH(argv), argv);

    Py_Finalize();

    return 0;
}


static void
dump_config(void)
{
    (void) PyRun_SimpleStringFlags(
        "import _testinternalcapi, json; "
        "print(json.dumps(_testinternalcapi.get_configs()))",
        0);
}


static int test_init_initialize_config(void)
{
    _testembed_initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static void config_set_argv(PyConfig *config, Py_ssize_t argc, wchar_t * const *argv)
{
    PyStatus status = PyConfig_SetArgv(config, argc, argv);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(config);
        Py_ExitStatusException(status);
    }
}


static void
config_set_wide_string_list(PyConfig *config, PyWideStringList *list,
                            Py_ssize_t length, wchar_t **items)
{
    PyStatus status = PyConfig_SetWideStringList(config, list, length, items);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(config);
        Py_ExitStatusException(status);
    }
}


static int check_init_compat_config(int preinit)
{
    PyStatus status;

    if (preinit) {
        PyPreConfig preconfig;
        _PyPreConfig_InitCompatConfig(&preconfig);

        status = Py_PreInitialize(&preconfig);
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }
    }

    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_preinit_compat_config(void)
{
    return check_init_compat_config(1);
}


static int test_init_compat_config(void)
{
    return check_init_compat_config(0);
}


static int test_init_global_config(void)
{
    /* FIXME: test Py_IgnoreEnvironmentFlag */

    putenv("PYTHONUTF8=0");
    Py_UTF8Mode = 1;

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

    _testembed_initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_from_config(void)
{
    PyPreConfig preconfig;
    _PyPreConfig_InitCompatConfig(&preconfig);

    putenv("PYTHONMALLOC=malloc_debug");
#ifndef Py_GIL_DISABLED
    preconfig.allocator = PYMEM_ALLOCATOR_MALLOC;
#else
    preconfig.allocator = PYMEM_ALLOCATOR_MIMALLOC;
#endif

    putenv("PYTHONUTF8=0");
    Py_UTF8Mode = 0;
    preconfig.utf8_mode = 1;

    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    config.install_signal_handlers = 0;

    /* FIXME: test use_environment */

    putenv("PYTHONHASHSEED=42");
    config.use_hash_seed = 1;
    config.hash_seed = 123;

    /* dev_mode=1 is tested in test_init_dev_mode() */

    putenv("PYTHONFAULTHANDLER=");
    config.faulthandler = 1;

    putenv("PYTHONTRACEMALLOC=0");
    config.tracemalloc = 2;

    putenv("PYTHONPROFILEIMPORTTIME=1");
    config.import_time = 2;

    putenv("PYTHONNODEBUGRANGES=0");
    config.code_debug_ranges = 0;

    config.show_ref_count = 1;
    /* FIXME: test dump_refs: bpo-34223 */

    putenv("PYTHONMALLOCSTATS=0");
    config.malloc_stats = 1;

    putenv("PYTHONPYCACHEPREFIX=env_pycache_prefix");
    config_set_string(&config, &config.pycache_prefix, L"conf_pycache_prefix");

    config_set_string(&config, &config.program_name, L"./conf_program_name");

    wchar_t* argv[] = {
        L"python3",
        L"-W",
        L"cmdline_warnoption",
        L"-X",
        L"cmdline_xoption",
        L"-c",
        L"pass",
        L"arg2",
    };
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config.parse_argv = 1;

    wchar_t* xoptions[3] = {
        L"config_xoption1=3",
        L"config_xoption2=",
        L"config_xoption3",
    };
    config_set_wide_string_list(&config, &config.xoptions,
                                Py_ARRAY_LENGTH(xoptions), xoptions);

    wchar_t* warnoptions[1] = {
        L"config_warnoption",
    };
    config_set_wide_string_list(&config, &config.warnoptions,
                                Py_ARRAY_LENGTH(warnoptions), warnoptions);

    /* FIXME: test pythonpath_env */
    /* FIXME: test home */
    /* FIXME: test path config: module_search_path .. dll_path */

    putenv("PYTHONPLATLIBDIR=env_platlibdir");
    config_set_string(&config, &config.platlibdir, L"my_platlibdir");

    putenv("PYTHONVERBOSE=0");
    Py_VerboseFlag = 0;
    config.verbose = 1;

    Py_NoSiteFlag = 0;
    config.site_import = 0;

    Py_BytesWarningFlag = 0;
    config.bytes_warning = 1;

    putenv("PYTHONINSPECT=");
    Py_InspectFlag = 0;
    config.inspect = 1;

    Py_InteractiveFlag = 0;
    config.interactive = 1;

    putenv("PYTHONOPTIMIZE=0");
    Py_OptimizeFlag = 1;
    config.optimization_level = 2;

    /* FIXME: test parser_debug */

    putenv("PYTHONDONTWRITEBYTECODE=");
    Py_DontWriteBytecodeFlag = 0;
    config.write_bytecode = 0;

    Py_QuietFlag = 0;
    config.quiet = 1;

    config.configure_c_stdio = 1;

    putenv("PYTHONUNBUFFERED=");
    Py_UnbufferedStdioFlag = 0;
    config.buffered_stdio = 0;

    putenv("PYTHONIOENCODING=cp424");
    config_set_string(&config, &config.stdio_encoding, L"iso8859-1");
    config_set_string(&config, &config.stdio_errors, L"replace");

    putenv("PYTHONNOUSERSITE=");
    Py_NoUserSiteDirectory = 0;
    config.user_site_directory = 0;

    config_set_string(&config, &config.check_hash_pycs_mode, L"always");

    Py_FrozenFlag = 0;
    config.pathconfig_warnings = 0;

    config.safe_path = 1;
#ifdef Py_STATS
    putenv("PYTHONSTATS=");
    config._pystats = 1;
#endif

    putenv("PYTHONINTMAXSTRDIGITS=6666");
    config.int_max_str_digits = 31337;
    config.cpu_count = 4321;

    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int check_init_parse_argv(int parse_argv)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.parse_argv = parse_argv;

    wchar_t* argv[] = {
        L"./argv0",
        L"-E",
        L"-c",
        L"pass",
        L"arg1",
        L"-v",
        L"arg3",
    };
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_parse_argv(void)
{
    return check_init_parse_argv(1);
}


static int test_init_dont_parse_argv(void)
{
    return check_init_parse_argv(0);
}


static void set_most_env_vars(void)
{
    putenv("PYTHONHASHSEED=42");
#ifndef Py_GIL_DISABLED
    putenv("PYTHONMALLOC=malloc");
#else
    putenv("PYTHONMALLOC=mimalloc");
#endif
    putenv("PYTHONTRACEMALLOC=2");
    putenv("PYTHONPROFILEIMPORTTIME=1");
    putenv("PYTHONNODEBUGRANGES=1");
    putenv("PYTHONMALLOCSTATS=1");
    putenv("PYTHONUTF8=1");
    putenv("PYTHONVERBOSE=1");
    putenv("PYTHONINSPECT=1");
    putenv("PYTHONOPTIMIZE=2");
    putenv("PYTHONDONTWRITEBYTECODE=1");
    putenv("PYTHONUNBUFFERED=1");
    putenv("PYTHONPYCACHEPREFIX=env_pycache_prefix");
    putenv("PYTHONNOUSERSITE=1");
    putenv("PYTHONFAULTHANDLER=1");
    putenv("PYTHONIOENCODING=iso8859-1:replace");
    putenv("PYTHONPLATLIBDIR=env_platlibdir");
    putenv("PYTHONSAFEPATH=1");
    putenv("PYTHONINTMAXSTRDIGITS=4567");
#ifdef Py_STATS
    putenv("PYTHONSTATS=1");
#endif
    putenv("PYTHONPERFSUPPORT=1");
}


static void set_all_env_vars(void)
{
    set_most_env_vars();

    putenv("PYTHONWARNINGS=EnvVar");
    putenv("PYTHONPATH=/my/path");
}


static int test_init_compat_env(void)
{
    /* Test initialization from environment variables */
    Py_IgnoreEnvironmentFlag = 0;
    set_all_env_vars();
    _testembed_initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_python_env(void)
{
    set_all_env_vars();

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static void set_all_env_vars_dev_mode(void)
{
    putenv("PYTHONMALLOC=");
    putenv("PYTHONFAULTHANDLER=");
    putenv("PYTHONDEVMODE=1");
}


static int test_init_env_dev_mode(void)
{
    /* Test initialization from environment variables */
    Py_IgnoreEnvironmentFlag = 0;
    set_all_env_vars_dev_mode();
    _testembed_initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_env_dev_mode_alloc(void)
{
    /* Test initialization from environment variables */
    Py_IgnoreEnvironmentFlag = 0;
    set_all_env_vars_dev_mode();
#ifndef Py_GIL_DISABLED
    putenv("PYTHONMALLOC=malloc");
#else
    putenv("PYTHONMALLOC=mimalloc");
#endif
    _testembed_initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_isolated_flag(void)
{
    /* Test PyConfig.isolated=1 */
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    Py_IsolatedFlag = 0;
    config.isolated = 1;
    // These options are set to 1 by isolated=1
    config.safe_path = 0;
    config.use_environment = 1;
    config.user_site_directory = 1;

    config_set_program_name(&config);
    set_all_env_vars();
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


/* PyPreConfig.isolated=1, PyConfig.isolated=0 */
static int test_preinit_isolated1(void)
{
    PyPreConfig preconfig;
    _PyPreConfig_InitCompatConfig(&preconfig);

    preconfig.isolated = 1;

    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    config_set_program_name(&config);
    set_all_env_vars();
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


/* PyPreConfig.isolated=0, PyConfig.isolated=1 */
static int test_preinit_isolated2(void)
{
    PyPreConfig preconfig;
    _PyPreConfig_InitCompatConfig(&preconfig);

    preconfig.isolated = 0;

    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    /* Test PyConfig.isolated=1 */
    PyConfig config;
    _PyConfig_InitCompatConfig(&config);

    Py_IsolatedFlag = 0;
    config.isolated = 1;

    config_set_program_name(&config);
    set_all_env_vars();
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_preinit_dont_parse_argv(void)
{
    PyPreConfig preconfig;
    PyPreConfig_InitIsolatedConfig(&preconfig);

    preconfig.isolated = 0;

    /* -X dev must be ignored by isolated preconfiguration */
    wchar_t *argv[] = {L"python3",
                       L"-E",
                       L"-I",
                       L"-P",
                       L"-X", L"dev",
                       L"-X", L"utf8",
                       L"script.py"};
    PyStatus status = Py_PreInitializeFromArgs(&preconfig,
                                               Py_ARRAY_LENGTH(argv), argv);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    config.isolated = 0;

    /* Pre-initialize implicitly using argv: make sure that -X dev
       is used to configure the allocation in preinitialization */
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_preinit_parse_argv(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    /* Pre-initialize implicitly using argv: make sure that -X dev
       is used to configure the allocation in preinitialization */
    wchar_t *argv[] = {L"python3", L"-X", L"dev", L"-P", L"script.py"};
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}




static void set_all_global_config_variables(void)
{
    Py_IsolatedFlag = 0;
    Py_IgnoreEnvironmentFlag = 0;
    Py_BytesWarningFlag = 2;
    Py_InspectFlag = 1;
    Py_InteractiveFlag = 1;
    Py_OptimizeFlag = 1;
    Py_DebugFlag = 1;
    Py_VerboseFlag = 1;
    Py_QuietFlag = 1;
    Py_FrozenFlag = 0;
    Py_UnbufferedStdioFlag = 1;
    Py_NoSiteFlag = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_NoUserSiteDirectory = 1;
#ifdef MS_WINDOWS
    Py_LegacyWindowsStdioFlag = 1;
#endif
}


static int check_preinit_isolated_config(int preinit)
{
    PyStatus status;
    PyPreConfig *rt_preconfig;

    /* environment variables must be ignored */
    set_all_env_vars();

    /* global configuration variables must be ignored */
    set_all_global_config_variables();

    if (preinit) {
        PyPreConfig preconfig;
        PyPreConfig_InitIsolatedConfig(&preconfig);

        status = Py_PreInitialize(&preconfig);
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }

        rt_preconfig = &_PyRuntime.preconfig;
        assert(rt_preconfig->isolated == 1);
        assert(rt_preconfig->use_environment == 0);
    }

    PyConfig config;
    PyConfig_InitIsolatedConfig(&config);

    config_set_program_name(&config);
    init_from_config_clear(&config);

    rt_preconfig = &_PyRuntime.preconfig;
    assert(rt_preconfig->isolated == 1);
    assert(rt_preconfig->use_environment == 0);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_preinit_isolated_config(void)
{
    return check_preinit_isolated_config(1);
}


static int test_init_isolated_config(void)
{
    return check_preinit_isolated_config(0);
}


static int check_init_python_config(int preinit)
{
    /* global configuration variables must be ignored */
    set_all_global_config_variables();
    Py_IsolatedFlag = 1;
    Py_IgnoreEnvironmentFlag = 1;
    Py_FrozenFlag = 1;
    Py_UnbufferedStdioFlag = 1;
    Py_NoSiteFlag = 1;
    Py_DontWriteBytecodeFlag = 1;
    Py_NoUserSiteDirectory = 1;
#ifdef MS_WINDOWS
    Py_LegacyWindowsStdioFlag = 1;
#endif

    if (preinit) {
        PyPreConfig preconfig;
        PyPreConfig_InitPythonConfig(&preconfig);

        PyStatus status = Py_PreInitialize(&preconfig);
        if (PyStatus_Exception(status)) {
            Py_ExitStatusException(status);
        }
    }

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_preinit_python_config(void)
{
    return check_init_python_config(1);
}


static int test_init_python_config(void)
{
    return check_init_python_config(0);
}


static int test_init_dont_configure_locale(void)
{
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);

    preconfig.configure_locale = 0;
    preconfig.coerce_c_locale = 1;
    preconfig.coerce_c_locale_warn = 1;

    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_dev_mode(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    putenv("PYTHONFAULTHANDLER=");
    putenv("PYTHONMALLOC=");
    config.dev_mode = 1;
    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}

static PyObject *_open_code_hook(PyObject *path, void *data)
{
    if (PyUnicode_CompareWithASCIIString(path, "$$test-filename") == 0) {
        return PyLong_FromVoidPtr(data);
    }
    PyObject *io = PyImport_ImportModule("_io");
    if (!io) {
        return NULL;
    }
    return PyObject_CallMethod(io, "open", "Os", path, "rb");
}

static int test_open_code_hook(void)
{
    int result = 0;

    /* Provide a hook */
    result = PyFile_SetOpenCodeHook(_open_code_hook, &result);
    if (result) {
        printf("Failed to set hook\n");
        return 1;
    }
    /* A second hook should fail */
    result = PyFile_SetOpenCodeHook(_open_code_hook, &result);
    if (!result) {
        printf("Should have failed to set second hook\n");
        return 2;
    }

    Py_IgnoreEnvironmentFlag = 0;
    _testembed_initialize();
    result = 0;

    PyObject *r = PyFile_OpenCode("$$test-filename");
    if (!r) {
        PyErr_Print();
        result = 3;
    } else {
        void *cmp = PyLong_AsVoidPtr(r);
        Py_DECREF(r);
        if (cmp != &result) {
            printf("Did not get expected result from hook\n");
            result = 4;
        }
    }

    if (!result) {
        PyObject *io = PyImport_ImportModule("_io");
        PyObject *r = io
            ? PyObject_CallMethod(io, "open_code", "s", "$$test-filename")
            : NULL;
        if (!r) {
            PyErr_Print();
            result = 5;
        } else {
            void *cmp = PyLong_AsVoidPtr(r);
            Py_DECREF(r);
            if (cmp != &result) {
                printf("Did not get expected result from hook\n");
                result = 6;
            }
        }
        Py_XDECREF(io);
    }

    Py_Finalize();
    return result;
}

static int _audit_hook_clear_count = 0;

static int _audit_hook(const char *event, PyObject *args, void *userdata)
{
    assert(args && PyTuple_CheckExact(args));
    if (strcmp(event, "_testembed.raise") == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Intentional error");
        return -1;
    } else if (strcmp(event, "_testembed.set") == 0) {
        if (!PyArg_ParseTuple(args, "n", userdata)) {
            return -1;
        }
        return 0;
    } else if (strcmp(event, "cpython._PySys_ClearAuditHooks") == 0) {
        _audit_hook_clear_count += 1;
    }
    return 0;
}

static int _test_audit(Py_ssize_t setValue)
{
    Py_ssize_t sawSet = 0;

    Py_IgnoreEnvironmentFlag = 0;
    PySys_AddAuditHook(_audit_hook, &sawSet);
    _testembed_initialize();

    if (PySys_Audit("_testembed.raise", NULL) == 0) {
        printf("No error raised");
        return 1;
    }
    if (PySys_Audit("_testembed.nop", NULL) != 0) {
        printf("Nop event failed");
        /* Exception from above may still remain */
        PyErr_Clear();
        return 2;
    }
    if (!PyErr_Occurred()) {
        printf("Exception not preserved");
        return 3;
    }
    PyErr_Clear();

    if (PySys_Audit("_testembed.set", "n", setValue) != 0) {
        PyErr_Print();
        printf("Set event failed");
        return 4;
    }
    if (PyErr_Occurred()) {
        printf("Exception raised");
        return 5;
    }

    if (sawSet != 42) {
        printf("Failed to see *userData change\n");
        return 6;
    }

    return 0;
}

static int test_audit(void)
{
    int result = _test_audit(42);
    Py_Finalize();
    if (_audit_hook_clear_count != 1) {
        return 0x1000 | _audit_hook_clear_count;
    }
    return result;
}

static int test_audit_tuple(void)
{
#define ASSERT(TEST, EXITCODE) \
    if (!(TEST)) { \
        printf("ERROR test failed at %s:%i\n", __FILE__, __LINE__); \
        return (EXITCODE); \
    }

    Py_ssize_t sawSet = 0;

    // we need at least one hook, otherwise code checking for
    // PySys_AuditTuple() is skipped.
    PySys_AddAuditHook(_audit_hook, &sawSet);
    _testembed_initialize();

    ASSERT(!PyErr_Occurred(), 0);

    // pass Python tuple object
    PyObject *tuple = Py_BuildValue("(i)", 444);
    if (tuple == NULL) {
        goto error;
    }
    ASSERT(PySys_AuditTuple("_testembed.set", tuple) == 0, 10);
    ASSERT(!PyErr_Occurred(), 11);
    ASSERT(sawSet == 444, 12);
    Py_DECREF(tuple);

    // pass Python int object
    PyObject *int_arg = PyLong_FromLong(555);
    if (int_arg == NULL) {
        goto error;
    }
    ASSERT(PySys_AuditTuple("_testembed.set", int_arg) == -1, 20);
    ASSERT(PyErr_ExceptionMatches(PyExc_TypeError), 21);
    PyErr_Clear();
    Py_DECREF(int_arg);

    // NULL is accepted and means "no arguments"
    ASSERT(PySys_AuditTuple("_testembed.test_audit_tuple", NULL) == 0, 30);
    ASSERT(!PyErr_Occurred(), 31);

    Py_Finalize();
    return 0;

error:
    PyErr_Print();
    return 1;

#undef ASSERT
}

static volatile int _audit_subinterpreter_interpreter_count = 0;

static int _audit_subinterpreter_hook(const char *event, PyObject *args, void *userdata)
{
    printf("%s\n", event);
    if (strcmp(event, "cpython.PyInterpreterState_New") == 0) {
        _audit_subinterpreter_interpreter_count += 1;
    }
    return 0;
}

static int test_audit_subinterpreter(void)
{
    Py_IgnoreEnvironmentFlag = 0;
    PySys_AddAuditHook(_audit_subinterpreter_hook, NULL);
    _testembed_initialize();

    Py_NewInterpreter();
    Py_NewInterpreter();
    Py_NewInterpreter();

    Py_Finalize();

    switch (_audit_subinterpreter_interpreter_count) {
        case 3: return 0;
        case 0: return -1;
        default: return _audit_subinterpreter_interpreter_count;
    }
}

typedef struct {
    const char* expected;
    int exit;
} AuditRunCommandTest;

static int _audit_hook_run(const char *eventName, PyObject *args, void *userData)
{
    AuditRunCommandTest *test = (AuditRunCommandTest*)userData;
    if (strcmp(eventName, test->expected)) {
        return 0;
    }

    if (test->exit) {
        PyObject *msg = PyUnicode_FromFormat("detected %s(%R)", eventName, args);
        if (msg) {
            printf("%s\n", PyUnicode_AsUTF8(msg));
            Py_DECREF(msg);
        }
        exit(test->exit);
    }

    PyErr_Format(PyExc_RuntimeError, "detected %s(%R)", eventName, args);
    return -1;
}

static int test_audit_run_command(void)
{
    AuditRunCommandTest test = {"cpython.run_command"};
    wchar_t *argv[] = {PROGRAM_NAME, L"-c", L"pass"};

    Py_IgnoreEnvironmentFlag = 0;
    PySys_AddAuditHook(_audit_hook_run, (void*)&test);

    return Py_Main(Py_ARRAY_LENGTH(argv), argv);
}

static int test_audit_run_file(void)
{
    AuditRunCommandTest test = {"cpython.run_file"};
    wchar_t *argv[] = {PROGRAM_NAME, L"filename.py"};

    Py_IgnoreEnvironmentFlag = 0;
    PySys_AddAuditHook(_audit_hook_run, (void*)&test);

    return Py_Main(Py_ARRAY_LENGTH(argv), argv);
}

static int run_audit_run_test(int argc, wchar_t **argv, void *test)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.argv.length = argc;
    config.argv.items = argv;
    config.parse_argv = 1;
    config.program_name = argv[0];
    config.interactive = 1;
    config.isolated = 0;
    config.use_environment = 1;
    config.quiet = 1;

    PySys_AddAuditHook(_audit_hook_run, test);

    PyStatus status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    return Py_RunMain();
}

static int test_audit_run_interactivehook(void)
{
    AuditRunCommandTest test = {"cpython.run_interactivehook", 10};
    wchar_t *argv[] = {PROGRAM_NAME};
    return run_audit_run_test(Py_ARRAY_LENGTH(argv), argv, &test);
}

static int test_audit_run_startup(void)
{
    AuditRunCommandTest test = {"cpython.run_startup", 10};
    wchar_t *argv[] = {PROGRAM_NAME};
    return run_audit_run_test(Py_ARRAY_LENGTH(argv), argv, &test);
}

static int test_audit_run_stdin(void)
{
    AuditRunCommandTest test = {"cpython.run_stdin"};
    wchar_t *argv[] = {PROGRAM_NAME};
    return run_audit_run_test(Py_ARRAY_LENGTH(argv), argv, &test);
}

static int test_init_read_set(void)
{
    PyStatus status;
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config_set_string(&config, &config.program_name, L"./init_read_set");

    status = PyConfig_Read(&config);
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    status = PyWideStringList_Insert(&config.module_search_paths,
                                     1, L"test_path_insert1");
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    status = PyWideStringList_Append(&config.module_search_paths,
                                     L"test_path_append");
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    /* override executable computed by PyConfig_Read() */
    config_set_string(&config, &config.executable, L"my_executable");
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;

fail:
    PyConfig_Clear(&config);
    Py_ExitStatusException(status);
}


static int test_init_sys_add(void)
{
    PySys_AddXOption(L"sysadd_xoption");
    PySys_AddXOption(L"faulthandler");
    PySys_AddWarnOption(L"ignore:::sysadd_warnoption");

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    wchar_t* argv[] = {
        L"python3",
        L"-W",
        L"ignore:::cmdline_warnoption",
        L"-X",
        L"cmdline_xoption",
    };
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config.parse_argv = 1;

    PyStatus status;
    status = PyWideStringList_Append(&config.xoptions,
                                     L"config_xoption");
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    status = PyWideStringList_Append(&config.warnoptions,
                                     L"ignore:::config_warnoption");
    if (PyStatus_Exception(status)) {
        goto fail;
    }

    config_set_program_name(&config);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;

fail:
    PyConfig_Clear(&config);
    Py_ExitStatusException(status);
}


static int test_init_setpath(void)
{
    char *env = getenv("TESTPATH");
    if (!env) {
        error("missing TESTPATH env var");
        return 1;
    }
    wchar_t *path = Py_DecodeLocale(env, NULL);
    if (path == NULL) {
        error("failed to decode TESTPATH");
        return 1;
    }
    Py_SetPath(path);
    PyMem_RawFree(path);
    putenv("TESTPATH=");

    Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_setpath_config(void)
{
    PyPreConfig preconfig;
    PyPreConfig_InitPythonConfig(&preconfig);

    /* Explicitly preinitializes with Python preconfiguration to avoid
      Py_SetPath() implicit preinitialization with compat preconfiguration. */
    PyStatus status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    char *env = getenv("TESTPATH");
    if (!env) {
        error("missing TESTPATH env var");
        return 1;
    }
    wchar_t *path = Py_DecodeLocale(env, NULL);
    if (path == NULL) {
        error("failed to decode TESTPATH");
        return 1;
    }
    Py_SetPath(path);
    PyMem_RawFree(path);
    putenv("TESTPATH=");

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config_set_string(&config, &config.program_name, L"conf_program_name");
    config_set_string(&config, &config.executable, L"conf_executable");
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_setpythonhome(void)
{
    char *env = getenv("TESTHOME");
    if (!env) {
        error("missing TESTHOME env var");
        return 1;
    }
    wchar_t *home = Py_DecodeLocale(env, NULL);
    if (home == NULL) {
        error("failed to decode TESTHOME");
        return 1;
    }
    Py_SetPythonHome(home);
    PyMem_RawFree(home);
    putenv("TESTHOME=");

    Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_is_python_build(void)
{
    // gh-91985: in-tree builds fail to check for build directory landmarks
    // under the effect of 'home' or PYTHONHOME environment variable.
    char *env = getenv("TESTHOME");
    if (!env) {
        error("missing TESTHOME env var");
        return 1;
    }
    wchar_t *home = Py_DecodeLocale(env, NULL);
    if (home == NULL) {
        error("failed to decode TESTHOME");
        return 1;
    }

    PyConfig config;
    _PyConfig_InitCompatConfig(&config);
    config_set_program_name(&config);
    config_set_string(&config, &config.home, home);
    PyMem_RawFree(home);
    putenv("TESTHOME=");

    // Use an impossible value so we can detect whether it isn't updated
    // during initialization.
    config._is_python_build = INT_MAX;
    env = getenv("NEGATIVE_ISPYTHONBUILD");
    if (env && strcmp(env, "0") != 0) {
        config._is_python_build = INT_MIN;
    }
    init_from_config_clear(&config);
    Py_Finalize();
    // Second initialization
    config._is_python_build = -1;
    init_from_config_clear(&config);
    dump_config();  // home and _is_python_build are cached in _Py_path_config
    Py_Finalize();
    return 0;
}


static int test_init_warnoptions(void)
{
    putenv("PYTHONWARNINGS=ignore:::env1,ignore:::env2");

    PySys_AddWarnOption(L"ignore:::PySys_AddWarnOption1");
    PySys_AddWarnOption(L"ignore:::PySys_AddWarnOption2");

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.dev_mode = 1;
    config.bytes_warning = 1;

    config_set_program_name(&config);

    PyStatus status;
    status = PyWideStringList_Append(&config.warnoptions,
                                     L"ignore:::PyConfig_BeforeRead");
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    wchar_t* argv[] = {
        L"python3",
        L"-Wignore:::cmdline1",
        L"-Wignore:::cmdline2"};
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config.parse_argv = 1;

    status = PyConfig_Read(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    status = PyWideStringList_Append(&config.warnoptions,
                                     L"ignore:::PyConfig_AfterRead");
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    status = PyWideStringList_Insert(&config.warnoptions,
                                     0, L"ignore:::PyConfig_Insert0");
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    init_from_config_clear(&config);
    dump_config();
    Py_Finalize();
    return 0;
}


static int initconfig_getint(PyInitConfig *config, const char *name)
{
    int64_t value;
    int res = PyInitConfig_GetInt(config, name, &value);
    assert(res == 0);
    assert(INT_MIN <= value && value <= INT_MAX);
    return (int)value;
}


static int test_initconfig_api(void)
{
    PyInitConfig *config = PyInitConfig_Create();
    if (config == NULL) {
        printf("Init allocation error\n");
        return 1;
    }

    if (PyInitConfig_SetInt(config, "configure_locale", 1) < 0) {
        goto error;
    }

    if (PyInitConfig_SetInt(config, "dev_mode", 1) < 0) {
        goto error;
    }

    if (PyInitConfig_SetInt(config, "hash_seed", 10) < 0) {
        goto error;
    }

    if (PyInitConfig_SetInt(config, "perf_profiling", 2) < 0) {
        goto error;
    }

    // Set a UTF-8 string (program_name)
    if (PyInitConfig_SetStr(config, "program_name", PROGRAM_NAME_UTF8) < 0) {
        goto error;
    }

    // Set a UTF-8 string (pycache_prefix)
    if (PyInitConfig_SetStr(config, "pycache_prefix",
                            "conf_pycache_prefix") < 0) {
        goto error;
    }

    // Set a list of UTF-8 strings (argv)
    char* xoptions[] = {"faulthandler"};
    if (PyInitConfig_SetStrList(config, "xoptions",
                                Py_ARRAY_LENGTH(xoptions), xoptions) < 0) {
        goto error;
    }

    if (Py_InitializeFromInitConfig(config) < 0) {
        goto error;
    }
    PyInitConfig_Free(config);
    PyInitConfig_Free(NULL);

    dump_config();
    Py_Finalize();
    return 0;

error:
    {
        const char *err_msg;
        (void)PyInitConfig_GetError(config, &err_msg);
        printf("Python init failed: %s\n", err_msg);
        exit(1);
    }
}


static int test_initconfig_get_api(void)
{
    PyInitConfig *config = PyInitConfig_Create();
    if (config == NULL) {
        printf("Init allocation error\n");
        return 1;
    }

    // test PyInitConfig_HasOption()
    assert(PyInitConfig_HasOption(config, "verbose") == 1);
    assert(PyInitConfig_HasOption(config, "utf8_mode") == 1);
    assert(PyInitConfig_HasOption(config, "non-existent") == 0);

    // test PyInitConfig_GetInt()
    assert(initconfig_getint(config, "dev_mode") == 0);
    assert(PyInitConfig_SetInt(config, "dev_mode", 1) == 0);
    assert(initconfig_getint(config, "dev_mode") == 1);

    // test PyInitConfig_GetInt() on a PyPreConfig option
    assert(initconfig_getint(config, "utf8_mode") == 0);
    assert(PyInitConfig_SetInt(config, "utf8_mode", 1) == 0);
    assert(initconfig_getint(config, "utf8_mode") == 1);

    // test PyInitConfig_GetStr()
    char *str;
    assert(PyInitConfig_GetStr(config, "program_name", &str) == 0);
    assert(str == NULL);
    assert(PyInitConfig_SetStr(config, "program_name", PROGRAM_NAME_UTF8) == 0);
    assert(PyInitConfig_GetStr(config, "program_name", &str) == 0);
    assert(strcmp(str, PROGRAM_NAME_UTF8) == 0);
    free(str);

    // test PyInitConfig_GetStrList() and PyInitConfig_FreeStrList()
    size_t length;
    char **items;
    assert(PyInitConfig_GetStrList(config, "xoptions", &length, &items) == 0);
    assert(length == 0);

    char* xoptions[] = {"faulthandler"};
    assert(PyInitConfig_SetStrList(config, "xoptions",
                                   Py_ARRAY_LENGTH(xoptions), xoptions) == 0);

    assert(PyInitConfig_GetStrList(config, "xoptions", &length, &items) == 0);
    assert(length == 1);
    assert(strcmp(items[0], "faulthandler") == 0);
    PyInitConfig_FreeStrList(length, items);

    // Setting hash_seed sets use_hash_seed
    assert(initconfig_getint(config, "use_hash_seed") == 0);
    assert(PyInitConfig_SetInt(config, "hash_seed", 123) == 0);
    assert(initconfig_getint(config, "use_hash_seed") == 1);

    // Setting module_search_paths sets module_search_paths_set
    assert(initconfig_getint(config, "module_search_paths_set") == 0);
    char* paths[] = {"search", "path"};
    assert(PyInitConfig_SetStrList(config, "module_search_paths",
                                   Py_ARRAY_LENGTH(paths), paths) == 0);
    assert(initconfig_getint(config, "module_search_paths_set") == 1);

    return 0;
}


static int test_initconfig_exit(void)
{
    PyInitConfig *config = PyInitConfig_Create();
    if (config == NULL) {
        printf("Init allocation error\n");
        return 1;
    }

    char *argv[] = {PROGRAM_NAME_UTF8, "--help"};
    assert(PyInitConfig_SetStrList(config, "argv",
                                   Py_ARRAY_LENGTH(argv), argv) == 0);

    assert(PyInitConfig_SetInt(config, "parse_argv", 1) == 0);

    assert(Py_InitializeFromInitConfig(config) < 0);

    int exitcode;
    assert(PyInitConfig_GetExitCode(config, &exitcode) == 1);
    assert(exitcode == 0);

    const char *err_msg;
    assert(PyInitConfig_GetError(config, &err_msg) == 1);
    assert(strcmp(err_msg, "exit code 0") == 0);

    PyInitConfig_Free(config);
    return 0;
}


static PyModuleDef_Slot extension_slots[] = {
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef extension_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "my_test_extension",
    .m_size = 0,
    .m_slots = extension_slots,
};

static PyObject* init_my_test_extension(void)
{
    return PyModuleDef_Init(&extension_module);
}


static int test_initconfig_module(void)
{
    PyInitConfig *config = PyInitConfig_Create();
    if (config == NULL) {
        printf("Init allocation error\n");
        return 1;
    }

    if (PyInitConfig_SetStr(config, "program_name", PROGRAM_NAME_UTF8) < 0) {
        goto error;
    }

    if (PyInitConfig_AddModule(config, "my_test_extension",
                               init_my_test_extension) < 0) {
        goto error;
    }

    if (Py_InitializeFromInitConfig(config) < 0) {
        goto error;
    }
    PyInitConfig_Free(config);

    if (PyRun_SimpleString("import my_test_extension") < 0) {
        fprintf(stderr, "unable to import my_test_extension\n");
        exit(1);
    }

    Py_Finalize();
    return 0;

error:
    {
        const char *err_msg;
        (void)PyInitConfig_GetError(config, &err_msg);
        printf("Python init failed: %s\n", err_msg);
        exit(1);
    }
}


static void configure_init_main(PyConfig *config)
{
    wchar_t* argv[] = {
        L"python3", L"-c",
        (L"import _testinternalcapi, json; "
         L"print(json.dumps(_testinternalcapi.get_configs()))"),
        L"arg2"};

    config->parse_argv = 1;

    config_set_argv(config, Py_ARRAY_LENGTH(argv), argv);
    config_set_string(config, &config->program_name, L"./python3");
}


static int test_init_run_main(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    configure_init_main(&config);
    init_from_config_clear(&config);

    return Py_RunMain();
}


static int test_run_main(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    wchar_t *argv[] = {L"python3", L"-c",
                       (L"import sys; "
                        L"print(f'Py_RunMain(): sys.argv={sys.argv}')"),
                       L"arg2"};
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config_set_string(&config, &config.program_name, L"./python3");
    init_from_config_clear(&config);

    return Py_RunMain();
}


static int test_run_main_loop(void)
{
    // bpo-40413: Calling Py_InitializeFromConfig()+Py_RunMain() multiple
    // times must not crash.
    for (int i=0; i<5; i++) {
        int exitcode = test_run_main();
        if (exitcode != 0) {
            return exitcode;
        }
    }
    return 0;
}


static int test_get_argc_argv(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    wchar_t *argv[] = {L"python3", L"-c", L"pass", L"arg2"};
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    config_set_string(&config, &config.program_name, L"./python3");

    // Calling PyConfig_Read() twice must not change Py_GetArgcArgv() result.
    // The second call is done by Py_InitializeFromConfig().
    PyStatus status = PyConfig_Read(&config);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

    init_from_config_clear(&config);

    int get_argc;
    wchar_t **get_argv;
    Py_GetArgcArgv(&get_argc, &get_argv);
    printf("argc: %i\n", get_argc);
    assert(get_argc == Py_ARRAY_LENGTH(argv));
    for (int i=0; i < get_argc; i++) {
        printf("argv[%i]: %ls\n", i, get_argv[i]);
        assert(wcscmp(get_argv[i], argv[i]) == 0);
    }

    Py_Finalize();

    printf("\n");
    printf("test ok\n");
    return 0;
}


static int check_use_frozen_modules(const char *rawval)
{
    wchar_t optval[100];
    if (rawval == NULL) {
        wcscpy(optval, L"frozen_modules");
    }
    else if (swprintf(optval, 100,
#if defined(_MSC_VER)
        L"frozen_modules=%S",
#else
        L"frozen_modules=%s",
#endif
        rawval) < 0) {
        error("rawval is too long");
        return -1;
    }

    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    config.parse_argv = 1;

    wchar_t* argv[] = {
        L"./argv0",
        L"-X",
        optval,
        L"-c",
        L"pass",
    };
    config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
    init_from_config_clear(&config);

    dump_config();
    Py_Finalize();
    return 0;
}

static int test_init_use_frozen_modules(void)
{
    const char *envvar = getenv("TESTFROZEN");
    return check_use_frozen_modules(envvar);
}


static int test_unicode_id_init(void)
{
    // bpo-42882: Test that _PyUnicode_FromId() works
    // when Python is initialized multiples times.

    // This is equivalent to `_Py_IDENTIFIER(test_unicode_id_init)`
    // but since `_Py_IDENTIFIER` is disabled when `Py_BUILD_CORE`
    // is defined, it is manually expanded here.
    static _Py_Identifier PyId_test_unicode_id_init = {
        .string = "test_unicode_id_init",
        .index = -1,
    };

    // Initialize Python once without using the identifier
    _testembed_initialize();
    Py_Finalize();

    // Now initialize Python multiple times and use the identifier.
    // The first _PyUnicode_FromId() call initializes the identifier index.
    for (int i=0; i<3; i++) {
        _testembed_initialize();

        PyObject *str1, *str2;

        str1 = _PyUnicode_FromId(&PyId_test_unicode_id_init);
        assert(str1 != NULL);
        assert(_Py_IsImmortal(str1));

        str2 = PyUnicode_FromString("test_unicode_id_init");
        assert(str2 != NULL);

        assert(PyUnicode_Compare(str1, str2) == 0);

        Py_DECREF(str2);

        Py_Finalize();
    }
    return 0;
}


static int test_init_main_interpreter_settings(void)
{
    _testembed_initialize();
    (void) PyRun_SimpleStringFlags(
        "import _testinternalcapi, json; "
        "print(json.dumps(_testinternalcapi.get_interp_settings(0)))",
        0);
    Py_Finalize();
    return 0;
}

static void do_init(void *unused)
{
    _testembed_initialize();
    Py_Finalize();
}

static int test_init_in_background_thread(void)
{
    PyThread_handle_t handle;
    PyThread_ident_t ident;
    if (PyThread_start_joinable_thread(&do_init, NULL, &ident, &handle) < 0) {
        return -1;
    }
    return PyThread_join_thread(handle);
}


#ifndef MS_WINDOWS
#include "test_frozenmain.h"      // M_test_frozenmain

static int test_frozenmain(void)
{
    static struct _frozen frozen_modules[4] = {
        {"__main__", M_test_frozenmain, sizeof(M_test_frozenmain)},
        {0, 0, 0}   // sentinel
    };

    char* argv[] = {
        "./argv0",
        "-E",
        "arg1",
        "arg2",
    };
    PyImport_FrozenModules = frozen_modules;
    return Py_FrozenMain(Py_ARRAY_LENGTH(argv), argv);
}
#endif  // !MS_WINDOWS

static int test_repeated_init_and_inittab(void)
{
    // bpo-44441: Py_RunMain() must reset PyImport_Inittab at exit.
    // It must be possible to call PyImport_AppendInittab() or
    // PyImport_ExtendInittab() before each Python initialization.
    for (int i=1; i <= INIT_LOOPS; i++) {
        printf("--- Pass %d ---\n", i);

        // Call PyImport_AppendInittab() at each iteration
        if (PyImport_AppendInittab(EMBEDDED_EXT_NAME,
                                   &PyInit_embedded_ext) != 0) {
            fprintf(stderr, "PyImport_AppendInittab() failed\n");
            return 1;
        }

        // Initialize Python
        wchar_t* argv[] = {PROGRAM_NAME, L"-c", L"pass"};
        PyConfig config;
        PyConfig_InitPythonConfig(&config);
        config.isolated = 1;
        config_set_argv(&config, Py_ARRAY_LENGTH(argv), argv);
        init_from_config_clear(&config);

        // Py_RunMain() calls _PyImport_Fini2() which resets PyImport_Inittab
        int exitcode = Py_RunMain();
        if (exitcode != 0) {
            return exitcode;
        }
    }
    return 0;
}

static void wrap_allocator(PyMemAllocatorEx *allocator);
static void unwrap_allocator(PyMemAllocatorEx *allocator);

static void *
malloc_wrapper(void *ctx, size_t size)
{
    PyMemAllocatorEx *allocator = (PyMemAllocatorEx *)ctx;
    unwrap_allocator(allocator);
    PyEval_GetFrame();  // BOOM!
    wrap_allocator(allocator);
    return allocator->malloc(allocator->ctx, size);
}

static void *
calloc_wrapper(void *ctx, size_t nelem, size_t elsize)
{
    PyMemAllocatorEx *allocator = (PyMemAllocatorEx *)ctx;
    return allocator->calloc(allocator->ctx, nelem, elsize);
}

static void *
realloc_wrapper(void *ctx, void *ptr, size_t new_size)
{
    PyMemAllocatorEx *allocator = (PyMemAllocatorEx *)ctx;
    return allocator->realloc(allocator->ctx, ptr, new_size);
}

static void
free_wrapper(void *ctx, void *ptr)
{
    PyMemAllocatorEx *allocator = (PyMemAllocatorEx *)ctx;
    allocator->free(allocator->ctx, ptr);
}

static void
wrap_allocator(PyMemAllocatorEx *allocator)
{
    PyMem_GetAllocator(PYMEM_DOMAIN_OBJ, allocator);
    PyMemAllocatorEx wrapper = {
        .malloc = &malloc_wrapper,
        .calloc = &calloc_wrapper,
        .realloc = &realloc_wrapper,
        .free = &free_wrapper,
        .ctx = allocator,
    };
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, &wrapper);
}

static void
unwrap_allocator(PyMemAllocatorEx *allocator)
{
    PyMem_SetAllocator(PYMEM_DOMAIN_OBJ, allocator);
}

static int
test_get_incomplete_frame(void)
{
    _testembed_initialize();
    PyMemAllocatorEx allocator;
    wrap_allocator(&allocator);
    // Force an allocation with an incomplete (generator) frame:
    int result = PyRun_SimpleString("(_ for _ in ())");
    unwrap_allocator(&allocator);
    Py_Finalize();
    return result;
}

static void
do_gilstate_ensure(void *event_ptr)
{
    PyEvent *event = (PyEvent *)event_ptr;
    // Signal to the calling thread that we've started
    _PyEvent_Notify(event);
    PyGILState_Ensure(); // This should hang
    assert(NULL);
}

static int
test_gilstate_after_finalization(void)
{
    _testembed_initialize();
    Py_Finalize();
    PyThread_handle_t handle;
    PyThread_ident_t ident;
    PyEvent event = {0};
    if (PyThread_start_joinable_thread(&do_gilstate_ensure, &event, &ident, &handle) < 0) {
        return -1;
    }
    PyEvent_Wait(&event);
    // We're now pretty confident that the thread went for
    // PyGILState_Ensure(), but that means it got hung.
    return PyThread_detach_thread(handle);
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
    // Python initialization
    {"test_repeated_init_exec", test_repeated_init_exec},
    {"test_repeated_simple_init", test_repeated_simple_init},
    {"test_forced_io_encoding", test_forced_io_encoding},
    {"test_import_in_subinterpreters", test_import_in_subinterpreters},
    {"test_repeated_init_and_subinterpreters", test_repeated_init_and_subinterpreters},
    {"test_repeated_init_and_inittab", test_repeated_init_and_inittab},
    {"test_pre_initialization_api", test_pre_initialization_api},
    {"test_pre_initialization_sys_options", test_pre_initialization_sys_options},
    {"test_bpo20891", test_bpo20891},
    {"test_initialize_twice", test_initialize_twice},
    {"test_initialize_pymain", test_initialize_pymain},
    {"test_init_initialize_config", test_init_initialize_config},
    {"test_preinit_compat_config", test_preinit_compat_config},
    {"test_init_compat_config", test_init_compat_config},
    {"test_init_global_config", test_init_global_config},
    {"test_init_from_config", test_init_from_config},
    {"test_init_parse_argv", test_init_parse_argv},
    {"test_init_dont_parse_argv", test_init_dont_parse_argv},
    {"test_init_compat_env", test_init_compat_env},
    {"test_init_python_env", test_init_python_env},
    {"test_init_env_dev_mode", test_init_env_dev_mode},
    {"test_init_env_dev_mode_alloc", test_init_env_dev_mode_alloc},
    {"test_init_dont_configure_locale", test_init_dont_configure_locale},
    {"test_init_dev_mode", test_init_dev_mode},
    {"test_init_isolated_flag", test_init_isolated_flag},
    {"test_preinit_isolated_config", test_preinit_isolated_config},
    {"test_init_isolated_config", test_init_isolated_config},
    {"test_preinit_python_config", test_preinit_python_config},
    {"test_init_python_config", test_init_python_config},
    {"test_preinit_isolated1", test_preinit_isolated1},
    {"test_preinit_isolated2", test_preinit_isolated2},
    {"test_preinit_parse_argv", test_preinit_parse_argv},
    {"test_preinit_dont_parse_argv", test_preinit_dont_parse_argv},
    {"test_init_read_set", test_init_read_set},
    {"test_init_run_main", test_init_run_main},
    {"test_init_sys_add", test_init_sys_add},
    {"test_init_setpath", test_init_setpath},
    {"test_init_setpath_config", test_init_setpath_config},
    {"test_init_setpythonhome", test_init_setpythonhome},
    {"test_init_is_python_build", test_init_is_python_build},
    {"test_init_warnoptions", test_init_warnoptions},
    {"test_initconfig_api", test_initconfig_api},
    {"test_initconfig_get_api", test_initconfig_get_api},
    {"test_initconfig_exit", test_initconfig_exit},
    {"test_initconfig_module", test_initconfig_module},
    {"test_run_main", test_run_main},
    {"test_run_main_loop", test_run_main_loop},
    {"test_get_argc_argv", test_get_argc_argv},
    {"test_init_use_frozen_modules", test_init_use_frozen_modules},
    {"test_init_main_interpreter_settings", test_init_main_interpreter_settings},
    {"test_init_in_background_thread", test_init_in_background_thread},

    // Audit
    {"test_open_code_hook", test_open_code_hook},
    {"test_audit", test_audit},
    {"test_audit_tuple", test_audit_tuple},
    {"test_audit_subinterpreter", test_audit_subinterpreter},
    {"test_audit_run_command", test_audit_run_command},
    {"test_audit_run_file", test_audit_run_file},
    {"test_audit_run_interactivehook", test_audit_run_interactivehook},
    {"test_audit_run_startup", test_audit_run_startup},
    {"test_audit_run_stdin", test_audit_run_stdin},

    // Specific C API
    {"test_unicode_id_init", test_unicode_id_init},
#ifndef MS_WINDOWS
    {"test_frozenmain", test_frozenmain},
#endif
    {"test_get_incomplete_frame", test_get_incomplete_frame},
    {"test_gilstate_after_finalization", test_gilstate_after_finalization},
    {NULL, NULL}
};


int main(int argc, char *argv[])
{
    main_argc = argc;
    main_argv = argv;

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
