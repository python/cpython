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

extern PyStatus _PyTuple_InitTypes(PyInterpreterState *);
extern void _PyTuple_Fini(PyInterpreterState *);


/* other API */

#ifndef WITH_FREELISTS
// without freelists
// for tuples only store empty tuple singleton
#  define PyTuple_MAXSAVESIZE 0
#  define PyTuple_MAXFREELIST 0
#endif

/* Speed optimization to avoid frequent malloc/free of small tuples */
#ifndef PyTuple_MAXSAVESIZE
   // Largest tuple to save on free list
#  define PyTuple_MAXSAVESIZE 20
#endif
#ifndef PyTuple_MAXFREELIST
   // Maximum number of tuples of each size to save
#  define PyTuple_MAXFREELIST 2000
#endif

struct _Py_tuple_state {
#if PyTuple_MAXSAVESIZE > 0
    /* Each entry up to PyTuple_MAXSAVESIZE is a free list.
       The empty tuple is handled separately, hence declaring one fewer. */
    PyTupleObject *free_list[PyTuple_MAXSAVESIZE - 1];
    int numfree[PyTuple_MAXSAVESIZE - 1];
#endif
};

#define _PyTuple_ITEMS(op) (_PyTuple_CAST(op)->ob_item)

extern PyObject *_PyTuple_FromArray(PyObject *const *, Py_ssize_t);
extern PyObject *_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
