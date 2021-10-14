#ifndef Py_CPYTHON_BYTEARRAYOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Object layout */
typedef struct {
    PyObject_VAR_HEAD
    Py_ssize_t ob_alloc;   /* How many bytes allocated in ob_bytes */
    char *ob_bytes;        /* Physical backing buffer */
    char *ob_start;        /* Logical start inside ob_bytes */
    Py_ssize_t ob_exports; /* How many buffer exports */
} PyByteArrayObject;

PyAPI_DATA(char) _PyByteArray_empty_string[];

// Static inline function, trading safety for speed
static inline char* _PyByteArray_AS_STRING(PyByteArrayObject *self) {
    assert(PyByteArray_Check(self));
    if (Py_SIZE(self)) {
        return self->ob_start;
    }
    else {
        return _PyByteArray_empty_string;
    }
}
#define PyByteArray_AS_STRING(op) _PyByteArray_AS_STRING((PyByteArrayObject *)(op))

// Macro, trading safety for speed
#define PyByteArray_GET_SIZE(self) (assert(PyByteArray_Check(self)), Py_SIZE(self))
