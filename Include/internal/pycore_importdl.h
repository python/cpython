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


typedef enum ext_module_kind {
    _Py_ext_module_kind_UNKNOWN = 0,
    _Py_ext_module_kind_SINGLEPHASE = 1,
    _Py_ext_module_kind_MULTIPHASE = 2,
    _Py_ext_module_kind_INVALID = 3,
} _Py_ext_module_kind;

typedef enum ext_module_origin {
    _Py_ext_module_origin_CORE = 1,
    _Py_ext_module_origin_BUILTIN = 2,
    _Py_ext_module_origin_DYNAMIC = 3,
} _Py_ext_module_origin;

/* Input for loading an extension module. */
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
    _Py_ext_module_origin origin;
    const char *hook_prefix;
    const char *newcontext;
};
extern void _Py_ext_module_loader_info_clear(
    struct _Py_ext_module_loader_info *info);
extern int _Py_ext_module_loader_info_init(
    struct _Py_ext_module_loader_info *info,
    PyObject *name,
    PyObject *filename,
    _Py_ext_module_origin origin);
extern int _Py_ext_module_loader_info_init_for_core(
    struct _Py_ext_module_loader_info *p_info,
    PyObject *name);
extern int _Py_ext_module_loader_info_init_for_builtin(
    struct _Py_ext_module_loader_info *p_info,
    PyObject *name);
#ifdef HAVE_DYNAMIC_LOADING
extern int _Py_ext_module_loader_info_init_from_spec(
    struct _Py_ext_module_loader_info *info,
    PyObject *spec);
#endif

/* The result from running an extension module's init function. */
struct _Py_ext_module_loader_result {
    PyModuleDef *def;
    PyObject *module;
    _Py_ext_module_kind kind;
    struct _Py_ext_module_loader_result_error *err;
    struct _Py_ext_module_loader_result_error {
        enum _Py_ext_module_loader_result_error_kind {
            _Py_ext_module_loader_result_EXCEPTION = 0,
            _Py_ext_module_loader_result_ERR_MISSING = 1,
            _Py_ext_module_loader_result_ERR_UNREPORTED_EXC = 2,
            _Py_ext_module_loader_result_ERR_UNINITIALIZED = 3,
            _Py_ext_module_loader_result_ERR_NONASCII_NOT_MULTIPHASE = 4,
            _Py_ext_module_loader_result_ERR_NOT_MODULE = 5,
            _Py_ext_module_loader_result_ERR_MISSING_DEF = 6,
        } kind;
        PyObject *exc;
    } _err;
};
extern void _Py_ext_module_loader_result_clear(
    struct _Py_ext_module_loader_result *res);
extern void _Py_ext_module_loader_result_apply_error(
    struct _Py_ext_module_loader_result *res,
    const char *name);

/* The module init function. */
typedef PyObject *(*PyModInitFunction)(void);
#ifdef HAVE_DYNAMIC_LOADING
extern PyModInitFunction _PyImport_GetModInitFunc(
    struct _Py_ext_module_loader_info *info,
    FILE *fp);
#endif
extern int _PyImport_RunModInitFunc(
    PyModInitFunction p0,
    struct _Py_ext_module_loader_info *info,
    struct _Py_ext_module_loader_result *p_res);


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
