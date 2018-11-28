#ifndef Py_CPYTHON_TUPLEOBJECT_H
#  error "this header file must not be included directly"
#endif

#ifdef __cplusplus
extern "C" {
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

/* Macros and inline functions trading safety for speed */

/* Cast argument to PyTupleObject* type. */
#define _PyTuple_CAST(op) (assert(PyTuple_Check(op)), (PyTupleObject *)(op))

#define PyTuple_GET_SIZE(op)    Py_SIZE(_PyTuple_CAST(op))

static inline PyObject* _PyTuple_GET_ITEM(PyTupleObject *op, Py_ssize_t i)
{
    assert(0 <= i && i < Py_SIZE(op));
    return op->ob_item[i];
}

#define PyTuple_GET_ITEM(op, i) _PyTuple_GET_ITEM(_PyTuple_CAST(op), i)

static inline void _PyTuple_SET_ITEM(PyTupleObject *op, Py_ssize_t i, PyObject *item)
{
    assert(0 <= i && i < Py_SIZE(op));
    op->ob_item[i] = item;
}

/* Macro, *only* to be used to fill in brand new tuples */
#define PyTuple_SET_ITEM(op, i, v) _PyTuple_SET_ITEM(_PyTuple_CAST(op), i, v)

PyAPI_FUNC(void) _PyTuple_DebugMallocStats(FILE *out);

#ifdef __cplusplus
}
#endif
