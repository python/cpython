#ifndef Py_INTERNAL_CODE_H
#define Py_INTERNAL_CODE_H
#ifdef __cplusplus
extern "C" {
#endif
 
typedef struct {
    PyObject *ptr;  /* Cached pointer (borrowed reference) */
    uint64_t globals_ver;  /* ma_version of global dict */
    uint64_t builtins_ver; /* ma_version of builtin dict */
} _PyOpcache_LoadGlobal;

typedef struct {
    PyTypeObject *type;
    Py_ssize_t hint;
    unsigned int tp_version_tag;
} _PyOpCodeOpt_LoadAttr;

typedef struct {
    Py_hash_t hash;
    PyTypeObject *type;
    PyObject *meth;
    unsigned int tp_version_tag;
} _PyOpCodeOpt_LoadMethod;

struct _PyOpcache {
    union {
        _PyOpcache_LoadGlobal lg;
        _PyOpCodeOpt_LoadMethod lm;
        _PyOpCodeOpt_LoadAttr la;
    } u;
    char optimized;
};

/* Private API */
int _PyCode_InitOpcache(PyCodeObject *co);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
