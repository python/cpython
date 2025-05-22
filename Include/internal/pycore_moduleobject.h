#ifndef Py_INTERNAL_MODULEOBJECT_H
#define Py_INTERNAL_MODULEOBJECT_H
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

typedef int (*_Py_modexecfunc)(PyObject *);

typedef struct {
    PyObject_HEAD
    PyObject *md_dict;
    // The PyModuleDef used to define the module, if any.
    // (used to be `md_def`; renamed because all uses need inspection)
    PyModuleDef *md_def_or_null;
    void *md_state;
    PyObject *md_weaklist;
    // for logging purposes after md_dict is cleared
    PyObject *md_name;
#ifdef Py_GIL_DISABLED
    void *md_gil;
#endif
    Py_ssize_t md_size;
    traverseproc md_traverse;
    inquiry md_clear;
    freefunc md_free;
    void *md_token;
    _Py_modexecfunc md_exec;  /* only set if md_def_or_null is NULL */
} PyModuleObject;

static inline PyModuleDef* _PyModule_GetDefOrNull(PyObject *mod) {
    assert(PyModule_Check(mod));
    return ((PyModuleObject *)mod)->md_def_or_null;
}

static inline void* _PyModule_GetState(PyObject* mod) {
    assert(PyModule_Check(mod));
    return ((PyModuleObject *)mod)->md_state;
}

static inline PyObject* _PyModule_GetDict(PyObject *mod) {
    assert(PyModule_Check(mod));
    PyObject *dict = ((PyModuleObject *)mod) -> md_dict;
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
