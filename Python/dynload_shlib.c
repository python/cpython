
/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "internal/pystate.h"
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
    "." SOABI ".so",
    ".abi" PYTHON_ABI_STRING ".so",
    ".so",
#endif  /* __CYGWIN__ */
    NULL,
};

static struct {
    dev_t dev;
    ino_t ino;
    void *handle;
} handles[128];
static int nhandles = 0;


dl_funcptr
_PyImport_FindSharedFuncptr(const char *prefix,
                            const char *shortname,
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
                  LEAD_UNDERSCORE "%.20s_%.200s", prefix, shortname);

    if (fp != NULL) {
        int i;
        struct _Py_stat_struct status;
        if (_Py_fstat(fileno(fp), &status) == -1)
            return NULL;
        for (i = 0; i < nhandles; i++) {
            if (status.st_dev == handles[i].dev &&
                status.st_ino == handles[i].ino) {
                p = (dl_funcptr) dlsym(handles[i].handle,
                                       funcname);
                return p;
            }
        }
        if (nhandles < 128) {
            handles[nhandles].dev = status.st_dev;
            handles[nhandles].ino = status.st_ino;
        }
    }

    dlopenflags = PyThreadState_GET()->interp->dlopenflags;

    handle = dlopen(pathname, dlopenflags);

    if (handle == NULL) {
        PyObject *mod_name;
        PyObject *path;
        PyObject *error_ob;
        const char *error = dlerror();
        if (error == NULL)
            error = "unknown dlopen() error";
        error_ob = PyUnicode_FromString(error);
        if (error_ob == NULL)
            return NULL;
        mod_name = PyUnicode_FromString(shortname);
        if (mod_name == NULL) {
            Py_DECREF(error_ob);
            return NULL;
        }
        path = PyUnicode_FromString(pathname);
        if (path == NULL) {
            Py_DECREF(error_ob);
            Py_DECREF(mod_name);
            return NULL;
        }
        PyErr_SetImportError(error_ob, mod_name, path);
        Py_DECREF(error_ob);
        Py_DECREF(mod_name);
        Py_DECREF(path);
        return NULL;
    }
    if (fp != NULL && nhandles < 128)
        handles[nhandles++].handle = handle;
    p = (dl_funcptr) dlsym(handle, funcname);
    return p;
}
