/* Thread and interpreter state structures and their interfaces */


#ifndef Py_PYSTATE_H
#define Py_PYSTATE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pythread.h"
#include "coreconfig.h"

/* This limitation is for performance and simplicity. If needed it can be
removed (with effort). */
#define MAX_CO_EXTRA_USERS 255

/* Forward declarations for PyFrameObject, PyThreadState
   and PyInterpreterState */
struct _frame;
struct _ts;
struct _is;

#ifdef Py_LIMITED_API
typedef struct _ts PyThreadState;
typedef struct _is PyInterpreterState;
#else
/* PyThreadState and PyInterpreterState are defined in cpython/pystate.h */
#endif

/* State unique per thread */

PyAPI_FUNC(struct _is *) PyInterpreterState_New(void);
PyAPI_FUNC(void) PyInterpreterState_Clear(struct _is *);
PyAPI_FUNC(void) PyInterpreterState_Delete(struct _is *);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03070000
/* New in 3.7 */
PyAPI_FUNC(int64_t) PyInterpreterState_GetID(struct _is *);
#endif
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03030000
/* New in 3.3 */
PyAPI_FUNC(int) PyState_AddModule(PyObject*, struct PyModuleDef*);
PyAPI_FUNC(int) PyState_RemoveModule(struct PyModuleDef*);
#endif
PyAPI_FUNC(PyObject*) PyState_FindModule(struct PyModuleDef*);

PyAPI_FUNC(struct _ts *) PyThreadState_New(struct _is *);
PyAPI_FUNC(void) PyThreadState_Clear(struct _ts *);
PyAPI_FUNC(void) PyThreadState_Delete(struct _ts *);
PyAPI_FUNC(void) PyThreadState_DeleteCurrent(void);

/* Get the current thread state.

   When the current thread state is NULL, this issues a fatal error (so that
   the caller needn't check for NULL).

   The caller must hold the GIL.

   See also PyThreadState_GET() and _PyThreadState_GET(). */
PyAPI_FUNC(struct _ts *) PyThreadState_Get(void);

/* Get the current Python thread state.

   Macro using PyThreadState_Get() or _PyThreadState_GET() depending if
   pycore_pystate.h is included or not (this header redefines the macro).

   If PyThreadState_Get() is used, issue a fatal error if the current thread
   state is NULL.

   See also PyThreadState_Get() and _PyThreadState_GET(). */
#define PyThreadState_GET() PyThreadState_Get()

PyAPI_FUNC(struct _ts *) PyThreadState_Swap(struct _ts *);
PyAPI_FUNC(PyObject *) PyThreadState_GetDict(void);
PyAPI_FUNC(int) PyThreadState_SetAsyncExc(unsigned long, PyObject *);

typedef
    enum {PyGILState_LOCKED, PyGILState_UNLOCKED}
        PyGILState_STATE;


/* Ensure that the current thread is ready to call the Python
   C API, regardless of the current state of Python, or of its
   thread lock.  This may be called as many times as desired
   by a thread so long as each call is matched with a call to
   PyGILState_Release().  In general, other thread-state APIs may
   be used between _Ensure() and _Release() calls, so long as the
   thread-state is restored to its previous state before the Release().
   For example, normal use of the Py_BEGIN_ALLOW_THREADS/
   Py_END_ALLOW_THREADS macros are acceptable.

   The return value is an opaque "handle" to the thread state when
   PyGILState_Ensure() was called, and must be passed to
   PyGILState_Release() to ensure Python is left in the same state. Even
   though recursive calls are allowed, these handles can *not* be shared -
   each unique call to PyGILState_Ensure must save the handle for its
   call to PyGILState_Release.

   When the function returns, the current thread will hold the GIL.

   Failure is a fatal error.
*/
PyAPI_FUNC(PyGILState_STATE) PyGILState_Ensure(void);

/* Release any resources previously acquired.  After this call, Python's
   state will be the same as it was prior to the corresponding
   PyGILState_Ensure() call (but generally this state will be unknown to
   the caller, hence the use of the GILState API.)

   Every call to PyGILState_Ensure must be matched by a call to
   PyGILState_Release on the same thread.
*/
PyAPI_FUNC(void) PyGILState_Release(PyGILState_STATE);

/* Helper/diagnostic function - get the current thread state for
   this thread.  May return NULL if no GILState API has been used
   on the current thread.  Note that the main thread always has such a
   thread-state, even if no auto-thread-state call has been made
   on the main thread.
*/
PyAPI_FUNC(struct _ts *) PyGILState_GetThisThreadState(void);


#ifndef Py_LIMITED_API
#  define Py_CPYTHON_PYSTATE_H
#  include  "cpython/pystate.h"
#  undef Py_CPYTHON_PYSTATE_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYSTATE_H */
