
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


const struct filedescr _PyImport_DynLoadFiletab[] = {
#ifdef __CYGWIN__
    {".dll", "rb", C_EXTENSION},
    {"module.dll", "rb", C_EXTENSION},
#else
#if defined(PYOS_OS2) && defined(PYCC_GCC)
    {".pyd", "rb", C_EXTENSION},
    {".dll", "rb", C_EXTENSION},
#else
#ifdef __VMS
    {".exe", "rb", C_EXTENSION},
    {".EXE", "rb", C_EXTENSION},
    {"module.exe", "rb", C_EXTENSION},
    {"MODULE.EXE", "rb", C_EXTENSION},
#else
    {".so", "rb", C_EXTENSION},
    {"module.so", "rb", C_EXTENSION},
#endif
#endif
#endif
    {0, 0}
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


dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
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
                  LEAD_UNDERSCORE "init%.200s", shortname);

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

    if (Py_VerboseFlag)
        PySys_WriteStderr("dlopen(\"%s\", %x);\n", pathname,
                          dlopenflags);

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
        const char *error = dlerror();
        if (error == NULL)
            error = "unknown dlopen() error";
        PyErr_SetString(PyExc_ImportError, error);
        return NULL;
    }
    if (fp != NULL && nhandles < 128)
        handles[nhandles++].handle = handle;
    p = (dl_funcptr) dlsym(handle, funcname);
    return p;
}
