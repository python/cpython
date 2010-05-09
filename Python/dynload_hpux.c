
/* Support for dynamic loading of extension modules */

#include "dl.h"
#include <errno.h>

#include "Python.h"
#include "importdl.h"

#if defined(__hp9000s300)
#define FUNCNAME_PATTERN "_init%.200s"
#else
#define FUNCNAME_PATTERN "init%.200s"
#endif

const struct filedescr _PyImport_DynLoadFiletab[] = {
    {SHLIB_EXT, "rb", C_EXTENSION},
    {"module"SHLIB_EXT, "rb", C_EXTENSION},
    {0, 0}
};

dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
                                    const char *pathname, FILE *fp)
{
    dl_funcptr p;
    shl_t lib;
    int flags;
    char funcname[258];

    flags = BIND_FIRST | BIND_DEFERRED;
    if (Py_VerboseFlag) {
        flags = BIND_FIRST | BIND_IMMEDIATE |
            BIND_NONFATAL | BIND_VERBOSE;
        printf("shl_load %s\n",pathname);
    }
    lib = shl_load(pathname, flags, 0);
    /* XXX Chuck Blake once wrote that 0 should be BIND_NOSTART? */
    if (lib == NULL) {
        char buf[256];
        if (Py_VerboseFlag)
            perror(pathname);
        PyOS_snprintf(buf, sizeof(buf), "Failed to load %.200s",
                      pathname);
        PyErr_SetString(PyExc_ImportError, buf);
        return NULL;
    }
    PyOS_snprintf(funcname, sizeof(funcname), FUNCNAME_PATTERN, shortname);
    if (Py_VerboseFlag)
        printf("shl_findsym %s\n", funcname);
    if (shl_findsym(&lib, funcname, TYPE_UNDEFINED, (void *) &p) == -1) {
        shl_unload(lib);
        p = NULL;
    }
    if (p == NULL && Py_VerboseFlag)
        perror(funcname);

    return p;
}
