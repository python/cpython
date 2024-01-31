#ifndef Py_INTERNAL_FREELIST_H
#define Py_INTERNAL_FREELIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// PyTuple_MAXSAVESIZE - largest tuple to save on free list
// PyTuple_MAXFREELIST - maximum number of tuples of each size to save

#ifdef WITH_FREELISTS
// with freelists
#  define PyTuple_MAXSAVESIZE 20
#  define PyTuple_NFREELISTS PyTuple_MAXSAVESIZE
#  define PyTuple_MAXFREELIST 2000
#  define PyList_MAXFREELIST 80
#  define PyFloat_MAXFREELIST 100
#  define PyContext_MAXFREELIST 255
# define _PyAsyncGen_MAXFREELIST 80
#  define _PyObjectStackChunk_MAXFREELIST 4
#else
#  define PyTuple_NFREELISTS 0
#  define PyTuple_MAXFREELIST 0
#  define PyList_MAXFREELIST 0
#  define PyFloat_MAXFREELIST 0
#  define PyContext_MAXFREELIST 0
#  define _PyAsyncGen_MAXFREELIST 0
#  define _PyObjectStackChunk_MAXFREELIST 0
#endif

struct _Py_list_state {
#ifdef WITH_FREELISTS
    PyListObject *free_list[PyList_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_tuple_state {
#if WITH_FREELISTS
    /* There is one freelist for each size from 1 to PyTuple_MAXSAVESIZE.
       The empty tuple is handled separately.

       Each tuple stored in the array is the head of the linked list
       (and the next available tuple) for that size.  The actual tuple
       object is used as the linked list node, with its first item
       (ob_item[0]) pointing to the next node (i.e. the previous head).
       Each linked list is initially NULL. */
    PyTupleObject *free_list[PyTuple_NFREELISTS];
    int numfree[PyTuple_NFREELISTS];
#else
    char _unused;  // Empty structs are not allowed.
#endif
};

struct _Py_float_state {
#ifdef WITH_FREELISTS
    /* Special free list
       free_list is a singly-linked list of available PyFloatObjects,
       linked via abuse of their ob_type members. */
    int numfree;
    PyFloatObject *free_list;
#endif
};

struct _Py_slice_state {
#ifdef WITH_FREELISTS
    /* Using a cache is very effective since typically only a single slice is
       created and then deleted again. */
    PySliceObject *slice_cache;
#endif
};

struct _Py_context_state {
#ifdef WITH_FREELISTS
    // List of free PyContext objects
    PyContext *freelist;
    int numfree;
#endif
};

struct _Py_async_gen_state {
#ifdef WITH_FREELISTS
    /* Freelists boost performance 6-10%; they also reduce memory
       fragmentation, as _PyAsyncGenWrappedValue and PyAsyncGenASend
       are short-living objects that are instantiated for every
       __anext__() call. */
    struct _PyAsyncGenWrappedValue* value_freelist[_PyAsyncGen_MAXFREELIST];
    int value_numfree;

    struct PyAsyncGenASend* asend_freelist[_PyAsyncGen_MAXFREELIST];
    int asend_numfree;
#endif
};

struct _PyObjectStackChunk;

struct _Py_object_stack_state {
    struct _PyObjectStackChunk *free_list;
    Py_ssize_t numfree;
};

typedef struct _Py_freelist_state {
    struct _Py_float_state floats;
    struct _Py_tuple_state tuples;
    struct _Py_list_state lists;
    struct _Py_slice_state slices;
    struct _Py_context_state contexts;
    struct _Py_async_gen_state async_gens;
    struct _Py_object_stack_state object_stacks;
} _PyFreeListState;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
