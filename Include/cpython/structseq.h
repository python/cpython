#ifndef Py_CPYTHON_STRUCTSEQ_H
#  error "this header file must not be included directly"
#endif

PyAPI_FUNC(void) PyStructSequence_InitType(PyTypeObject *type,
                                           PyStructSequence_Desc *desc);
PyAPI_FUNC(int) PyStructSequence_InitType2(PyTypeObject *type,
                                           PyStructSequence_Desc *desc);

typedef PyTupleObject PyStructSequence;
#define PyStructSequence_SET_ITEM PyStructSequence_SetItem
#define PyStructSequence_GET_ITEM PyStructSequence_GetItem
