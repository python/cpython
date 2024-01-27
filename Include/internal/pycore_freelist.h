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
#else
#  define PyTuple_NFREELISTS 0
#  define PyTuple_MAXFREELIST 0
#  define PyList_MAXFREELIST 0
#  define PyFloat_MAXFREELIST 0
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

typedef struct _Py_freelist_state {
    struct _Py_float_state float_state;
    struct _Py_tuple_state tuple_state;
    struct _Py_list_state list_state;
} _PyFreeListState;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
