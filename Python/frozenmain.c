
/* Python interpreter main program for frozen scripts */

#include "Python.h"
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
    char *p;
    int i, n, sts = 1;
    int inspect = 0;
    int unbuffered = 0;
    char *oldloc = NULL;
    wchar_t **argv_copy = NULL;
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2 = NULL;

    if (argc > 0) {
        argv_copy = PyMem_RawMalloc(sizeof(wchar_t*) * argc);
        argv_copy2 = PyMem_RawMalloc(sizeof(wchar_t*) * argc);
        if (!argv_copy || !argv_copy2) {
            fprintf(stderr, "out of memory\n");
            goto error;
        }
    }

    Py_FrozenFlag = 1; /* Suppress errors from getpath.c */

    if ((p = Py_GETENV("PYTHONINSPECT")) && *p != '\0')
        inspect = 1;
    if ((p = Py_GETENV("PYTHONUNBUFFERED")) && *p != '\0')
        unbuffered = 1;

    if (unbuffered) {
        setbuf(stdin, (char *)NULL);
        setbuf(stdout, (char *)NULL);
        setbuf(stderr, (char *)NULL);
    }

    oldloc = _PyMem_RawStrdup(setlocale(LC_ALL, NULL));
    if (!oldloc) {
        fprintf(stderr, "out of memory\n");
        goto error;
    }

    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
        argv_copy[i] = Py_DecodeLocale(argv[i], NULL);
        argv_copy2[i] = argv_copy[i];
        if (!argv_copy[i]) {
            fprintf(stderr, "Unable to decode the command line argument #%i\n",
                            i + 1);
            argc = i;
            goto error;
        }
    }
    setlocale(LC_ALL, oldloc);
    PyMem_RawFree(oldloc);
    oldloc = NULL;

#ifdef MS_WINDOWS
    PyInitFrozenExtensions();
#endif /* MS_WINDOWS */
    if (argc >= 1)
        Py_SetProgramName(argv_copy[0]);
    Py_Initialize();
#ifdef MS_WINDOWS
    PyWinFreeze_ExeInit();
#endif

    if (Py_VerboseFlag)
        fprintf(stderr, "Python %s\n%s\n",
            Py_GetVersion(), Py_GetCopyright());

    PySys_SetArgv(argc, argv_copy);

    n = PyImport_ImportFrozenModule("__main__");
    if (n == 0)
        Py_FatalError("__main__ not frozen");
    if (n < 0) {
        PyErr_Print();
        sts = 1;
    }
    else
        sts = 0;

    if (inspect && isatty((int)fileno(stdin)))
        sts = PyRun_AnyFile(stdin, "<stdin>") != 0;

#ifdef MS_WINDOWS
    PyWinFreeze_ExeTerm();
#endif
    Py_Finalize();

error:
    PyMem_RawFree(argv_copy);
    if (argv_copy2) {
        for (i = 0; i < argc; i++)
            PyMem_RawFree(argv_copy2[i]);
        PyMem_RawFree(argv_copy2);
    }
    PyMem_RawFree(oldloc);
    return sts;
}
