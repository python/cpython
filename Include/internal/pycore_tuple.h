#ifndef Py_INTERNAL_TUPLE_H
#define Py_INTERNAL_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"        // _PyObject_GC_IS_TRACKED
#include "pycore_structs.h"       // _PyStackRef

extern void _PyTuple_MaybeUntrack(PyObject *);
extern void _PyTuple_DebugMallocStats(FILE *out);

/* runtime lifecycle */

extern PyStatus _PyTuple_InitGlobalObjects(PyInterpreterState *);


/* other API */

#define _PyTuple_ITEMS(op) _Py_RVALUE(_PyTuple_CAST(op)->ob_item)

PyAPI_FUNC(PyObject *)_PyTuple_FromArray(PyObject *const *, Py_ssize_t);
PyAPI_FUNC(PyObject *)_PyTuple_FromStackRefStealOnSuccess(const union _PyStackRef *, Py_ssize_t);
PyAPI_FUNC(PyObject *)_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);

typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyTupleObject *it_seq; /* Set to NULL when iterator is exhausted */
} _PyTupleIterObject;

#define _PyTuple_RESET_HASH_CACHE(op)       \
    do {                                    \
        assert(op != NULL);                 \
        _PyTuple_CAST(op)->ob_hash = -1;    \
    } while (0)

/* bpo-42536: If reusing a tuple object, this should be called to re-track it
   with the garbage collector and reset its hash cache. */
static inline void
_PyTuple_Recycle(PyObject *op)
{
    _PyTuple_RESET_HASH_CACHE(op);
    if (!_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_TRACK(op);
    }
}

/* Below are the official constants from the xxHash specification. Optimizing
   compilers should emit a single "rotate" instruction for the
   _PyTuple_HASH_XXROTATE() expansion. If that doesn't happen for some important
   platform, the macro could be changed to expand to a platform-specific rotate
   spelling instead.
*/
#if SIZEOF_PY_UHASH_T > 4
#define _PyTuple_HASH_XXPRIME_1 ((Py_uhash_t)11400714785074694791ULL)
#define _PyTuple_HASH_XXPRIME_2 ((Py_uhash_t)14029467366897019727ULL)
#define _PyTuple_HASH_XXPRIME_5 ((Py_uhash_t)2870177450012600261ULL)
#define _PyTuple_HASH_XXROTATE(x) ((x << 31) | (x >> 33))  /* Rotate left 31 bits */
#else
#define _PyTuple_HASH_XXPRIME_1 ((Py_uhash_t)2654435761UL)
#define _PyTuple_HASH_XXPRIME_2 ((Py_uhash_t)2246822519UL)
#define _PyTuple_HASH_XXPRIME_5 ((Py_uhash_t)374761393UL)
#define _PyTuple_HASH_XXROTATE(x) ((x << 13) | (x >> 19))  /* Rotate left 13 bits */
#endif
#define _PyTuple_HASH_EMPTY (_PyTuple_HASH_XXPRIME_5 + (_PyTuple_HASH_XXPRIME_5 ^ 3527539UL))

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
