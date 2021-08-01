/* Frame object interface */

#ifndef Py_CPYTHON_FRAMEOBJECT_H
#  error "this header file must not be included directly"
#endif

/* Starting in CPython 3.11, CPython separates the frame state between the
 * full "introspection frames" exposed by the Python and C runtime state
 * introspection APIs, and internal lighter weight "execution frames", which
 * are simple C structures owned by either the interpreter eval loop (while
 * executing ordinary functions), by a generator or coroutine object (for
 * frames that are able to be suspended), or by their corresponding
 * introspection frame (if an instrospection API has been invoked and the
 * introspection frame created).
 *
 * This split storage eliminates a lot of allocation and deallocation of full
 * Python objects during code execution, providing a significant speed gain
 * over the previous approach of using full Python objects for both introspection
 * and code execution.
 */
// Declaration of _PyExecFrame is in cpython/pystate.h for use in PyThreadState

struct _frame {
    PyObject_HEAD
    struct _frame *f_back;      /* previous frame, or NULL */
    _PyExecFrame *f_xframe;     /* points to the frame runtime data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_own_locals_memory;   /* This frame owns the memory for the locals */
};

/* Standard object interface */

PyAPI_DATA(PyTypeObject) PyFrame_Type;

#define PyFrame_Check(op) Py_IS_TYPE(op, &PyFrame_Type)

PyAPI_FUNC(PyFrameObject *) PyFrame_New(PyThreadState *, PyCodeObject *,
                                        PyObject *, PyObject *);

/* The rest of the interface is specific for frame objects */

/* Conversions between "fast locals" and locals in dictionary */

PyAPI_FUNC(void) PyFrame_LocalsToFast(PyFrameObject *, int);

PyAPI_FUNC(int) PyFrame_FastToLocalsWithError(PyFrameObject *f);
PyAPI_FUNC(void) PyFrame_FastToLocals(PyFrameObject *);

PyAPI_FUNC(void) _PyFrame_DebugMallocStats(FILE *out);

PyAPI_FUNC(PyFrameObject *) PyFrame_GetBack(PyFrameObject *frame);
