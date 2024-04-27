#ifndef Py_INTERNAL_IMPORTDL_H
#define Py_INTERNAL_IMPORTDL_H

#include "patchlevel.h"           // PY_MAJOR_VERSION

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


extern const char *_PyImport_DynLoadFiletab[];


typedef PyObject *(*PyModInitFunction)(void);

struct _Py_ext_module_loader_info {
    PyObject *filename;
#ifndef MS_WINDOWS
    PyObject *filename_encoded;
#endif
    PyObject *name;
    PyObject *name_encoded;
    /* path is always a borrowed ref of name or filename,
     * depending on if it's builtin or not. */
    PyObject *path;
    const char *hook_prefix;
    const char *newcontext;
};
extern void _Py_ext_module_loader_info_clear(
    struct _Py_ext_module_loader_info *info);
extern int _Py_ext_module_loader_info_init(
    struct _Py_ext_module_loader_info *info,
    PyObject *name,
    PyObject *filename);
extern int _Py_ext_module_loader_info_init_from_spec(
    struct _Py_ext_module_loader_info *info,
    PyObject *spec);

extern PyObject *_PyImport_LoadDynamicModuleWithSpec(
    struct _Py_ext_module_loader_info *info,
    PyObject *spec,
    FILE *fp);


/* Max length of module suffix searched for -- accommodates "module.slb" */
#define MAXSUFFIXSIZE 12

#ifdef MS_WINDOWS
#include <windows.h>
typedef FARPROC dl_funcptr;

#ifdef _DEBUG
#  define PYD_DEBUG_SUFFIX "_d"
#else
#  define PYD_DEBUG_SUFFIX ""
#endif

#ifdef Py_GIL_DISABLED
#  define PYD_THREADING_TAG "t"
#else
#  define PYD_THREADING_TAG ""
#endif

#ifdef PYD_PLATFORM_TAG
#  define PYD_SOABI "cp" Py_STRINGIFY(PY_MAJOR_VERSION) Py_STRINGIFY(PY_MINOR_VERSION) PYD_THREADING_TAG "-" PYD_PLATFORM_TAG
#else
#  define PYD_SOABI "cp" Py_STRINGIFY(PY_MAJOR_VERSION) Py_STRINGIFY(PY_MINOR_VERSION) PYD_THREADING_TAG
#endif

#define PYD_TAGGED_SUFFIX PYD_DEBUG_SUFFIX "." PYD_SOABI ".pyd"
#define PYD_UNTAGGED_SUFFIX PYD_DEBUG_SUFFIX ".pyd"

#else
typedef void (*dl_funcptr)(void);
#endif


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IMPORTDL_H */
