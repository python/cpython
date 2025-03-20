#ifndef Py_INTERNAL_TUPLE_H
#define Py_INTERNAL_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_object.h"            // _PyObject_GC_IS_TRACKED
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

#define _PyTuple_RESET_HASH_CACHE(op) (_PyTuple_CAST(op)->ob_hash = -1)

/*
   bpo-42536: If reusing a tuple object, this should be called to re-track it
   with the garbage collector and reset its hash cache. */
static inline void
_PyTuple_Recycle(PyObject *op)
{
    _PyTuple_RESET_HASH_CACHE(op);
    if (!_PyObject_GC_IS_TRACKED(op)) {
        _PyObject_GC_TRACK(op);
    }
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
