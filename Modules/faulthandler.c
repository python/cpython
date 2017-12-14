#include "Python.h"
#include "pythread.h"
#include <signal.h>
#include <object.h>
#include <frameobject.h>
#include <signal.h>
#if defined(HAVE_PTHREAD_SIGMASK) && !defined(HAVE_BROKEN_PTHREAD_SIGMASK)
#  include <pthread.h>
#endif
#ifdef MS_WINDOWS
#  include <windows.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#  include <sys/resource.h>
#endif

/* Allocate at maximum 100 MB of the stack to raise the stack overflow */
#define STACK_OVERFLOW_MAX_SIZE (100*1024*1024)

#ifdef WITH_THREAD
#  define FAULTHANDLER_LATER
#endif

#ifndef MS_WINDOWS
   /* register() is useless on Windows, because only SIGSEGV, SIGABRT and
      SIGILL can be handled by the process, and these signals can only be used
      with enable(), not using register() */
#  define FAULTHANDLER_USER
#endif

#define PUTS(fd, str) _Py_write_noraise(fd, str, strlen(str))

_Py_IDENTIFIER(enable);
_Py_IDENTIFIER(fileno);
_Py_IDENTIFIER(flush);
_Py_IDENTIFIER(stderr);

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
    PyInterpreterState *interp;
#ifdef MS_WINDOWS
    void *exc_handler;
#endif
} fatal_error = {0, NULL, -1, 0};

#ifdef FAULTHANDLER_LATER
static struct {
    PyObject *file;
    int fd;
    PY_TIMEOUT_T timeout_us;   /* timeout in microseconds */
    int repeat;
    PyInterpreterState *interp;
    int exit;
    char *header;
    size_t header_len;
    /* The main thread always holds this lock. It is only released when
       faulthandler_thread() is interrupted before this thread exits, or at
       Python exit. */
    PyThread_type_lock cancel_event;
    /* released by child thread when joined */
    PyThread_type_lock running;
} thread;
#endif

#ifdef FAULTHANDLER_USER
typedef struct {
    int enabled;
    PyObject *file;
    int fd;
    int all_threads;
    int chain;
    _Py_sighandler_t previous;
    PyInterpreterState *interp;
} user_signal_t;

static user_signal_t *user_signals;

/* the following macros come from Python: Modules/signalmodule.c */
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

static void faulthandler_user(int signum);
#endif /* FAULTHANDLER_USER */


static fault_handler_t faulthandler_handlers[] = {
#ifdef SIGBUS
    {SIGBUS, 0, "Bus error", },
#endif
#ifdef SIGILL
    {SIGILL, 0, "Illegal instruction", },
#endif
    {SIGFPE, 0, "Floating point exception", },
    {SIGABRT, 0, "Aborted", },
    /* define SIGSEGV at the end to make it the default choice if searching the
       handler fails in faulthandler_fatal_error() */
    {SIGSEGV, 0, "Segmentation fault", }
};
static const size_t faulthandler_nsignals = \
    Py_ARRAY_LENGTH(faulthandler_handlers);

#ifdef HAVE_SIGALTSTACK
static stack_t stack;
static stack_t old_stack;
#endif


/* Get the file descriptor of a file by calling its fileno() method and then
   call its flush() method.

   If file is NULL or Py_None, use sys.stderr as the new file.
   If file is an integer, it will be treated as file descriptor.

   On success, return the file descriptor and write the new file into *file_ptr.
   On error, return -1. */

static int
faulthandler_get_fileno(PyObject **file_ptr)
{
    PyObject *result;
    long fd_long;
    int fd;
    PyObject *file = *file_ptr;

    if (file == NULL || file == Py_None) {
        file = _PySys_GetObjectId(&PyId_stderr);
        if (file == NULL) {
            PyErr_SetString(PyExc_RuntimeError, "unable to get sys.stderr");
            return -1;
        }
        if (file == Py_None) {
            PyErr_SetString(PyExc_RuntimeError, "sys.stderr is None");
            return -1;
        }
    }
    else if (PyLong_Check(file)) {
        fd = _PyLong_AsInt(file);
        if (fd == -1 && PyErr_Occurred())
            return -1;
        if (fd < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "file is not a valid file descripter");
            return -1;
        }
        *file_ptr = NULL;
        return fd;
    }

    result = _PyObject_CallMethodId(file, &PyId_fileno, NULL);
    if (result == NULL)
        return -1;

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
        return -1;
    }

    result = _PyObject_CallMethodId(file, &PyId_flush, NULL);
    if (result != NULL)
        Py_DECREF(result);
    else {
        /* ignore flush() error */
        PyErr_Clear();
    }
    *file_ptr = file;
    return fd;
}

/* Get the state of the current thread: only call this function if the current
   thread holds the GIL. Raise an exception on error. */
static PyThreadState*
get_thread_state(void)
{
    PyThreadState *tstate = _PyThreadState_UncheckedGet();
    if (tstate == NULL) {
        /* just in case but very unlikely... */
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to get the current thread state");
        return NULL;
    }
    return tstate;
}

static void
faulthandler_dump_traceback(int fd, int all_threads,
                            PyInterpreterState *interp)
{
    static volatile int reentrant = 0;
    PyThreadState *tstate;

    if (reentrant)
        return;

    reentrant = 1;

#ifdef WITH_THREAD
    /* SIGSEGV, SIGFPE, SIGABRT, SIGBUS and SIGILL are synchronous signals and
       are thus delivered to the thread that caused the fault. Get the Python
       thread state of the current thread.

       PyThreadState_Get() doesn't give the state of the thread that caused the
       fault if the thread released the GIL, and so this function cannot be
       used. Read the thread local storage (TLS) instead: call
       PyGILState_GetThisThreadState(). */
    tstate = PyGILState_GetThisThreadState();
#else
    tstate = _PyThreadState_UncheckedGet();
#endif

    if (all_threads) {
        (void)_Py_DumpTracebackThreads(fd, NULL, tstate);
    }
    else {
        if (tstate != NULL)
            _Py_DumpTraceback(fd, tstate);
    }

    reentrant = 0;
}

static PyObject*
faulthandler_dump_traceback_py(PyObject *self,
                               PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"file", "all_threads", NULL};
    PyObject *file = NULL;
    int all_threads = 1;
    PyThreadState *tstate;
    const char *errmsg;
    int fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|Oi:dump_traceback", kwlist,
        &file, &all_threads))
        return NULL;

    fd = faulthandler_get_fileno(&file);
    if (fd < 0)
        return NULL;

    tstate = get_thread_state();
    if (tstate == NULL)
        return NULL;

    if (all_threads) {
        errmsg = _Py_DumpTracebackThreads(fd, NULL, tstate);
        if (errmsg != NULL) {
            PyErr_SetString(PyExc_RuntimeError, errmsg);
            return NULL;
        }
    }
    else {
        _Py_DumpTraceback(fd, tstate);
    }

    if (PyErr_CheckSignals())
        return NULL;

    Py_RETURN_NONE;
}

static void
faulthandler_disable_fatal_handler(fault_handler_t *handler)
{
    if (!handler->enabled)
        return;
    handler->enabled = 0;
#ifdef HAVE_SIGACTION
    (void)sigaction(handler->signum, &handler->previous, NULL);
#else
    (void)signal(handler->signum, handler->previous);
#endif
}


/* Handler for SIGSEGV, SIGFPE, SIGABRT, SIGBUS and SIGILL signals.

   Display the current Python traceback, restore the previous handler and call
   the previous handler.

   On Windows, don't explicitly call the previous handler, because the Windows
   signal handler would not be called (for an unknown reason). The execution of
   the program continues at faulthandler_fatal_error() exit, but the same
   instruction will raise the same fault (signal), and so the previous handler
   will be called.

   This function is signal-safe and should only call signal-safe functions. */

static void
faulthandler_fatal_error(int signum)
{
    const int fd = fatal_error.fd;
    size_t i;
    fault_handler_t *handler = NULL;
    int save_errno = errno;

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
    faulthandler_disable_fatal_handler(handler);

    PUTS(fd, "Fatal Python error: ");
    PUTS(fd, handler->name);
    PUTS(fd, "\n\n");

    faulthandler_dump_traceback(fd, fatal_error.all_threads,
                                fatal_error.interp);

    errno = save_errno;
#ifdef MS_WINDOWS
    if (signum == SIGSEGV) {
        /* don't explicitly call the previous handler for SIGSEGV in this signal
           handler, because the Windows signal handler would not be called */
        return;
    }
#endif
    /* call the previous signal handler: it is called immediately if we use
       sigaction() thanks to SA_NODEFER flag, otherwise it is deferred */
    raise(signum);
}

#ifdef MS_WINDOWS
static int
faulthandler_ignore_exception(DWORD code)
{
    /* bpo-30557: ignore exceptions which are not errors */
    if (!(code & 0x80000000)) {
        return 1;
    }
    /* bpo-31701: ignore MSC and COM exceptions
       E0000000 + code */
    if (code == 0xE06D7363 /* MSC exception ("Emsc") */
        || code == 0xE0434352 /* COM Callable Runtime exception ("ECCR") */) {
        return 1;
    }
    /* Interesting exception: log it with the Python traceback */
    return 0;
}

static LONG WINAPI
faulthandler_exc_handler(struct _EXCEPTION_POINTERS *exc_info)
{
    const int fd = fatal_error.fd;
    DWORD code = exc_info->ExceptionRecord->ExceptionCode;
    DWORD flags = exc_info->ExceptionRecord->ExceptionFlags;

    if (faulthandler_ignore_exception(code)) {
        /* ignore the exception: call the next exception handler */
        return EXCEPTION_CONTINUE_SEARCH;
    }

    PUTS(fd, "Windows fatal exception: ");
    switch (code)
    {
    /* only format most common errors */
    case EXCEPTION_ACCESS_VIOLATION: PUTS(fd, "access violation"); break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: PUTS(fd, "float divide by zero"); break;
    case EXCEPTION_FLT_OVERFLOW: PUTS(fd, "float overflow"); break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO: PUTS(fd, "int divide by zero"); break;
    case EXCEPTION_INT_OVERFLOW: PUTS(fd, "integer overflow"); break;
    case EXCEPTION_IN_PAGE_ERROR: PUTS(fd, "page error"); break;
    case EXCEPTION_STACK_OVERFLOW: PUTS(fd, "stack overflow"); break;
    default:
        PUTS(fd, "code 0x");
        _Py_DumpHexadecimal(fd, code, 8);
    }
    PUTS(fd, "\n\n");

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        /* disable signal handler for SIGSEGV */
        size_t i;
        for (i=0; i < faulthandler_nsignals; i++) {
            fault_handler_t *handler = &faulthandler_handlers[i];
            if (handler->signum == SIGSEGV) {
                faulthandler_disable_fatal_handler(handler);
                break;
            }
        }
    }

    faulthandler_dump_traceback(fd, fatal_error.all_threads,
                                fatal_error.interp);

    /* call the next exception handler */
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

/* Install the handler for fatal signals, faulthandler_fatal_error(). */

static int
faulthandler_enable(void)
{
    size_t i;

    if (fatal_error.enabled) {
        return 0;
    }
    fatal_error.enabled = 1;

    for (i=0; i < faulthandler_nsignals; i++) {
        fault_handler_t *handler;
#ifdef HAVE_SIGACTION
        struct sigaction action;
#endif
        int err;

        handler = &faulthandler_handlers[i];
        assert(!handler->enabled);
#ifdef HAVE_SIGACTION
        action.sa_handler = faulthandler_fatal_error;
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
            return -1;
        }

        handler->enabled = 1;
    }

#ifdef MS_WINDOWS
    assert(fatal_error.exc_handler == NULL);
    fatal_error.exc_handler = AddVectoredExceptionHandler(1, faulthandler_exc_handler);
#endif
    return 0;
}

static PyObject*
faulthandler_py_enable(PyObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"file", "all_threads", NULL};
    PyObject *file = NULL;
    int all_threads = 1;
    int fd;
    PyThreadState *tstate;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|Oi:enable", kwlist, &file, &all_threads))
        return NULL;

    fd = faulthandler_get_fileno(&file);
    if (fd < 0)
        return NULL;

    tstate = get_thread_state();
    if (tstate == NULL)
        return NULL;

    Py_XINCREF(file);
    Py_XSETREF(fatal_error.file, file);
    fatal_error.fd = fd;
    fatal_error.all_threads = all_threads;
    fatal_error.interp = tstate->interp;

    if (faulthandler_enable() < 0) {
        return NULL;
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
            faulthandler_disable_fatal_handler(handler);
        }
    }
#ifdef MS_WINDOWS
    if (fatal_error.exc_handler != NULL) {
        RemoveVectoredExceptionHandler(fatal_error.exc_handler);
        fatal_error.exc_handler = NULL;
    }
#endif
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
    int ok;
#if defined(HAVE_PTHREAD_SIGMASK) && !defined(HAVE_BROKEN_PTHREAD_SIGMASK)
    sigset_t set;

    /* we don't want to receive any signal */
    sigfillset(&set);
    pthread_sigmask(SIG_SETMASK, &set, NULL);
#endif

    do {
        st = PyThread_acquire_lock_timed(thread.cancel_event,
                                         thread.timeout_us, 0);
        if (st == PY_LOCK_ACQUIRED) {
            PyThread_release_lock(thread.cancel_event);
            break;
        }
        /* Timeout => dump traceback */
        assert(st == PY_LOCK_FAILURE);

        _Py_write_noraise(thread.fd, thread.header, (int)thread.header_len);

        errmsg = _Py_DumpTracebackThreads(thread.fd, thread.interp, NULL);
        ok = (errmsg == NULL);

        if (thread.exit)
            _exit(1);
    } while (ok && thread.repeat);

    /* The only way out */
    PyThread_release_lock(thread.running);
}

static void
cancel_dump_traceback_later(void)
{
    /* Notify cancellation */
    PyThread_release_lock(thread.cancel_event);

    /* Wait for thread to join */
    PyThread_acquire_lock(thread.running, 1);
    PyThread_release_lock(thread.running);

    /* The main thread should always hold the cancel_event lock */
    PyThread_acquire_lock(thread.cancel_event, 1);

    Py_CLEAR(thread.file);
    if (thread.header) {
        PyMem_Free(thread.header);
        thread.header = NULL;
    }
}

static char*
format_timeout(double timeout)
{
    unsigned long us, sec, min, hour;
    double intpart, fracpart;
    char buffer[100];

    fracpart = modf(timeout, &intpart);
    sec = (unsigned long)intpart;
    us = (unsigned long)(fracpart * 1e6);
    min = sec / 60;
    sec %= 60;
    hour = min / 60;
    min %= 60;

    if (us != 0)
        PyOS_snprintf(buffer, sizeof(buffer),
                      "Timeout (%lu:%02lu:%02lu.%06lu)!\n",
                      hour, min, sec, us);
    else
        PyOS_snprintf(buffer, sizeof(buffer),
                      "Timeout (%lu:%02lu:%02lu)!\n",
                      hour, min, sec);

    return _PyMem_Strdup(buffer);
}

static PyObject*
faulthandler_dump_traceback_later(PyObject *self,
                                   PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"timeout", "repeat", "file", "exit", NULL};
    double timeout;
    PY_TIMEOUT_T timeout_us;
    int repeat = 0;
    PyObject *file = NULL;
    int fd;
    int exit = 0;
    PyThreadState *tstate;
    char *header;
    size_t header_len;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "d|iOi:dump_traceback_later", kwlist,
        &timeout, &repeat, &file, &exit))
        return NULL;
    if ((timeout * 1e6) >= (double) PY_TIMEOUT_MAX) {
        PyErr_SetString(PyExc_OverflowError,  "timeout value is too large");
        return NULL;
    }
    timeout_us = (PY_TIMEOUT_T)(timeout * 1e6);
    if (timeout_us <= 0) {
        PyErr_SetString(PyExc_ValueError, "timeout must be greater than 0");
        return NULL;
    }

    tstate = get_thread_state();
    if (tstate == NULL)
        return NULL;

    fd = faulthandler_get_fileno(&file);
    if (fd < 0)
        return NULL;

    /* format the timeout */
    header = format_timeout(timeout);
    if (header == NULL)
        return PyErr_NoMemory();
    header_len = strlen(header);

    /* Cancel previous thread, if running */
    cancel_dump_traceback_later();

    Py_XINCREF(file);
    Py_XSETREF(thread.file, file);
    thread.fd = fd;
    thread.timeout_us = timeout_us;
    thread.repeat = repeat;
    thread.interp = tstate->interp;
    thread.exit = exit;
    thread.header = header;
    thread.header_len = header_len;

    /* Arm these locks to serve as events when released */
    PyThread_acquire_lock(thread.running, 1);

    if (PyThread_start_new_thread(faulthandler_thread, NULL) == -1) {
        PyThread_release_lock(thread.running);
        Py_CLEAR(thread.file);
        PyMem_Free(header);
        thread.header = NULL;
        PyErr_SetString(PyExc_RuntimeError,
                        "unable to start watchdog thread");
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyObject*
faulthandler_cancel_dump_traceback_later_py(PyObject *self)
{
    cancel_dump_traceback_later();
    Py_RETURN_NONE;
}
#endif  /* FAULTHANDLER_LATER */

#ifdef FAULTHANDLER_USER
static int
faulthandler_register(int signum, int chain, _Py_sighandler_t *p_previous)
{
#ifdef HAVE_SIGACTION
    struct sigaction action;
    action.sa_handler = faulthandler_user;
    sigemptyset(&action.sa_mask);
    /* if the signal is received while the kernel is executing a system
       call, try to restart the system call instead of interrupting it and
       return EINTR. */
    action.sa_flags = SA_RESTART;
    if (chain) {
        /* do not prevent the signal from being received from within its
           own signal handler */
        action.sa_flags = SA_NODEFER;
    }
#ifdef HAVE_SIGALTSTACK
    if (stack.ss_sp != NULL) {
        /* Call the signal handler on an alternate signal stack
           provided by sigaltstack() */
        action.sa_flags |= SA_ONSTACK;
    }
#endif
    return sigaction(signum, &action, p_previous);
#else
    _Py_sighandler_t previous;
    previous = signal(signum, faulthandler_user);
    if (p_previous != NULL)
        *p_previous = previous;
    return (previous == SIG_ERR);
#endif
}

/* Handler of user signals (e.g. SIGUSR1).

   Dump the traceback of the current thread, or of all threads if
   thread.all_threads is true.

   This function is signal safe and should only call signal safe functions. */

static void
faulthandler_user(int signum)
{
    user_signal_t *user;
    int save_errno = errno;

    user = &user_signals[signum];
    if (!user->enabled)
        return;

    faulthandler_dump_traceback(user->fd, user->all_threads, user->interp);

#ifdef HAVE_SIGACTION
    if (user->chain) {
        (void)sigaction(signum, &user->previous, NULL);
        errno = save_errno;

        /* call the previous signal handler */
        raise(signum);

        save_errno = errno;
        (void)faulthandler_register(signum, user->chain, NULL);
        errno = save_errno;
    }
#else
    if (user->chain) {
        errno = save_errno;
        /* call the previous signal handler */
        user->previous(signum);
    }
#endif
}

static int
check_signum(int signum)
{
    unsigned int i;

    for (i=0; i < faulthandler_nsignals; i++) {
        if (faulthandler_handlers[i].signum == signum) {
            PyErr_Format(PyExc_RuntimeError,
                         "signal %i cannot be registered, "
                         "use enable() instead",
                         signum);
            return 0;
        }
    }
    if (signum < 1 || NSIG <= signum) {
        PyErr_SetString(PyExc_ValueError, "signal number out of range");
        return 0;
    }
    return 1;
}

static PyObject*
faulthandler_register_py(PyObject *self,
                         PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"signum", "file", "all_threads", "chain", NULL};
    int signum;
    PyObject *file = NULL;
    int all_threads = 1;
    int chain = 0;
    int fd;
    user_signal_t *user;
    _Py_sighandler_t previous;
    PyThreadState *tstate;
    int err;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "i|Oii:register", kwlist,
        &signum, &file, &all_threads, &chain))
        return NULL;

    if (!check_signum(signum))
        return NULL;

    tstate = get_thread_state();
    if (tstate == NULL)
        return NULL;

    fd = faulthandler_get_fileno(&file);
    if (fd < 0)
        return NULL;

    if (user_signals == NULL) {
        user_signals = PyMem_Malloc(NSIG * sizeof(user_signal_t));
        if (user_signals == NULL)
            return PyErr_NoMemory();
        memset(user_signals, 0, NSIG * sizeof(user_signal_t));
    }
    user = &user_signals[signum];

    if (!user->enabled) {
        err = faulthandler_register(signum, chain, &previous);
        if (err) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }

        user->previous = previous;
    }

    Py_XINCREF(file);
    Py_XSETREF(user->file, file);
    user->fd = fd;
    user->all_threads = all_threads;
    user->chain = chain;
    user->interp = tstate->interp;
    user->enabled = 1;

    Py_RETURN_NONE;
}

static int
faulthandler_unregister(user_signal_t *user, int signum)
{
    if (!user->enabled)
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

    if (!check_signum(signum))
        return NULL;

    if (user_signals == NULL)
        Py_RETURN_FALSE;

    user = &user_signals[signum];
    change = faulthandler_unregister(user, signum);
    return PyBool_FromLong(change);
}
#endif   /* FAULTHANDLER_USER */


static void
faulthandler_suppress_crash_report(void)
{
#ifdef MS_WINDOWS
    UINT mode;

    /* Configure Windows to not display the Windows Error Reporting dialog */
    mode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(mode | SEM_NOGPFAULTERRORBOX);
#endif

#ifdef HAVE_SYS_RESOURCE_H
    struct rlimit rl;

    /* Disable creation of core dump */
    if (getrlimit(RLIMIT_CORE, &rl) == 0) {
        rl.rlim_cur = 0;
        setrlimit(RLIMIT_CORE, &rl);
    }
#endif

#ifdef _MSC_VER
    /* Visual Studio: configure abort() to not display an error message nor
       open a popup asking to report the fault. */
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

static PyObject *
faulthandler_read_null(PyObject *self, PyObject *args)
{
    volatile int *x;
    volatile int y;

    faulthandler_suppress_crash_report();
    x = NULL;
    y = *x;
    return PyLong_FromLong(y);

}

static void
faulthandler_raise_sigsegv(void)
{
    faulthandler_suppress_crash_report();
#if defined(MS_WINDOWS)
    /* For SIGSEGV, faulthandler_fatal_error() restores the previous signal
       handler and then gives back the execution flow to the program (without
       explicitly calling the previous error handler). In a normal case, the
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
}

static PyObject *
faulthandler_sigsegv(PyObject *self, PyObject *args)
{
    int release_gil = 0;
    if (!PyArg_ParseTuple(args, "|i:_sigsegv", &release_gil))
        return NULL;

    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        faulthandler_raise_sigsegv();
        Py_END_ALLOW_THREADS
    } else {
        faulthandler_raise_sigsegv();
    }
    Py_RETURN_NONE;
}

#ifdef WITH_THREAD
static void
faulthandler_fatal_error_thread(void *plock)
{
    PyThread_type_lock *lock = (PyThread_type_lock *)plock;

    Py_FatalError("in new thread");

    /* notify the caller that we are done */
    PyThread_release_lock(lock);
}

static PyObject *
faulthandler_fatal_error_c_thread(PyObject *self, PyObject *args)
{
    long thread;
    PyThread_type_lock lock;

    faulthandler_suppress_crash_report();

    lock = PyThread_allocate_lock();
    if (lock == NULL)
        return PyErr_NoMemory();

    PyThread_acquire_lock(lock, WAIT_LOCK);

    thread = PyThread_start_new_thread(faulthandler_fatal_error_thread, lock);
    if (thread == -1) {
        PyThread_free_lock(lock);
        PyErr_SetString(PyExc_RuntimeError, "unable to start the thread");
        return NULL;
    }

    /* wait until the thread completes: it will never occur, since Py_FatalError()
       exits the process immediately. */
    PyThread_acquire_lock(lock, WAIT_LOCK);
    PyThread_release_lock(lock);
    PyThread_free_lock(lock);

    Py_RETURN_NONE;
}
#endif

static PyObject *
faulthandler_sigfpe(PyObject *self, PyObject *args)
{
    /* Do an integer division by zero: raise a SIGFPE on Intel CPU, but not on
       PowerPC. Use volatile to disable compile-time optimizations. */
    volatile int x = 1, y = 0, z;
    faulthandler_suppress_crash_report();
    z = x / y;
    /* If the division by zero didn't raise a SIGFPE (e.g. on PowerPC),
       raise it manually. */
    raise(SIGFPE);
    /* This line is never reached, but we pretend to make something with z
       to silence a compiler warning. */
    return PyLong_FromLong(z);
}

static PyObject *
faulthandler_sigabrt(PyObject *self, PyObject *args)
{
    faulthandler_suppress_crash_report();
    abort();
    Py_RETURN_NONE;
}

static PyObject *
faulthandler_fatal_error_py(PyObject *self, PyObject *args)
{
    char *message;
    int release_gil = 0;
    if (!PyArg_ParseTuple(args, "y|i:fatal_error", &message, &release_gil))
        return NULL;
    faulthandler_suppress_crash_report();
    if (release_gil) {
        Py_BEGIN_ALLOW_THREADS
        Py_FatalError(message);
        Py_END_ALLOW_THREADS
    }
    else {
        Py_FatalError(message);
    }
    Py_RETURN_NONE;
}

#if defined(HAVE_SIGALTSTACK) && defined(HAVE_SIGACTION)
#define FAULTHANDLER_STACK_OVERFLOW

#ifdef __INTEL_COMPILER
   /* Issue #23654: Turn off ICC's tail call optimization for the
    * stack_overflow generator. ICC turns the recursive tail call into
    * a loop. */
#  pragma intel optimization_level 0
#endif
static
uintptr_t
stack_overflow(uintptr_t min_sp, uintptr_t max_sp, size_t *depth)
{
    /* allocate 4096 bytes on the stack at each call */
    unsigned char buffer[4096];
    uintptr_t sp = (uintptr_t)&buffer;
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
    uintptr_t sp = (uintptr_t)&depth;
    uintptr_t stop;

    faulthandler_suppress_crash_report();
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
#endif   /* defined(HAVE_SIGALTSTACK) && defined(HAVE_SIGACTION) */


static int
faulthandler_traverse(PyObject *module, visitproc visit, void *arg)
{
#ifdef FAULTHANDLER_USER
    unsigned int signum;
#endif

#ifdef FAULTHANDLER_LATER
    Py_VISIT(thread.file);
#endif
#ifdef FAULTHANDLER_USER
    if (user_signals != NULL) {
        for (signum=0; signum < NSIG; signum++)
            Py_VISIT(user_signals[signum].file);
    }
#endif
    Py_VISIT(fatal_error.file);
    return 0;
}

#ifdef MS_WINDOWS
static PyObject *
faulthandler_raise_exception(PyObject *self, PyObject *args)
{
    unsigned int code, flags = 0;
    if (!PyArg_ParseTuple(args, "I|I:_raise_exception", &code, &flags))
        return NULL;
    faulthandler_suppress_crash_report();
    RaiseException(code, flags, 0, NULL);
    Py_RETURN_NONE;
}
#endif

PyDoc_STRVAR(module_doc,
"faulthandler module.");

static PyMethodDef module_methods[] = {
    {"enable",
     (PyCFunction)faulthandler_py_enable, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("enable(file=sys.stderr, all_threads=True): "
               "enable the fault handler")},
    {"disable", (PyCFunction)faulthandler_disable_py, METH_NOARGS,
     PyDoc_STR("disable(): disable the fault handler")},
    {"is_enabled", (PyCFunction)faulthandler_is_enabled, METH_NOARGS,
     PyDoc_STR("is_enabled()->bool: check if the handler is enabled")},
    {"dump_traceback",
     (PyCFunction)faulthandler_dump_traceback_py, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("dump_traceback(file=sys.stderr, all_threads=True): "
               "dump the traceback of the current thread, or of all threads "
               "if all_threads is True, into file")},
#ifdef FAULTHANDLER_LATER
    {"dump_traceback_later",
     (PyCFunction)faulthandler_dump_traceback_later, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("dump_traceback_later(timeout, repeat=False, file=sys.stderrn, exit=False):\n"
               "dump the traceback of all threads in timeout seconds,\n"
               "or each timeout seconds if repeat is True. If exit is True, "
               "call _exit(1) which is not safe.")},
    {"cancel_dump_traceback_later",
     (PyCFunction)faulthandler_cancel_dump_traceback_later_py, METH_NOARGS,
     PyDoc_STR("cancel_dump_traceback_later():\ncancel the previous call "
               "to dump_traceback_later().")},
#endif

#ifdef FAULTHANDLER_USER
    {"register",
     (PyCFunction)faulthandler_register_py, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("register(signum, file=sys.stderr, all_threads=True, chain=False): "
               "register a handler for the signal 'signum': dump the "
               "traceback of the current thread, or of all threads if "
               "all_threads is True, into file")},
    {"unregister",
     faulthandler_unregister_py, METH_VARARGS|METH_KEYWORDS,
     PyDoc_STR("unregister(signum): unregister the handler of the signal "
                "'signum' registered by register()")},
#endif

    {"_read_null", faulthandler_read_null, METH_NOARGS,
     PyDoc_STR("_read_null(): read from NULL, raise "
               "a SIGSEGV or SIGBUS signal depending on the platform")},
    {"_sigsegv", faulthandler_sigsegv, METH_VARARGS,
     PyDoc_STR("_sigsegv(release_gil=False): raise a SIGSEGV signal")},
#ifdef WITH_THREAD
    {"_fatal_error_c_thread", faulthandler_fatal_error_c_thread, METH_NOARGS,
     PyDoc_STR("fatal_error_c_thread(): "
               "call Py_FatalError() in a new C thread.")},
#endif
    {"_sigabrt", faulthandler_sigabrt, METH_NOARGS,
     PyDoc_STR("_sigabrt(): raise a SIGABRT signal")},
    {"_sigfpe", (PyCFunction)faulthandler_sigfpe, METH_NOARGS,
     PyDoc_STR("_sigfpe(): raise a SIGFPE signal")},
    {"_fatal_error", faulthandler_fatal_error_py, METH_VARARGS,
     PyDoc_STR("_fatal_error(message): call Py_FatalError(message)")},
#ifdef FAULTHANDLER_STACK_OVERFLOW
    {"_stack_overflow", (PyCFunction)faulthandler_stack_overflow, METH_NOARGS,
     PyDoc_STR("_stack_overflow(): recursive call to raise a stack overflow")},
#endif
#ifdef MS_WINDOWS
    {"_raise_exception", faulthandler_raise_exception, METH_VARARGS,
     PyDoc_STR("raise_exception(code, flags=0): Call RaiseException(code, flags).")},
#endif
    {NULL, NULL}  /* sentinel */
};

static struct PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "faulthandler",
    module_doc,
    0, /* non-negative size to be able to unload the module */
    module_methods,
    NULL,
    faulthandler_traverse,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_faulthandler(void)
{
    PyObject *m = PyModule_Create(&module_def);
    if (m == NULL)
        return NULL;

    /* Add constants for unit tests */
#ifdef MS_WINDOWS
    /* RaiseException() codes (prefixed by an underscore) */
    if (PyModule_AddIntConstant(m, "_EXCEPTION_ACCESS_VIOLATION",
                                EXCEPTION_ACCESS_VIOLATION))
        return NULL;
    if (PyModule_AddIntConstant(m, "_EXCEPTION_INT_DIVIDE_BY_ZERO",
                                EXCEPTION_INT_DIVIDE_BY_ZERO))
        return NULL;
    if (PyModule_AddIntConstant(m, "_EXCEPTION_STACK_OVERFLOW",
                                EXCEPTION_STACK_OVERFLOW))
        return NULL;

    /* RaiseException() flags (prefixed by an underscore) */
    if (PyModule_AddIntConstant(m, "_EXCEPTION_NONCONTINUABLE",
                                EXCEPTION_NONCONTINUABLE))
        return NULL;
    if (PyModule_AddIntConstant(m, "_EXCEPTION_NONCONTINUABLE_EXCEPTION",
                                EXCEPTION_NONCONTINUABLE_EXCEPTION))
        return NULL;
#endif

    return m;
}

/* Call faulthandler.enable() if the PYTHONFAULTHANDLER environment variable
   is defined, or if sys._xoptions has a 'faulthandler' key. */

static int
faulthandler_env_options(void)
{
    PyObject *xoptions, *key, *module, *res;
    char *p;

    if (!((p = Py_GETENV("PYTHONFAULTHANDLER")) && *p != '\0')) {
        /* PYTHONFAULTHANDLER environment variable is missing
           or an empty string */
        int has_key;

        xoptions = PySys_GetXOptions();
        if (xoptions == NULL)
            return -1;

        key = PyUnicode_FromString("faulthandler");
        if (key == NULL)
            return -1;

        has_key = PyDict_Contains(xoptions, key);
        Py_DECREF(key);
        if (has_key <= 0)
            return has_key;
    }

    module = PyImport_ImportModule("faulthandler");
    if (module == NULL) {
        return -1;
    }
    res = _PyObject_CallMethodId(module, &PyId_enable, NULL);
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
        err = sigaltstack(&stack, &old_stack);
        if (err) {
            PyMem_Free(stack.ss_sp);
            stack.ss_sp = NULL;
        }
    }
#endif
#ifdef FAULTHANDLER_LATER
    thread.file = NULL;
    thread.cancel_event = PyThread_allocate_lock();
    thread.running = PyThread_allocate_lock();
    if (!thread.cancel_event || !thread.running) {
        PyErr_SetString(PyExc_RuntimeError,
                        "could not allocate locks for faulthandler");
        return -1;
    }
    PyThread_acquire_lock(thread.cancel_event, 1);
#endif

    return faulthandler_env_options();
}

void _PyFaulthandler_Fini(void)
{
#ifdef FAULTHANDLER_USER
    unsigned int signum;
#endif

#ifdef FAULTHANDLER_LATER
    /* later */
    if (thread.cancel_event) {
        cancel_dump_traceback_later();
        PyThread_release_lock(thread.cancel_event);
        PyThread_free_lock(thread.cancel_event);
        thread.cancel_event = NULL;
    }
    if (thread.running) {
        PyThread_free_lock(thread.running);
        thread.running = NULL;
    }
#endif

#ifdef FAULTHANDLER_USER
    /* user */
    if (user_signals != NULL) {
        for (signum=0; signum < NSIG; signum++)
            faulthandler_unregister(&user_signals[signum], signum);
        PyMem_Free(user_signals);
        user_signals = NULL;
    }
#endif

    /* fatal */
    faulthandler_disable();
#ifdef HAVE_SIGALTSTACK
    if (stack.ss_sp != NULL) {
        /* Fetch the current alt stack */
        stack_t current_stack;
        if (sigaltstack(NULL, &current_stack) == 0) {
            if (current_stack.ss_sp == stack.ss_sp) {
                /* The current alt stack is the one that we installed.
                 It is safe to restore the old stack that we found when
                 we installed ours */
                sigaltstack(&old_stack, NULL);
            } else {
                /* Someone switched to a different alt stack and didn't
                   restore ours when they were done (if they're done).
                   There's not much we can do in this unlikely case */
            }
        }
        PyMem_Free(stack.ss_sp);
        stack.ss_sp = NULL;
    }
#endif
}
