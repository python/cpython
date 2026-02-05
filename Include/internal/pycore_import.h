#ifndef Py_LIMITED_API
#ifndef Py_INTERNAL_IMPORT_H
#define Py_INTERNAL_IMPORT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_hashtable.h"     // _Py_hashtable_t
#include "pycore_interp_structs.h" // _import_state

extern int _PyImport_IsInitialized(PyInterpreterState *);

// Export for 'pyexpat' shared extension
PyAPI_FUNC(int) _PyImport_SetModule(PyObject *name, PyObject *module);

// Export for 'math' shared extension
PyAPI_FUNC(int) _PyImport_SetModuleString(const char *name, PyObject* module);

extern void _PyImport_AcquireLock(PyInterpreterState *interp);
extern void _PyImport_ReleaseLock(PyInterpreterState *interp);
extern void _PyImport_ReInitLock(PyInterpreterState *interp);

// This is used exclusively for the sys and builtins modules:
extern int _PyImport_FixupBuiltin(
    PyThreadState *tstate,
    PyObject *mod,
    const char *name,            /* UTF-8 encoded string */
    PyObject *modules
    );

#ifdef HAVE_DLOPEN
#  include <dlfcn.h>              // RTLD_NOW, RTLD_LAZY
#  if HAVE_DECL_RTLD_NOW
#    define _Py_DLOPEN_FLAGS RTLD_NOW
#  else
#    define _Py_DLOPEN_FLAGS RTLD_LAZY
#  endif
#  define DLOPENFLAGS_INIT .dlopenflags = _Py_DLOPEN_FLAGS,
#else
#  define _Py_DLOPEN_FLAGS 0
#  define DLOPENFLAGS_INIT
#endif

#define IMPORTS_INIT \
    { \
        DLOPENFLAGS_INIT \
        .find_and_load = { \
            .header = 1, \
        }, \
    }

extern void _PyImport_ClearCore(PyInterpreterState *interp);

extern Py_ssize_t _PyImport_GetNextModuleIndex(void);
extern const char * _PyImport_ResolveNameWithPackageContext(const char *name);
extern const char * _PyImport_SwapPackageContext(const char *newcontext);

extern int _PyImport_GetDLOpenFlags(PyInterpreterState *interp);
extern void _PyImport_SetDLOpenFlags(PyInterpreterState *interp, int new_val);

extern PyObject * _PyImport_InitModules(PyInterpreterState *interp);
extern PyObject * _PyImport_GetModules(PyInterpreterState *interp);
extern PyObject * _PyImport_GetModulesRef(PyInterpreterState *interp);
extern void _PyImport_ClearModules(PyInterpreterState *interp);

extern void _PyImport_ClearModulesByIndex(PyInterpreterState *interp);

extern int _PyImport_InitDefaultImportFunc(PyInterpreterState *interp);
extern int _PyImport_IsDefaultImportFunc(
        PyInterpreterState *interp,
        PyObject *func);

extern PyObject * _PyImport_GetImportlibLoader(
        PyInterpreterState *interp,
        const char *loader_name);
extern PyObject * _PyImport_GetImportlibExternalLoader(
        PyInterpreterState *interp,
        const char *loader_name);
extern PyObject * _PyImport_BlessMyLoader(
        PyInterpreterState *interp,
        PyObject *module_globals);
extern PyObject * _PyImport_ImportlibModuleRepr(
        PyInterpreterState *interp,
        PyObject *module);


extern PyStatus _PyImport_Init(void);
extern void _PyImport_Fini(void);
extern void _PyImport_Fini2(void);

extern PyStatus _PyImport_InitCore(
        PyThreadState *tstate,
        PyObject *sysmod,
        int importlib);
extern PyStatus _PyImport_InitExternal(PyThreadState *tstate);
extern void _PyImport_FiniCore(PyInterpreterState *interp);
extern void _PyImport_FiniExternal(PyInterpreterState *interp);


extern PyObject* _PyImport_GetBuiltinModuleNames(void);

struct _module_alias {
    const char *name;                 /* ASCII encoded string */
    const char *orig;                 /* ASCII encoded string */
};

// Export these 3 symbols for test_ctypes
PyAPI_DATA(const struct _frozen*) _PyImport_FrozenBootstrap;
PyAPI_DATA(const struct _frozen*) _PyImport_FrozenStdlib;
PyAPI_DATA(const struct _frozen*) _PyImport_FrozenTest;

extern const struct _module_alias * _PyImport_FrozenAliases;

extern int _PyImport_CheckSubinterpIncompatibleExtensionAllowed(
    const char *name);


// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(int) _PyImport_ClearExtension(PyObject *name, PyObject *filename);

#ifdef Py_GIL_DISABLED
// Assuming that the GIL is enabled from a call to
// _PyEval_EnableGILTransient(), resolve the transient request depending on the
// state of the module argument:
// - If module is NULL or a PyModuleObject with md_gil == Py_MOD_GIL_NOT_USED,
//   call _PyEval_DisableGIL().
// - Otherwise, call _PyImport_EnableGILAndWarn
//
// This function may raise an exception.
extern int _PyImport_CheckGILForModule(PyObject *module, PyObject *module_name);
// Assuming that the GIL is enabled from a call to
// _PyEval_EnableGILTransient(), call _PyEval_EnableGILPermanent().
// If the GIL was not already enabled permanently, issue a warning referencing
// the module's name.
// Leave a message in verbose mode.
//
// This function may raise an exception.
extern int _PyImport_EnableGILAndWarn(PyThreadState *, PyObject *module_name);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_IMPORT_H */
#endif /* !Py_LIMITED_API */
