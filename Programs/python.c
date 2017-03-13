/* Minimal main program -- everything is loaded from the library */

#include "Python.h"
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

/* Helpers to better handle the legacy C locale
 *
 * The legacy C locale assumes ASCII as the default text encoding, which
 * causes problems not only for the CPython runtime, but also other
 * components like GNU readline.
 *
 * Accordingly, when the CLI detects it, it attempts to coerce it to a
 * more capable UTF-8 based alternative.
 *
 * See the documentation of the PYTHONCOERCECLOCALE setting for more details.
 *
 */

#ifdef PY_COERCE_C_LOCALE
static const char *_C_LOCALE_COERCION_WARNING =
    "Python detected LC_CTYPE=C: %.20s coerced to %.20s (set another locale "
    "or PYTHONCOERCECLOCALE=0 to disable this locale coercion behaviour).\n";

typedef struct _CandidateLocale {
    const char *locale_name;
    int category;
} _LocaleCoercionTarget;

static _LocaleCoercionTarget _TARGET_LOCALES[] = {
    { "C.UTF-8", LC_ALL },
    { "C.utf8", LC_ALL },
    { "UTF-8", LC_CTYPE },
    { NULL, 0 }
};

void
_coerce_default_locale_settings(const _LocaleCoercionTarget *target)
{
    const char *newloc = target->locale_name;
    int category = target->category;

    /* Reset locale back to currently configured defaults */
    setlocale(LC_ALL, "");

    /* Set the relevant locale environment variables */
    if (category == LC_ALL) {
        const char *env_vars_updated = "LC_ALL & LANG";
        if (setenv("LC_ALL", newloc, 1)) {
            fprintf(stderr,
                    "Error setting LC_ALL, skipping C locale coercion\n");
            return;
        }
        if (setenv("LANG", newloc, 1)) {
            fprintf(stderr,
                    "Error setting LANG during C locale coercion\n");
            env_vars_updated = "LC_ALL";
        }
        fprintf(stderr, _C_LOCALE_COERCION_WARNING, env_vars_updated, newloc);
    } else if (category == LC_CTYPE) {
        if (setenv("LC_CTYPE", newloc, 1)) {
            fprintf(stderr,
                    "Error setting LC_CTYPE, skipping C locale coercion\n");
            return;
        }
        fprintf(stderr, _C_LOCALE_COERCION_WARNING, "LC_CTYPE", newloc);
    } else {
        fprintf(stderr, "Locale coercion must target LC_ALL or LC_CTYPE\n");
        return;
    }

    /* Set PYTHONIOENCODING if not already set */
    if (setenv("PYTHONIOENCODING", "utf-8:surrogateescape", 0)) {
        fprintf(stderr,
                "Error setting PYTHONIOENCODING during C locale coercion\n");
    }

    /* Reconfigure with the overridden environment variables */
    setlocale(LC_ALL, "");
}

void
_handle_legacy_c_locale(void)
{
    const char *coerce_c_locale = getenv("PYTHONCOERCECLOCALE");
    /* We ignore the Python -E and -I flags here, as we need to sort out
     * the locale settings *before* we try to do anything with the command
     * line arguments. For cross-platform debugging purposes, we also need
     * to give end users a way to force even scripts that are otherwise
     * isolated from their environment to use the legacy ASCII-centric C
     * locale.
    */
    if (coerce_c_locale == NULL || strncmp(coerce_c_locale, "0", 2) != 0) {
        /* PYTHONCOERCECLOCALE is not set, or is not set to exactly "0" */
        const _LocaleCoercionTarget *target = NULL;
        for (target = _TARGET_LOCALES; target->locale_name; target++) {
            const char *reconfigured_locale = setlocale(target->category,
                                                        target->locale_name);
            if (reconfigured_locale != NULL) {
                /* Successfully configured locale, so make it the default */
                _coerce_default_locale_settings(target);
                return;
            }
        }

    }
    /* No C locale warning here, as Py_Initialize will emit one later */
}
#endif

int
main(int argc, char **argv)
{
    wchar_t **argv_copy;
    /* We need a second copy, as Python might modify the first one. */
    wchar_t **argv_copy2;
    int i, res;
    char *oldloc;

    /* Force malloc() allocator to bootstrap Python */
    (void)_PyMem_SetupAllocators("malloc");

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
    setlocale(LC_ALL, "");

#ifdef PY_COERCE_C_LOCALE
    /* When the LC_CTYPE category still claims to be using the C locale,
       assume configuration error and try for a UTF-8 based locale instead */
    const char *ctype_loc = setlocale(LC_CTYPE, NULL);
    if (ctype_loc != NULL && strcmp(ctype_loc, "C") == 0) {
        _handle_legacy_c_locale();
    }
#endif

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

    res = Py_Main(argc, argv_copy);

    /* Force again malloc() allocator to release memory blocks allocated
       before Py_Main() */
    (void)_PyMem_SetupAllocators("malloc");

    for (i = 0; i < argc; i++) {
        PyMem_RawFree(argv_copy2[i]);
    }
    PyMem_RawFree(argv_copy);
    PyMem_RawFree(argv_copy2);
    return res;
}
#endif
