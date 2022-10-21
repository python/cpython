#ifndef Py_INTERNAL_ABSTRACT_H
#define Py_INTERNAL_ABSTRACT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Fast inlined version of PyIndex_Check()
static inline int
_PyIndex_Check(PyObject *obj)
{
    PyNumberMethods *tp_as_number = Py_TYPE(obj)->tp_as_number;
    return (tp_as_number != NULL && tp_as_number->nb_index != NULL);
}

PyObject *_PyNumber_PowerNoMod(PyObject *lhs, PyObject *rhs);
PyObject *_PyNumber_InPlacePowerNoMod(PyObject *lhs, PyObject *rhs);

PyAPI_FUNC(PyObject *) _PyBuffer_MutableBufferGetter(PyObject *instance, void *ignored);

#define _PY_BUFFER_MUTABLE_BUFFER_GETSET { "__mutable_buffer__", _PyBuffer_MutableBufferGetter, NULL, NULL },

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ABSTRACT_H */
