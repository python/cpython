/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Support for dynamic loading of extension modules */

#include "Python.h"
#include "importdl.h"

#include <sys/types.h>
#include <sys/stat.h>
#if defined(__NetBSD__) && (NetBSD < 199712)
#include <nlist.h>
#include <link.h>
#define dlerror() "error in dynamic linking"
#else
#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 1
#endif


const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".so", "rb", C_EXTENSION},
	{"module.so", "rb", C_EXTENSION},
	{0, 0}
};

static struct {
	dev_t dev;
	ino_t ino;
	void *handle;
} handles[128];
static int nhandles = 0;


dl_funcptr _PyImport_GetDynLoadFunc(const char *name, const char *funcname,
				    const char *pathname, FILE *fp)
{
	dl_funcptr p;
	void *handle;

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

#ifdef RTLD_NOW
	/* RTLD_NOW: resolve externals now
	   (i.e. core dump now if some are missing) */
	handle = dlopen(pathname, RTLD_NOW);
#else
	if (Py_VerboseFlag)
		printf("dlopen(\"%s\", %d);\n", pathname,
		       RTLD_LAZY);
	handle = dlopen(pathname, RTLD_LAZY);
#endif /* RTLD_NOW */
	if (handle == NULL) {
		PyErr_SetString(PyExc_ImportError, dlerror());
		return NULL;
	}
	if (fp != NULL && nhandles < 128)
		handles[nhandles++].handle = handle;
	p = (dl_funcptr) dlsym(handle, funcname);
	return p;
}
