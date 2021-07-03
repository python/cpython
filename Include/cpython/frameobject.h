/* Frame object interface */

#ifndef Py_CPYTHON_FRAMEOBJECT_H
#  error "this header file must not be included directly"
#endif

/* These values are chosen so that the inline functions below all
 * compare f_state to zero.
 */
enum _framestate {
    FRAME_CREATED = -2,
    FRAME_SUSPENDED = -1,
    FRAME_EXECUTING = 0,
    FRAME_RETURNED = 1,
    FRAME_UNWINDING = 2,
    FRAME_RAISED = 3,
    FRAME_CLEARED = 4
};

typedef signed char PyFrameState;

struct _frame {
    PyObject_HEAD
    struct _frame *f_back;      /* previous frame, or NULL */
    PyObject **f_valuestack;    /* points after the last local */
    PyObject *f_trace;          /* Trace function */
    /* Borrowed reference to a generator, or NULL */
    PyObject *f_gen;
    int f_stackdepth;           /* Depth of value stack */
    int f_lasti;                /* Last instruction if called */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    PyFrameState f_state;       /* What state the frame is in */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_own_locals_memory;   /* This frame owns the memory for the locals */
    PyObject **f_localsptr;     /* Pointer to locals, cells, free */
};

static inline int _PyFrame_IsRunnable(struct _frame *f) {
    return f->f_state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(struct _frame *f) {
    return f->f_state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(struct _frame *f) {
    return f->f_state > FRAME_EXECUTING;
}

/* Standard object interface */

PyAPI_DATA(PyTypeObject) PyFrame_Type;

#define PyFrame_Check(op) Py_IS_TYPE(op, &PyFrame_Type)

PyAPI_FUNC(PyFrameObject *) PyFrame_New(PyThreadState *, PyCodeObject *,
                                        PyObject *, PyObject *);

/* only internal use */
PyFrameObject*
_PyFrame_New_NoTrack(PyThreadState *, PyFrameConstructor *, PyObject *, PyObject **);


/* The rest of the interface is specific for frame objects */

/* Legacy conversions between "fast locals" and locals in dictionary */
PyAPI_FUNC(int) PyFrame_FastToLocalsWithError(PyFrameObject *f);
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject *);

/* This always raises RuntimeError now (use the PyLocals_* API instead) */
PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject *, int);

/* Frame object memory management */
PyAPI_FUNC(int) PyFrame_ClearFreeList(void);
PyAPI_FUNC(void) _PyFrame_DebugMallocStats(FILE *out);

PyAPI_FUNC(PyFrameObject *) PyFrame_GetBack(PyFrameObject *frame);

/* Fast locals proxy allows for reliable write-through from trace functions */
// TODO: Perhaps this should be hidden, and API users told to query for
//       PyFrame_GetLocalsReturnsCopy() instead. Having this available
//       seems like a nice way to let folks write some useful debug assertions,
//       though.
PyAPI_DATA(PyTypeObject) _PyFastLocalsProxy_Type;
#define _PyFastLocalsProxy_CheckExact(self) \
    (Py_TYPE(self) == &_PyFastLocalsProxy_Type)


// Underlying implementation API supporting the stable PyLocals_*() APIs
// TODO: Add specific test cases for these (as any PyLocals_* tests won't cover
//       checking the status of a frame other than the currently active one)
PyAPI_FUNC(PyObject *) PyFrame_GetLocals(PyFrameObject *);

// TODO: Implement the rest of these, and add API tests
PyAPI_FUNC(PyObject *) PyFrame_GetLocalsCopy(PyFrameObject *);
PyAPI_FUNC(PyObject *) PyFrame_GetLocalsView(PyFrameObject *);
PyAPI_FUNC(int) PyFrame_GetLocalsReturnsCopy(PyFrameObject *);

// Underlying API supporting PyEval_GetLocals()
PyAPI_FUNC(PyObject *) _PyFrame_BorrowLocals(PyFrameObject *);

/* Force an update of any selectively updated views previously returned by
 * PyFrame_GetLocalsView(frame). Currently also needed in CPython when
 * accessing the f_locals attribute directly and it is not a plain dict
 * instance (otherwise it may report stale information).
 */
PyAPI_FUNC(int) PyFrame_RefreshLocalsView(PyFrameObject *);

#ifdef __cplusplus
}
#endif
