#ifndef Py_INTERNAL_UNIQUEID_H
#define Py_INTERNAL_UNIQUEID_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#ifdef Py_GIL_DISABLED

// This contains code for allocating unique ids to objects for per-thread
// reference counting.
//
// Per-thread reference counting is used along with deferred reference
// counting to avoid scaling bottlenecks due to reference count contention.
//
// An id of 0 is used to indicate that an object doesn't use per-thread
// refcounting. This value is used when the object is finalized by the GC
// and during interpreter shutdown to allow the object to be
// deallocated promptly when the object's refcount reaches zero.
//
// Each entry implicitly represents a unique id based on its offset in the
// table. Non-allocated entries form a free-list via the 'next' pointer.
// Allocated entries store the corresponding PyObject.

#define _Py_INVALID_UNIQUE_ID 0

// Assigns the next id from the pool of ids.
extern Py_ssize_t _PyObject_AssignUniqueId(PyObject *obj);

// Releases the allocated id back to the pool.
extern void _PyObject_ReleaseUniqueId(Py_ssize_t unique_id);

// Releases the allocated id back to the pool.
extern void _PyObject_DisablePerThreadRefcounting(PyObject *obj);

// Merges the per-thread reference counts into the corresponding objects.
extern void _PyObject_MergePerThreadRefcounts(_PyThreadStateImpl *tstate);

// Like _PyObject_MergePerThreadRefcounts, but also frees the per-thread
// array of refcounts.
extern void _PyObject_FinalizePerThreadRefcounts(_PyThreadStateImpl *tstate);

// Frees the interpreter's pool of type ids.
extern void _PyObject_FinalizeUniqueIdPool(PyInterpreterState *interp);

// Increfs the object, resizing the thread-local refcount array if necessary.
PyAPI_FUNC(void) _PyObject_ThreadIncrefSlow(PyObject *obj, size_t idx);

#endif   /* Py_GIL_DISABLED */

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_UNIQUEID_H */
