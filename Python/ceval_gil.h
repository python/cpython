/*
 * Implementation of the Global Interpreter Lock (GIL).
 */

#include <stdlib.h>
#include <errno.h>


/* First some general settings */

#define INTERVAL (_PyRuntime.ceval.gil.interval >= 1 ? _PyRuntime.ceval.gil.interval : 1)


/*
   Notes about the implementation:

   - The GIL is just a boolean variable (locked) whose access is protected
     by a mutex (gil_mutex), and whose changes are signalled by a condition
     variable (gil_cond). gil_mutex is taken for short periods of time,
     and therefore mostly uncontended.

   - In the GIL-holding thread, the main loop (PyEval_EvalFrameEx) must be
     able to release the GIL on demand by another thread. A volatile boolean
     variable (gil_drop_request) is used for that purpose, which is checked
     at every turn of the eval loop. That variable is set after a wait of
     `interval` microseconds on `gil_cond` has timed out.

      [Actually, another volatile boolean variable (eval_breaker) is used
       which ORs several conditions into one. Volatile booleans are
       sufficient as inter-thread signalling means since Python is run
       on cache-coherent architectures only.]

   - A thread wanting to take the GIL will first let pass a given amount of
     time (`interval` microseconds) before setting gil_drop_request. This
     encourages a defined switching period, but doesn't enforce it since
     opcodes can take an arbitrary time to execute.

     The `interval` value is available for the user to read and modify
     using the Python API `sys.{get,set}switchinterval()`.

   - When a thread releases the GIL and gil_drop_request is set, that thread
     ensures that another GIL-awaiting thread gets scheduled.
     It does so by waiting on a condition variable (switch_cond) until
     the value of last_holder is changed to something else than its
     own thread state pointer, indicating that another thread was able to
     take the GIL.

     This is meant to prohibit the latency-adverse behaviour on multi-core
     machines where one thread would speculatively release the GIL, but still
     run and end up being the first to re-acquire it, making the "timeslices"
     much longer than expected.
     (Note: this mechanism is enabled with FORCE_SWITCHING above)
*/

#include "condvar.h"

#define MUTEX_INIT(mut) \
    if (PyMUTEX_INIT(&(mut))) { \
        Py_FatalError("PyMUTEX_INIT(" #mut ") failed"); };
#define MUTEX_FINI(mut) \
    if (PyMUTEX_FINI(&(mut))) { \
        Py_FatalError("PyMUTEX_FINI(" #mut ") failed"); };
#define MUTEX_LOCK(mut) \
    if (PyMUTEX_LOCK(&(mut))) { \
        Py_FatalError("PyMUTEX_LOCK(" #mut ") failed"); };
#define MUTEX_UNLOCK(mut) \
    if (PyMUTEX_UNLOCK(&(mut))) { \
        Py_FatalError("PyMUTEX_UNLOCK(" #mut ") failed"); };

#define COND_INIT(cond) \
    if (PyCOND_INIT(&(cond))) { \
        Py_FatalError("PyCOND_INIT(" #cond ") failed"); };
#define COND_FINI(cond) \
    if (PyCOND_FINI(&(cond))) { \
        Py_FatalError("PyCOND_FINI(" #cond ") failed"); };
#define COND_SIGNAL(cond) \
    if (PyCOND_SIGNAL(&(cond))) { \
        Py_FatalError("PyCOND_SIGNAL(" #cond ") failed"); };
#define COND_WAIT(cond, mut) \
    if (PyCOND_WAIT(&(cond), &(mut))) { \
        Py_FatalError("PyCOND_WAIT(" #cond ") failed"); };
#define COND_TIMED_WAIT(cond, mut, microseconds, timeout_result) \
    { \
        int r = PyCOND_TIMEDWAIT(&(cond), &(mut), (microseconds)); \
        if (r < 0) \
            Py_FatalError("PyCOND_WAIT(" #cond ") failed"); \
        if (r) /* 1 == timeout, 2 == impl. can't say, so assume timeout */ \
            timeout_result = 1; \
        else \
            timeout_result = 0; \
    } \


#define DEFAULT_INTERVAL 5000

static void _gil_initialize(struct _gil_runtime_state *state)
{
    _Py_atomic_int uninitialized = {-1};
    state->locked = uninitialized;
    state->interval = DEFAULT_INTERVAL;
}

static int gil_created(void)
{
    return (_Py_atomic_load_explicit(&_PyRuntime.ceval.gil.locked,
                                     _Py_memory_order_acquire)
            ) >= 0;
}

static void create_gil(void)
{
    MUTEX_INIT(_PyRuntime.ceval.gil.mutex);
#ifdef FORCE_SWITCHING
    MUTEX_INIT(_PyRuntime.ceval.gil.switch_mutex);
#endif
    COND_INIT(_PyRuntime.ceval.gil.cond);
#ifdef FORCE_SWITCHING
    COND_INIT(_PyRuntime.ceval.gil.switch_cond);
#endif
    _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder, 0);
    _Py_ANNOTATE_RWLOCK_CREATE(&_PyRuntime.ceval.gil.locked);
    _Py_atomic_store_explicit(&_PyRuntime.ceval.gil.locked, 0,
                              _Py_memory_order_release);
}

static void destroy_gil(void)
{
    /* some pthread-like implementations tie the mutex to the cond
     * and must have the cond destroyed first.
     */
    COND_FINI(_PyRuntime.ceval.gil.cond);
    MUTEX_FINI(_PyRuntime.ceval.gil.mutex);
#ifdef FORCE_SWITCHING
    COND_FINI(_PyRuntime.ceval.gil.switch_cond);
    MUTEX_FINI(_PyRuntime.ceval.gil.switch_mutex);
#endif
    _Py_atomic_store_explicit(&_PyRuntime.ceval.gil.locked, -1,
                              _Py_memory_order_release);
    _Py_ANNOTATE_RWLOCK_DESTROY(&_PyRuntime.ceval.gil.locked);
}

static void recreate_gil(void)
{
    _Py_ANNOTATE_RWLOCK_DESTROY(&_PyRuntime.ceval.gil.locked);
    /* XXX should we destroy the old OS resources here? */
    create_gil();
}

static void drop_gil(PyThreadState *tstate)
{
    if (!_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked))
        Py_FatalError("drop_gil: GIL is not locked");
    /* tstate is allowed to be NULL (early interpreter init) */
    if (tstate != NULL) {
        /* Sub-interpreter support: threads might have been switched
           under our feet using PyThreadState_Swap(). Fix the GIL last
           holder variable so that our heuristics work. */
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                 (uintptr_t)tstate);
    }

    MUTEX_LOCK(_PyRuntime.ceval.gil.mutex);
    _Py_ANNOTATE_RWLOCK_RELEASED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);
    _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 0);
    COND_SIGNAL(_PyRuntime.ceval.gil.cond);
    MUTEX_UNLOCK(_PyRuntime.ceval.gil.mutex);

#ifdef FORCE_SWITCHING
    if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil_drop_request) &&
        tstate != NULL)
    {
        MUTEX_LOCK(_PyRuntime.ceval.gil.switch_mutex);
        /* Not switched yet => wait */
        if (((PyThreadState*)_Py_atomic_load_relaxed(
                    &_PyRuntime.ceval.gil.last_holder)
            ) == tstate)
        {
        RESET_GIL_DROP_REQUEST();
            /* NOTE: if COND_WAIT does not atomically start waiting when
               releasing the mutex, another thread can run through, take
               the GIL and drop it again, and reset the condition
               before we even had a chance to wait for it. */
            COND_WAIT(_PyRuntime.ceval.gil.switch_cond,
                      _PyRuntime.ceval.gil.switch_mutex);
    }
        MUTEX_UNLOCK(_PyRuntime.ceval.gil.switch_mutex);
    }
#endif
}

static void take_gil(PyThreadState *tstate)
{
    int err;
    if (tstate == NULL)
        Py_FatalError("take_gil: NULL tstate");

    err = errno;
    MUTEX_LOCK(_PyRuntime.ceval.gil.mutex);

    if (!_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked))
        goto _ready;

    while (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked)) {
        int timed_out = 0;
        unsigned long saved_switchnum;

        saved_switchnum = _PyRuntime.ceval.gil.switch_number;
        COND_TIMED_WAIT(_PyRuntime.ceval.gil.cond, _PyRuntime.ceval.gil.mutex,
                        INTERVAL, timed_out);
        /* If we timed out and no switch occurred in the meantime, it is time
           to ask the GIL-holding thread to drop it. */
        if (timed_out &&
            _Py_atomic_load_relaxed(&_PyRuntime.ceval.gil.locked) &&
            _PyRuntime.ceval.gil.switch_number == saved_switchnum) {
            SET_GIL_DROP_REQUEST();
        }
    }
_ready:
#ifdef FORCE_SWITCHING
    /* This mutex must be taken before modifying
       _PyRuntime.ceval.gil.last_holder (see drop_gil()). */
    MUTEX_LOCK(_PyRuntime.ceval.gil.switch_mutex);
#endif
    /* We now hold the GIL */
    _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.locked, 1);
    _Py_ANNOTATE_RWLOCK_ACQUIRED(&_PyRuntime.ceval.gil.locked, /*is_write=*/1);

    if (tstate != (PyThreadState*)_Py_atomic_load_relaxed(
                    &_PyRuntime.ceval.gil.last_holder))
    {
        _Py_atomic_store_relaxed(&_PyRuntime.ceval.gil.last_holder,
                                 (uintptr_t)tstate);
        ++_PyRuntime.ceval.gil.switch_number;
    }

#ifdef FORCE_SWITCHING
    COND_SIGNAL(_PyRuntime.ceval.gil.switch_cond);
    MUTEX_UNLOCK(_PyRuntime.ceval.gil.switch_mutex);
#endif
    if (_Py_atomic_load_relaxed(&_PyRuntime.ceval.gil_drop_request)) {
        RESET_GIL_DROP_REQUEST();
    }
    if (tstate->async_exc != NULL) {
        _PyEval_SignalAsyncExc();
    }

    MUTEX_UNLOCK(_PyRuntime.ceval.gil.mutex);
    errno = err;
}

void _PyEval_SetSwitchInterval(unsigned long microseconds)
{
    _PyRuntime.ceval.gil.interval = microseconds;
}

unsigned long _PyEval_GetSwitchInterval()
{
    return _PyRuntime.ceval.gil.interval;
}
