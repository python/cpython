#include <Python.h>
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
