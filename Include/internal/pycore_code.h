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

struct _PyOpcache {
    union {
        _PyOpcache_LoadGlobal lg;
        _PyOpCodeOpt_LoadAttr la;
    } u;
    char optimized;
};


// We would use an enum if C let us specify the storage type.
typedef unsigned char _PyFastLocalKind;
// Note that these all fit within _PyFastLocalKind, as do combinations.
#define CO_FAST_POSONLY     0x01
#define CO_FAST_POSORKW     0x02
#define CO_FAST_VARARGS     0x04
#define CO_FAST_KWONLY      0x08
#define CO_FAST_VARKWARGS   0x10
#define CO_FAST_LOCALONLY   0x20
#define CO_FAST_CELL        0x40
#define CO_FAST_FREE        0x80

#define CO_FAST_ARG (CO_FAST_POSONLY | CO_FAST_POSORKW | CO_FAST_VARARGS | \
                     CO_FAST_KWONLY | CO_FAST_VARKWARGS)
#define CO_FAST_LOCAL (CO_FAST_ARG | CO_FAST_LOCALONLY)
#define CO_FAST_ANY (CO_FAST_LOCAL | CO_FAST_CELL | CO_FAST_FREE)

typedef _PyFastLocalKind *_PyFastLocalKinds;

static inline int
_PyCode_InitFastLocalKinds(int num, _PyFastLocalKinds *pkinds)
{
    if (num == 0) {
        *pkinds = NULL;
        return 0;
    }
    _PyFastLocalKinds kinds = PyMem_NEW(_PyFastLocalKind, num);
    if (kinds == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    *pkinds = kinds;
    return 0;
}

static inline void
_PyCode_ClearFastLocalKinds(_PyFastLocalKinds kinds)
{
    if (kinds != NULL) {
        PyMem_Free(kinds);
    }
}

struct _PyCodeConstructor {
    /* metadata */
    PyObject *filename;
    PyObject *name;
    int flags;

    /* the code */
    PyObject *code;
    int firstlineno;
    PyObject *linetable;

    /* used by the code */
    PyObject *consts;
    PyObject *names;

    /* mapping frame offsets to information */
    PyObject *fastlocalnames;
    _PyFastLocalKinds fastlocalkinds;

    /* args (within varnames) */
    int argcount;
    int posonlyargcount;
    int kwonlyargcount;

    /* needed to create the frame */
    int stacksize;

    /* used by the eval loop */
    PyObject *exceptiontable;
};

// Using an "arguments struct" like this is helpful for maintainability
// in a case such as this with many parameters.  It does bear a risk:
// if the struct changes and callers are not updated properly then the
// compiler will not catch problems (like a missing argument).  This can
// cause hard-to-debug problems.  The risk is mitigated by the use of
// check_code() in codeobject.c.  However, we may decide to switch
// back to a regular function signature.  Regardless, this approach
// wouldn't be appropriate if this weren't a strictly internal API.
// (See the comments in https://github.com/python/cpython/pull/26258.)
PyAPI_FUNC(int) _PyCode_Validate(struct _PyCodeConstructor *);
PyAPI_FUNC(PyCodeObject *) _PyCode_New(struct _PyCodeConstructor *);


/* Private API */

int _PyCode_InitOpcache(PyCodeObject *co);

/* Getters for internal PyCodeObject data. */
PyAPI_FUNC(PyObject *) _PyCode_GetVarnames(PyCodeObject *);
PyAPI_FUNC(PyObject *) _PyCode_GetCellvars(PyCodeObject *);
PyAPI_FUNC(PyObject *) _PyCode_GetFreevars(PyCodeObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
