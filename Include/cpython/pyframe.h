#ifndef Py_CPYTHON_PYFRAME_H
#  error "this header file must not be included directly"
#endif

PyAPI_DATA(PyTypeObject) PyFrame_Type;
PyAPI_DATA(PyTypeObject) PyFrameLocalsProxy_Type;

#define PyFrame_Check(op) Py_IS_TYPE((op), &PyFrame_Type)
#define PyFrameLocalsProxy_Check(op) Py_IS_TYPE((op), &PyFrameLocalsProxy_Type)

PyAPI_FUNC(PyFrameObject *) PyFrame_GetBack(PyFrameObject *frame);
PyAPI_FUNC(PyObject *) PyFrame_GetLocals(PyFrameObject *frame);

PyAPI_FUNC(PyObject *) PyFrame_GetGlobals(PyFrameObject *frame);
PyAPI_FUNC(PyObject *) PyFrame_GetBuiltins(PyFrameObject *frame);

PyAPI_FUNC(PyObject *) PyFrame_GetGenerator(PyFrameObject *frame);
PyAPI_FUNC(int) PyFrame_GetLasti(PyFrameObject *frame);
PyAPI_FUNC(PyObject*) PyFrame_GetVar(PyFrameObject *frame, PyObject *name);
PyAPI_FUNC(PyObject*) PyFrame_GetVarString(PyFrameObject *frame, const char *name);

/* The following functions are for use by debuggers and other tools
 * implementing custom frame evaluators with PEP 523. */

struct _PyInterpreterFrame;

/* Returns the code object of the frame (strong reference).
 * Does not raise an exception.
 * If allocation and reference count changes are not permitted, use
 * PyUnstable_InterpreterFrame_GetCodeBorrowed instead. */
PyAPI_FUNC(PyObject *) PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame);

/* Returns the code object of the frame as a borrowed reference.
 * Use instead of PyUnstable_InterpreterFrame_GetCode when allocation and
 * reference count changes are not permitted (e.g. from a custom memory
 * allocator hook).  Does not allocate, does not change any reference counts,
 * does not acquire or release the GIL, does not raise an exception.  Uses
 * heuristics to detect freed memory; not 100% reliable. */
PyAPI_FUNC(PyObject *) PyUnstable_InterpreterFrame_GetCodeBorrowed(struct _PyInterpreterFrame *frame);

/* Returns a byte offset into the last executed instruction.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame);

/* Returns the currently executing line number, or -1 if there is no line number.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLine(struct _PyInterpreterFrame *frame);

/* Returns the currently executing line number, or -1 if there is no line
 * number or the frame is invalid.
 * Unlike PyUnstable_InterpreterFrame_GetLine, validates the code object and
 * instruction offset before accessing the line table rather than asserting
 * them, avoiding assertion failures when the frame state may be partially
 * torn down.  Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLineChecked(struct _PyInterpreterFrame *frame);


/* Returns the innermost Python frame of the thread state, or NULL if the
 * thread has no Python frame or freed memory is detected.  Skips over
 * incomplete frames (interpreter entry trampolines and frames that have not
 * yet begun executing) automatically.
 * Does not allocate memory, does not acquire or release the GIL, does not
 * raise an exception, does not re-enter the interpreter.  Racy reads from
 * other threads are intentional and suppressed (_Py_NO_SANITIZE_THREAD).
 * Uses heuristics to detect freed memory; not 100% reliable. */
PyAPI_FUNC(struct _PyInterpreterFrame *)
PyUnstable_ThreadState_GetCurrentFrame(PyThreadState *tstate);

/* Returns the frame that called *frame*, or NULL if *frame* is the outermost
 * frame or freed memory is detected.  Skips over incomplete frames
 * (interpreter entry trampolines and frames that have not yet begun
 * executing) automatically.
 * Does not allocate memory, does not acquire or release the GIL, does not
 * raise an exception, does not re-enter the interpreter.  Racy reads from
 * other threads are intentional and suppressed (_Py_NO_SANITIZE_THREAD).
 * Uses heuristics to detect freed memory; not 100% reliable. */
PyAPI_FUNC(struct _PyInterpreterFrame *)
PyUnstable_InterpreterFrame_GetCaller(struct _PyInterpreterFrame *frame);

#define PyUnstable_EXECUTABLE_KIND_SKIP 0
#define PyUnstable_EXECUTABLE_KIND_PY_FUNCTION 1
#define PyUnstable_EXECUTABLE_KIND_BUILTIN_FUNCTION 3
#define PyUnstable_EXECUTABLE_KIND_METHOD_DESCRIPTOR 4
#define PyUnstable_EXECUTABLE_KINDS 5

PyAPI_DATA(const PyTypeObject *) const PyUnstable_ExecutableKinds[PyUnstable_EXECUTABLE_KINDS+1];
