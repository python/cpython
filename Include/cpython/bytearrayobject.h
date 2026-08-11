#ifndef Py_CPYTHON_BYTEARRAYOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Object layout */
typedef struct {
    PyObject_VAR_HEAD
    /* How many bytes allocated in ob_bytes

       In the current implementation this is equivalent to Py_SIZE(ob_bytes_object).
       The value is always loaded and stored atomically for thread safety.
       There are API compatibilty concerns with removing so keeping for now. */
    Py_ssize_t ob_alloc;
    char *ob_bytes;        /* Physical backing buffer */
    char *ob_start;        /* Logical start inside ob_bytes */
    Py_ssize_t ob_exports; /* How many buffer exports */
    PyObject *ob_bytes_object;  /* PyBytes for zero-copy bytes conversion */
} PyByteArrayObject;

/* Macros and static inline functions, trading safety for speed */
#define _PyByteArray_CAST(op) \
    (assert(PyByteArray_Check(op)), _Py_CAST(PyByteArrayObject*, op))

static inline char* PyByteArray_AS_STRING(PyObject *op)
{
    return _PyByteArray_CAST(op)->ob_start;
}
#define PyByteArray_AS_STRING(self) PyByteArray_AS_STRING(_PyObject_CAST(self))

static inline Py_ssize_t PyByteArray_GET_SIZE(PyObject *op) {
    PyByteArrayObject *self = _PyByteArray_CAST(op);
#ifdef Py_GIL_DISABLED
    return _Py_atomic_load_ssize_relaxed(&(_PyVarObject_CAST(self)->ob_size));
#else
    return Py_SIZE(self);
#endif
}
#define PyByteArray_GET_SIZE(self) PyByteArray_GET_SIZE(_PyObject_CAST(self))
