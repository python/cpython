#ifndef Py_TRACEBACK_H
#define Py_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

/* Traceback interface */

PyAPI_FUNC(int) PyTraceBack_Here(PyFrameObject *);
PyAPI_FUNC(int) PyTraceBack_Print(PyObject *, PyObject *);

/* Reveal traceback type so we can typecheck traceback objects */
PyAPI_DATA(PyTypeObject) PyTraceBack_Type;
#define PyTraceBack_Check(v) Py_IS_TYPE((v), &PyTraceBack_Type)

/* Write the Python traceback into the file 'fd'. For example:

       Traceback (most recent call first):
         File "xxx", line xxx in <xxx>
         File "xxx", line xxx in <xxx>
         ...
         File "xxx", line xxx in <xxx>

   This function is written for debug purpose only, to dump the traceback in
   the worst case: after a segmentation fault, at fatal error, etc. That's why,
   it is very limited. Strings are truncated to 100 characters and encoded to
   ASCII with backslashreplace. It doesn't write the source code, only the
   function name, filename and line number of each frame. Write only the first
   100 frames: if the traceback is truncated, write the line " ...".

   This function is signal safe. */
PyAPI_FUNC(void) PyUnstable_DumpTraceback(int fd, PyThreadState *tstate);

/* Write the traceback of all threads into the file 'fd'. current_thread can be
   NULL.

   Return NULL on success, or an error message on error.

   This function is written for debug purpose only. It calls
   PyUnstable_DumpTraceback() for each thread, and so has the same limitations. It
   only writes the traceback of the first 100 threads: write "..." if there are
   more threads.

   If current_tstate is NULL, the function tries to get the Python thread state
   of the current thread. It is not an error if the function is unable to get
   the current Python thread state.

   If interp is NULL, the function tries to get the interpreter state from
   the current Python thread state, or from
   _PyGILState_GetInterpreterStateUnsafe() in last resort.

   It is better to pass NULL to interp and current_tstate, the function tries
   different options to retrieve this information.

   This function is signal safe. */
PyAPI_FUNC(const char*) PyUnstable_DumpTracebackThreads(
    int fd,
    PyInterpreterState *interp,
    PyThreadState *current_tstate);

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_TRACEBACK_H
#  include "cpython/traceback.h"
#  undef Py_CPYTHON_TRACEBACK_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_TRACEBACK_H */
