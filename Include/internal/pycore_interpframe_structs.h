/* Structures used by pycore_debug_offsets.h.
 *
 * See InternalDocs/frames.md for an explanation of the frame stack
 * including explanation of the PyFrameObject and _PyInterpreterFrame
 * structs.
 */

#ifndef Py_INTERNAL_INTERP_FRAME_STRUCTS_H
#define Py_INTERNAL_INTERP_FRAME_STRUCTS_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_structs.h"       // _PyStackRef
#include "pycore_typedefs.h"      // _PyInterpreterFrame

#ifdef __cplusplus
extern "C" {
#endif

enum _frameowner {
    // The frame is allocated on per-thread memory that will be freed or transferred when
    // the frame unwinds.
    FRAME_OWNED_BY_THREAD = 0x00,
    // The frame is allocated in a generator and may out-live the execution.
    FRAME_OWNED_BY_GENERATOR = 0x01,
    // A flag which indicates the frame is owned externally. May be combined with
    // FRAME_OWNED_BY_THREAD or FRAME_OWNED_BY_GENERATOR. The frame may only have
    // _PyInterpreterFrameFields. To access other fields and ensure they are up to
    // date _PyFrame_EnsureFrameFullyInitialized must be called first.
    FRAME_OWNED_EXTERNALLY = 0x02,
    // The frame is owned by the frame object (indicating the frame has unwound).
    FRAME_OWNED_BY_FRAME_OBJECT = 0x04,
    // The frame is a sentinel frame for entry to the interpreter loop
    FRAME_OWNED_BY_INTERPRETER = 0x08,
};

struct _PyInterpreterFrameCore {
    _PyStackRef f_executable; /* Deferred or strong reference (code object or None) */
    struct _PyInterpreterFrameCore *previous;
    char owner;
};

struct _PyInterpreterFrame {
    _PyInterpreterFrameCore core;
    _PyStackRef f_funcobj; /* Deferred or strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    _Py_CODEUNIT *instr_ptr; /* Instruction currently executing (or about to begin) */
    _PyStackRef *stackpointer;
#ifdef Py_GIL_DISABLED
    /* Index of thread-local bytecode containing instr_ptr. */
    int32_t tlbc_index;
#endif
    uint16_t return_offset;  /* Only relevant during a function call */
#ifdef Py_DEBUG
    uint8_t visited:1;
    uint8_t lltrace:7;
#else
    uint8_t visited;
#endif
    /* Locals and stack */
    _PyStackRef localsplus[1];
};


/* _PyGenObject_HEAD defines the initial segment of generator
   and coroutine objects. */
#define _PyGenObject_HEAD(prefix)                                           \
    PyObject_HEAD                                                           \
    /* List of weak reference. */                                           \
    PyObject *prefix##_weakreflist;                                         \
    /* Name of the generator. */                                            \
    PyObject *prefix##_name;                                                \
    /* Qualified name of the generator. */                                  \
    PyObject *prefix##_qualname;                                            \
    _PyErr_StackItem prefix##_exc_state;                                    \
    PyObject *prefix##_origin_or_finalizer;                                 \
    char prefix##_hooks_inited;                                             \
    char prefix##_closed;                                                   \
    char prefix##_running_async;                                            \
    /* The frame */                                                         \
    int8_t prefix##_frame_state;                                            \
    _PyInterpreterFrame prefix##_iframe;                                    \

struct _PyGenObject {
    /* The gi_ prefix is intended to remind of generator-iterator. */
    _PyGenObject_HEAD(gi)
};

struct _PyCoroObject {
    _PyGenObject_HEAD(cr)
};

struct _PyAsyncGenObject {
    _PyGenObject_HEAD(ag)
};

typedef struct _PyInterpreterFrame * (*_PyFrame_Reifier)(struct _PyInterpreterFrameCore *, PyObject *reifier);

typedef struct {
    PyObject_HEAD
    PyCodeObject *ef_code;
    PyObject *ef_state;
    _PyFrame_Reifier ef_reifier;
} PyUnstable_PyExternalExecutable;

#undef _PyGenObject_HEAD


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_STRUCTS_H
