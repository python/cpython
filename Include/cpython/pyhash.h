#ifndef Py_CPYTHON_HASH_H
#  error "this header file must not be included directly"
#endif

/* Prime multiplier used in string and various other hashes. */
#define _PyHASH_MULTIPLIER 1000003UL  /* 0xf4243 */

/* Parameters used for the numeric hash implementation.  See notes for
   _Py_HashDouble in Python/pyhash.c.  Numeric hashes are based on
   reduction modulo the prime 2**_PyHASH_BITS - 1. */

#if SIZEOF_VOID_P >= 8
#  define _PyHASH_BITS 61
#else
#  define _PyHASH_BITS 31
#endif

#define _PyHASH_MODULUS (((size_t)1 << _PyHASH_BITS) - 1)
#define _PyHASH_INF 314159
#define _PyHASH_IMAG _PyHASH_MULTIPLIER

/* Helpers for hash functions */
PyAPI_FUNC(Py_hash_t) _Py_HashDouble(PyObject *, double);

// Kept for backward compatibility
#define _Py_HashPointer Py_HashPointer


/* hash function definition */
typedef struct {
    Py_hash_t (*const hash)(const void *, Py_ssize_t);
    const char *name;
    const int hash_bits;
    const int seed_bits;
} PyHash_FuncDef;

PyAPI_FUNC(PyHash_FuncDef*) PyHash_GetFuncDef(void);

PyAPI_FUNC(Py_hash_t) Py_HashPointer(const void *ptr);
