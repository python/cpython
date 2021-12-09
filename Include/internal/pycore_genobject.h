#ifndef Py_INTERNAL_GENOBJECT_H
#define Py_INTERNAL_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern void _PyAsyncGen_Fini(PyInterpreterState *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GENOBJECT_H */
