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
 * Does not raise an exception. */
PyAPI_FUNC(PyObject *) PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame);

/* Returns a byte offset into the last executed instruction.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame);

/* Returns the currently executing line number, or -1 if there is no line number.
 * Does not raise an exception. */
PyAPI_FUNC(int) PyUnstable_InterpreterFrame_GetLine(struct _PyInterpreterFrame *frame);

/* External frame introspection.
 *
 * Frames with the `external` bit set were pushed by something other than
 * the VM. Such frames have a `struct _PyInterpreterFrameExternal` header
 * immediately before the `struct _PyInterpreterFrame` in memory. The header
 * holds a strong reference to a `_PyExternalFrameIntrospector` object which
 * knows how to visit external references for the garbage collector.
 *
 * The introspector is created with a GC visitor function. During collection
 * (free-threaded build), the GC calls the visitor with the frame and a
 * callback; the visitor must invoke the callback once for each PyObject*
 * referenced by external frame state.
 */

struct _PyExternalFrameIntrospector;

/* GC visitor for external frames.
 *
 * `interp` is the interpreter being collected, `frame` is the external
 * frame, `callback` must be invoked once for each PyObject* referenced
 * by external frame state (with `arg` passed through). The callback
 * follows gcvisitobjects_t conventions (return 1 to continue, 0 to stop).
 *
 * Returns 0 if the frame is complete (all references were visited),
 * nonzero if the frame is incomplete and the GC must skip collecting
 * objects with deferred references.
 */
typedef int (*PyUnstable_ExternalFrameGCVisitor)(
    PyInterpreterState *interp,
    struct _PyInterpreterFrame *frame,
    gcvisitobjects_t callback,
    void *arg);

PyAPI_DATA(PyTypeObject) PyUnstable_ExternalFrameIntrospector_Type;

#define PyUnstable_ExternalFrameIntrospector_Check(op) \
    Py_IS_TYPE((op), &PyUnstable_ExternalFrameIntrospector_Type)

/* Create a new _PyExternalFrameIntrospector holding `gc_visitor`.
 * Returns a new reference, or NULL with an exception set if `gc_visitor`
 * is NULL or allocation fails. */
PyAPI_FUNC(PyObject *) PyUnstable_ExternalFrameIntrospector_New(
    PyUnstable_ExternalFrameGCVisitor gc_visitor);

/* Return the gc_visitor stored in `introspector`.
 * Returns NULL with a TypeError if `introspector` is not an introspector. */
PyAPI_FUNC(PyUnstable_ExternalFrameGCVisitor)
PyUnstable_ExternalFrameIntrospector_GetGCVisitor(PyObject *introspector);
