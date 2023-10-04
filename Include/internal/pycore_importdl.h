#ifndef Py_INTERNAL_IMPORTDL_H
#define Py_INTERNAL_IMPORTDL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


extern const char *_PyImport_DynLoadFiletab[];

extern PyObject *_PyImport_LoadDynamicModuleWithSpec(PyObject *spec, FILE *);

typedef PyObject *(*PyModInitFunction)(void);

/* Max length of module suffix searched for -- accommodates "module.slb" */
#define MAXSUFFIXSIZE 12

#ifdef MS_WINDOWS
#include <windows.h>
typedef FARPROC dl_funcptr;
extern const char *_Py_SOABI;
#else
typedef void (*dl_funcptr)(void);
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IMPORTDL_H */
