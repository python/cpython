#ifndef Py_INTERNAL_STRUCTSEQ_H
#define Py_INTERNAL_STRUCTSEQ_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* other API */

PyAPI_FUNC(PyTypeObject *) _PyStructSequence_NewType(
    PyStructSequence_Desc *desc,
    unsigned long tp_flags);

extern int _PyStructSequence_InitBuiltinWithFlags(
    PyInterpreterState *interp,
    PyTypeObject *type,
    PyStructSequence_Desc *desc,
    unsigned long tp_flags);

static inline int
_PyStructSequence_InitBuiltin(PyInterpreterState *interp,
                              PyTypeObject *type,
                              PyStructSequence_Desc *desc)
{
    return _PyStructSequence_InitBuiltinWithFlags(interp, type, desc, 0);
}

extern void _PyStructSequence_FiniBuiltin(
    PyInterpreterState *interp,
    PyTypeObject *type);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_STRUCTSEQ_H */
