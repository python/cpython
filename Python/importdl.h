#ifndef Py_IMPORTDL_H
#define Py_IMPORTDL_H

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Definitions for dynamic loading of extension modules */
enum filetype {
	SEARCH_ERROR,
	PY_SOURCE,
	PY_COMPILED,
	C_EXTENSION,
	PY_RESOURCE, /* Mac only */
	PKG_DIRECTORY,
	C_BUILTIN,
	PY_FROZEN,
	PY_CODERESOURCE /* Mac only */
};

struct filedescr {
	char *suffix;
	char *mode;
	enum filetype type;
};
extern struct filedescr * _PyImport_Filetab;
extern const struct filedescr _PyImport_DynLoadFiletab[];

extern PyObject *_PyImport_LoadDynamicModule
	Py_PROTO((char *name, char *pathname, FILE *));

/* Max length of module suffix searched for -- accommodates "module.slb" */
#define MAXSUFFIXSIZE 12

#ifdef MS_WINDOWS
#include <windows.h>
typedef FARPROC dl_funcptr;
#else
#ifdef PYOS_OS2
typedef int (* APIENTRY dl_funcptr)();
#else
typedef void (*dl_funcptr)(void);
#endif
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_IMPORTDL_H */
