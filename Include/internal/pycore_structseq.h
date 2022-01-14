#ifndef Py_INTERNAL_STRUCTSEQ_H
#define Py_INTERNAL_STRUCTSEQ_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* runtime lifecycle */

extern PyStatus _PyStructSequence_InitState(PyInterpreterState *);


/* other API */

PyAPI_FUNC(int) _PyStructSequence_InitType(
    PyTypeObject *type,
    PyStructSequence_Desc *desc,
    unsigned long tp_flags);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STRUCTSEQ_H */
