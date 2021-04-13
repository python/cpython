#ifndef Py_INTERNAL_MODULEOBJECT_H
#define Py_INTERNAL_MODULEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

typedef struct {
    PyObject_HEAD
    PyObject *md_dict;
    struct PyModuleDef *md_def;
    void *md_state;
    PyObject *md_weaklist;
    PyObject *md_name;  /* for logging purposes after md_dict is cleared */
} PyModuleObject;

static inline PyModuleDef* _PyModule_GetDef(PyObject *m) {
    assert(PyModule_Check(m));
    return ((PyModuleObject *)m)->md_def;
}

static inline void* _PyModule_GetState(PyObject* m) {
    assert(PyModule_Check(m));
    return ((PyModuleObject *)m)->md_state;
}


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_MODULEOBJECT_H */
