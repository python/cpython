/* Frame object interface */

#ifndef Py_CPYTHON_FRAMEOBJECT_H
#  error "this header file must not be included directly"
#endif

struct _PyInterpreterFrame;

/* Standard object interface */

PyAPI_FUNC(PyFrameObject *) PyFrame_New(PyThreadState *, PyCodeObject *,
                                        PyObject *, PyObject *);

/* The rest of the interface is specific for frame objects */

/* Conversions between "fast locals" and locals in dictionary */

PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject *, int);

/* -- Caveat emptor --
 * The concept of entry frames is an implementation detail of the CPython
 * interpreter. This API is considered unstable and is provided for the
 * convenience of debuggers, profilers and state-inspecting tools. Notice that
 * this API can be changed in future minor versions if the underlying frame
 * mechanism change or the concept of an 'entry frame' or its semantics becomes
 * obsolete or outdated. */

PyAPI_FUNC(int) _PyFrame_IsEntryFrame(PyFrameObject *frame);

PyAPI_FUNC(int) PyFrame_FastToLocalsWithError(PyFrameObject *f);
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject *);

/* The following functions are for use by debuggers and other tools
 * implementing custom frame evaluators with PEP 523. */

/* Returns the code object of the frame (strong reference).
 * Does not raise an exception. */
PyAPI_FUNC(PyObject *) PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame);

/* Returns a byte ofsset into the last executed instruction.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame);

/* Returns the currently executing line number, or -1 if there is no line number.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLine(struct _PyInterpreterFrame *frame);
