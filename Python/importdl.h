#ifndef Py_IMPORTDL_H
#define Py_IMPORTDL_H

#ifdef __cplusplus
extern "C" {
#endif


extern const char *_PyImport_DynLoadFiletab[];

extern PyObject *_PyImport_LoadDynamicModuleWithSpec(PyObject *spec, FILE *);

/* Max length of module suffix searched for -- accommodates "module.slb" */
#define MAXSUFFIXSIZE 12

#ifdef MS_WINDOWS
#include <windows.h>
typedef FARPROC dl_funcptr;
#else
typedef void (*dl_funcptr)(void);
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_IMPORTDL_H */
