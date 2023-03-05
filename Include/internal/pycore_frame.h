#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "pycore_code.h"         // STATS

/* See Objects/frame_layout.md for an explanation of the frame stack
 * including explanation of the PyFrameObject and _PyInterpreterFrame
 * structs. */


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
    char *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);


/* Tagged pointers on the stack */

typedef union {
    uintptr_t val;
    PyObject *obj;
} _tagged_ptr;

static_assert(sizeof(_tagged_ptr) == sizeof(PyObject *),
              "tagged pointer must be the same size as object pointer");

static inline bool is_tagged(_tagged_ptr tp) {
    return tp.val & 1;
}

static inline PyObject *detag(_tagged_ptr tp) {
    return (PyObject *) (tp.val & ~1);
}

static inline _tagged_ptr untagged(PyObject *p) {
    return (_tagged_ptr) { .val = (uintptr_t) p };
}

static inline _tagged_ptr tagged(PyObject *p) {
    return (_tagged_ptr) { .val = (uintptr_t) p | 1 };
}

static inline void decref_unless_tagged(_tagged_ptr tp) {
    if (!is_tagged(tp)) {
        Py_DECREF(detag(tp));
    }
}

static inline void xdecref_unless_tagged(_tagged_ptr tp) {
    if (!is_tagged(tp)) {
        Py_XDECREF(detag(tp));
    }
}

static inline void incref_if_tagged(_tagged_ptr tp) {
    if (is_tagged(tp)) {
        Py_INCREF(detag(tp));
    }
}

#define CLEAR_TAGGED(tp) do { \
    if (!is_tagged(tp)) { \
        Py_CLEAR(tp.obj); \
    } \
    else { \
        tp.obj = NULL; \
    } \
} while (0)

#define STEAL(tp) (is_tagged(tp) ? Py_NewRef(detag(tp)) : tp.obj)

static inline PyObject **plain_obj_array(_tagged_ptr *args, int size) {
    while (--size >= 0) {
        incref_if_tagged(args[size]);
        args[size].obj = detag(args[size]);
    }
    return &args[0].obj;
}


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
    FRAME_OWNED_BY_FRAME_OBJECT = 2,
    FRAME_OWNED_BY_CSTACK = 3,
};

typedef struct _PyInterpreterFrame {
    PyCodeObject *f_code; /* Strong reference */
    struct _PyInterpreterFrame *previous;
    PyObject *f_funcobj; /* Strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    // NOTE: This is not necessarily the last instruction started in the given
    // frame. Rather, it is the code unit *prior to* the *next* instruction. For
    // example, it may be an inline CACHE entry, an instruction we just jumped
    // over, or (in the case of a newly-created frame) a totally invalid value:
    _Py_CODEUNIT *prev_instr;
    int stacktop;  /* Offset of TOS from localsplus  */
    uint16_t yield_offset;
    char owner;
    /* Locals and stack (locals are object ptrs, stack are tagged ptrs) */
    PyObject *localsplus[1];
} _PyInterpreterFrame;

static inline _tagged_ptr *localsplus_as_tagged_ptr(_PyInterpreterFrame *f) {
    return (_tagged_ptr *) f->localsplus;
}

static inline PyObject **localsplus_as_object_ptr(_PyInterpreterFrame *f) {
    return (PyObject **) f->localsplus;
}

#define _PyInterpreterFrame_LASTI(IF) \
    ((int)((IF)->prev_instr - _PyCode_CODE((IF)->f_code)))

static inline _tagged_ptr *_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return localsplus_as_tagged_ptr(f) + f->f_code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    _tagged_ptr tp = localsplus_as_tagged_ptr(f)[f->stacktop - 1];
    assert(detag(tp) != NULL);
    return detag(tp);
}

static inline PyObject *_PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    f->stacktop--;
    _tagged_ptr tp = localsplus_as_tagged_ptr(f)[f->stacktop];
    incref_if_tagged(tp);
    return detag(tp);
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, PyObject *value) {
    localsplus_as_tagged_ptr(f)[f->stacktop] = untagged(value);
    f->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((int)((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *)))

static inline int
_PyFrame_NumSlotsForCodeObject(PyCodeObject *code)
{
    /* This function needs to remain in sync with the calculation of
     * co_framesize in Tools/build/deepfreeze.py */
    assert(code->co_framesize >= FRAME_SPECIALS_SIZE);
    return code->co_framesize - FRAME_SPECIALS_SIZE;
}

void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest);

/* Consumes reference to func and locals.
   Does not initialize frame->previous, which happens
   when frame is linked into the frame stack.
 */
static inline void
_PyFrame_Initialize(
    _PyInterpreterFrame *frame, PyFunctionObject *func,
    PyObject *locals, PyCodeObject *code, int null_locals_from)
{
    frame->f_funcobj = (PyObject *)func;
    frame->f_code = (PyCodeObject *)Py_NewRef(code);
    frame->f_builtins = func->func_builtins;
    frame->f_globals = func->func_globals;
    frame->f_locals = locals;
    frame->stacktop = code->co_nlocalsplus;
    frame->frame_obj = NULL;
    frame->prev_instr = _PyCode_CODE(code) - 1;
    frame->yield_offset = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;

    for (int i = null_locals_from; i < code->co_nlocalsplus; i++) {
        localsplus_as_object_ptr(frame)[i] = NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject **
_PyFrame_GetLocalsArray_ObjectPtr(_PyInterpreterFrame *frame)
{
    return (PyObject **) frame->localsplus;
}

static inline _tagged_ptr *
_PyFrame_GetLocalsArray_TaggedPtr(_PyInterpreterFrame *frame)
{
    return (_tagged_ptr *) frame->localsplus;
}

static inline _tagged_ptr *
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    return (_tagged_ptr *) (frame->localsplus + frame->stacktop);
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, _tagged_ptr *stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - (_tagged_ptr *) frame->localsplus);
}

/* Determine whether a frame is incomplete.
 * A frame is incomplete if it is part way through
 * creating cell objects or a generator or coroutine.
 *
 * Frames on the frame stack are incomplete until the
 * first RESUME instruction.
 * Frames owned by a generator are always complete.
 */
static inline bool
_PyFrame_IsIncomplete(_PyInterpreterFrame *frame)
{
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
    frame->prev_instr < _PyCode_CODE(frame->f_code) + frame->f_code->_co_firsttraceable;
}

static inline _PyInterpreterFrame *
_PyFrame_GetFirstComplete(_PyInterpreterFrame *frame)
{
    while (frame && _PyFrame_IsIncomplete(frame)) {
        frame = frame->previous;
    }
    return frame;
}

static inline _PyInterpreterFrame *
_PyThreadState_GetFrame(PyThreadState *tstate)
{
    return _PyFrame_GetFirstComplete(tstate->cframe->current_frame);
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

    assert(!_PyFrame_IsIncomplete(frame));
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
_PyFrame_ClearExceptCode(_PyInterpreterFrame * frame);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(_PyInterpreterFrame *frame);

void
_PyFrame_LocalsToFast(_PyInterpreterFrame *frame, int clear);

static inline bool
_PyThreadState_HasStackSpace(PyThreadState *tstate, int size)
{
    assert(
        (tstate->datastack_top == NULL && tstate->datastack_limit == NULL)
        ||
        (tstate->datastack_top != NULL && tstate->datastack_limit != NULL)
    );
    return tstate->datastack_top != NULL &&
        size < tstate->datastack_limit - tstate->datastack_top;
}

extern _PyInterpreterFrame *
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size);

void _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, PyFunctionObject *func, int null_locals_from)
{
    CALL_STAT_INC(frames_pushed);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    _PyFrame_Initialize(new_frame, func, NULL, code, null_locals_from);
    return new_frame;
}

int _PyInterpreterFrame_GetLine(_PyInterpreterFrame *frame);

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
