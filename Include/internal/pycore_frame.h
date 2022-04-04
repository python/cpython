#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

/* Starting in CPython 3.11, CPython separates the frame state between the
 * full frame objects exposed by the Python and C runtime state introspection
 * APIs, and internal lighter weight interpreter frames, which are simple C
 * structures owned by either the interpreter eval loop (while executing
 * ordinary functions), by a generator or coroutine object (for frames that
 * are able to be suspended), or by their corresponding full frame object (if
 * a state instrospection API has been invoked and the full frame object has
 * taken responsibility for the lifecycle of the interpreter frame).
 *
 * This split storage eliminates a lot of allocation and deallocation of full
 * Python objects during code execution, providing a significant speed gain
 * over the previous approach of using full Python objects for both
 * introspection and code execution.
 *
 * Struct names:
 *
 *   * PyFrameObject: the full Python frame object
 *   * _PyInterpreterFrame: the lightweight frame struct used by the eval loop
 *   * _PyCFrame: a struct that lives on the C stack and allows Python level
 *     recursive evaluation to be decoupled from recursive C level invocation
 *     of the bytecode eval loop
 *       * See pystate.h for more details on this struct
 *
 * Field naming conventions:
 *
 *   * full frame object fields have an "f_*" (or "_f_*") prefix
 *   * new interpreter frame fields have no prefix
 *   * Several interpreter frame fields have the "f_*" prefix as a result of
 *     trying to keep diffs as small as was feasible when splitting the original
 *     frame struct definition in two. The following are all interpreter frame
 *     fields, NOT full frame object fields:
 *     * f_func
 *     * f_globals
 *     * f_builtins
 *     * f_locals
 *     * f_code
 *     * f_lasti
 *     * f_state
 *   * Renaming those fields was considered but ultimately deemed too disruptive
 *     to key third party projects that were trying to keep up with the Python
 *     3.11 code evaluation changes during the alpha release cycle
 *     (see bpo-44800 for details)
 *
 * Naming conventions for local variables, function parameters and fields in other structs:
 *
 *   * "frame" and "f" may refer to either full frame objects or interpreter frames
 *     * the context of use or the field naming conventions usually make the
 *       type being referenced unambiguous in code reviews
 *   * the following alternative names are used when more clarity is needed:
 *     * full frame objects: "frame_obj" (and variants like "frameobj" or "fobj")
 *     * interpreter frame structs: "frame_data" or "iframe"
 *   * "current frame" should NOT be abbreviated as "cframe", as the latter now
 *     typically refers to _PyCFrame structs
 *
 * Function/macro parameter types:
 *
 *   * "PyFrame_*" functions and other public C API functions that relate to
 *     frames accept full frame object pointers
 *   * "_PyFrame_*" functions and other private C API functions that relate to
 *     frames accept either full frame object or interpreter frame pointers.
 *     Check the specific function signatures for details.
 *
 * Function return types:
 *
 *   * Public C API functions will only ever return full frame object pointers
 *   * Private C API functions with an underscore prefix may return interpreter
 *     frame pointers instead. Check the specific function signatures for details.
 */

struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _PyInterpreterFrame *f_frame; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_fast_as_locals;      /* Have the fast locals of this frame been converted to a dict? */
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);


/* other API */

typedef enum _framestate {
    FRAME_CREATED = -2,
    FRAME_SUSPENDED = -1,
    FRAME_EXECUTING = 0,
    FRAME_COMPLETED = 1,
    FRAME_CLEARED = 4
} PyFrameState;

enum _frameowner {
    FRAME_OWNED_BY_THREAD = 0,
    FRAME_OWNED_BY_GENERATOR = 1,
    FRAME_OWNED_BY_FRAME_OBJECT = 2
};

/*
    frame->f_lasti refers to the index of the last instruction,
    unless it's -1 in which case next_instr should be first_instr.
*/

typedef struct _PyInterpreterFrame {
    PyFunctionObject *f_func; /* Strong reference */
    PyObject *f_globals; /* Borrowed reference */
    PyObject *f_builtins; /* Borrowed reference */
    PyObject *f_locals; /* Strong reference, may be NULL */
    PyCodeObject *f_code; /* Strong reference */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL */
    struct _PyInterpreterFrame *previous;
    int f_lasti;       /* Last instruction if called */
    int stacktop;     /* Offset of TOS from localsplus  */
    bool is_entry;  // Whether this is the "root" frame for the current _PyCFrame.
    char owner;
    PyObject *localsplus[1];
} _PyInterpreterFrame;

static inline PyObject **_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return f->localsplus + f->f_code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, PyObject *value) {
    f->localsplus[f->stacktop] = value;
    f->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *))

void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest);

/* Consumes reference to func */
static inline void
_PyFrame_InitializeSpecials(
    _PyInterpreterFrame *frame, PyFunctionObject *func,
    PyObject *locals, int nlocalsplus)
{
    frame->f_func = func;
    frame->f_code = (PyCodeObject *)Py_NewRef(func->func_code);
    frame->f_builtins = func->func_builtins;
    frame->f_globals = func->func_globals;
    frame->f_locals = Py_XNewRef(locals);
    frame->stacktop = nlocalsplus;
    frame->frame_obj = NULL;
    frame->f_lasti = -1;
    frame->is_entry = false;
    frame->owner = FRAME_OWNED_BY_THREAD;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->localsplus;
}

static inline PyObject**
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    return frame->localsplus+frame->stacktop;
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - frame->localsplus);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyInterpreterFrame *frame)
{
    PyFrameObject *res =  frame->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame);
}

/* Clears all references in the frame.
 * If take is non-zero, then the _PyInterpreterFrame frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_Clear(_PyInterpreterFrame * frame);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(_PyInterpreterFrame *frame);

void
_PyFrame_LocalsToFast(_PyInterpreterFrame *frame, int clear);

extern _PyInterpreterFrame *
_PyThreadState_BumpFramePointerSlow(PyThreadState *tstate, size_t size);

static inline _PyInterpreterFrame *
_PyThreadState_BumpFramePointer(PyThreadState *tstate, size_t size)
{
    PyObject **base = tstate->datastack_top;
    if (base) {
        PyObject **top = base + size;
        assert(tstate->datastack_limit);
        if (top < tstate->datastack_limit) {
            tstate->datastack_top = top;
            return (_PyInterpreterFrame *)base;
        }
    }
    return _PyThreadState_BumpFramePointerSlow(tstate, size);
}

void _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Consume reference to func */
_PyInterpreterFrame *
_PyFrame_Push(PyThreadState *tstate, PyFunctionObject *func);


static inline
PyGenObject *_PyFrame_GetGenerator(_PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    size_t offset_in_gen = offsetof(PyGenObject, gi_iframe);
    return (PyGenObject *)(((char *)frame) - offset_in_gen);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
