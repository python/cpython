#ifndef Py_CPYTHON_MEMORYOBJECT_H
#  error "this header file must not be included directly"
#endif

/* The structs are declared here so that macros can work, but they shouldn't
   be considered public. Don't access their fields directly, use the macros
   and functions instead! */
#define _Py_MANAGED_BUFFER_RELEASED    0x001  /* access to exporter blocked */
#define _Py_MANAGED_BUFFER_FREE_FORMAT 0x002  /* free format */

typedef struct {
    PyObject_HEAD
    int flags;          /* state flags */
    Py_ssize_t exports; /* number of direct memoryview exports */
    Py_buffer master; /* snapshot buffer obtained from the original exporter */
} _PyManagedBufferObject;


/* memoryview state flags */
#define _Py_MEMORYVIEW_RELEASED    0x001  /* access to master buffer blocked */
#define _Py_MEMORYVIEW_C           0x002  /* C-contiguous layout */
#define _Py_MEMORYVIEW_FORTRAN     0x004  /* Fortran contiguous layout */
#define _Py_MEMORYVIEW_SCALAR      0x008  /* scalar: ndim = 0 */
#define _Py_MEMORYVIEW_PIL         0x010  /* PIL-style layout */
#define _Py_MEMORYVIEW_RESTRICTED  0x020  /* Disallow new references to the memoryview's buffer */

typedef struct {
    PyObject_VAR_HEAD
    _PyManagedBufferObject *mbuf; /* managed buffer */
    Py_hash_t hash;               /* hash value for read-only views */
    int flags;                    /* state flags */
    Py_ssize_t exports;           /* number of buffer re-exports */
    Py_buffer view;               /* private copy of the exporter's view */
    PyObject *weakreflist;
    Py_ssize_t ob_array[1];       /* shape, strides, suboffsets */
} PyMemoryViewObject;

#define _PyMemoryView_CAST(op) _Py_CAST(PyMemoryViewObject*, op)

/* Get a pointer to the memoryview's private copy of the exporter's buffer. */
static inline Py_buffer* PyMemoryView_GET_BUFFER(PyObject *op) {
    return (&_PyMemoryView_CAST(op)->view);
}
#define PyMemoryView_GET_BUFFER(op) PyMemoryView_GET_BUFFER(_PyObject_CAST(op))

/* Get a pointer to the exporting object (this may be NULL!). */
static inline PyObject* PyMemoryView_GET_BASE(PyObject *op) {
    return _PyMemoryView_CAST(op)->view.obj;
}
#define PyMemoryView_GET_BASE(op) PyMemoryView_GET_BASE(_PyObject_CAST(op))
