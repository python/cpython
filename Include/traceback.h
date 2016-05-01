
#ifndef Py_TRACEBACK_H
#define Py_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pystate.h"

struct _frame;

/* Traceback interface */
#ifndef Py_LIMITED_API
typedef struct _traceback {
    PyObject_HEAD
    struct _traceback *tb_next;
    struct _frame *tb_frame;
    int tb_lasti;
    int tb_lineno;
} PyTracebackObject;
#endif

PyAPI_FUNC(int) PyTraceBack_Here(struct _frame *);
PyAPI_FUNC(int) PyTraceBack_Print(PyObject *, PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _Py_DisplaySourceLine(PyObject *, PyObject *, int, int);
PyAPI_FUNC(void) _PyTraceback_Add(const char *, const char *, int);
#endif

/* Reveal traceback type so we can typecheck traceback objects */
PyAPI_DATA(PyTypeObject) PyTraceBack_Type;
#define PyTraceBack_Check(v) (Py_TYPE(v) == &PyTraceBack_Type)

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

PyAPI_FUNC(void) _Py_DumpTraceback(
    int fd,
    PyThreadState *tstate);

/* Write the traceback of all threads into the file 'fd'. current_thread can be
   NULL. Return NULL on success, or an error message on error.

   This function is written for debug purpose only. It calls
   _Py_DumpTraceback() for each thread, and so has the same limitations. It
   only write the traceback of the first 100 threads: write "..." if there are
   more threads.

   This function is signal safe. */

PyAPI_FUNC(const char*) _Py_DumpTracebackThreads(
    int fd, PyInterpreterState *interp,
    PyThreadState *current_thread);


#ifdef __cplusplus
}
#endif
#endif /* !Py_TRACEBACK_H */
