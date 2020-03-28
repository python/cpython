#ifndef Py_INTERNAL_PYMEM_H
#define Py_INTERNAL_PYMEM_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pymem.h"   /* PyMemAllocatorName */


/* GC runtime state */

/* If we change this, we need to change the default value in the
   signature of gc.collect. */
#define NUM_GENERATIONS 3
/*
   NOTE: about untracking of mutable objects.

   Certain types of container cannot participate in a reference cycle, and
   so do not need to be tracked by the garbage collector. Untracking these
   objects reduces the cost of garbage collections. However, determining
   which objects may be untracked is not free, and the costs must be
   weighed against the benefits for garbage collection.

   There are two possible strategies for when to untrack a container:

   i) When the container is created.
   ii) When the container is examined by the garbage collector.

   Tuples containing only immutable objects (integers, strings etc, and
   recursively, tuples of immutable objects) do not need to be tracked.
   The interpreter creates a large number of tuples, many of which will
   not survive until garbage collection. It is therefore not worthwhile
   to untrack eligible tuples at creation time.

   Instead, all tuples except the empty tuple are tracked when created.
   During garbage collection it is determined whether any surviving tuples
   can be untracked. A tuple can be untracked if all of its contents are
   already not tracked. Tuples are examined for untracking in all garbage
   collection cycles. It may take more than one cycle to untrack a tuple.

   Dictionaries containing only immutable objects also do not need to be
   tracked. Dictionaries are untracked when created. If a tracked item is
   inserted into a dictionary (either as a key or value), the dictionary
   becomes tracked. During a full garbage collection (all generations),
   the collector will untrack any dictionaries whose contents are not
   tracked.

   The module provides the python function is_tracked(obj), which returns
   the CURRENT tracking status of the object. Subsequent garbage
   collections may change the tracking status of the object.

   Untracking of certain containers was introduced in issue #4688, and
   the algorithm was refined in response to issue #14775.
*/

struct gc_generation {
    PyGC_Head head;
    int threshold; /* collection threshold */
    int count; /* count of allocations or collections of younger
                  generations */
};

/* Running stats per generation */
struct gc_generation_stats {
    /* total number of collections */
    Py_ssize_t collections;
    /* total number of collected objects */
    Py_ssize_t collected;
    /* total number of uncollectable objects (put into gc.garbage) */
    Py_ssize_t uncollectable;
};

struct _gc_runtime_state {
    /* List of objects that still need to be cleaned up, singly linked
     * via their gc headers' gc_prev pointers.  */
    PyObject *trash_delete_later;
    /* Current call-stack depth of tp_dealloc calls. */
    int trash_delete_nesting;

    int enabled;
    int debug;
    /* linked lists of container objects */
    struct gc_generation generations[NUM_GENERATIONS];
    PyGC_Head *generation0;
    /* a permanent generation which won't be collected */
    struct gc_generation permanent_generation;
    struct gc_generation_stats generation_stats[NUM_GENERATIONS];
    /* true if we are currently running the collector */
    int collecting;
    /* list of uncollectable objects */
    PyObject *garbage;
    /* a list of callbacks to be invoked when collection is performed */
    PyObject *callbacks;
    /* This is the number of objects that survived the last full
       collection. It approximates the number of long lived objects
       tracked by the GC.

       (by "full collection", we mean a collection of the oldest
       generation). */
    Py_ssize_t long_lived_total;
    /* This is the number of objects that survived all "non-full"
       collections, and are awaiting to undergo a full collection for
       the first time. */
    Py_ssize_t long_lived_pending;
};

PyAPI_FUNC(void) _PyGC_InitState(struct _gc_runtime_state *);


/* Set the memory allocator of the specified domain to the default.
   Save the old allocator into *old_alloc if it's non-NULL.
   Return on success, or return -1 if the domain is unknown. */
PyAPI_FUNC(int) _PyMem_SetDefaultAllocator(
    PyMemAllocatorDomain domain,
    PyMemAllocatorEx *old_alloc);

/* Special bytes broadcast into debug memory blocks at appropriate times.
   Strings of these are unlikely to be valid addresses, floats, ints or
   7-bit ASCII.

   - PYMEM_CLEANBYTE: clean (newly allocated) memory
   - PYMEM_DEADBYTE dead (newly freed) memory
   - PYMEM_FORBIDDENBYTE: untouchable bytes at each end of a block

   Byte patterns 0xCB, 0xDB and 0xFB have been replaced with 0xCD, 0xDD and
   0xFD to use the same values than Windows CRT debug malloc() and free().
   If modified, _PyMem_IsPtrFreed() should be updated as well. */
#define PYMEM_CLEANBYTE      0xCD
#define PYMEM_DEADBYTE       0xDD
#define PYMEM_FORBIDDENBYTE  0xFD

/* Heuristic checking if a pointer value is newly allocated
   (uninitialized), newly freed or NULL (is equal to zero).

   The pointer is not dereferenced, only the pointer value is checked.

   The heuristic relies on the debug hooks on Python memory allocators which
   fills newly allocated memory with CLEANBYTE (0xCD) and newly freed memory
   with DEADBYTE (0xDD). Detect also "untouchable bytes" marked
   with FORBIDDENBYTE (0xFD). */
static inline int _PyMem_IsPtrFreed(void *ptr)
{
    uintptr_t value = (uintptr_t)ptr;
#if SIZEOF_VOID_P == 8
    return (value == 0
            || value == (uintptr_t)0xCDCDCDCDCDCDCDCD
            || value == (uintptr_t)0xDDDDDDDDDDDDDDDD
            || value == (uintptr_t)0xFDFDFDFDFDFDFDFD);
#elif SIZEOF_VOID_P == 4
    return (value == 0
            || value == (uintptr_t)0xCDCDCDCD
            || value == (uintptr_t)0xDDDDDDDD
            || value == (uintptr_t)0xFDFDFDFD);
#else
#  error "unknown pointer size"
#endif
}

PyAPI_FUNC(int) _PyMem_GetAllocatorName(
    const char *name,
    PyMemAllocatorName *allocator);

/* Configure the Python memory allocators.
   Pass PYMEM_ALLOCATOR_DEFAULT to use default allocators.
   PYMEM_ALLOCATOR_NOT_SET does nothing. */
PyAPI_FUNC(int) _PyMem_SetupAllocators(PyMemAllocatorName allocator);

/* bpo-35053: Expose _Py_tracemalloc_config for _Py_NewReference()
   which access directly _Py_tracemalloc_config.tracing for best
   performances. */
struct _PyTraceMalloc_Config {
    /* Module initialized?
       Variable protected by the GIL */
    enum {
        TRACEMALLOC_NOT_INITIALIZED,
        TRACEMALLOC_INITIALIZED,
        TRACEMALLOC_FINALIZED
    } initialized;

    /* Is tracemalloc tracing memory allocations?
       Variable protected by the GIL */
    int tracing;

    /* limit of the number of frames in a traceback, 1 by default.
       Variable protected by the GIL. */
    int max_nframe;

    /* use domain in trace key?
       Variable protected by the GIL. */
    int use_domain;
};

#define _PyTraceMalloc_Config_INIT \
    {.initialized = TRACEMALLOC_NOT_INITIALIZED, \
     .tracing = 0, \
     .max_nframe = 1, \
     .use_domain = 0}

PyAPI_DATA(struct _PyTraceMalloc_Config) _Py_tracemalloc_config;


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYMEM_H */
