#ifndef Py_IMPORTDL_H
#define Py_IMPORTDL_H

#ifdef __cplusplus
extern "C" {
#endif


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
	PY_CODERESOURCE, /* Mac only */
	IMP_HOOK
};

struct filedescr {
	char *suffix;
	char *mode;
	enum filetype type;
};
extern struct filedescr * _PyImport_Filetab;
extern const struct filedescr _PyImport_DynLoadFiletab[];

extern PyObject *_PyImport_LoadDynamicModule(char *name, char *pathname,
					     FILE *);

/* Max length of module suffix searched for -- accommodates "module.slb" */
#define MAXSUFFIXSIZE 12

#ifdef MS_WINDOWS
#include <windows.h>
typedef FARPROC dl_funcptr;
#else
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#include <os2def.h>
typedef int (* APIENTRY dl_funcptr)();
#else
typedef void (*dl_funcptr)(void);
#endif
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_IMPORTDL_H */
