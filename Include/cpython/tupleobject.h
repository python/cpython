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

/* Macros trading safety for speed */

/* Cast argument to PyTupleObject* type. */
#define _PyTuple_CAST(op) (assert(PyTuple_Check(op)), (PyTupleObject *)(op))

#define PyTuple_GET_SIZE(op) _Py_RVALUE(Py_SIZE(_PyTuple_CAST(op)))

// Don't use _Py_RVALUE() for now since many C extensions abuse
// PyTuple_GET_ITEM() to get the PyTuple_GET_ITEM.ob_item array
// using: "&PyTuple_GET_ITEM(tuple, 0)".
#define PyTuple_GET_ITEM(op, i) (_PyTuple_CAST(op)->ob_item[i])

/* Macro, *only* to be used to fill in brand new tuples */
#define PyTuple_SET_ITEM(op, i, v) _Py_RVALUE(_PyTuple_CAST(op)->ob_item[i] = (v))

PyAPI_FUNC(void) _PyTuple_DebugMallocStats(FILE *out);
