
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "importdl.h"

#include <sys/types.h>
#include <sys/stat.h>

#if defined(__NetBSD__)
#include <sys/param.h>
#if (NetBSD < 199712)
#include <nlist.h>
#include <link.h>
#define dlerror() "error in dynamic linking"
#endif
#endif /* NetBSD */

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#else
#if defined(PYOS_OS2) && defined(PYCC_GCC)
#include "dlfcn.h"
#endif
#endif

#if (defined(__OpenBSD__) || defined(__NetBSD__)) && !defined(__ELF__)
#define LEAD_UNDERSCORE "_"
#else
#define LEAD_UNDERSCORE ""
#endif

/* The .so extension module ABI tag, supplied by the Makefile via
   Makefile.pre.in and configure.  This is used to discriminate between
   incompatible .so files so that extensions for different Python builds can
   live in the same directory.  E.g. foomodule.cpython-32.so
*/

const char *_PyImport_DynLoadFiletab[] = {
#ifdef __CYGWIN__
    ".dll",
#else  /* !__CYGWIN__ */
#if defined(PYOS_OS2) && defined(PYCC_GCC)
    ".pyd",
    ".dll",
#else  /* !(defined(PYOS_OS2) && defined(PYCC_GCC)) */
#ifdef __VMS
    ".exe",
    ".EXE",
#else  /* !__VMS */
    "." SOABI ".so",
    ".abi" PYTHON_ABI_STRING ".so",
    ".so",
#endif  /* __VMS */
#endif  /* defined(PYOS_OS2) && defined(PYCC_GCC) */
#endif  /* __CYGWIN__ */
    NULL,
};

static struct {
    dev_t dev;
#ifdef __VMS
    ino_t ino[3];
#else
    ino_t ino;
#endif
    void *handle;
} handles[128];
static int nhandles = 0;


dl_funcptr _PyImport_GetDynLoadFunc(const char *shortname,
                                    const char *pathname, FILE *fp)
{
    dl_funcptr p;
    void *handle;
    char funcname[258];
    char pathbuf[260];
    int dlopenflags=0;

    if (strchr(pathname, '/') == NULL) {
        /* Prefix bare filename with "./" */
        PyOS_snprintf(pathbuf, sizeof(pathbuf), "./%-.255s", pathname);
        pathname = pathbuf;
    }

    PyOS_snprintf(funcname, sizeof(funcname),
                  LEAD_UNDERSCORE "PyInit_%.200s", shortname);

    if (fp != NULL) {
        int i;
        struct stat statb;
        fstat(fileno(fp), &statb);
        for (i = 0; i < nhandles; i++) {
            if (statb.st_dev == handles[i].dev &&
                statb.st_ino == handles[i].ino) {
                p = (dl_funcptr) dlsym(handles[i].handle,
                                       funcname);
                return p;
            }
        }
        if (nhandles < 128) {
            handles[nhandles].dev = statb.st_dev;
#ifdef __VMS
            handles[nhandles].ino[0] = statb.st_ino[0];
            handles[nhandles].ino[1] = statb.st_ino[1];
            handles[nhandles].ino[2] = statb.st_ino[2];
#else
            handles[nhandles].ino = statb.st_ino;
#endif
        }
    }

#if !(defined(PYOS_OS2) && defined(PYCC_GCC))
    dlopenflags = PyThreadState_GET()->interp->dlopenflags;
#endif

#ifdef __VMS
    /* VMS currently don't allow a pathname, use a logical name instead */
    /* Concatenate 'python_module_' and shortname */
    /* so "import vms.bar" will use the logical python_module_bar */
    /* As C module use only one name space this is probably not a */
    /* important limitation */
    PyOS_snprintf(pathbuf, sizeof(pathbuf), "python_module_%-.200s",
                  shortname);
    pathname = pathbuf;
#endif

    handle = dlopen(pathname, dlopenflags);

    if (handle == NULL) {
        PyObject *mod_name = NULL;
        PyObject *path = NULL;
        PyObject *error_ob = NULL;
        const char *error = dlerror();
        if (error == NULL)
            error = "unknown dlopen() error";
        error_ob = PyUnicode_FromString(error);
        path = PyUnicode_FromString(pathname);
        mod_name = PyUnicode_FromString(shortname);
        PyErr_SetImportError(error_ob, mod_name, path);
        Py_DECREF(error_ob);
        Py_DECREF(path);
        Py_DECREF(mod_name);
        return NULL;
    }
    if (fp != NULL && nhandles < 128)
        handles[nhandles++].handle = handle;
    p = (dl_funcptr) dlsym(handle, funcname);
    return p;
}
