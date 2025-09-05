#import <XCTest/XCTest.h>
#import <Python/Python.h>

@interface iOSTestbedTests : XCTestCase

@end

@implementation iOSTestbedTests


- (void)testPython {
    const char **argv;
    int exit_code;
    int failed;
    PyStatus status;
    PyPreConfig preconfig;
    PyConfig config;
    PyObject *app_packages_path;
    PyObject *method_args;
    PyObject *result;
    PyObject *site_module;
    PyObject *site_addsitedir_attr;
    PyObject *sys_module;
    PyObject *sys_path_attr;
    NSArray *test_args;
    NSString *python_home;
    NSString *path;
    wchar_t *wtmp_str;

    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];

    // Set some other common environment indicators to disable color, as the
    // Xcode log can't display color. Stdout will report that it is *not* a
    // TTY.
    setenv("NO_COLOR", "1", true);
    setenv("PYTHON_COLORS", "0", true);

    // Arguments to pass into the test suite runner.
    // argv[0] must identify the process; any subsequent arg
    // will be handled as if it were an argument to `python -m test`
    // The processInfo arguments contain the binary that is running,
    // followed by the arguments defined in the test plan. This means:
    //    run_module = test_args[1]
    //    argv = ["iOSTestbed"] + test_args[2:]
    test_args = [[NSProcessInfo processInfo] arguments];
    if (test_args == NULL) {
        NSLog(@"Unable to identify test arguments.");
    }
    NSLog(@"Test arguments: %@", test_args);
    argv = malloc(sizeof(char *) * ([test_args count] - 1));
    argv[0] = "iOSTestbed";
    for (int i = 1; i < [test_args count] - 1; i++) {
        argv[i] = [[test_args objectAtIndex:i+1] UTF8String];
    }

    // Generate an isolated Python configuration.
    NSLog(@"Configuring isolated Python...");
    PyPreConfig_InitIsolatedConfig(&preconfig);
    PyConfig_InitIsolatedConfig(&config);

    // Configure the Python interpreter:
    // Enforce UTF-8 encoding for stderr, stdout, file-system encoding and locale.
    // See https://docs.python.org/3/library/os.html#python-utf-8-mode.
    preconfig.utf8_mode = 1;
    // Use the system logger for stdout/err
    config.use_system_logger = 1;
    // Don't buffer stdio. We want output to appears in the log immediately
    config.buffered_stdio = 0;
    // Don't write bytecode; we can't modify the app bundle
    // after it has been signed.
    config.write_bytecode = 0;
    // Ensure that signal handlers are installed
    config.install_signal_handlers = 1;
    // Run the test module.
    config.run_module = Py_DecodeLocale([[test_args objectAtIndex:1] UTF8String], NULL);
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
    status = PyConfig_SetBytesArgv(&config, [test_args count] - 1, (char**) argv);
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

    // Add app_packages as a site directory. This both adds to sys.path,
    // and ensures that any .pth files in that directory will be executed.
    site_module = PyImport_ImportModule("site");
    if (site_module == NULL) {
        XCTFail(@"Could not import site module");
        return;
    }

    site_addsitedir_attr = PyObject_GetAttrString(site_module, "addsitedir");
    if (site_addsitedir_attr == NULL || !PyCallable_Check(site_addsitedir_attr)) {
        XCTFail(@"Could not access site.addsitedir");
        return;
    }

    path = [NSString stringWithFormat:@"%@/app_packages", resourcePath, nil];
    NSLog(@"App packages path: %@", path);
    wtmp_str = Py_DecodeLocale([path UTF8String], NULL);
    app_packages_path = PyUnicode_FromWideChar(wtmp_str, wcslen(wtmp_str));
    if (app_packages_path == NULL) {
        XCTFail(@"Could not convert app_packages path to unicode");
        return;
    }
    PyMem_RawFree(wtmp_str);

    method_args = Py_BuildValue("(O)", app_packages_path);
    if (method_args == NULL) {
        XCTFail(@"Could not create arguments for site.addsitedir");
        return;
    }

    result = PyObject_CallObject(site_addsitedir_attr, method_args);
    if (result == NULL) {
        XCTFail(@"Could not add app_packages directory using site.addsitedir");
        return;
    }

    // Add test code to sys.path
    sys_module = PyImport_ImportModule("sys");
    if (sys_module == NULL) {
        XCTFail(@"Could not import sys module");
        return;
    }

    sys_path_attr = PyObject_GetAttrString(sys_module, "path");
    if (sys_path_attr == NULL) {
        XCTFail(@"Could not access sys.path");
        return;
    }

    path = [NSString stringWithFormat:@"%@/app", resourcePath, nil];
    NSLog(@"App path: %@", path);
    wtmp_str = Py_DecodeLocale([path UTF8String], NULL);
    failed = PyList_Insert(sys_path_attr, 0, PyUnicode_FromString([path UTF8String]));
    if (failed) {
        XCTFail(@"Unable to add app to sys.path");
        return;
    }
    PyMem_RawFree(wtmp_str);

    // Ensure the working directory is the app folder.
    chdir([path UTF8String]);

    // Start the test suite. Print a separator to differentiate Python startup logs from app logs
    NSLog(@"---------------------------------------------------------------------------");

    exit_code = Py_RunMain();
    XCTAssertEqual(exit_code, 0, @"Test suite did not pass");

    NSLog(@"---------------------------------------------------------------------------");

    Py_Finalize();
}


@end
