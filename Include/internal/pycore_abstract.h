#ifndef Py_INTERNAL_ABSTRACT_H
#define Py_INTERNAL_ABSTRACT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* Return 1 if the specified index is outside the range[0, limit) */
#define valid_index(index, limit) ((size_t)index < (size_t)limit)
    /* The cast to size_t lets us use just a single comparison
        to check whether index is in the range: 0 <= index < limit.

        See:  Section 14.2 "Bounds Checking" in the Agner Fog
        optimization manual found at:
        https://www.agner.org/optimize/optimizing_cpp.pdf
    */

// Fast inlined version of PyIndex_Check()
static inline int
_PyIndex_Check(PyObject *obj)
{
    PyNumberMethods *tp_as_number = Py_TYPE(obj)->tp_as_number;
    return (tp_as_number != NULL && tp_as_number->nb_index != NULL);
}

PyObject *_PyNumber_PowerNoMod(PyObject *lhs, PyObject *rhs);
PyObject *_PyNumber_InPlacePowerNoMod(PyObject *lhs, PyObject *rhs);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ABSTRACT_H */
