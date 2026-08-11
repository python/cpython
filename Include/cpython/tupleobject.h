#ifndef Py_CPYTHON_TUPLEOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    /* Cached hash.  Initially set to -1. */
    Py_hash_t ob_hash;
    /* ob_item contains space for 'ob_size' elements.
       Items must normally not be NULL, except during construction when
       the tuple is not yet visible outside the function that builds it. */
    PyObject *ob_item[1];
} PyTupleObject;

PyAPI_FUNC(int) _PyTuple_Resize(PyObject **, Py_ssize_t);

/* Cast argument to PyTupleObject* type. */
#define _PyTuple_CAST(op) \
    (assert(PyTuple_Check(op)), _Py_CAST(PyTupleObject*, (op)))

// Macros and static inline functions, trading safety for speed

static inline Py_ssize_t PyTuple_GET_SIZE(PyObject *op) {
    PyTupleObject *tuple = _PyTuple_CAST(op);
    return Py_SIZE(tuple);
}
#define PyTuple_GET_SIZE(op) PyTuple_GET_SIZE(_PyObject_CAST(op))

#define PyTuple_GET_ITEM(op, index) (_PyTuple_CAST(op)->ob_item[(index)])

/* Function *only* to be used to fill in brand new tuples */
static inline void
PyTuple_SET_ITEM(PyObject *op, Py_ssize_t index, PyObject *value) {
    PyTupleObject *tuple = _PyTuple_CAST(op);
    assert(0 <= index);
    assert(index < Py_SIZE(tuple));
    tuple->ob_item[index] = value;
}
#define PyTuple_SET_ITEM(op, index, value) \
    PyTuple_SET_ITEM(_PyObject_CAST(op), (index), _PyObject_CAST(value))

PyAPI_FUNC(PyObject*) PyTuple_FromArray(
    PyObject *const *array,
    Py_ssize_t size);
