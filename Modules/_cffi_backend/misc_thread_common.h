#ifndef WITH_THREAD
# error "xxx no-thread configuration not tested, please report if you need that"
#endif
#include "pythread.h"


struct cffi_tls_s {
    /* The current thread's ThreadCanaryObj.  This is only non-null in
       case cffi builds the thread state here.  It remains null if this
       thread had already a thread state provided by CPython. */
    struct thread_canary_s *local_thread_canary;

#ifndef USE__THREAD
    /* The saved errno.  If the C compiler supports '__thread', then
       we use that instead. */
    int saved_errno;
#endif

#ifdef MS_WIN32
    /* The saved lasterror, on Windows. */
    int saved_lasterror;
#endif
};

static struct cffi_tls_s *get_cffi_tls(void);   /* in misc_thread_posix.h 
                                                   or misc_win32.h */


/* We try to keep the PyThreadState around in a thread not started by
 * Python but where cffi callbacks occur.  If we didn't do that, then
 * the standard logic in PyGILState_Ensure() and PyGILState_Release()
 * would create a new PyThreadState and completely free it for every
 * single call.  For some applications, this is a huge slow-down.
 *
 * As shown by issue #362, it is quite messy to do.  The current
 * solution is to keep the PyThreadState alive by incrementing its
 * 'gilstate_counter'.  We detect thread shut-down, and we put the
 * PyThreadState inside a list of zombies (we can't free it
 * immediately because we don't have the GIL at that point in time).
 * We also detect other pieces of code (notably Py_Finalize()) which
 * clear and free PyThreadStates under our feet, using ThreadCanaryObj.
 */

#define TLS_ZOM_LOCK() PyThread_acquire_lock(cffi_zombie_lock, WAIT_LOCK)
#define TLS_ZOM_UNLOCK() PyThread_release_lock(cffi_zombie_lock)
static PyThread_type_lock cffi_zombie_lock = NULL;


/* A 'canary' object is created in a thread when there is a callback
   invoked, and that thread has no PyThreadState so far.  It is an
   object of reference count equal to 1, which is stored in the
   PyThreadState->dict.  Two things can occur then:

   1. The PyThreadState can be forcefully cleared by Py_Finalize().
      Then thread_canary_dealloc() is called, and we have to cancel
      the hacks we did to keep the PyThreadState alive.

   2. The thread finishes.  In that case, we put the canary in a list
      of zombies, and at some convenient time later when we have the
      GIL, we free all PyThreadStates in the zombie list.

   Some more fun comes from the fact that thread_canary_dealloc() can
   be called at a point where the canary is in the zombie list already.
   Also, the various pieces are freed at specific points in time, and
   we must make sure not to access already-freed structures:

    - the struct cffi_tls_s is valid until the thread shuts down, and
      then it is freed by cffi_thread_shutdown().

    - the canary is a normal Python object, but we have a borrowed
      reference to it from cffi_tls_s.local_thread_canary.
 */

typedef struct thread_canary_s {
    PyObject_HEAD
    struct thread_canary_s *zombie_prev, *zombie_next;
    PyThreadState *tstate;
    struct cffi_tls_s *tls;
} ThreadCanaryObj;

static PyTypeObject ThreadCanary_Type;    /* forward */
static ThreadCanaryObj cffi_zombie_head;

static void
_thread_canary_detach_with_lock(ThreadCanaryObj *ob)
{
    /* must be called with both the GIL and TLS_ZOM_LOCK. */
    ThreadCanaryObj *p, *n;
    p = ob->zombie_prev;
    n = ob->zombie_next;
    p->zombie_next = n;
    n->zombie_prev = p;
    ob->zombie_prev = NULL;
    ob->zombie_next = NULL;
}

static void
thread_canary_dealloc(ThreadCanaryObj *ob)
{
    /* this ThreadCanaryObj is being freed: if it is in the zombie
       chained list, remove it.  Thread-safety: 'zombie_next' amd
       'local_thread_canary' accesses need to be protected with
       the TLS_ZOM_LOCK.
     */
    //fprintf(stderr, "entering dealloc(%p)\n", ob);
    TLS_ZOM_LOCK();
    if (ob->zombie_next != NULL) {
        //fprintf(stderr, "thread_canary_dealloc(%p): ZOMBIE\n", ob);
        _thread_canary_detach_with_lock(ob);
    }
    else {
        //fprintf(stderr, "thread_canary_dealloc(%p): not a zombie\n", ob);
    }

    if (ob->tls != NULL) {
        //fprintf(stderr, "thread_canary_dealloc(%p): was local_thread_canary\n", ob);
        assert(ob->tls->local_thread_canary == ob);
        ob->tls->local_thread_canary = NULL;
    }
    TLS_ZOM_UNLOCK();

    PyObject_Del((PyObject *)ob);
}

static void
thread_canary_make_zombie(ThreadCanaryObj *ob)
{
    /* This must be called without the GIL, but with the TLS_ZOM_LOCK.
       It must be called at most once for a given ThreadCanaryObj. */
    ThreadCanaryObj *last;

    //fprintf(stderr, "thread_canary_make_zombie(%p)\n", ob);
    if (ob->zombie_next)
        Py_FatalError("cffi: ThreadCanaryObj is already a zombie");
    last = cffi_zombie_head.zombie_prev;
    ob->zombie_next = &cffi_zombie_head;
    ob->zombie_prev = last;
    last->zombie_next = ob;
    cffi_zombie_head.zombie_prev = ob;
    //fprintf(stderr, "thread_canary_make_zombie(%p) DONE\n", ob);
}

static void
thread_canary_free_zombies(void)
{
    /* This must be called with the GIL. */
    if (cffi_zombie_head.zombie_next == &cffi_zombie_head)
        return;    /* fast path */

    while (1) {
        ThreadCanaryObj *ob;
        PyThreadState *tstate = NULL;

        TLS_ZOM_LOCK();
        ob = cffi_zombie_head.zombie_next;
        if (ob != &cffi_zombie_head) {
            tstate = ob->tstate;
            //fprintf(stderr, "thread_canary_free_zombie(%p) tstate=%p\n", ob, tstate);
            _thread_canary_detach_with_lock(ob);
            if (tstate == NULL)
                Py_FatalError("cffi: invalid ThreadCanaryObj->tstate");
        }
        TLS_ZOM_UNLOCK();

        if (tstate == NULL)
            break;
        PyThreadState_Clear(tstate);  /* calls thread_canary_dealloc on 'ob',
                                         but now ob->zombie_next == NULL. */
#if PY_VERSION_HEX >= 0x030C0000
        /* this might be a bug introduced in 3.12, or just me abusing the
           API around there.  The issue is that PyThreadState_Delete()
           called on a random old tstate will clear the *current* thread
           notion of what PyGILState_GetThisThreadState() should be, at
           least if the internal 'bound_gilstate' flag is set in the old
           tstate.  Workaround: clear that flag. */
        tstate->_status.bound_gilstate = 0;
#endif
        PyThreadState_Delete(tstate);
        //fprintf(stderr, "thread_canary_free_zombie: cleared and deleted tstate=%p\n", tstate);
    }
    //fprintf(stderr, "thread_canary_free_zombie: end\n");
}

static void
thread_canary_register(PyThreadState *tstate)
{
    /* called with the GIL; 'tstate' is the current PyThreadState. */
    ThreadCanaryObj *canary;
    PyObject *tdict;
    struct cffi_tls_s *tls;
    int err;

    /* first free the zombies, if any */
    thread_canary_free_zombies();

    tls = get_cffi_tls();
    if (tls == NULL)
        goto ignore_error;

    tdict = PyThreadState_GetDict();
    if (tdict == NULL)
        goto ignore_error;

    canary = PyObject_New(ThreadCanaryObj, &ThreadCanary_Type);
    //fprintf(stderr, "thread_canary_register(%p): tstate=%p tls=%p\n", canary, tstate, tls);
    if (canary == NULL)
        goto ignore_error;
    canary->zombie_prev = NULL;
    canary->zombie_next = NULL;
    canary->tstate = tstate;
    canary->tls = tls;

    err = PyDict_SetItemString(tdict, "cffi.thread.canary", (PyObject *)canary);
    Py_DECREF(canary);
    if (err < 0)
        goto ignore_error;

    /* thread-safety: we have the GIL here, and 'tstate' is the one that
       corresponds to our own thread.  We are allocating a new 'canary'
       and setting it up for our own thread, both in 'tdict' (which owns
       the reference) and in 'tls->local_thread_canary' (which doesn't). */
    assert(Py_REFCNT(canary) == 1);
    tls->local_thread_canary = canary;
    tstate->gilstate_counter++;
    /* ^^^ this means 'tstate' will never be automatically freed by
           PyGILState_Release() */
    //fprintf(stderr, "CANARY: ready, tstate=%p, tls=%p, canary=%p\n", tstate, tls, canary);
    return;

 ignore_error:
    //fprintf(stderr, "CANARY: IGNORED ERROR\n");
    PyErr_Clear();
}

static PyTypeObject ThreadCanary_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_cffi_backend.thread_canary",
    sizeof(ThreadCanaryObj),
    0,
    (destructor)thread_canary_dealloc,          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                         /* tp_flags */
};

static void init_cffi_tls_zombie(void)
{
    cffi_zombie_head.zombie_next = &cffi_zombie_head;
    cffi_zombie_head.zombie_prev = &cffi_zombie_head;
    cffi_zombie_lock = PyThread_allocate_lock();
    if (cffi_zombie_lock == NULL)
        PyErr_SetString(PyExc_SystemError, "can't allocate cffi_zombie_lock");
}

static void cffi_thread_shutdown(void *p)
{
    /* this function is called from misc_thread_posix or misc_win32
       when a thread is about to end. */
    struct cffi_tls_s *tls = (struct cffi_tls_s *)p;

    /* thread-safety: this field 'local_thread_canary' can be reset
       to NULL in parallel, protected by TLS_ZOM_LOCK. */
    TLS_ZOM_LOCK();
    if (tls->local_thread_canary != NULL) {
        tls->local_thread_canary->tls = NULL;
        thread_canary_make_zombie(tls->local_thread_canary);
    }
    TLS_ZOM_UNLOCK();
    //fprintf(stderr, "thread_shutdown(%p)\n", tls);
    free(tls);
}

/* USE__THREAD is defined by setup.py if it finds that it is
   syntactically valid to use "__thread" with this C compiler. */
#ifdef USE__THREAD

static __thread int cffi_saved_errno = 0;
static void save_errno_only(void) { cffi_saved_errno = errno; }
static void restore_errno_only(void) { errno = cffi_saved_errno; }

#else

static void save_errno_only(void)
{
    int saved = errno;
    struct cffi_tls_s *tls = get_cffi_tls();
    if (tls != NULL)
        tls->saved_errno = saved;
}

static void restore_errno_only(void)
{
    struct cffi_tls_s *tls = get_cffi_tls();
    if (tls != NULL)
        errno = tls->saved_errno;
}

#endif


/* MESS.  We can't use PyThreadState_GET(), because that calls
   PyThreadState_Get() which fails an assert if the result is NULL.
   
   * in Python 2.7 and <= 3.4, the variable _PyThreadState_Current
     is directly available, so use that.

   * in Python 3.5, the variable is available too, but it might be
     the case that the headers don't define it (this changed in 3.5.1).
     In case we're compiling with 3.5.x with x >= 1, we need to
     manually define this variable.

   * in Python >= 3.6 there is _PyThreadState_UncheckedGet().
     It was added in 3.5.2 but should never be used in 3.5.x
     because it is not available in 3.5.0 or 3.5.1.
*/
#if PY_VERSION_HEX >= 0x03050100 && PY_VERSION_HEX < 0x03060000
PyAPI_DATA(void *volatile) _PyThreadState_Current;
#endif

static PyThreadState *get_current_ts(void)
{
#if PY_VERSION_HEX >= 0x030D0000
    return PyThreadState_GetUnchecked();
#elif PY_VERSION_HEX >= 0x03060000
    return _PyThreadState_UncheckedGet();
#elif defined(_Py_atomic_load_relaxed)
    return (PyThreadState*)_Py_atomic_load_relaxed(&_PyThreadState_Current);
#else
    return (PyThreadState*)_PyThreadState_Current;  /* assume atomic read */
#endif
}

static PyGILState_STATE gil_ensure(void)
{
    /* Called at the start of a callback.  Replacement for
       PyGILState_Ensure().
    */
    PyGILState_STATE result;
    PyThreadState *ts = PyGILState_GetThisThreadState();
    //fprintf(stderr, "%p: gil_ensure(), tstate=%p, tls=%p\n", get_cffi_tls(), ts, get_cffi_tls());

    if (ts != NULL) {
        ts->gilstate_counter++;
        if (ts != get_current_ts()) {
            /* common case: 'ts' is our non-current thread state and
               we have to make it current and acquire the GIL */
            PyEval_RestoreThread(ts);
            //fprintf(stderr, "%p: gil_ensure(), tstate=%p MADE CURRENT\n", get_cffi_tls(), ts);
            return PyGILState_UNLOCKED;
        }
        else {
            //fprintf(stderr, "%p: gil_ensure(), tstate=%p ALREADY CURRENT\n", get_cffi_tls(), ts);
            return PyGILState_LOCKED;
        }
    }
    else {
        /* no thread state here so far. */
        result = PyGILState_Ensure();
        assert(result == PyGILState_UNLOCKED);

        ts = PyGILState_GetThisThreadState();
        //fprintf(stderr, "%p: gil_ensure(), made a new tstate=%p\n", get_cffi_tls(), ts);
        assert(ts != NULL);
        assert(ts == get_current_ts());
        assert(ts->gilstate_counter >= 1);

        /* Use the ThreadCanary mechanism to keep 'ts' alive until the
           thread really shuts down */
        thread_canary_register(ts);

        assert(ts == PyGILState_GetThisThreadState());
        return result;
    }
}

static void gil_release(PyGILState_STATE oldstate)
{
    //fprintf(stderr, "%p: gil_release(%d), tls=%p\n", get_cffi_tls(), (int)oldstate, get_cffi_tls());
    PyGILState_Release(oldstate);
}
