#ifndef Py_INTERNAL_TYPEID_H
#define Py_INTERNAL_TYPEID_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_GIL_DISABLED

// This contains code for allocating unique ids to heap type objects
// and re-using those ids when the type is deallocated.
//
// The type ids are used to implement per-thread reference counts of
// heap type objects to avoid contention on the reference count fields
// of heap type objects. Static type objects are immortal, so contention
// is not an issue for those types.
//
// Type id of -1 is used to indicate a type doesn't use thread-local
// refcounting. This value is used when a type object is finalized by the GC
// and during interpreter shutdown to allow the type object to be
// deallocated promptly when the object's refcount reaches zero.
//
// Each entry implicitly represents a type id based on it's offset in the
// table. Non-allocated entries form a free-list via the 'next' pointer.
// Allocated entries store the corresponding PyTypeObject.
typedef union _Py_type_id_entry {
    // Points to the next free type id, when part of the freelist
    union _Py_type_id_entry *next;

    // Stores the type object when the id is assigned
    PyHeapTypeObject *type;
} _Py_type_id_entry;

struct _Py_type_id_pool {
    PyMutex mutex;

    // combined table of types with allocated type ids and unallocated
    // type ids.
    _Py_type_id_entry *table;

    // Next entry to allocate inside 'table' or NULL
    _Py_type_id_entry *freelist;

    // size of 'table'
    Py_ssize_t size;
};

// Assigns the next id from the pool of type ids.
extern void _PyType_AssignId(PyHeapTypeObject *type);

// Releases the allocated type id back to the pool.
extern void _PyType_ReleaseId(PyHeapTypeObject *type);

// Merges the thread-local reference counts into the corresponding types.
extern void _PyType_MergeThreadLocalRefcounts(_PyThreadStateImpl *tstate);

// Like _PyType_MergeThreadLocalRefcounts, but also frees the thread-local
// array of refcounts.
extern void _PyType_FinalizeThreadLocalRefcounts(_PyThreadStateImpl *tstate);

// Frees the interpreter's pool of type ids.
extern void _PyType_FinalizeIdPool(PyInterpreterState *interp);

// Increfs the type, resizing the thread-local refcount array if necessary.
PyAPI_FUNC(void) _PyType_IncrefSlow(PyHeapTypeObject *type);

#endif   /* Py_GIL_DISABLED */

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TYPEID_H */
