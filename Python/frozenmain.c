/* Python interpreter main program for frozen scripts */

#include "Python.h"
#include "pycore_runtime.h"  // _PyRuntime_Initialize()
#include <locale.h>

#ifdef MS_WINDOWS
extern void PyWinFreeze_ExeInit(void);
extern void PyWinFreeze_ExeTerm(void);
extern int PyInitFrozenExtensions(void);
#endif

/* Main program */

int
Py_FrozenMain(int argc, char **argv)
{
    PyStatus status = _PyRuntime_Initialize();
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    // Suppress errors from getpath.c
    config.pathconfig_warnings = 0;
    // Don't parse command line options like -E
    config.parse_argv = 0;

    status = PyConfig_SetBytesArgv(&config, argc, argv);
    if (PyStatus_Exception(status)) {
        PyConfig_Clear(&config);
        Py_ExitStatusException(status);
    }

    const char *p;
    int inspect = 0;
    if ((p = Py_GETENV("PYTHONINSPECT")) && *p != '\0') {
        inspect = 1;
    }

#ifdef MS_WINDOWS
    PyInitFrozenExtensions();
#endif /* MS_WINDOWS */

    status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        Py_ExitStatusException(status);
    }

#ifdef MS_WINDOWS
    PyWinFreeze_ExeInit();
#endif

    if (Py_VerboseFlag) {
        fprintf(stderr, "Python %s\n%s\n",
                Py_GetVersion(), Py_GetCopyright());
    }

    int sts = 1;
    int n = PyImport_ImportFrozenModule("__main__");
    if (n == 0) {
        Py_FatalError("the __main__ module is not frozen");
    }
    if (n < 0) {
        PyErr_Print();
        sts = 1;
    }
    else {
        sts = 0;
    }

    if (inspect && isatty((int)fileno(stdin))) {
        sts = PyRun_AnyFile(stdin, "<stdin>") != 0;
    }

#ifdef MS_WINDOWS
    PyWinFreeze_ExeTerm();
#endif
    if (Py_FinalizeEx() < 0) {
        sts = 120;
    }
    return sts;
}
