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

#if defined(PyTuple_MAXSAVESIZE) && PyTuple_MAXSAVESIZE <= 0
   // A build indicated that tuple freelists should not be used.
#  define PyTuple_NFREELISTS 0
#  undef PyTuple_MAXSAVESIZE
#  undef PyTuple_MAXFREELIST

#elif !defined(WITH_FREELISTS)
#  define PyTuple_NFREELISTS 0
#  undef PyTuple_MAXSAVESIZE
#  undef PyTuple_MAXFREELIST

#else
   // We are using a freelist for tuples.
#  ifndef PyTuple_MAXSAVESIZE
#    define PyTuple_MAXSAVESIZE 20
#  endif
#  define PyTuple_NFREELISTS PyTuple_MAXSAVESIZE
#  ifndef PyTuple_MAXFREELIST
#    define PyTuple_MAXFREELIST 2000
#  endif
#endif

struct _Py_tuple_state {
#if PyTuple_NFREELISTS > 0
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

#define _PyTuple_ITEMS(op) (_PyTuple_CAST(op)->ob_item)

extern PyObject *_PyTuple_FromArray(PyObject *const *, Py_ssize_t);
extern PyObject *_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);

/* A faster, but unsafe, version of PyTuple_New for use in performance-
 * critical paths.
 *
 * Zeroing the contents of a tuple is surprisingly expensive, so for
 * cases where the contents of the tuple will immediately be overwritten,
 * time can be saved by calling this non-zeroing version.
 *
 * This tuple is not safe to be seen by any other code since it will
 * have garbage where PyObject pointers are expected. If there is any
 * chance that the initialization code will raise an exception or do
 * a GC collection, it is not safe to use this function.
 */
PyAPI_FUNC(PyObject *) _PyTuple_New_Nonzeroed(Py_ssize_t size);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
