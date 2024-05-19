#ifndef Py_CPYTHON_BYTESOBJECT_H
#  error "this header file must not be included directly"
#endif

typedef struct {
    PyObject_VAR_HEAD
    Py_DEPRECATED(3.11) Py_hash_t ob_shash;
    char ob_sval[1];

    /* Invariants:
     *     ob_sval contains space for 'ob_size+1' elements.
     *     ob_sval[ob_size] == 0.
     *     ob_shash is the hash of the byte string or -1 if not computed yet.
     */
} PyBytesObject;

PyAPI_FUNC(int) _PyBytes_Resize(PyObject **, Py_ssize_t);

/* Macros and static inline functions, trading safety for speed */
#define _PyBytes_CAST(op) \
    (assert(PyBytes_Check(op)), _Py_CAST(PyBytesObject*, op))

static inline char* PyBytes_AS_STRING(PyObject *op)
{
    return _PyBytes_CAST(op)->ob_sval;
}
#define PyBytes_AS_STRING(op) PyBytes_AS_STRING(_PyObject_CAST(op))

static inline Py_ssize_t PyBytes_GET_SIZE(PyObject *op) {
    PyBytesObject *self = _PyBytes_CAST(op);
    return Py_SIZE(self);
}
#define PyBytes_GET_SIZE(self) PyBytes_GET_SIZE(_PyObject_CAST(self))
