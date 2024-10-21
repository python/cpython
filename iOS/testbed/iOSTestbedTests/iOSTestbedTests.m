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
        "-uall",  // Enable all resources
        "--single-process",  // always run all tests sequentially in a single process
        "--rerun",  // Re-run failed tests in verbose mode
        "-W",  // Display test output on failure
        // To run a subset of tests, add the test names below; e.g.,
        // "test_os",
        // "test_sys",
    };

    // Start a Python interpreter.
    int exit_code;
    PyStatus status;
    PyPreConfig preconfig;
    PyConfig config;
    NSString *python_home;
    wchar_t *wtmp_str;

    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];

    // Generate an isolated Python configuration.
    NSLog(@"Configuring isolated Python...");
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
    // Ensure that signal handlers are installed
    config.install_signal_handlers = 1;
    // Run the test module.
    config.run_module = Py_DecodeLocale("test", NULL);
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

    // Start the test suite. Print a separator to differentiate Python startup logs from app logs
    NSLog(@"---------------------------------------------------------------------------");

    exit_code = Py_RunMain();
    XCTAssertEqual(exit_code, 0, @"Python test suite did not pass");

    NSLog(@"---------------------------------------------------------------------------");

    Py_Finalize();
}


@end
