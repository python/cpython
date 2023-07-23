#ifndef Py_INTERNAL_TUPLE_H
#define Py_INTERNAL_TUPLE_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern void _PyTuple_MaybeUntrack(PyObject *);
extern void _PyTuple_DebugMallocStats(FILE *out);
extern PyObject* _PyTuple_NewNoTrack(Py_ssize_t size);
extern int _PyTuple_ResizeNoTrack(PyObject **pv, Py_ssize_t newsize);

/* runtime lifecycle */

extern PyStatus _PyTuple_InitGlobalObjects(PyInterpreterState *);
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

#define _PyTuple_ITEMS(op) _Py_RVALUE(_PyTuple_CAST(op)->ob_item)

extern PyObject *_PyTuple_FromArray(PyObject *const *, Py_ssize_t);
extern PyObject *_PyTuple_FromArraySteal(PyObject *const *, Py_ssize_t);


typedef struct {
    PyObject_HEAD
    Py_ssize_t it_index;
    PyTupleObject *it_seq; /* Set to NULL when iterator is exhausted */
} _PyTupleIterObject;


// --- _PyTupleBuilder API ---------------------------------------------------

typedef struct _PyTupleBuilder {
    PyObject* small_tuple[16];
    PyObject *tuple;
    PyObject **items;
    size_t size;
    size_t allocated;
} _PyTupleBuilder;

static inline int
_PyTupleBuilder_Alloc(_PyTupleBuilder *builder, size_t size)
{
    if (size > (size_t)PY_SSIZE_T_MAX) {
        /* Check for overflow */
        PyErr_NoMemory();
        return -1;
    }
    if (size <= builder->allocated) {
        return 0;
    }

    if (size <= Py_ARRAY_LENGTH(builder->small_tuple)) {
        assert(builder->tuple == NULL);
        builder->items = builder->small_tuple;
        builder->allocated = Py_ARRAY_LENGTH(builder->small_tuple);
        return 0;
    }

    assert(size >= 1);
    if (builder->tuple != NULL) {
        if (_PyTuple_ResizeNoTrack(&builder->tuple, (Py_ssize_t)size) < 0) {
            return -1;
        }
    }
    else {
        builder->tuple = _PyTuple_NewNoTrack((Py_ssize_t)size);
        if (builder->tuple == NULL) {
            return -1;
        }

        if (builder->size > 0) {
            memcpy(_PyTuple_ITEMS(builder->tuple),
                   builder->small_tuple,
                   builder->size * sizeof(builder->small_tuple[0]));
        }
    }
    builder->items = _PyTuple_ITEMS(builder->tuple);
    builder->allocated = size;
    return 0;
}

static inline int
_PyTupleBuilder_Init(_PyTupleBuilder *builder, Py_ssize_t size)
{
    memset(builder, 0, sizeof(*builder));

    int res;
    if (size > 0) {
        res = _PyTupleBuilder_Alloc(builder, (size_t)size);
    }
    else {
        res = 0;
    }
    return res;
}

// The tuple builder must have already enough allocated items to store item.
static inline void
_PyTupleBuilder_AppendUnsafe(_PyTupleBuilder *builder, PyObject *item)
{
    assert(builder->items != NULL);
    assert(builder->size < builder->allocated);
    builder->items[builder->size] = item;
    builder->size++;
}

static inline int
_PyTupleBuilder_Append(_PyTupleBuilder *builder, PyObject *item)
{
    if (builder->size >= (size_t)PY_SSIZE_T_MAX) {
        // prevent integer overflow
        PyErr_NoMemory();
        return -1;
    }
    if (builder->size >= builder->allocated) {
        size_t allocated = builder->size;
        allocated += (allocated >> 2);  // Over-allocate by 25%
        if (_PyTupleBuilder_Alloc(builder, allocated) < 0) {
            return -1;
        }
    }
    _PyTupleBuilder_AppendUnsafe(builder, item);
    return 0;
}

static inline void
_PyTupleBuilder_Dealloc(_PyTupleBuilder *builder)
{
    Py_CLEAR(builder->tuple);
    builder->items = NULL;
    builder->size = 0;
    builder->allocated = 0;
}

static inline PyObject*
_PyTupleBuilder_Finish(_PyTupleBuilder *builder)
{
    if (builder->size == 0) {
        _PyTupleBuilder_Dealloc(builder);
        // return the empty tuple singleton
        return PyTuple_New(0);
    }

    if (builder->tuple != NULL) {
        if (_PyTuple_ResizeNoTrack(&builder->tuple, (Py_ssize_t)builder->size) < 0) {
            _PyTupleBuilder_Dealloc(builder);
            return NULL;
        }

        PyObject *result = builder->tuple;
        builder->tuple = NULL;
        // Avoid _PyObject_GC_TRACK() to avoid including pycore_object.h
        PyObject_GC_Track(result);
        return result;
    }
    else {
        PyObject *tuple = _PyTuple_FromArraySteal(builder->items,
                                                  (Py_ssize_t)builder->size);
        builder->size = 0;
        return tuple;
    }
}


#ifdef __cplusplus
}
#endif
#endif   /* !Py_INTERNAL_TUPLE_H */
