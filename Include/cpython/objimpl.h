#ifndef Py_CPYTHON_OBJIMPL_H
#  error "this header file must not be included directly"
#endif

static inline size_t _PyObject_SIZE(PyTypeObject *type) {
    return _Py_STATIC_CAST(size_t, type->tp_basicsize);
}

/* _PyObject_VAR_SIZE returns the number of bytes (as size_t) allocated for a
   vrbl-size object with nitems items, exclusive of gc overhead (if any).  The
   value is rounded up to the closest multiple of sizeof(void *), in order to
   ensure that pointer fields at the end of the object are correctly aligned
   for the platform (this is of special importance for subclasses of, e.g.,
   str or int, so that pointers can be stored after the embedded data).

   Note that there's no memory wastage in doing this, as malloc has to
   return (at worst) pointer-aligned memory anyway.
*/
#if ((SIZEOF_VOID_P - 1) & SIZEOF_VOID_P) != 0
#   error "_PyObject_VAR_SIZE requires SIZEOF_VOID_P be a power of 2"
#endif

static inline size_t _PyObject_VAR_SIZE(PyTypeObject *type, Py_ssize_t nitems) {
    size_t size = _Py_STATIC_CAST(size_t, type->tp_basicsize);
    size += _Py_STATIC_CAST(size_t, nitems) * _Py_STATIC_CAST(size_t, type->tp_itemsize);
    return _Py_SIZE_ROUND_UP(size, SIZEOF_VOID_P);
}


/* This example code implements an object constructor with a custom
   allocator, where PyObject_New is inlined, and shows the important
   distinction between two steps (at least):
       1) the actual allocation of the object storage;
       2) the initialization of the Python specific fields
      in this storage with PyObject_{Init, InitVar}.

   PyObject *
   YourObject_New(...)
   {
       PyObject *op;

       op = (PyObject *) Your_Allocator(_PyObject_SIZE(YourTypeStruct));
       if (op == NULL) {
           return PyErr_NoMemory();
       }

       PyObject_Init(op, &YourTypeStruct);

       op->ob_field = value;
       ...
       return op;
   }

   Note that in C++, the use of the new operator usually implies that
   the 1st step is performed automatically for you, so in a C++ class
   constructor you would start directly with PyObject_Init/InitVar. */


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


/* Test if an object implements the garbage collector protocol */
PyAPI_FUNC(int) PyObject_IS_GC(PyObject *obj);


// Test if a type supports weak references
PyAPI_FUNC(int) PyType_SUPPORTS_WEAKREFS(PyTypeObject *type);

PyAPI_FUNC(PyObject **) PyObject_GET_WEAKREFS_LISTPTR(PyObject *op);

PyAPI_FUNC(PyObject *) PyUnstable_Object_GC_NewWithExtraData(PyTypeObject *,
                                                             size_t);


/* Visit all live GC-capable objects, similar to gc.get_objects(None). The
 * supplied callback is called on every such object with the void* arg set
 * to the supplied arg. Returning 0 from the callback ends iteration, returning
 * 1 allows iteration to continue. Returning any other value may result in
 * undefined behaviour.
 *
 * If new objects are (de)allocated by the callback it is undefined if they
 * will be visited.

 * Garbage collection is disabled during operation. Explicitly running a
 * collection in the callback may lead to undefined behaviour e.g. visiting the
 * same objects multiple times or not at all.
 */
typedef int (*gcvisitobjects_t)(PyObject*, void*);
PyAPI_FUNC(void) PyUnstable_GC_VisitObjects(gcvisitobjects_t callback, void* arg);
