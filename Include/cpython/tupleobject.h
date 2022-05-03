#ifndef Py_CPYTHON_TUPLEOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* ob_item contains space for 'ob_size' elements.
       Items must normally not be NULL, except during construction when
       the tuple is not yet visible outside the function that builds it. */
    PyObject *ob_item[1];
} PyTupleObject;

PyAPI_FUNC(int) _PyTuple_Resize(PyObject **, Py_ssize_t);
PyAPI_FUNC(void) _PyTuple_MaybeUntrack(PyObject *);

/* Cast argument to PyTupleObject* type. */
#define _PyTuple_CAST(op) \
    (assert(PyTuple_Check(op)), _Py_reinterpret_cast(PyTupleObject*, (op)))

// Macros and static inline functions, trading safety for speed

static inline Py_ssize_t PyTuple_GET_SIZE(PyTupleObject *op) {
    return Py_SIZE(op);
}
#define PyTuple_GET_SIZE(op) PyTuple_GET_SIZE(_PyTuple_CAST(op))

#define PyTuple_GET_ITEM(op, index) (_PyTuple_CAST(op)->ob_item[index])

/* Function *only* to be used to fill in brand new tuples */
static inline void
PyTuple_SET_ITEM(PyTupleObject *op, Py_ssize_t index, PyObject *value) {
    op->ob_item[index] = value;
}
#define PyTuple_SET_ITEM(op, index, value) \
    PyTuple_SET_ITEM(_PyTuple_CAST(op), index, _PyObject_CAST(value))

PyAPI_FUNC(void) _PyTuple_DebugMallocStats(FILE *out);
