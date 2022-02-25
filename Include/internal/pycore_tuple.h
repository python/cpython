#ifndef Py_INTERNAL_TUPLE_H
#define Py_INTERNAL_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "tupleobject.h"   /* _PyTuple_CAST() */


/* runtime lifecycle */

extern PyStatus _PyTuple_InitGlobalObjects(PyInterpreterState *);
extern PyStatus _PyTuple_InitTypes(PyInterpreterState *);
extern void _PyTuple_Fini(PyInterpreterState *);


/* other API */

// PyTuple_MAXSAVESIZE - largest tuple to save on free list
// PyTuple_MAXFREELIST - maximum number of tuples of each size to save

#if PyTuple_MAXSAVESIZE <= 0
   // A build indicated that no tuple freelists should be used.
#  define PyTuple_NFREELISTS 0
#  undef PyTuple_MAXSAVESIZE

#elif !defined(WITH_FREELISTS)
   // Only store the empty tuple singleton.
#  define PyTuple_NFREELISTS 1
#  ifndef PyTuple_MAXSAVESIZE
#    define PyTuple_MAXSAVESIZE 0
#  endif
#  ifndef PyTuple_MAXFREELIST
#    define PyTuple_MAXFREELIST 1
#  endif

#else
#  ifndef PyTuple_MAXSAVESIZE
#    define PyTuple_MAXSAVESIZE 20
#  endif
#  define PyTuple_NFREELISTS (PyTuple_MAXSAVESIZE + 1)
#  ifndef PyTuple_MAXFREELIST
#    define PyTuple_MAXFREELIST 2000
#  endif
#endif

struct _Py_tuple_state {
#if PyTuple_NFREELISTS > 0
    /* There is one freelist for each size from 1 to PyTuple_MAXSAVESIZE.
       Entry 0 is the empty tuple () of which at most one instance
       will be allocated.

       Each tuple stored in the array is the head of the linked list
       (and the next available tuple) for that size.  The actual tuple
       object is used as the linked list node, with its first item
       (ob_item[0]) pointing to the next node (i.e. the previous head).
       Each linked list is initially NULL. */
    PyTupleObject *free_list[PyTuple_NFREELISTS];
    int numfree[PyTuple_NFREELISTS];
#endif
};

#define _PyTuple_ITEMS(op) (_PyTuple_CAST(op)->ob_item)

extern PyObject *_PyTuple_FromArray(PyObject *const *, Py_ssize_t);
extern PyObject *_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
