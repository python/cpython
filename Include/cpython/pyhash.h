#ifndef Py_CPYTHON_HASH_H
#  error "this header file must not be included directly"
#endif

/* hash function definition */
typedef struct {
    Py_hash_t (*const hash)(const void *, Py_ssize_t);
    const char *name;
    const int hash_bits;
    const int seed_bits;
} PyHash_FuncDef;

PyAPI_FUNC(PyHash_FuncDef*) PyHash_GetFuncDef(void);
