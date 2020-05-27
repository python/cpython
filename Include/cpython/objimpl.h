#ifndef Py_CPYTHON_OBJIMPL_H
#  error "this header file must not be included directly"
#endif

/* Inline functions trading binary compatibility for speed:
   PyObject_INIT() is the fast version of PyObject_Init(), and
   PyObject_INIT_VAR() is the fast version of PyObject_InitVar().

   These inline functions must not be called with op=NULL. */
static inline PyObject*
_PyObject_INIT(PyObject *op, PyTypeObject *typeobj)
{
    assert(op != NULL);
    Py_SET_TYPE(op, typeobj);
    if (PyType_GetFlags(typeobj) & Py_TPFLAGS_HEAPTYPE) {
        Py_INCREF(typeobj);
    }
    _Py_NewReference(op);
    return op;
}

#define PyObject_INIT(op, typeobj) \
    _PyObject_INIT(_PyObject_CAST(op), (typeobj))

static inline PyVarObject*
_PyObject_INIT_VAR(PyVarObject *op, PyTypeObject *typeobj, Py_ssize_t size)
{
    assert(op != NULL);
    Py_SET_SIZE(op, size);
    PyObject_INIT((PyObject *)op, typeobj);
    return op;
}

#define PyObject_INIT_VAR(op, typeobj, size) \
    _PyObject_INIT_VAR(_PyVarObject_CAST(op), (typeobj), (size))


/* This function returns the number of allocated memory blocks, regardless of size */
PyAPI_FUNC(Py_ssize_t) _Py_GetAllocatedBlocks(void);

/* Macros */
#ifdef WITH_PYMALLOC
PyAPI_FUNC(int) _PyObject_DebugMallocStats(FILE *out);
#endif


typedef struct {
    /* user context passed as the first argument to the 2 functions */
    void *ctx;

    /* allocate an arena of size bytes */
    void* (*alloc) (void *ctx, size_t size);

    /* free an arena */
    void (*free) (void *ctx, void *ptr, size_t size);
} PyObjectArenaAllocator;

/* Get the arena allocator. */
PyAPI_FUNC(void) PyObject_GetArenaAllocator(PyObjectArenaAllocator *allocator);

/* Set the arena allocator. */
PyAPI_FUNC(void) PyObject_SetArenaAllocator(PyObjectArenaAllocator *allocator);


PyAPI_FUNC(Py_ssize_t) _PyGC_CollectNoFail(void);
PyAPI_FUNC(Py_ssize_t) _PyGC_CollectIfEnabled(void);


/* Test if an object implements the garbage collector protocol */
PyAPI_FUNC(int) PyObject_IS_GC(PyObject *obj);


/* Code built with Py_BUILD_CORE must include pycore_gc.h instead which
   defines a different _PyGC_FINALIZED() macro. */
#ifndef Py_BUILD_CORE
   // Kept for backward compatibility with Python 3.8
#  define _PyGC_FINALIZED(o) PyObject_GC_IsFinalized(o)
#endif

PyAPI_FUNC(PyObject *) _PyObject_GC_Malloc(size_t size);
PyAPI_FUNC(PyObject *) _PyObject_GC_Calloc(size_t size);


/* Test if a type supports weak references */
#define PyType_SUPPORTS_WEAKREFS(t) ((t)->tp_weaklistoffset > 0)

PyAPI_FUNC(PyObject **) PyObject_GET_WEAKREFS_LISTPTR(PyObject *op);
