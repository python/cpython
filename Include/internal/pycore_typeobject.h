#ifndef Py_INTERNAL_TYPEOBJECT_H
#define Py_INTERNAL_TYPEOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern PyStatus _PyTypes_InitState(PyInterpreterState *);
extern PyStatus _PyTypes_InitTypes(PyInterpreterState *);
extern void _PyTypes_Fini(PyInterpreterState *);


/* other API */

extern PyStatus _PyTypes_InitSlotDefs(void);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TYPEOBJECT_H */
