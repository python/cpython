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

PyAPI_FUNC(void) _PyStolenTuple_Free(PyObject *self);

#define _PyTuple_ITEMS(op) _Py_RVALUE(_PyTuple_CAST(op)->ob_item)

PyAPI_FUNC(PyObject *)_PyTuple_FromStackRefStealOnSuccess(const union _PyStackRef *, Py_ssize_t);
PyAPI_FUNC(PyObject *)_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);
PyAPI_FUNC(PyObject *) _PyTuple_BinarySlice(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_Concat(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_Repeat(PyObject *self, Py_ssize_t n);

PyAPI_FUNC(PyObject *) _PyTuple_FromPair(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) _PyTuple_FromPairSteal(PyObject *, PyObject *);

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


#ifndef NDEBUG
// Check that a tuple can be mutated in-place.
// Usage: assert(_PyTuple_CanMutate(v));
static int
_PyTuple_CanMutate(PyTupleObject *tuple)
{
    assert(PyTuple_Check(tuple));

    // Do not check _PyObject_IsUniquelyReferenced(): the function is called
    // by _PyTupleWriter_CheckConsistency() and a tuple writer can be used by
    // multiple threads using external locking.
    assert(Py_REFCNT(tuple) == 1);

    // The empty tuple must not be modified
    assert(1 <= Py_SIZE(tuple));
    assert(tuple != &_Py_SINGLETON(tuple_empty));

    // The hash value must not be cached yet. Otherwise, it means that the
    // tuple object was already used in Python somehow (ex: as a dictionary
    // key).
    assert(tuple->ob_hash == -1);

    return 1;
}
#endif


/* --- PyTupleWriter API -------------------------------------------------- */

#ifndef NDEBUG
static inline int
_PyTupleWriter_CheckConsistency(PyTupleWriter *writer)
{
    assert(writer != NULL);
    assert(PyTuple_CheckExact(_PyObject_CAST(writer)));
    PyTupleObject *tuple = (PyTupleObject*)writer;
    assert(0 <= Py_SIZE(tuple));
    if (Py_SIZE(tuple) != 0) {
        // Do not use _PyObject_IsUniquelyReferenced(): the caller can have
        // its own lock to prevent a writer from being used by two threads
        // at the same time.
        assert(Py_REFCNT(tuple) == 1);

        assert(!_PyObject_GC_IS_TRACKED(tuple));
    }
    else {
        assert(tuple == &_Py_SINGLETON(tuple_empty));
    }
    return 1;
}
#endif


static inline void
_PyTupleWriter_SET_ITEM_STEAL(PyTupleWriter *writer,
                              Py_ssize_t index, PyObject *item)
{
    assert(_PyTupleWriter_CheckConsistency(writer));
    PyTupleObject *tuple = _PyTuple_CAST(writer);

    assert(_PyTuple_CanMutate(tuple));
    assert(0 <= index && index < Py_SIZE(tuple));
    assert(tuple->ob_item[index] == NULL);
    assert(item != NULL);

    tuple->ob_item[index] = item;
}


static inline void
_PyTupleWriter_SET_ITEM(PyTupleWriter *writer,
                        Py_ssize_t index, PyObject *item)
{
    assert(item != NULL);
    return _PyTupleWriter_SET_ITEM_STEAL(writer, index, Py_NewRef(item));
}

#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
