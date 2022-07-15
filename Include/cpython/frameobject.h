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

typedef enum {
    TRY_BLOCK_READY,        /* when a try block is just pushed */
    TRY_BLOCK_RAISED,       /* when an exception has raised */
    TRY_BLOCK_PASSED,       /* when calling the finally block without exception */
} PyTryBlockType;

typedef struct {
    PyTryBlockType b_type;      /* what kind of block this is */
    int b_handler;              /* where to jump to find handler */
    int b_sextuple;             /* where to save exception with its cause */
} PyTryBlock;

struct _frame {
    PyObject_VAR_HEAD
    struct _frame *f_back;      /* previous frame, or NULL */
    PyCodeObject *f_code;       /* code segment */
    PyObject *f_builtins;       /* builtin symbol table (PyDictObject) */
    PyObject *f_globals;        /* global symbol table (PyDictObject) */
    PyObject *f_locals;         /* local symbol table (any mapping) */
    PyObject *f_trace;          /* Trace function */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */

    /* Borrowed reference to a generator, or NULL */
    PyObject *f_gen;

    _Py_CODEUNIT *f_last_instr; /* Last instruction if called */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    int f_iblock;               /* index in f_blockstack */
    PyFrameState f_state;       /* What state the frame is in */
    PyTryBlock f_blockstack[CO_MAXBLOCKS];   /* for try blocks */
    PyObject *f_localsplus[1];  /* locals+tmps+consts+cells+frees, dynamically sized */
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
_PyFrame_New_NoTrack(PyThreadState *, PyFrameConstructor *, PyObject *);


static inline PyObject **
frame_locals(PyFrameObject *f) {
    return f->f_localsplus;
}

static inline Py_ssize_t
frame_local_num(PyCodeObject *code) {
    return code->co_nlocals;
}

static inline PyObject **
frame_tmps(PyFrameObject *f) {
    return frame_locals(f) + frame_local_num(f->f_code);
}

static inline Py_ssize_t
frame_tmp_num(PyCodeObject *code) {
    return code->co_ntmps;
}

static inline PyObject **
frame_consts(PyFrameObject *f) {
    return frame_tmps(f) + frame_tmp_num(f->f_code);
}

static inline Py_ssize_t
frame_const_num(PyCodeObject *code) {
    return PyTuple_GET_SIZE(code->co_consts);
}

static inline PyObject **
frame_cells_and_frees(PyFrameObject *f) {
    return frame_consts(f) + frame_const_num(f->f_code);
}

static inline Py_ssize_t
frame_cell_num(PyCodeObject *code) {
    return PyTuple_GET_SIZE(code->co_cellvars);
}

static inline Py_ssize_t
frame_free_num(PyCodeObject *code) {
    return PyTuple_GET_SIZE(code->co_freevars);
}

static inline _Py_CODEUNIT *
frame_first_instr(PyFrameObject *f) {
    return (_Py_CODEUNIT *) PyBytes_AS_STRING(f->f_code->co_code);
}

static inline int
frame_instr_offset(PyFrameObject *f) {
    return (int) ((char *) f->f_last_instr - (char *) frame_first_instr(f));
}

static inline int
frame_has_executed(PyFrameObject *f) {
    return f->f_last_instr >= frame_first_instr(f);
}

/* The rest of the interface is specific for frame objects */

/* Block management functions */
static inline void
PyFrame_BlockPush(PyFrameObject *f, PyTryBlockType type,
                  int handler, _Py_OPARG sextuple) {
    PyTryBlock *b = &f->f_blockstack[f->f_iblock++];
    b->b_type = type;
    b->b_handler = handler;
    b->b_sextuple = Py_SAFE_DOWNCAST(sextuple, _Py_OPARG, int);
}

static inline PyTryBlock *
PyFrame_BlockTop(PyFrameObject *f) {
    assert(f->f_iblock > 0);
    return &f->f_blockstack[f->f_iblock - 1];
}

static inline PyTryBlock *
PyFrame_BlockPop(PyFrameObject *f) {
    assert(f->f_iblock > 0);
    return &f->f_blockstack[--f->f_iblock];
}

/* Conversions between "fast locals" and locals in dictionary */
PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject *, int);

PyAPI_FUNC(int) PyFrame_FastToLocalsWithError(PyFrameObject *f);

PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject *);

PyAPI_FUNC(void) _PyFrame_DebugMallocStats(FILE *out);

PyAPI_FUNC(PyFrameObject *)PyFrame_GetBack(PyFrameObject *frame);

extern PyObject _Py_UndefinedStruct;
#define Py_Undefined (&_Py_UndefinedStruct)
