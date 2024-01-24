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

extern int _PyObject_HasLen(PyObject *o);

/* === Sequence protocol ================================================ */

#define PY_ITERSEARCH_COUNT    1
#define PY_ITERSEARCH_INDEX    2
#define PY_ITERSEARCH_CONTAINS 3

/* Iterate over seq.

   Result depends on the operation:

   PY_ITERSEARCH_COUNT:  return # of times obj appears in seq; -1 if
     error.
   PY_ITERSEARCH_INDEX:  return 0-based index of first occurrence of
     obj in seq; set ValueError and return -1 if none found;
     also return -1 on error.
   PY_ITERSEARCH_CONTAINS:  return 1 if obj in seq, else 0; -1 on
     error. */
extern Py_ssize_t _PySequence_IterSearch(PyObject *seq,
                                         PyObject *obj, int operation);

/* === Mapping protocol ================================================= */

extern int _PyObject_RealIsInstance(PyObject *inst, PyObject *cls);

extern int _PyObject_RealIsSubclass(PyObject *derived, PyObject *cls);

// Convert Python int to Py_ssize_t. Do nothing if the argument is None.
// Export for '_bisect' shared extension.
PyAPI_FUNC(int) _Py_convert_optional_to_ssize_t(PyObject *, void *);

// Same as PyNumber_Index() but can return an instance of a subclass of int.
// Export for 'math' shared extension.
PyAPI_FUNC(PyObject*) _PyNumber_Index(PyObject *o);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_ABSTRACT_H */
