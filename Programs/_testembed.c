#ifndef Py_BUILD_CORE_MODULE
#  define Py_BUILD_CORE_MODULE
#endif

/* Always enable assertion (even in release mode) */
#undef NDEBUG

#include <Python.h>
#include "pycore_initconfig.h"    // _PyConfig_InitCompatConfig()
#include "pycore_runtime.h"       // _PyRuntime
#include <Python.h>
#include <inttypes.h>
#include <stdio.h>
#include <wchar.h>

/*********************************************************
 * Embedded interpreter tests that need a custom exe
 *
 * Executed via 'EmbeddingTests' in Lib/test/test_capi.py
 *********************************************************/

/* Use path starting with "./" avoids a search along the PATH */
#define PROGRAM_NAME L"./_testembed"

static void _testembed_Py_Initialize(void)
{
    Py_SetProgramName(PROGRAM_NAME);
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
    check_stdio_details("iso8859-1", NULL);
    printf("--- Set encoding and errors ---\n");
    check_stdio_details("iso8859-1", "replace");

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
    /* the test doesn't support custom memory allocators */
    putenv("PYTHONMALLOC=");

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
    /* the test doesn't support custom memory allocators */
    putenv("PYTHONMALLOC=");

    /* bpo-20891: Calling PyGILState_Ensure in a non-Python thread must not
       crash. */
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
                       (L"import sys; "
                        L"print(f'Py_Main() after Py_Initialize: "
                        L"sys.argv={sys.argv}')"),
                       L"arg2"};
    _testembed_Py_Initialize();

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
    _testembed_Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static void config_set_string(PyConfig *config, wchar_t **config_str, const wchar_t *str)
{
    PyStatus status = PyConfig_SetString(config, config_str, str);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(config);
        Py_ExitStatusException(status);
    }
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
    PyPreConfig preconfig;
    _PyPreConfig_InitCompatConfig(&preconfig);

    putenv("PYTHONMALLOC=malloc_debug");
    preconfig.allocator = PYMEM_ALLOCATOR_MALLOC;

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

    putenv("PYTHONOLDPARSER=1");
    config._use_peg_parser = 0;

    /* FIXME: test use_environment */

    putenv("PYTHONHASHSEED=42");
    config.use_hash_seed = 1;
    config.hash_seed = 123;

    /* dev_mode=1 is tested in test_init_dev_mode() */

    putenv("PYTHONFAULTHANDLER=");
    config.faulthandler = 1;

    putenv("PYTHONTRACEMALLOC=0");
    config.tracemalloc = 2;

    putenv("PYTHONPROFILEIMPORTTIME=0");
    config.import_time = 1;

    config.show_ref_count = 1;
    /* FIXME: test dump_refs: bpo-34223 */

    putenv("PYTHONMALLOCSTATS=0");
    config.malloc_stats = 1;

    putenv("PYTHONPYCACHEPREFIX=env_pycache_prefix");
    config_set_string(&config, &config.pycache_prefix, L"conf_pycache_prefix");

    Py_SetProgramName(L"./globalvar");
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
    Py_SetStandardStreamEncoding("ascii", "ignore");
#ifdef MS_WINDOWS
    /* Py_SetStandardStreamEncoding() sets Py_LegacyWindowsStdioFlag to 1.
       Force it to 0 through the config. */
    config.legacy_windows_stdio = 0;
#endif
    config_set_string(&config, &config.stdio_encoding, L"iso8859-1");
    config_set_string(&config, &config.stdio_errors, L"replace");

    putenv("PYTHONNOUSERSITE=");
    Py_NoUserSiteDirectory = 0;
    config.user_site_directory = 0;

    config_set_string(&config, &config.check_hash_pycs_mode, L"always");

    Py_FrozenFlag = 0;
    config.pathconfig_warnings = 0;

    config._isolated_interpreter = 1;

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
    putenv("PYTHONMALLOC=malloc");
    putenv("PYTHONTRACEMALLOC=2");
    putenv("PYTHONPROFILEIMPORTTIME=1");
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
    putenv("PYTHONOLDPARSER=1");
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
    _testembed_Py_Initialize();
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
    _testembed_Py_Initialize();
    dump_config();
    Py_Finalize();
    return 0;
}


static int test_init_env_dev_mode_alloc(void)
{
    /* Test initialization from environment variables */
    Py_IgnoreEnvironmentFlag = 0;
    set_all_env_vars_dev_mode();
    putenv("PYTHONMALLOC=malloc");
    _testembed_Py_Initialize();
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
    wchar_t *argv[] = {L"python3", L"-X", L"dev", L"script.py"};
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
    _testembed_Py_Initialize();
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

static int _audit_hook(const char *event, PyObject *args, void *userdata)
{
    if (strcmp(event, "_testembed.raise") == 0) {
        PyErr_SetString(PyExc_RuntimeError, "Intentional error");
        return -1;
    } else if (strcmp(event, "_testembed.set") == 0) {
        if (!PyArg_ParseTuple(args, "n", userdata)) {
            return -1;
        }
        return 0;
    }
    return 0;
}

static int _test_audit(Py_ssize_t setValue)
{
    Py_ssize_t sawSet = 0;

    Py_IgnoreEnvironmentFlag = 0;
    PySys_AddAuditHook(_audit_hook, &sawSet);
    _testembed_Py_Initialize();

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

    if (sawSet != 42) {
        printf("Failed to see *userData change\n");
        return 5;
    }
    return 0;
}

static int test_audit(void)
{
    int result = _test_audit(42);
    Py_Finalize();
    return result;
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
    _testembed_Py_Initialize();

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

    status = PyConfig_SetBytesString(&config, &config.program_name,
                                     "./init_read_set");
    if (PyStatus_Exception(status)) {
        goto fail;
    }

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
        fprintf(stderr, "missing TESTPATH env var\n");
        return 1;
    }
    wchar_t *path = Py_DecodeLocale(env, NULL);
    if (path == NULL) {
        fprintf(stderr, "failed to decode TESTPATH\n");
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
        fprintf(stderr, "missing TESTPATH env var\n");
        return 1;
    }
    wchar_t *path = Py_DecodeLocale(env, NULL);
    if (path == NULL) {
        fprintf(stderr, "failed to decode TESTPATH\n");
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
        fprintf(stderr, "missing TESTHOME env var\n");
        return 1;
    }
    wchar_t *home = Py_DecodeLocale(env, NULL);
    if (home == NULL) {
        fprintf(stderr, "failed to decode TESTHOME\n");
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


static int test_init_main(void)
{
    PyConfig config;
    PyConfig_InitPythonConfig(&config);

    configure_init_main(&config);
    config._init_main = 0;
    init_from_config_clear(&config);

    /* sys.stdout don't exist yet: it is created by _Py_InitializeMain() */
    int res = PyRun_SimpleString(
        "import sys; "
        "print('Run Python code before _Py_InitializeMain', "
               "file=sys.stderr)");
    if (res < 0) {
        exit(1);
    }

    PyStatus status = _Py_InitializeMain();
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

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
    {"test_forced_io_encoding", test_forced_io_encoding},
    {"test_repeated_init_and_subinterpreters", test_repeated_init_and_subinterpreters},
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
    {"test_init_main", test_init_main},
    {"test_init_sys_add", test_init_sys_add},
    {"test_init_setpath", test_init_setpath},
    {"test_init_setpath_config", test_init_setpath_config},
    {"test_init_setpythonhome", test_init_setpythonhome},
    {"test_init_warnoptions", test_init_warnoptions},
    {"test_run_main", test_run_main},

    {"test_open_code_hook", test_open_code_hook},
    {"test_audit", test_audit},
    {"test_audit_subinterpreter", test_audit_subinterpreter},
    {"test_audit_run_command", test_audit_run_command},
    {"test_audit_run_file", test_audit_run_file},
    {"test_audit_run_interactivehook", test_audit_run_interactivehook},
    {"test_audit_run_startup", test_audit_run_startup},
    {"test_audit_run_stdin", test_audit_run_stdin},
    {NULL, NULL}
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
