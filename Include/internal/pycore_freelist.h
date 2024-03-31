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
#  define PyDict_MAXFREELIST 80
#  define PyFloat_MAXFREELIST 100
#  define PyContext_MAXFREELIST 255
# define _PyAsyncGen_MAXFREELIST 80
#  define _PyObjectStackChunk_MAXFREELIST 4
#else
#  define PyTuple_NFREELISTS 0
#  define PyTuple_MAXFREELIST 0
#  define PyList_MAXFREELIST 0
#  define PyDict_MAXFREELIST 0
#  define PyFloat_MAXFREELIST 0
#  define PyContext_MAXFREELIST 0
#  define _PyAsyncGen_MAXFREELIST 0
#  define _PyObjectStackChunk_MAXFREELIST 0
#endif

struct _Py_list_freelist {
#ifdef WITH_FREELISTS
    PyListObject *items[PyList_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_tuple_freelist {
#if WITH_FREELISTS
    /* There is one freelist for each size from 1 to PyTuple_MAXSAVESIZE.
       The empty tuple is handled separately.

       Each tuple stored in the array is the head of the linked list
       (and the next available tuple) for that size.  The actual tuple
       object is used as the linked list node, with its first item
       (ob_item[0]) pointing to the next node (i.e. the previous head).
       Each linked list is initially NULL. */
    PyTupleObject *items[PyTuple_NFREELISTS];
    int numfree[PyTuple_NFREELISTS];
#else
    char _unused;  // Empty structs are not allowed.
#endif
};

struct _Py_float_freelist {
#ifdef WITH_FREELISTS
    /* Special free list
       free_list is a singly-linked list of available PyFloatObjects,
       linked via abuse of their ob_type members. */
    int numfree;
    PyFloatObject *items;
#endif
};

struct _Py_dict_freelist {
#ifdef WITH_FREELISTS
    /* Dictionary reuse scheme to save calls to malloc and free */
    PyDictObject *items[PyDict_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_dictkeys_freelist {
#ifdef WITH_FREELISTS
    /* Dictionary keys reuse scheme to save calls to malloc and free */
    PyDictKeysObject *items[PyDict_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_slice_freelist {
#ifdef WITH_FREELISTS
    /* Using a cache is very effective since typically only a single slice is
       created and then deleted again. */
    PySliceObject *slice_cache;
#endif
};

struct _Py_context_freelist {
#ifdef WITH_FREELISTS
    // List of free PyContext objects
    PyContext *items;
    int numfree;
#endif
};

struct _Py_async_gen_freelist {
#ifdef WITH_FREELISTS
    /* Freelists boost performance 6-10%; they also reduce memory
       fragmentation, as _PyAsyncGenWrappedValue and PyAsyncGenASend
       are short-living objects that are instantiated for every
       __anext__() call. */
    struct _PyAsyncGenWrappedValue* items[_PyAsyncGen_MAXFREELIST];
    int numfree;
#endif
};

struct _Py_async_gen_asend_freelist {
#ifdef WITH_FREELISTS
    struct PyAsyncGenASend* items[_PyAsyncGen_MAXFREELIST];
    int numfree;
#endif
};

struct _PyObjectStackChunk;

struct _Py_object_stack_freelist {
    struct _PyObjectStackChunk *items;
    Py_ssize_t numfree;
};

struct _Py_object_freelists {
    struct _Py_float_freelist floats;
    struct _Py_tuple_freelist tuples;
    struct _Py_list_freelist lists;
    struct _Py_dict_freelist dicts;
    struct _Py_dictkeys_freelist dictkeys;
    struct _Py_slice_freelist slices;
    struct _Py_context_freelist contexts;
    struct _Py_async_gen_freelist async_gens;
    struct _Py_async_gen_asend_freelist async_gen_asends;
    struct _Py_object_stack_freelist object_stacks;
};

extern void _PyObject_ClearFreeLists(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyTuple_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyFloat_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyList_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PySlice_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyDict_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyAsyncGen_ClearFreeLists(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyContext_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);
extern void _PyObjectStackChunk_ClearFreeList(struct _Py_object_freelists *freelists, int is_finalization);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
