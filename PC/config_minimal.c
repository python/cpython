/* Module configuration */

/* This file contains the table of built-in modules.
    See create_builtin() in import.c. */

#include "Python.h"

#ifdef Py_ENABLE_SHARED
/* Define extern variables omitted from minimal builds */
void *PyWin_DLLhModule = NULL;
#endif


extern PyObject* PyInit_faulthandler(void);
extern PyObject* PyInit__tracemalloc(void);
extern PyObject* PyInit_gc(void);
extern PyObject* PyInit_nt(void);
extern PyObject* PyInit__signal(void);
#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)
extern PyObject* PyInit_winreg(void);
#endif

extern PyObject* PyInit__ast(void);
extern PyObject* PyInit__io(void);
extern PyObject* PyInit_atexit(void);
extern PyObject* _PyWarnings_Init(void);
extern PyObject* PyInit__string(void);
extern PyObject* PyInit__tokenize(void);

extern PyObject* PyMarshal_Init(void);
extern PyObject* PyInit__imp(void);

struct _inittab _PyImport_Inittab[] = {
    {"_ast", PyInit__ast},
    {"faulthandler", PyInit_faulthandler},
    {"gc", PyInit_gc},
    {"nt", PyInit_nt}, /* Use the NT os functions, not posix */
    {"_signal", PyInit__signal},
    {"_tokenize", PyInit__tokenize},
    {"_tracemalloc", PyInit__tracemalloc},

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM) || defined(MS_WINDOWS_GAMES)
    {"winreg", PyInit_winreg},
#endif

    /* This module "lives in" with marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives it with import.c */
    {"_imp", PyInit__imp},

    /* These entries are here for sys.builtin_module_names */
    {"builtins", NULL},
    {"sys", NULL},
    {"_warnings", _PyWarnings_Init},
    {"_string", PyInit__string},

    {"_io", PyInit__io},
    {"atexit", PyInit_atexit},

    /* Sentinel */
    {0, 0}
};
