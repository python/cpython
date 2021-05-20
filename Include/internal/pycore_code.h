#ifndef Py_INTERNAL_CODE_H
#define Py_INTERNAL_CODE_H
#ifdef __cplusplus
extern "C" {
#endif
 
#include <stdbool.h>


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


// XXX Can we do this with an enum instead?
typedef unsigned char _PyFastLocalKind;
#define CO_FAST_POSONLY     0x01
#define CO_FAST_POSORKW     0x02
#define CO_FAST_VARARGS     0x04
#define CO_FAST_KWONLY      0x09
#define CO_FAST_VARKWARGS   0x10
#define CO_FAST_LOCALONLY   0x20
#define CO_FAST_CELL        0x40
#define CO_FAST_FREE        0x80
#define CO_FAST_LOCAL (CO_FAST_POSONLY | CO_FAST_POSORKW | CO_FAST_VARARGS | \
                       CO_FAST_KWONLY | CO_FAST_VARKWARGS |                  \
                       CO_FAST_LOCALONLY)
#define CO_FAST_ANY (CO_FAST_LOCAL | CO_FAST_CELL | CO_FAST_FREE)


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
    PyObject *varnames;
    PyObject *cellvars;
    PyObject *freevars;

    /* args (within varnames) */
    int argcount;
    int posonlyargcount;
    int kwonlyargcount;

    /* needed to create the frame */
    int stacksize;

        /* used by the eval loop */
    PyObject *exceptiontable;
};

PyAPI_FUNC(PyCodeObject *) _PyCode_New(struct _PyCodeConstructor *);


/* Private API */

int _PyCode_InitOpcache(PyCodeObject *co);

PyAPI_FUNC(bool) _PyCode_HasFastlocals(PyCodeObject *, _PyFastLocalKind);
PyAPI_FUNC(Py_ssize_t) _PyCode_CellForLocal(PyCodeObject *, Py_ssize_t);

/* This does not fail.  A genative result means "no match". */
PyAPI_FUNC(Py_ssize_t)  _PyCode_GetFastlocalOffsetId(PyCodeObject *,
                                                     _Py_Identifier *,
                                                     _PyFastLocalKind);

// This is a speed hack for use in ceval.c.
#define _PyCode_LOCALVARS_ARRAY(co) \
    (((PyTupleObject *)((co)->co_varnames))->ob_item)
#define _PyCode_GET_LOCALVAR(co, offset) \
    (PyTuple_GetItem((co)->co_varnames, offset))

#define _PyCode_GET_CELLVAR(co, offset) \
    (PyTuple_GetItem((co)->co_cellvars, offset))

#define _PyCode_GET_FREEVAR(co, offset) \
    (PyTuple_GetItem((co)->co_freevars, offset))

#define _PyCode_CODE_IS_VALID(co)                                           \
    (PyBytes_Check((co)->co_code) &&                                          \
     PyBytes_GET_SIZE((co)->co_code) <= INT_MAX &&                            \
     PyBytes_GET_SIZE((co)->co_code) % sizeof(_Py_CODEUNIT) == 0 &&           \
     _Py_IS_ALIGNED(PyBytes_AS_STRING((co)->co_code), sizeof(_Py_CODEUNIT)))
#define _PyCode_GET_INSTRUCTIONS(co) \
    ((_Py_CODEUNIT *) PyBytes_AS_STRING((co)->co_code))
#define _PyCode_NUM_INSTRUCTIONS(co) \
    (PyBytes_Size((co)->co_code) / sizeof(_Py_CODEUNIT))


#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODE_H */
