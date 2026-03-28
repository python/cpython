#ifndef Py_INTERNAL_MODULEOBJECT_H
#define Py_INTERNAL_MODULEOBJECT_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern void _PyModule_Clear(PyObject *);
extern void _PyModule_ClearDict(PyObject *);
extern int _PyModuleSpec_IsInitializing(PyObject *);
extern int _PyModuleSpec_GetFileOrigin(PyObject *, PyObject **);
extern int _PyModule_IsPossiblyShadowing(PyObject *);

extern int _PyModule_IsExtension(PyObject *obj);

extern int _PyModule_InitModuleDictWatcher(PyInterpreterState *interp);

typedef int (*_Py_modexecfunc)(PyObject *);

typedef struct {
    PyObject_HEAD
    PyObject *md_dict;
    void *md_state;
    PyObject *md_weaklist;
    // for logging purposes after md_dict is cleared
    PyObject *md_name;
    bool md_token_is_def;  /* if true, `md_token` is the PyModuleDef */
#ifdef Py_GIL_DISABLED
    bool md_requires_gil;
#endif
    Py_ssize_t md_state_size;
    traverseproc md_state_traverse;
    inquiry md_state_clear;
    freefunc md_state_free;
    void *md_token;
    _Py_modexecfunc md_exec;  /* only set if md_token_is_def is true */
} PyModuleObject;

#define _PyModule_CAST(op) \
    (assert(PyModule_Check(op)), _Py_CAST(PyModuleObject*, (op)))

static inline PyModuleDef *_PyModule_GetDefOrNull(PyObject *arg) {
    PyModuleObject *mod = _PyModule_CAST(arg);
    if (mod->md_token_is_def) {
        return (PyModuleDef *)mod->md_token;
    }
    return NULL;
}

static inline PyModuleDef *_PyModule_GetToken(PyObject *arg) {
    PyModuleObject *mod = _PyModule_CAST(arg);
    return (PyModuleDef *)mod->md_token;
}

static inline void* _PyModule_GetState(PyObject* mod) {
    return _PyModule_CAST(mod)->md_state;
}

static inline PyObject* _PyModule_GetDict(PyObject *mod) {
    PyObject *dict = _PyModule_CAST(mod)->md_dict;
    // _PyModule_GetDict(mod) must not be used after calling module_clear(mod)
    assert(dict != NULL);
    return dict;  // borrowed reference
}

extern PyObject * _PyModule_GetFilenameObject(PyObject *);
extern Py_ssize_t _PyModule_GetFilenameUTF8(
        PyObject *module,
        char *buffer,
        Py_ssize_t maxlen);

PyObject* _Py_module_getattro_impl(PyModuleObject *m, PyObject *name, int suppress);
PyObject* _Py_module_getattro(PyObject *m, PyObject *name);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MODULEOBJECT_H */
