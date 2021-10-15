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

/* Macros, trading safety for speed */
#define _PyByteArray_CAST(op) \
    (assert(PyByteArray_Check(op)), (PyByteArrayObject *)(op))
#define PyByteArray_AS_STRING(self) \
    (assert(PyByteArray_Check(self)), \
     Py_SIZE(self) ? ((PyByteArrayObject *)(self))->ob_start : _PyByteArray_empty_string)
#define PyByteArray_GET_SIZE(self) \
    _Py_RVALUE(Py_SIZE(_PyByteArray_CAST(self)))
