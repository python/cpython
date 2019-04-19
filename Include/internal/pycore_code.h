#ifndef Py_INTERNAL_CODE_H
#define Py_INTERNAL_CODE_H
#ifdef __cplusplus
extern "C" {
#endif
 
typedef struct {
    PyObject *ptr;
    PY_UINT64_T globals_ver;
    PY_UINT64_T builtins_ver;
} _PyOpCodeOpt_LoadGlobal;

typedef struct _PyOpCodeOpt {
    union {
        _PyOpCodeOpt_LoadGlobal lg;
    } u;
    char optimized;
} _PyOpCodeOpt;

/* Private API */
int _PyCode_InitOptCache(PyCodeObject *co);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
