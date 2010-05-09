
/* Support for dynamic loading of extension modules */

#include <atheos/image.h>
#include <errno.h>

#include "Python.h"
#include "importdl.h"


const struct filedescr _PyImport_DynLoadFiletab[] = {
    {".so", "rb", C_EXTENSION},
    {"module.so", "rb", C_EXTENSION},
    {0, 0}
};

dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
                                    const char *pathname, FILE *fp)
{
    void *p;
    int lib;
    char funcname[258];

    if (Py_VerboseFlag)
        printf("load_library %s\n", pathname);

    lib = load_library(pathname, 0);
    if (lib < 0) {
        char buf[512];
        if (Py_VerboseFlag)
            perror(pathname);
        PyOS_snprintf(buf, sizeof(buf), "Failed to load %.200s: %.200s",
                      pathname, strerror(errno));
        PyErr_SetString(PyExc_ImportError, buf);
        return NULL;
    }
    PyOS_snprintf(funcname, sizeof(funcname), "init%.200s", shortname);
    if (Py_VerboseFlag)
        printf("get_symbol_address %s\n", funcname);
    if (get_symbol_address(lib, funcname, -1, &p) < 0) {
        p = NULL;
        if (Py_VerboseFlag)
            perror(funcname);
    }

    return (dl_funcptr) p;
}
