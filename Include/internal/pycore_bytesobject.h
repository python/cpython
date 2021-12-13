#ifndef Py_INTERNAL_BYTESOBJECT_H
#define Py_INTERNAL_BYTESOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern PyStatus _PyBytes_InitGlobalObjects(PyInterpreterState *);
extern PyStatus _PyBytes_InitTypes(PyInterpreterState *);
extern void _PyBytes_Fini(PyInterpreterState *);


/* other API */

struct _Py_bytes_state {
    PyObject *empty_string;
    PyBytesObject *characters[256];
};


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BYTESOBJECT_H */
