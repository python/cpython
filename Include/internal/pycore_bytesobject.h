#ifndef Py_INTERNAL_BYTESOBJECT_H
#define Py_INTERNAL_BYTESOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern PyStatus _PyBytes_InitTypes(PyInterpreterState *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_BYTESOBJECT_H */
