/* Definitions for bytecode */

#ifndef Py_LIMITED_API
#ifndef Py_CODE_H
#define Py_CODE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyObject *_co_code;
    PyObject *_co_varnames;
    PyObject *_co_cellvars;
    PyObject *_co_freevars;
} _PyCoCached;

typedef struct {
    int size;
    int capacity;
    struct _PyExecutorObject *executors[1];
} _PyExecutorArray;


#ifdef Py_GIL_DISABLED

/* Each thread specializes a thread-local copy of the bytecode in free-threaded
 * builds. These copies are stored on the code object in a `_PyCodeArray`. The
 * first entry in the array always points to the "main" copy of the bytecode
 * that is stored at the end of the code object.
 */
typedef struct {
    Py_ssize_t size;
    char *entries[1];
} _PyCodeArray;

#define _PyCode_DEF_THREAD_LOCAL_BYTECODE() \
    _PyCodeArray *co_tlbc;
#else
#define _PyCode_DEF_THREAD_LOCAL_BYTECODE()
#endif

// To avoid repeating ourselves in deepfreeze.py, all PyCodeObject members are
// defined in this macro:
#define _PyCode_DEF(SIZE) {                                                    \
    PyObject_VAR_HEAD                                                          \
                                                                               \
    /* Note only the following fields are used in hash and/or comparisons      \
     *                                                                         \
     * - co_name                                                               \
     * - co_argcount                                                           \
     * - co_posonlyargcount                                                    \
     * - co_kwonlyargcount                                                     \
     * - co_nlocals                                                            \
     * - co_stacksize                                                          \
     * - co_flags                                                              \
     * - co_firstlineno                                                        \
     * - co_consts                                                             \
     * - co_names                                                              \
     * - co_localsplusnames                                                    \
     * This is done to preserve the name and line number for tracebacks        \
     * and debuggers; otherwise, constant de-duplication would collapse        \
     * identical functions/lambdas defined on different lines.                 \
     */                                                                        \
                                                                               \
    /* These fields are set with provided values on new code objects. */       \
                                                                               \
    /* The hottest fields (in the eval loop) are grouped here at the top. */   \
    PyObject *co_consts;           /* list (constants used) */                 \
    PyObject *co_names;            /* list of strings (names used) */          \
    PyObject *co_exceptiontable;   /* Byte string encoding exception handling  \
                                      table */                                 \
    int co_flags;                  /* CO_..., see below */                     \
                                                                               \
    /* The rest are not so impactful on performance. */                        \
    int co_argcount;              /* #arguments, except *args */               \
    int co_posonlyargcount;       /* #positional only arguments */             \
    int co_kwonlyargcount;        /* #keyword only arguments */                \
    int co_stacksize;             /* #entries needed for evaluation stack */   \
    int co_firstlineno;           /* first source line number */               \
                                                                               \
    /* redundant values (derived from co_localsplusnames and                   \
       co_localspluskinds) */                                                  \
    int co_nlocalsplus;           /* number of spaces for holding local, cell, \
                                     and free variables */                     \
    int co_framesize;             /* Size of frame in words */                 \
    int co_nlocals;               /* number of local variables */              \
    int co_ncellvars;             /* total number of cell variables */         \
    int co_nfreevars;             /* number of free variables */               \
    uint32_t co_version;          /* version number */                         \
                                                                               \
    PyObject *co_localsplusnames; /* tuple mapping offsets to names */         \
    PyObject *co_localspluskinds; /* Bytes mapping to local kinds (one byte    \
                                     per variable) */                          \
    PyObject *co_filename;        /* unicode (where it was loaded from) */     \
    PyObject *co_name;            /* unicode (name, for reference) */          \
    PyObject *co_qualname;        /* unicode (qualname, for reference) */      \
    PyObject *co_linetable;       /* bytes object that holds location info */  \
    PyObject *co_weakreflist;     /* to support weakrefs to code objects */    \
    _PyExecutorArray *co_executors;      /* executors from optimizer */        \
    _PyCoCached *_co_cached;      /* cached co_* attributes */                 \
    uintptr_t _co_instrumentation_version; /* current instrumentation version */ \
    struct _PyCoMonitoringData *_co_monitoring; /* Monitoring data */          \
    Py_ssize_t _co_unique_id;     /* ID used for per-thread refcounting */   \
    int _co_firsttraceable;       /* index of first traceable instruction */   \
    /* Scratch space for extra data relating to the code object.               \
       Type is a void* to keep the format private in codeobject.c to force     \
       people to go through the proper APIs. */                                \
    void *co_extra;                                                            \
    _PyCode_DEF_THREAD_LOCAL_BYTECODE()                                        \
    char co_code_adaptive[(SIZE)];                                             \
}

/* Bytecode object */
struct PyCodeObject _PyCode_DEF(1);

/* Masks for co_flags above */
#define CO_OPTIMIZED    0x0001
#define CO_NEWLOCALS    0x0002
#define CO_VARARGS      0x0004
#define CO_VARKEYWORDS  0x0008
#define CO_NESTED       0x0010
#define CO_GENERATOR    0x0020

/* The CO_COROUTINE flag is set for coroutine functions (defined with
   ``async def`` keywords) */
#define CO_COROUTINE            0x0080
#define CO_ITERABLE_COROUTINE   0x0100
#define CO_ASYNC_GENERATOR      0x0200

/* bpo-39562: These constant values are changed in Python 3.9
   to prevent collision with compiler flags. CO_FUTURE_ and PyCF_
   constants must be kept unique. PyCF_ constants can use bits from
   0x0100 to 0x10000. CO_FUTURE_ constants use bits starting at 0x20000. */
#define CO_FUTURE_DIVISION      0x20000
#define CO_FUTURE_ABSOLUTE_IMPORT 0x40000 /* do absolute imports by default */
#define CO_FUTURE_WITH_STATEMENT  0x80000
#define CO_FUTURE_PRINT_FUNCTION  0x100000
#define CO_FUTURE_UNICODE_LITERALS 0x200000

#define CO_FUTURE_BARRY_AS_BDFL  0x400000
#define CO_FUTURE_GENERATOR_STOP  0x800000
#define CO_FUTURE_ANNOTATIONS    0x1000000

#define CO_NO_MONITORING_EVENTS 0x2000000

/* Whether the code object has a docstring,
   If so, it will be the first item in co_consts
*/
#define CO_HAS_DOCSTRING 0x4000000

/* A function defined in class scope */
#define CO_METHOD  0x8000000

/* This should be defined if a future statement modifies the syntax.
   For example, when a keyword is added.
*/
#define PY_PARSER_REQUIRES_FUTURE_KEYWORD

#define CO_MAXBLOCKS 21 /* Max static block nesting within a function */

PyAPI_DATA(PyTypeObject) PyCode_Type;

#define PyCode_Check(op) Py_IS_TYPE((op), &PyCode_Type)

static inline Py_ssize_t PyCode_GetNumFree(PyCodeObject *op) {
    assert(PyCode_Check(op));
    return op->co_nfreevars;
}

static inline int PyUnstable_Code_GetFirstFree(PyCodeObject *op) {
    assert(PyCode_Check(op));
    return op->co_nlocalsplus - op->co_nfreevars;
}

Py_DEPRECATED(3.13) static inline int PyCode_GetFirstFree(PyCodeObject *op) {
    return PyUnstable_Code_GetFirstFree(op);
}

/* Unstable public interface */
PyAPI_FUNC(PyCodeObject *) PyUnstable_Code_New(
        int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, int, PyObject *,
        PyObject *);

PyAPI_FUNC(PyCodeObject *) PyUnstable_Code_NewWithPosOnlyArgs(
        int, int, int, int, int, int, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, PyObject *,
        PyObject *, PyObject *, PyObject *, int, PyObject *,
        PyObject *);
        /* same as struct above */
// Old names -- remove when this API changes:
_Py_DEPRECATED_EXTERNALLY(3.12) static inline PyCodeObject *
PyCode_New(
        int a, int b, int c, int d, int e, PyObject *f, PyObject *g,
        PyObject *h, PyObject *i, PyObject *j, PyObject *k,
        PyObject *l, PyObject *m, PyObject *n, int o, PyObject *p,
        PyObject *q)
{
    return PyUnstable_Code_New(
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q);
}
_Py_DEPRECATED_EXTERNALLY(3.12) static inline PyCodeObject *
PyCode_NewWithPosOnlyArgs(
        int a, int poac, int b, int c, int d, int e, PyObject *f, PyObject *g,
        PyObject *h, PyObject *i, PyObject *j, PyObject *k,
        PyObject *l, PyObject *m, PyObject *n, int o, PyObject *p,
        PyObject *q)
{
    return PyUnstable_Code_NewWithPosOnlyArgs(
        a, poac, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q);
}

/* Creates a new empty code object with the specified source location. */
PyAPI_FUNC(PyCodeObject *)
PyCode_NewEmpty(const char *filename, const char *funcname, int firstlineno);

/* Return the line number associated with the specified bytecode index
   in this code object.  If you just need the line number of a frame,
   use PyFrame_GetLineNumber() instead. */
PyAPI_FUNC(int) PyCode_Addr2Line(PyCodeObject *, int);

PyAPI_FUNC(int) PyCode_Addr2Location(PyCodeObject *, int, int *, int *, int *, int *);

#define PY_FOREACH_CODE_EVENT(V) \
    V(CREATE)                 \
    V(DESTROY)

typedef enum {
    #define PY_DEF_EVENT(op) PY_CODE_EVENT_##op,
    PY_FOREACH_CODE_EVENT(PY_DEF_EVENT)
    #undef PY_DEF_EVENT
} PyCodeEvent;


/*
 * A callback that is invoked for different events in a code object's lifecycle.
 *
 * The callback is invoked with a borrowed reference to co, after it is
 * created and before it is destroyed.
 *
 * If the callback sets an exception, it must return -1. Otherwise
 * it should return 0.
 */
typedef int (*PyCode_WatchCallback)(
  PyCodeEvent event,
  PyCodeObject* co);

/*
 * Register a per-interpreter callback that will be invoked for code object
 * lifecycle events.
 *
 * Returns a handle that may be passed to PyCode_ClearWatcher on success,
 * or -1 and sets an error if no more handles are available.
 */
PyAPI_FUNC(int) PyCode_AddWatcher(PyCode_WatchCallback callback);

/*
 * Clear the watcher associated with the watcher_id handle.
 *
 * Returns 0 on success or -1 if no watcher exists for the provided id.
 */
PyAPI_FUNC(int) PyCode_ClearWatcher(int watcher_id);

/* for internal use only */
struct _opaque {
    int computed_line;
    const uint8_t *lo_next;
    const uint8_t *limit;
};

typedef struct _line_offsets {
    int ar_start;
    int ar_end;
    int ar_line;
    struct _opaque opaque;
} PyCodeAddressRange;

/* Update *bounds to describe the first and one-past-the-last instructions in the
   same line as lasti.  Return the number of that line.
*/
PyAPI_FUNC(int) _PyCode_CheckLineNumber(int lasti, PyCodeAddressRange *bounds);

PyAPI_FUNC(PyObject*) PyCode_Optimize(PyObject *code, PyObject* consts,
                                      PyObject *names, PyObject *lnotab);

PyAPI_FUNC(int) PyUnstable_Code_GetExtra(
    PyObject *code, Py_ssize_t index, void **extra);
PyAPI_FUNC(int) PyUnstable_Code_SetExtra(
    PyObject *code, Py_ssize_t index, void *extra);
// Old names -- remove when this API changes:
_Py_DEPRECATED_EXTERNALLY(3.12) static inline int
_PyCode_GetExtra(PyObject *code, Py_ssize_t index, void **extra)
{
    return PyUnstable_Code_GetExtra(code, index, extra);
}
_Py_DEPRECATED_EXTERNALLY(3.12) static inline int
_PyCode_SetExtra(PyObject *code, Py_ssize_t index, void *extra)
{
    return PyUnstable_Code_SetExtra(code, index, extra);
}

/* Equivalent to getattr(code, 'co_code') in Python.
   Returns a strong reference to a bytes object. */
PyAPI_FUNC(PyObject *) PyCode_GetCode(PyCodeObject *code);
/* Equivalent to getattr(code, 'co_varnames') in Python. */
PyAPI_FUNC(PyObject *) PyCode_GetVarnames(PyCodeObject *code);
/* Equivalent to getattr(code, 'co_cellvars') in Python. */
PyAPI_FUNC(PyObject *) PyCode_GetCellvars(PyCodeObject *code);
/* Equivalent to getattr(code, 'co_freevars') in Python. */
PyAPI_FUNC(PyObject *) PyCode_GetFreevars(PyCodeObject *code);

typedef enum _PyCodeLocationInfoKind {
    /* short forms are 0 to 9 */
    PY_CODE_LOCATION_INFO_SHORT0 = 0,
    /* one lineforms are 10 to 12 */
    PY_CODE_LOCATION_INFO_ONE_LINE0 = 10,
    PY_CODE_LOCATION_INFO_ONE_LINE1 = 11,
    PY_CODE_LOCATION_INFO_ONE_LINE2 = 12,

    PY_CODE_LOCATION_INFO_NO_COLUMNS = 13,
    PY_CODE_LOCATION_INFO_LONG = 14,
    PY_CODE_LOCATION_INFO_NONE = 15
} _PyCodeLocationInfoKind;

#ifdef __cplusplus
}
#endif
#endif  // !Py_CODE_H
#endif  // !Py_LIMITED_API
