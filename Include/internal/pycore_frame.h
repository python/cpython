/* See InternalDocs/frames.md for an explanation of the frame stack
 * including explanation of the PyFrameObject and _PyInterpreterFrame
 * structs. */

#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_typedefs.h"      // _PyInterpreterFrame


struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    _PyInterpreterFrame *f_frame; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    PyObject *f_extra_locals;   /* Dict for locals set by users using f_locals, could be NULL */
    /* This is purely for backwards compatibility for PyEval_GetLocals.
       PyEval_GetLocals requires a borrowed reference so the actual reference
       is stored here */
    PyObject *f_locals_cache;
    /* A tuple containing strong references to fast locals that were overwritten
     * via f_locals. Borrowed references to these locals may exist in frames
     * closer to the top of the stack. The references in this tuple act as
     * "support" for the borrowed references, ensuring that they remain valid.
     */
    PyObject *f_overwritten_fast_locals;
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);


/* other API */

typedef enum _framestate {
    FRAME_CREATED = -3,
    FRAME_SUSPENDED = -2,
    FRAME_SUSPENDED_YIELD_FROM = -1,
    FRAME_EXECUTING = 0,
    FRAME_COMPLETED = 1,
    FRAME_CLEARED = 4
} PyFrameState;

#define FRAME_STATE_SUSPENDED(S) ((S) == FRAME_SUSPENDED || (S) == FRAME_SUSPENDED_YIELD_FROM)
#define FRAME_STATE_FINISHED(S) ((S) >= FRAME_COMPLETED)

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
