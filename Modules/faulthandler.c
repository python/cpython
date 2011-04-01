#include "Python.h"
#include "pythread.h"
#include <signal.h>
#include <object.h>
#include <frameobject.h>
#include <signal.h>

#ifdef WITH_THREAD
#  define FAULTHANDLER_LATER
#endif

#ifndef MS_WINDOWS
   /* register() is useless on Windows, because only SIGSEGV and SIGILL can be
      handled by the process, and these signals can only be used with enable(),
      not using register() */
#  define FAULTHANDLER_USER
#endif

/* Allocate at maximum 100 MB of the stack to raise the stack overflow */
#define STACK_OVERFLOW_MAX_SIZE (100*1024*1024)

#define PUTS(fd, str) write(fd, str, strlen(str))

#ifdef HAVE_SIGACTION
typedef struct sigaction _Py_sighandler_t;
#else
typedef PyOS_sighandler_t _Py_sighandler_t;
#endif

typedef struct {
    int signum;
    int enabled;
    const char* name;
    _Py_sighandler_t previous;
    int all_threads;
} fault_handler_t;

static struct {
    int enabled;
    PyObject *file;
    int fd;
    int all_threads;
} fatal_error = {0, NULL, -1, 0};

#ifdef FAULTHANDLER_LATER
static struct {
    PyObject *file;
    int fd;
    PY_TIMEOUT_T timeout_ms;   /* timeout in microseconds */
    int repeat;
    int running;
    PyInterpreterState *interp;
    int exit;
    /* released by parent thread when cancel request */
    PyThread_type_lock cancel_event;
    /* released by child thread when joined */
    PyThread_type_lock join_event;
} thread;
#endif

#ifdef FAULTHANDLER_USER
typedef struct {
    int enabled;
    PyObject *file;
    int fd;
    int all_threads;
    _Py_sighandler_t previous;
} user_signal_t;

static user_signal_t *user_signals;

/* the following macros come from Python: Modules/signalmodule.c */
#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#define NSIG 12
#endif
#ifndef NSIG
# if defined(_NSIG)
#  define NSIG _NSIG            /* For BSD/SysV */
# elif defined(_SIGMAX)
#  define NSIG (_SIGMAX + 1)    /* For QNX */
# elif defined(SIGMAX)
#  define NSIG (SIGMAX + 1)     /* For djgpp */
# else
#  define NSIG 64               /* Use a reasonable default value */
# endif
#endif

#endif /* FAULTHANDLER_USER */


static fault_handler_t faulthandler_handlers[] = {
#ifdef SIGBUS
    {SIGBUS, 0, "Bus error", },
#endif
#ifdef SIGILL
    {SIGILL, 0, "Illegal instruction", },
#endif
    {SIGFPE, 0, "Floating point exception", },
    /* define SIGSEGV at the end to make it the default choice if searching the
       handler fails in faulthandler_fatal_error() */
    {SIGSEGV, 0, "Segmentation fault", }
};
static const unsigned char faulthandler_nsignals = \
    sizeof(faulthandler_handlers) / sizeof(faulthandler_handlers[0]);

#ifdef HAVE_SIGALTSTACK
static stack_t stack;
#endif


/* Get the file descriptor of a file by calling its fileno() method and then
   call its flush() method.

   If file is NULL or Py_None, use sys.stderr as the new file.

   On success, return the new file and write the file descriptor into *p_fd.
   On error, return NULL. */

static PyObject*
faulthandler_get_fileno(PyObject *file, int *p_fd)
{
    PyObject *result;
    long fd_long;
    int fd;

    if (file == NULL || file == Py_None) {
        file = PySys_GetObject("stderr");
        if (file == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "unable to get sys.stderr");
            return NULL;
        }
    }

    result = PyObject_CallMethod(file, "fileno", "");
    if (result == NULL)
        return NULL;

    fd = -1;
    if (PyLong_Check(result)) {
        fd_long = PyLong_AsLong(result);
        if (0 <= fd_long && fd_long < INT_MAX)
            fd = (int)fd_long;
    }
    Py_DECREF(result);

    if (fd == -1) {
        PyErr_SetString(PyExc_RuntimeError,
                        "file.fileno() is not a valid file descriptor");
        return NULL;
    }

    result = PyObject_CallMethod(file, "flush", "");
    if (result != NULL)
        Py_DECREF(result);
    else {
        /* ignore flush() error */
        PyErr_Clear();
    }
    *p_fd = fd;
    return file;
}

static PyObject*
faulthandler_dump_traceback_py(PyObject *self,
                               PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"file", "all_threads", NULL};
    PyObject *file = NULL;
    int all_threads = 0;
    PyThreadState *tstate;
    const char *errmsg;
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|Oi:dump_traceback", kwlist,
        &file, &all_threads))
        return NULL;

    file = faulthandler_get_fileno(file, &fd);
    if (file == NULL)
        return NULL;

    /* The caller holds the GIL and so PyThreadState_Get() can be used */
    tstate = PyThreadState_Get();
    if (tstate == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to get the current thread state");
        return NULL;
    }

    if (all_threads) {
        errmsg = _Py_DumpTracebackThreads(fd, tstate->interp, tstate);
        if (errmsg != NULL) {
            PyErr_SetString(PyExc_RuntimeError, errmsg);
            return NULL;
        }
    }
    else {
        _Py_DumpTraceback(fd, tstate);
    }
    Py_RETURN_NONE;
}


/* Handler of SIGSEGV, SIGFPE, SIGBUS and SIGILL signals.

   Display the current Python traceback, restore the previous handler and call
   the previous handler.

   On Windows, don't call explictly the previous handler, because Windows
   signal handler would not be called (for an unknown reason). The execution of
   the program continues at faulthandler_fatal_error() exit, but the same
   instruction will raise the same fault (signal), and so the previous handler
   will be called.

   This function is signal safe and should only call signal safe functions. */

static void
faulthandler_fatal_error(
    int signum
#ifdef HAVE_SIGACTION
    , siginfo_t *siginfo, void *ucontext
#endif
)
{
    const int fd = fatal_error.fd;
    unsigned int i;
    fault_handler_t *handler = NULL;
    PyThreadState *tstate;

    if (!fatal_error.enabled)
        return;

    for (i=0; i < faulthandler_nsignals; i++) {
        handler = &faulthandler_handlers[i];
        if (handler->signum == signum)
            break;
    }
    if (handler == NULL) {
        /* faulthandler_nsignals == 0 (unlikely) */
        return;
    }

    /* restore the previous handler */
#ifdef HAVE_SIGACTION
    (void)sigaction(handler->signum, &handler->previous, NULL);
#else
    (void)signal(handler->signum, handler->previous);
#endif
    handler->enabled = 0;

    PUTS(fd, "Fatal Python error: ");
    PUTS(fd, handler->name);
    PUTS(fd, "\n\n");

    /* SIGSEGV, SIGFPE, SIGBUS and SIGILL are synchronous signals and so are
       delivered to the thread that caused the fault. Get the Python thread
       state of the current thread.

       PyThreadState_Get() doesn't give the state of the thread that caused the
       fault if the thread released the GIL, and so this function cannot be
       used. Read the thread local storage (TLS) instead: call
       PyGILState_GetThisThreadState(). */
    tstate = PyGILState_GetThisThreadState();
    if (tstate == NULL)
        return;

    if (fatal_error.all_threads)
        _Py_DumpTracebackThreads(fd, tstate->interp, tstate);
    else
        _Py_DumpTraceback(fd, tstate);

#ifndef MS_WINDOWS
    /* call the previous signal handler: it is called if we use sigaction()
       thanks to SA_NODEFER flag, otherwise it is deferred */
    raise(signum);
#else
    /* on Windows, don't call explictly the previous handler, because Windows
       signal handler would not be called */
#endif
}

/* Install handler for fatal signals (SIGSEGV, SIGFPE, ...). */

static PyObject*
faulthandler_enable(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"file", "all_threads", NULL};
    PyObject *file = NULL;
    int all_threads = 0;
    unsigned int i;
    fault_handler_t *handler;
#ifdef HAVE_SIGACTION
    struct sigaction action;
#endif
    int err;
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|Oi:enable", kwlist, &file, &all_threads))
        return NULL;

    file = faulthandler_get_fileno(file, &fd);
    if (file == NULL)
        return NULL;

    Py_XDECREF(fatal_error.file);
    Py_INCREF(file);
    fatal_error.file = file;
    fatal_error.fd = fd;
    fatal_error.all_threads = all_threads;

    if (!fatal_error.enabled) {
        fatal_error.enabled = 1;

        for (i=0; i < faulthandler_nsignals; i++) {
            handler = &faulthandler_handlers[i];
#ifdef HAVE_SIGACTION
            action.sa_sigaction = faulthandler_fatal_error;
            sigemptyset(&action.sa_mask);
            /* Do not prevent the signal from being received from within
               its own signal handler */
            action.sa_flags = SA_NODEFER;
#ifdef HAVE_SIGALTSTACK
            if (stack.ss_sp != NULL) {
                /* Call the signal handler on an alternate signal stack
                   provided by sigaltstack() */
                action.sa_flags |= SA_ONSTACK;
            }
#endif
            err = sigaction(handler->signum, &action, &handler->previous);
#else
            handler->previous = signal(handler->signum,
                                       faulthandler_fatal_error);
            err = (handler->previous == SIG_ERR);
#endif
            if (err) {
                PyErr_SetFromErrno(PyExc_RuntimeError);
                return NULL;
            }
            handler->enabled = 1;
        }
    }
    Py_RETURN_NONE;
}

static void
faulthandler_disable(void)
{
    unsigned int i;
    fault_handler_t *handler;

    if (fatal_error.enabled) {
        fatal_error.enabled = 0;
        for (i=0; i < faulthandler_nsignals; i++) {
            handler = &faulthandler_handlers[i];
            if (!handler->enabled)
                continue;
#ifdef HAVE_SIGACTION
            (void)sigaction(handler->signum, &handler->previous, NULL);
#else
            (void)signal(handler->signum, handler->previous);
#endif
            handler->enabled = 0;
        }
    }

    Py_CLEAR(fatal_error.file);
}

static PyObject*
faulthandler_disable_py(PyObject *self)
{
    if (!fatal_error.enabled) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    faulthandler_disable();
    Py_INCREF(Py_True);
    return Py_True;
}

static PyObject*
faulthandler_is_enabled(PyObject *self)
{
    return PyBool_FromLong(fatal_error.enabled);
}

#ifdef FAULTHANDLER_LATER

static void
faulthandler_thread(void *unused)
{
    PyLockStatus st;
    const char* errmsg;
    PyThreadState *current;
    int ok;

    do {
        st = PyThread_acquire_lock_timed(thread.cancel_event,
                                         thread.timeout_ms, 0);
        if (st == PY_LOCK_ACQUIRED) {
            /* Cancelled by user */
            break;
        }
        /* Timeout => dump traceback */
        assert(st == PY_LOCK_FAILURE);

        /* get the thread holding the GIL, NULL if no thread hold the GIL */
        current = _Py_atomic_load_relaxed(&_PyThreadState_Current);

        errmsg = _Py_DumpTracebackThreads(thread.fd, thread.interp, current);
        ok = (errmsg == NULL);

        if (thread.exit)
            _exit(1);
    } while (ok && thread.repeat);

    /* The only way out */
    PyThread_release_lock(thread.cancel_event);
    PyThread_release_lock(thread.join_event);
}

static void
faulthandler_cancel_dump_tracebacks_later(void)
{
    if (thread.running) {
        /* Notify cancellation */
        PyThread_release_lock(thread.cancel_event);
    }
    /* Wait for thread to join */
    PyThread_acquire_lock(thread.join_event, 1);
    PyThread_release_lock(thread.join_event);
    thread.running = 0;
    Py_CLEAR(thread.file);
}

static PyObject*
faulthandler_dump_traceback_later(PyObject *self,
                                  PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"timeout", "repeat", "file", "exit", NULL};
    double timeout;
    PY_TIMEOUT_T timeout_ms;
    int repeat = 0;
    PyObject *file = NULL;
    int fd;
    int exit = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "d|iOi:dump_tracebacks_later", kwlist,
        &timeout, &repeat, &file, &exit))
        return NULL;
    timeout *= 1e6;
    if (timeout >= (double) PY_TIMEOUT_MAX) {
        PyErr_SetString(PyExc_OverflowError,  "timeout value is too large");
        return NULL;
    }
    timeout_ms = (PY_TIMEOUT_T)timeout;
    if (timeout_ms <= 0) {
        PyErr_SetString(PyExc_ValueError, "timeout must be greater than 0");
        return NULL;
    }

    file = faulthandler_get_fileno(file, &fd);
    if (file == NULL)
        return NULL;

    /* Cancel previous thread, if running */
    faulthandler_cancel_dump_tracebacks_later();

    Py_XDECREF(thread.file);
    Py_INCREF(file);
    thread.file = file;
    thread.fd = fd;
    thread.timeout_ms = timeout_ms;
    thread.repeat = repeat;
    thread.interp = PyThreadState_Get()->interp;
    thread.exit = exit;

    /* Arm these locks to serve as events when released */
    PyThread_acquire_lock(thread.join_event, 1);
    PyThread_acquire_lock(thread.cancel_event, 1);

    thread.running = 1;
    if (PyThread_start_new_thread(faulthandler_thread, NULL) == -1) {
        thread.running = 0;
        PyThread_release_lock(thread.join_event);
        PyThread_release_lock(thread.cancel_event);
        Py_CLEAR(thread.file);
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to start watchdog thread");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
faulthandler_cancel_dump_tracebacks_later_py(PyObject *self)
{
    faulthandler_cancel_dump_tracebacks_later();
    Py_RETURN_NONE;
}
#endif /* FAULTHANDLER_LATER */

#ifdef FAULTHANDLER_USER
/* Handler of user signals (e.g. SIGUSR1).

   Dump the traceback of the current thread, or of all threads if
   thread.all_threads is true.

   This function is signal safe and should only call signal safe functions. */

static void
faulthandler_user(int signum)
{
    user_signal_t *user;
    PyThreadState *tstate;

    user = &user_signals[signum];
    if (!user->enabled)
        return;

    /* PyThreadState_Get() doesn't give the state of the current thread if
       the thread doesn't hold the GIL. Read the thread local storage (TLS)
       instead: call PyGILState_GetThisThreadState(). */
    tstate = PyGILState_GetThisThreadState();
    if (tstate == NULL) {
        /* unable to get the current thread, do nothing */
        return;
    }

    if (user->all_threads)
        _Py_DumpTracebackThreads(user->fd, tstate->interp, tstate);
    else
        _Py_DumpTraceback(user->fd, tstate);
}

static PyObject*
faulthandler_register(PyObject *self,
                      PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"signum", "file", "all_threads", NULL};
    int signum;
    PyObject *file = NULL;
    int all_threads = 0;
    int fd;
    unsigned int i;
    user_signal_t *user;
    _Py_sighandler_t previous;
#ifdef HAVE_SIGACTION
    struct sigaction action;
#endif
    int err;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i|Oi:register", kwlist,
        &signum, &file, &all_threads))
        return NULL;

    if (signum < 1 || NSIG <= signum) {
        PyErr_SetString(PyExc_ValueError, "signal number out of range");
        return NULL;
    }

    for (i=0; i < faulthandler_nsignals; i++) {
        if (faulthandler_handlers[i].signum == signum) {
            PyErr_Format(PyExc_RuntimeError,
                         "signal %i cannot be registered by register(), "
                         "use enable() instead",
                         signum);
            return NULL;
        }
    }

    file = faulthandler_get_fileno(file, &fd);
    if (file == NULL)
        return NULL;

    if (user_signals == NULL) {
        user_signals = calloc(NSIG, sizeof(user_signal_t));
        if (user_signals == NULL)
            return PyErr_NoMemory();
    }
    user = &user_signals[signum];

    if (!user->enabled) {
#ifdef HAVE_SIGACTION
        action.sa_handler = faulthandler_user;
        sigemptyset(&action.sa_mask);
        /* if the signal is received while the kernel is executing a system
           call, try to restart the system call instead of interrupting it and
           return EINTR */
        action.sa_flags = SA_RESTART;
#ifdef HAVE_SIGALTSTACK
        if (stack.ss_sp != NULL) {
            /* Call the signal handler on an alternate signal stack
               provided by sigaltstack() */
            action.sa_flags |= SA_ONSTACK;
        }
#endif
        err = sigaction(signum, &action, &previous);
#else
        previous = signal(signum, faulthandler_user);
        err = (previous == SIG_ERR);
#endif
        if (err) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
    }

    Py_XDECREF(user->file);
    Py_INCREF(file);
    user->file = file;
    user->fd = fd;
    user->all_threads = all_threads;
    user->previous = previous;
    user->enabled = 1;

    Py_RETURN_NONE;
}

static int
faulthandler_unregister(user_signal_t *user, int signum)
{
    if (user->enabled)
        return 0;
    user->enabled = 0;
#ifdef HAVE_SIGACTION
    (void)sigaction(signum, &user->previous, NULL);
#else
    (void)signal(signum, user->previous);
#endif
    Py_CLEAR(user->file);
    user->fd = -1;
    return 1;
}

static PyObject*
faulthandler_unregister_py(PyObject *self, PyObject *args)
{
    int signum;
    user_signal_t *user;
    int change;

    if (!PyArg_ParseTuple(args, "i:unregister", &signum))
        return NULL;

    if (signum < 1 || NSIG <= signum) {
        PyErr_SetString(PyExc_ValueError, "signal number out of range");
        return NULL;
    }

    user = &user_signals[signum];
    change = faulthandler_unregister(user, signum);
    return PyBool_FromLong(change);
}
#endif   /* FAULTHANDLER_USER */


static PyObject *
faulthandler_read_null(PyObject *self, PyObject *args)
{
    int *x = NULL, y;
    int release_gil = 0;
    if (!PyArg_ParseTuple(args, "|i:_read_null", &release_gil))
        return NULL;
    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        y = *x;
        Py_END_ALLOW_THREADS
    } else
        y = *x;
    return PyLong_FromLong(y);

}

static PyObject *
faulthandler_sigsegv(PyObject *self, PyObject *args)
{
#if defined(MS_WINDOWS)
    /* faulthandler_fatal_error() restores the previous signal handler and then
       gives back the execution flow to the program. In a normal case, the
       SIGSEGV was raised by the kernel because of a fault, and so if the
       program retries to execute the same instruction, the fault will be
       raised again.

       Here the fault is simulated by a fake SIGSEGV signal raised by the
       application. We have to raise SIGSEGV at lease twice: once for
       faulthandler_fatal_error(), and one more time for the previous signal
       handler. */
    while(1)
        raise(SIGSEGV);
#else
    raise(SIGSEGV);
#endif
    Py_RETURN_NONE;
}

static PyObject *
faulthandler_sigfpe(PyObject *self, PyObject *args)
{
    /* Do an integer division by zero: raise a SIGFPE on Intel CPU, but not on
       PowerPC. Use volatile to disable compile-time optimizations. */
    volatile int x = 1, y = 0, z;
    z = x / y;
    /* if the division by zero didn't raise a SIGFPE, raise it manually */
    raise(SIGFPE);
    Py_RETURN_NONE;
}

#ifdef SIGBUS
static PyObject *
faulthandler_sigbus(PyObject *self, PyObject *args)
{
    raise(SIGBUS);
    Py_RETURN_NONE;
}
#endif

#ifdef SIGILL
static PyObject *
faulthandler_sigill(PyObject *self, PyObject *args)
{
#if defined(MS_WINDOWS)
    /* see faulthandler_sigsegv() for the explanation about while(1) */
    while(1)
        raise(SIGILL);
#else
    raise(SIGILL);
#endif
    Py_RETURN_NONE;
}
#endif

static PyObject *
faulthandler_fatal_error_py(PyObject *self, PyObject *args)
{
    char *message;
    if (!PyArg_ParseTuple(args, "y:fatal_error", &message))
        return NULL;
    Py_FatalError(message);
    Py_RETURN_NONE;
}

#if defined(HAVE_SIGALTSTACK) && defined(HAVE_SIGACTION)
void*
stack_overflow(void *min_sp, void *max_sp, size_t *depth)
{
    /* allocate 4096 bytes on the stack at each call */
    unsigned char buffer[4096];
    void *sp = &buffer;
    *depth += 1;
    if (sp < min_sp || max_sp < sp)
        return sp;
    buffer[0] = 1;
    buffer[4095] = 0;
    return stack_overflow(min_sp, max_sp, depth);
}

static PyObject *
faulthandler_stack_overflow(PyObject *self)
{
    size_t depth, size;
    void *sp = &depth, *stop;

    depth = 0;
    stop = stack_overflow(sp - STACK_OVERFLOW_MAX_SIZE,
                          sp + STACK_OVERFLOW_MAX_SIZE,
                          &depth);
    if (sp < stop)
        size = stop - sp;
    else
        size = sp - stop;
    PyErr_Format(PyExc_RuntimeError,
        "unable to raise a stack overflow (allocated %zu bytes "
        "on the stack, %zu recursive calls)",
        size, depth);
    return NULL;
}
#endif


static int
faulthandler_traverse(PyObject *module, visitproc visit, void *arg)
{
#ifdef FAULTHANDLER_USER
    unsigned int index;
#endif

#ifdef FAULTHANDLER_LATER
    Py_VISIT(thread.file);
#endif
#ifdef FAULTHANDLER_USER
    if (user_signals != NULL) {
        for (index=0; index < NSIG; index++)
            Py_VISIT(user_signals[index].file);
    }
#endif
    Py_VISIT(fatal_error.file);
    return 0;
}

PyDoc_STRVAR(module_doc,
"faulthandler module.");

static PyMethodDef module_methods[] = {
    {"enable",
     (PyCFunction)faulthandler_enable, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("enable(file=sys.stderr, all_threads=False): "
               "enable the fault handler")},
    {"disable", (PyCFunction)faulthandler_disable_py, METH_NOARGS,
     PyDoc_STR("disable(): disable the fault handler")},
    {"is_enabled", (PyCFunction)faulthandler_is_enabled, METH_NOARGS,
     PyDoc_STR("is_enabled()->bool: check if the handler is enabled")},
    {"dump_traceback",
     (PyCFunction)faulthandler_dump_traceback_py, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("dump_traceback(file=sys.stderr, all_threads=False): "
               "dump the traceback of the current thread, or of all threads "
               "if all_threads is True, into file")},
#ifdef FAULTHANDLER_LATER
    {"dump_tracebacks_later",
     (PyCFunction)faulthandler_dump_traceback_later, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("dump_tracebacks_later(timeout, repeat=False, file=sys.stderr):\n"
               "dump the traceback of all threads in timeout seconds,\n"
               "or each timeout seconds if repeat is True.")},
    {"cancel_dump_tracebacks_later",
     (PyCFunction)faulthandler_cancel_dump_tracebacks_later_py, METH_NOARGS,
     PyDoc_STR("cancel_dump_tracebacks_later():\ncancel the previous call "
               "to dump_tracebacks_later().")},
#endif

#ifdef FAULTHANDLER_USER
    {"register",
     (PyCFunction)faulthandler_register, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("register(signum, file=sys.stderr, all_threads=False): "
               "register an handler for the signal 'signum': dump the "
               "traceback of the current thread, or of all threads if "
               "all_threads is True, into file")},
    {"unregister",
     faulthandler_unregister_py, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("unregister(signum): unregister the handler of the signal "
                "'signum' registered by register()")},
#endif

    {"_read_null", faulthandler_read_null, METH_VARARGS,
     PyDoc_STR("_read_null(release_gil=False): read from NULL, raise "
               "a SIGSEGV or SIGBUS signal depending on the platform")},
    {"_sigsegv", faulthandler_sigsegv, METH_VARARGS,
     PyDoc_STR("_sigsegv(): raise a SIGSEGV signal")},
    {"_sigfpe", (PyCFunction)faulthandler_sigfpe, METH_NOARGS,
     PyDoc_STR("_sigfpe(): raise a SIGFPE signal")},
#ifdef SIGBUS
    {"_sigbus", (PyCFunction)faulthandler_sigbus, METH_NOARGS,
     PyDoc_STR("_sigbus(): raise a SIGBUS signal")},
#endif
#ifdef SIGILL
    {"_sigill", (PyCFunction)faulthandler_sigill, METH_NOARGS,
     PyDoc_STR("_sigill(): raise a SIGILL signal")},
#endif
    {"_fatal_error", faulthandler_fatal_error_py, METH_VARARGS,
     PyDoc_STR("_fatal_error(message): call Py_FatalError(message)")},
#if defined(HAVE_SIGALTSTACK) && defined(HAVE_SIGACTION)
    {"_stack_overflow", (PyCFunction)faulthandler_stack_overflow, METH_NOARGS,
     PyDoc_STR("_stack_overflow(): recursive call to raise a stack overflow")},
#endif
    {NULL, NULL} /* terminator */
};

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "faulthandler",
    module_doc,
    0, /* non negative size to be able to unload the module */
    module_methods,
    NULL,
    faulthandler_traverse,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_faulthandler(void)
{
    return PyModule_Create(&module_def);
}

/* Call faulthandler.enable() if PYTHONFAULTHANDLER environment variable is
   defined, or if sys._xoptions has a 'faulthandler' key. */

static int
faulthandler_env_options(void)
{
    PyObject *xoptions, *key, *module, *res;
    int enable;

    if (!Py_GETENV("PYTHONFAULTHANDLER")) {
        xoptions = PySys_GetXOptions();
        if (xoptions == NULL)
            return -1;

        key = PyUnicode_FromString("faulthandler");
        if (key == NULL)
            return -1;

        enable = PyDict_Contains(xoptions, key);
        Py_DECREF(key);
        if (!enable)
            return 0;
    }
    else
        enable = 1;

    module = PyImport_ImportModule("faulthandler");
    if (module == NULL) {
        return -1;
    }
    res = PyObject_CallMethod(module, "enable", "");
    Py_DECREF(module);
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

int _PyFaulthandler_Init(void)
{
#ifdef HAVE_SIGALTSTACK
    int err;

    /* Try to allocate an alternate stack for faulthandler() signal handler to
     * be able to allocate memory on the stack, even on a stack overflow. If it
     * fails, ignore the error. */
    stack.ss_flags = 0;
    stack.ss_size = SIGSTKSZ;
    stack.ss_sp = PyMem_Malloc(stack.ss_size);
    if (stack.ss_sp != NULL) {
        err = sigaltstack(&stack, NULL);
        if (err) {
            PyMem_Free(stack.ss_sp);
            stack.ss_sp = NULL;
        }
    }
#endif
#ifdef FAULTHANDLER_LATER
    thread.running = 0;
    thread.file = NULL;
    thread.cancel_event = PyThread_allocate_lock();
    thread.join_event = PyThread_allocate_lock();
    if (!thread.cancel_event || !thread.join_event) {
        PyErr_SetString(PyExc_RuntimeError,
                        "could not allocate locks for faulthandler");
        return -1;
    }
#endif

    return faulthandler_env_options();
}

void _PyFaulthandler_Fini(void)
{
#ifdef FAULTHANDLER_USER
    unsigned int i;
#endif

#ifdef FAULTHANDLER_LATER
    /* later */
    faulthandler_cancel_dump_tracebacks_later();
    if (thread.cancel_event) {
        PyThread_free_lock(thread.cancel_event);
        thread.cancel_event = NULL;
    }
    if (thread.join_event) {
        PyThread_free_lock(thread.join_event);
        thread.join_event = NULL;
    }
#endif

#ifdef FAULTHANDLER_USER
    /* user */
    if (user_signals != NULL) {
        for (i=0; i < NSIG; i++)
            faulthandler_unregister(&user_signals[i], i+1);
        free(user_signals);
        user_signals = NULL;
    }
#endif

    /* fatal */
    faulthandler_disable();
#ifdef HAVE_SIGALTSTACK
    if (stack.ss_sp != NULL) {
        PyMem_Free(stack.ss_sp);
        stack.ss_sp = NULL;
    }
#endif
}
