/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

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
	{".sl", "rb", C_EXTENSION},
	{"module.sl", "rb", C_EXTENSION},
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
		flags = DYNAMIC_PATH | BIND_FIRST | BIND_IMMEDIATE |
			BIND_NONFATAL | BIND_VERBOSE;
		printf("shl_load %s\n",pathname);
	}
	lib = shl_load(pathname, flags, 0);
	/* XXX Chuck Blake once wrote that 0 should be BIND_NOSTART? */
	if (lib == NULL) {
		char buf[256];
		if (Py_VerboseFlag)
			perror(pathname);
		sprintf(buf, "Failed to load %.200s", pathname);
		PyErr_SetString(PyExc_ImportError, buf);
		return NULL;
	}
	sprintf(funcname, FUNCNAME_PATTERN, shortname);
	if (Py_VerboseFlag)
		printf("shl_findsym %s\n", funcname);
	shl_findsym(&lib, funcname, TYPE_UNDEFINED, (void *) &p);
	if (p == NULL && Py_VerboseFlag)
		perror(funcname);

	return p;
}
