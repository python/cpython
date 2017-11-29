/* Minimal main program -- everything is loaded from the library */

#include "Python.h"
#include "internal/pystate.h"
#include <locale.h>

#ifdef __FreeBSD__
#include <fenv.h>
#endif

#ifdef MS_WINDOWS
int
wmain(int argc, wchar_t **argv)
{
    return Py_Main(argc, argv);
}
#else


int
main(int argc, char **argv)
{
    wchar_t **argv_copy;
    /* We need a second copy, as Python might modify the first one. */
    wchar_t **argv_copy2;
    int i, status;
    char *oldloc;

    _PyInitError err = _PyRuntime_Initialize();
    if (_Py_INIT_FAILED(err)) {
        fprintf(stderr, "Fatal Python error: %s\n", err.msg);
        fflush(stderr);
        exit(1);
    }

    /* Force default allocator, to be able to release memory above
       with a known allocator. */
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, NULL);

    argv_copy = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    argv_copy2 = (wchar_t **)PyMem_RawMalloc(sizeof(wchar_t*) * (argc+1));
    if (!argv_copy || !argv_copy2) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
#ifdef __FreeBSD__
    fedisableexcept(FE_OVERFLOW);
#endif

    oldloc = _PyMem_RawStrdup(setlocale(LC_ALL, NULL));
    if (!oldloc) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }

    /* Reconfigure the locale to the default for this process */
    _Py_SetLocaleFromEnv(LC_ALL);

    /* The legacy C locale assumes ASCII as the default text encoding, which
     * causes problems not only for the CPython runtime, but also other
     * components like GNU readline.
     *
     * Accordingly, when the CLI detects it, it attempts to coerce it to a
     * more capable UTF-8 based alternative.
     *
     * See the documentation of the PYTHONCOERCECLOCALE setting for more
     * details.
     */
    if (_Py_LegacyLocaleDetected()) {
        _Py_CoerceLegacyLocale();
    }

    /* Convert from char to wchar_t based on the locale settings */
    for (i = 0; i < argc; i++) {
        argv_copy[i] = Py_DecodeLocale(argv[i], NULL);
        if (!argv_copy[i]) {
            PyMem_RawFree(oldloc);
            fprintf(stderr, "Fatal Python error: "
                            "unable to decode the command line argument #%i\n",
                            i + 1);
            return 1;
        }
        argv_copy2[i] = argv_copy[i];
    }
    argv_copy2[argc] = argv_copy[argc] = NULL;

    setlocale(LC_ALL, oldloc);
    PyMem_RawFree(oldloc);

    status = Py_Main(argc, argv_copy);

    /* Py_Main() can change PyMem_RawMalloc() allocator, so restore the default
       to release memory blocks allocated before Py_Main() */
    _PyMem_SetDefaultAllocator(PYMEM_DOMAIN_RAW, NULL);

    for (i = 0; i < argc; i++) {
        PyMem_RawFree(argv_copy2[i]);
    }
    PyMem_RawFree(argv_copy);
    PyMem_RawFree(argv_copy2);
    return status;
}
#endif
