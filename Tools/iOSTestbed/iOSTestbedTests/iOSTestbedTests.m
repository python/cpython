#import <XCTest/XCTest.h>
#import <Python/Python.h>

@interface iOSTestbedTests : XCTestCase

@end

@implementation iOSTestbedTests


- (void)testPython {
    // Arguments to pass into the test suite runner.
    // argv[0] must identify the process; any subsequent arg
    // will be handled as if it were an argument to `python -m test`
    const char *argv[] = {
        "iOSTestbed", // argv[0] is the process that is running.
        "-uall,-gui,-curses",  // Enable most resources; GUI and curses tests won't work on iOS
        "-v",  // run in verbose mode so we get test failure information
        // To run a subset of tests, add the test names below; e.g.,
        // "test_os",
        // "test_sys",
    };

    // Start a Python interpreter.
    int success = -1;
    PyStatus status;
    PyPreConfig preconfig;
    PyConfig config;
    NSString *python_home;
    NSString *path;
    wchar_t *wtmp_str;

    PyObject *app_module;
    PyObject *module;
    PyObject *module_attr;
    PyObject *method_args;
    PyObject *result;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_traceback;
    PyObject *systemExit_code;

    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];

    // Extract Python version from bundle
    NSString *py_version_string = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"];

    // Generate an isolated Python configuration.
    NSLog(@"Configuring isolated Python %@...", py_version_string);
    PyPreConfig_InitIsolatedConfig(&preconfig);
    PyConfig_InitIsolatedConfig(&config);

    // Configure the Python interpreter:
    // Enforce UTF-8 encoding for stderr, stdout, file-system encoding and locale.
    // See https://docs.python.org/3/library/os.html#python-utf-8-mode.
    preconfig.utf8_mode = 1;
    // Don't buffer stdio. We want output to appears in the log immediately
    config.buffered_stdio = 0;
    // Don't write bytecode; we can't modify the app bundle
    // after it has been signed.
    config.write_bytecode = 0;
    // For debugging - enable verbose mode.
    // config.verbose = 1;

    NSLog(@"Pre-initializing Python runtime...");
    status = Py_PreInitialize(&preconfig);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to pre-initialize Python interpreter: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }

    // Set the home for the Python interpreter
    python_home = [NSString stringWithFormat:@"%@/python", resourcePath, nil];
    NSLog(@"PythonHome: %@", python_home);
    wtmp_str = Py_DecodeLocale([python_home UTF8String], NULL);
    status = PyConfig_SetString(&config, &config.home, wtmp_str);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to set PYTHONHOME: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }
    PyMem_RawFree(wtmp_str);

    // Set the stdlib location for the Python interpreter
    path = [NSString stringWithFormat:@"%@/python/lib/python%@", resourcePath, py_version_string, nil];
    NSLog(@"Stdlib dir: %@", path);
    wtmp_str = Py_DecodeLocale([path UTF8String], NULL);
    status = PyConfig_SetString(&config, &config.stdlib_dir, wtmp_str);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to set stdlib dir: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }
    PyMem_RawFree(wtmp_str);

    // Read the site config
    status = PyConfig_Read(&config);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to read site config: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }

    NSLog(@"Configure argc/argv...");
    status = PyConfig_SetBytesArgv(&config, sizeof(argv) / sizeof(char *), (char**) argv);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to configure argc/argv: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }

    NSLog(@"Initializing Python runtime...");
    status = Py_InitializeFromConfig(&config);
    if (PyStatus_Exception(status)) {
        XCTFail(@"Unable to initialize Python interpreter: %s", status.err_msg);
        PyConfig_Clear(&config);
        return;
    }

    // Start the test suite.
    //
    // From here to Py_ObjectCall(runmodule...) is effectively
    // a copy of Py_RunMain() (and, more specifically, the
    // pymain_run_module() method); we need to re-implement it
    // because we need to be able to inspect the error state of
    // the interpreter, not just the return code of the module.
    NSLog(@"Running CPython test suite");
    module = PyImport_ImportModule("runpy");
    if (module == NULL) {
        XCTFail(@"Could not import runpy module");
    }

    module_attr = PyObject_GetAttrString(module, "_run_module_as_main");
    if (module_attr == NULL) {
        XCTFail(@"Could not access runpy._run_module_as_main");
    }

    app_module = PyUnicode_FromString("test");
    if (app_module == NULL) {
        XCTFail(@"Could not convert module name to unicode");
    }

    method_args = Py_BuildValue("(Oi)", app_module, 0);
    if (method_args == NULL) {
        XCTFail(@"Could not create arguments for runpy._run_module_as_main");
    }

    // Print a separator to differentiate Python startup logs from app logs
    NSLog(@"---------------------------------------------------------------------------");

    // Invoke the app module
    result = PyObject_Call(module_attr, method_args, NULL);

    NSLog(@"---------------------------------------------------------------------------");

    // The test method doesn't return an object of any interest;
    // but if the call returns NULL, there's been a problem.
    if (result == NULL) {
        // Retrieve the current error state of the interpreter.
        PyErr_Fetch(&exc_type, &exc_value, &exc_traceback);
        PyErr_NormalizeException(&exc_type, &exc_value, &exc_traceback);

        if (exc_traceback == NULL) {
            XCTFail(@"Could not retrieve traceback");
        }

        if (PyErr_GivenExceptionMatches(exc_value, PyExc_SystemExit)) {
            systemExit_code = PyObject_GetAttrString(exc_value, "code");
            if (systemExit_code == NULL) {
                XCTFail(@"Could not determine exit code");
            }
            else {
                success = (int) PyLong_AsLong(systemExit_code);
                XCTAssertEqual(success, 0, @"Python test suite did not pass");
            }
        } else {
            PyErr_DisplayException(exc_value);
            XCTFail(@"Test suite generated exception");
        }
    }
    Py_Finalize();
}


@end
