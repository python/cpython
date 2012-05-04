#ifndef Py_IMPORTDL_H
#define Py_IMPORTDL_H

#ifdef __cplusplus
extern "C" {
#endif


extern const char *_PyImport_DynLoadFiletab[];

extern PyObject *_PyImport_LoadDynamicModule(PyObject *name, PyObject *pathname,
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
