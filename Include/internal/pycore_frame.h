#ifndef Py_INTERNAL_FRAMEDATA_H
#define Py_INTERNAL_FRAMEDATA_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Starting in CPython 3.11, CPython separates the frame state between the
 * full frame objects exposed by the Python and C runtime state introspection
 * APIs, and internal lighter weight frame data structs, which are simple C
 * structures owned by either the interpreter eval loop (while executing
 * ordinary functions), by a generator or coroutine object (for frames that
 * are able to be suspended), or by their corresponding full frame object (if
 * a state instrospection API has been invoked and the full frame object has
 * taken responsibility for the lifecycle of the frame data storage).
 *
 * This split storage eliminates a lot of allocation and deallocation of full
 * Python objects during code execution, providing a significant speed gain
 * over the previous approach of using full Python objects for both
 * introspection and code execution.
 *
 * Field naming conventions:
 *
 *   * full frame object fields have an "f_*" prefix
 *   * frame data struct fields have no prefix
 *
 * Variable naming conventions:
 *
 *   * "frame" and "f" are used for full frame objects
 *   * "fdata" is used for frame data structs
 *
 * Function/macro naming conventions:
 *
 *   * "PyFrame_*"" and "_PyFrame_*" functions accept a full frame object
 *   * "_Py_framedata_*" functions accept a frame data struct
 */


struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _Py_framedata *f_fdata; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_owns_frame;          /* This frame owns the frame */
    /* The frame data, if this frame object owns the frame */
    struct _Py_framedata *_f_owned_fdata[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);

/* runtime lifecycle */

extern void _PyFrame_Fini(PyInterpreterState *interp);


/* other API */

/* These values are chosen so that the inline functions below all
 * compare fdata->state to zero while keeping the property that most
 * state transitions move to a higher value frame state.
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

/*
    frame->lasti refers to the index of the last instruction,
    unless it's -1 in which case next_instr should be first_instr.
*/

typedef struct _Py_framedata {
    PyFunctionObject *func; /* Strong reference */
    PyObject *globals; /* Borrowed reference */
    PyObject *builtins; /* Borrowed reference */
    PyObject *locals; /* Strong reference, may be NULL */
    PyCodeObject *code; /* Strong reference */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL */
    struct _Py_framedata *previous;
    int lasti;       /* Last instruction if called */
    int stacktop;     /* Offset of TOS from localsplus  */
    PyFrameState state;  /* What state the frame is in */
    bool is_entry;  // Whether this is the "root" frame for the current _PyCFrame.
    bool is_generator;
    PyObject *localsplus[1];
} _Py_framedata;

static inline int _PyFrame_IsRunnable(_Py_framedata *f) {
    return f->state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(_Py_framedata *f) {
    return f->state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(_Py_framedata *f) {
    return f->state > FRAME_EXECUTING;
}

static inline PyObject **_PyFrame_Stackbase(_Py_framedata *f) {
    return f->localsplus + f->code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_Py_framedata *f) {
    assert(f->stacktop > f->code->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(_Py_framedata *f) {
    assert(f->stacktop > f->code->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(_Py_framedata *fdata, PyObject *value) {
    fdata->localsplus[fdata->stacktop] = value;
    fdata->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_Py_framedata)-1)/sizeof(PyObject *))

void _PyFrame_Copy(_Py_framedata *src, _Py_framedata *dest);

/* Consumes reference to func */
static inline void
_PyFrame_InitializeSpecials(
    _Py_framedata *fdata, PyFunctionObject *func,
    PyObject *locals, int nlocalsplus)
{
    fdata->func = func;
    fdata->code = (PyCodeObject *)Py_NewRef(func->func_code);
    fdata->builtins = func->func_builtins;
    fdata->globals = func->func_globals;
    fdata->locals = Py_XNewRef(locals);
    fdata->stacktop = nlocalsplus;
    fdata->frame_obj = NULL;
    fdata->lasti = -1;
    fdata->state = FRAME_CREATED;
    fdata->is_entry = false;
    fdata->is_generator = false;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(_Py_framedata *fdata)
{
    return fdata->localsplus;
}

static inline PyObject**
_PyFrame_GetStackPointer(_Py_framedata *fdata)
{
    return fdata->localsplus+fdata->stacktop;
}

static inline void
_PyFrame_SetStackPointer(_Py_framedata *fdata, PyObject **stack_pointer)
{
    fdata->stacktop = (int)(stack_pointer - fdata->localsplus);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_Py_framedata *fdata);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_Py_framedata *fdata)
{
    PyFrameObject *res = fdata->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(fdata);
}

/* Clears all references in the frame.
 * If take is non-zero, then the frame data
 * may be transfered to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_Clear(_Py_framedata * fdata);

int
_PyFrame_Traverse(_Py_framedata *fdata, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(_Py_framedata *fdata);

void
_PyFrame_LocalsToFast(_Py_framedata *fdata, int clear);

extern _Py_framedata *
_PyThreadState_BumpFramePointerSlow(PyThreadState *tstate, size_t size);

static inline _Py_framedata *
_PyThreadState_BumpFramePointer(PyThreadState *tstate, size_t size)
{
    PyObject **base = tstate->datastack_top;
    if (base) {
        PyObject **top = base + size;
        assert(tstate->datastack_limit);
        if (top < tstate->datastack_limit) {
            tstate->datastack_top = top;
            return (_Py_framedata *)base;
        }
    }
    return _PyThreadState_BumpFramePointerSlow(tstate, size);
}

void _PyThreadState_PopFrame(PyThreadState *tstate, _Py_framedata *fdata);

/* Consume reference to func */
_Py_framedata *
_PyFrame_Push(PyThreadState *tstate, PyFunctionObject *func);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAMEDATA_H */
