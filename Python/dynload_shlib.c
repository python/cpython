
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
	{".so", "rb", C_EXTENSION},
	{"module.so", "rb", C_EXTENSION},
#endif
	{0, 0}
};

static struct {
	dev_t dev;
	ino_t ino;
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
		sprintf(pathbuf, "./%-.255s", pathname);
		pathname = pathbuf;
	}

	sprintf(funcname, LEAD_UNDERSCORE "init%.200s", shortname);

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
			handles[nhandles].ino = statb.st_ino;
		}
	}

        dlopenflags = PyThreadState_Get()->interp->dlopenflags;

	if (Py_VerboseFlag)
		printf("dlopen(\"%s\", %x);\n", pathname, dlopenflags);

	handle = dlopen(pathname, dlopenflags);

	if (handle == NULL) {
		PyErr_SetString(PyExc_ImportError, dlerror());
		return NULL;
	}
	if (fp != NULL && nhandles < 128)
		handles[nhandles++].handle = handle;
	p = (dl_funcptr) dlsym(handle, funcname);
	return p;
}
