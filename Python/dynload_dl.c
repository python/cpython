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

#include "Python.h"
#include "importdl.h"


extern char *Py_GetProgramName();

const struct filedescr _PyImport_DynLoadFiletab[] = {
	{".o", "rb", C_EXTENSION},
	{"module.o", "rb", C_EXTENSION},
	{0, 0}
};


dl_funcptr _PyImport_GetDynLoadFunc(const char *fqname, const char *shortname,
				    const char *pathname, FILE *fp)
{
	char funcname[258];

	sprintf(funcname, "init%.200s", shortname);
	return dl_loadmod(Py_GetProgramName(), pathname, funcname);
}
