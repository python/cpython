#ifndef Py_INTERNAL_SETOBJECT_H
#define Py_INTERNAL_SETOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_abc' shared extension
PyAPI_FUNC(int) _PySet_NextEntry(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_NextEntryRef(
    PyObject *set,
    Py_ssize_t *pos,
    PyObject **key,
    Py_hash_t *hash);

// Export for '_pickle' shared extension
PyAPI_FUNC(int) _PySet_Update(PyObject *set, PyObject *iterable);

// Export for the gdb plugin's (python-gdb.py) benefit
PyAPI_DATA(PyObject *) _PySet_Dummy;

PyAPI_FUNC(int) _PySet_Contains(PySetObject *so, PyObject *key);

// Clears the set without acquiring locks. Used by _PyCode_Fini.
extern void _PySet_ClearInternal(PySetObject *so);

PyAPI_FUNC(int) _PySet_AddTakeRef(PySetObject *so, PyObject *key);

PyObject *
_PySet_Freeze(PyObject *set);

/* Bit mixer applied to each element hash by frozenset_hash() (and to each
   (key, value) pair hash by frozendict_hash()) before the results are
   combined with xor.

   Because xor does no mixing of its own, all of the avalanche has to come
   from this function: a change in *any* input bit must be able to affect
   *every* output bit.  In particular, high-order input bits must reach the
   low-order output bits, which are the ones a hash table looks at first
   (gh-150134).

   On 64-bit builds this is the SplitMix64 output function, including its
   additive "golden gamma" step, which also keeps 0 from being a fixed
   point.  On 32-bit builds it is the MurmurHash3 fmix32 finalizer. */
static inline Py_uhash_t
_PySet_ShuffleHashBits(Py_uhash_t h)
{
#if SIZEOF_PY_UHASH_T > 4
    h += (Py_uhash_t)0x9e3779b97f4a7c15ULL;
    h ^= h >> 30;
    h *= (Py_uhash_t)0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= (Py_uhash_t)0x94d049bb133111ebULL;
    h ^= h >> 31;
#else
    h += (Py_uhash_t)0x9e3779b9UL;
    h ^= h >> 16;
    h *= (Py_uhash_t)0x85ebca6bUL;
    h ^= h >> 13;
    h *= (Py_uhash_t)0xc2b2ae35UL;
    h ^= h >> 16;
#endif
    return h;
}

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_SETOBJECT_H
