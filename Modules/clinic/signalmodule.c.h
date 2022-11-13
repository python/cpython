/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(signal_default_int_handler__doc__,
"default_int_handler($module, signalnum, frame, /)\n"
"--\n"
"\n"
"The default handler for SIGINT installed by Python.\n"
"\n"
"It raises KeyboardInterrupt.");

#define SIGNAL_DEFAULT_INT_HANDLER_METHODDEF    \
    {"default_int_handler", _PyCFunction_CAST(signal_default_int_handler), METH_FASTCALL, signal_default_int_handler__doc__},

static PyObject *
signal_default_int_handler_impl(PyObject *module, int signalnum,
                                PyObject *frame);

static PyObject *
signal_default_int_handler(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int signalnum;
    PyObject *frame;

    if (!_PyArg_CheckPositional("default_int_handler", nargs, 2, 2)) {
        goto exit;
    }
    signalnum = _PyLong_AsInt(args[0]);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    frame = args[1];
    return_value = signal_default_int_handler_impl(module, signalnum, frame);

exit:
    return return_value;
}

#if defined(HAVE_ALARM)

PyDoc_STRVAR(signal_alarm__doc__,
"alarm($module, seconds, /)\n"
"--\n"
"\n"
"Arrange for SIGALRM to arrive after the given number of seconds.");

#define SIGNAL_ALARM_METHODDEF    \
    {"alarm", (PyCFunction)signal_alarm, METH_O, signal_alarm__doc__},

static long
signal_alarm_impl(PyObject *module, int seconds);

static PyObject *
signal_alarm(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int seconds;
    long _return_value;

    seconds = _PyLong_AsInt(arg);
    if (seconds == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = signal_alarm_impl(module, seconds);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_ALARM) */

#if defined(HAVE_PAUSE)

PyDoc_STRVAR(signal_pause__doc__,
"pause($module, /)\n"
"--\n"
"\n"
"Wait until a signal arrives.");

#define SIGNAL_PAUSE_METHODDEF    \
    {"pause", (PyCFunction)signal_pause, METH_NOARGS, signal_pause__doc__},

static PyObject *
signal_pause_impl(PyObject *module);

static PyObject *
signal_pause(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return signal_pause_impl(module);
}

#endif /* defined(HAVE_PAUSE) */

PyDoc_STRVAR(signal_raise_signal__doc__,
"raise_signal($module, signalnum, /)\n"
"--\n"
"\n"
"Send a signal to the executing process.");

#define SIGNAL_RAISE_SIGNAL_METHODDEF    \
    {"raise_signal", (PyCFunction)signal_raise_signal, METH_O, signal_raise_signal__doc__},

static PyObject *
signal_raise_signal_impl(PyObject *module, int signalnum);

static PyObject *
signal_raise_signal(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int signalnum;

    signalnum = _PyLong_AsInt(arg);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_raise_signal_impl(module, signalnum);

exit:
    return return_value;
}

PyDoc_STRVAR(signal_signal__doc__,
"signal($module, signalnum, handler, /)\n"
"--\n"
"\n"
"Set the action for the given signal.\n"
"\n"
"The action can be SIG_DFL, SIG_IGN, or a callable Python object.\n"
"The previous action is returned.  See getsignal() for possible return values.\n"
"\n"
"*** IMPORTANT NOTICE ***\n"
"A signal handler function is called with two arguments:\n"
"the first is the signal number, the second is the interrupted stack frame.");

#define SIGNAL_SIGNAL_METHODDEF    \
    {"signal", _PyCFunction_CAST(signal_signal), METH_FASTCALL, signal_signal__doc__},

static PyObject *
signal_signal_impl(PyObject *module, int signalnum, PyObject *handler);

static PyObject *
signal_signal(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int signalnum;
    PyObject *handler;

    if (!_PyArg_CheckPositional("signal", nargs, 2, 2)) {
        goto exit;
    }
    signalnum = _PyLong_AsInt(args[0]);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    handler = args[1];
    return_value = signal_signal_impl(module, signalnum, handler);

exit:
    return return_value;
}

PyDoc_STRVAR(signal_getsignal__doc__,
"getsignal($module, signalnum, /)\n"
"--\n"
"\n"
"Return the current action for the given signal.\n"
"\n"
"The return value can be:\n"
"  SIG_IGN -- if the signal is being ignored\n"
"  SIG_DFL -- if the default action for the signal is in effect\n"
"  None    -- if an unknown handler is in effect\n"
"  anything else -- the callable Python object used as a handler");

#define SIGNAL_GETSIGNAL_METHODDEF    \
    {"getsignal", (PyCFunction)signal_getsignal, METH_O, signal_getsignal__doc__},

static PyObject *
signal_getsignal_impl(PyObject *module, int signalnum);

static PyObject *
signal_getsignal(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int signalnum;

    signalnum = _PyLong_AsInt(arg);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_getsignal_impl(module, signalnum);

exit:
    return return_value;
}

PyDoc_STRVAR(signal_strsignal__doc__,
"strsignal($module, signalnum, /)\n"
"--\n"
"\n"
"Return the system description of the given signal.\n"
"\n"
"Returns the description of signal *signalnum*, such as \"Interrupt\"\n"
"for :const:`SIGINT`. Returns :const:`None` if *signalnum* has no\n"
"description. Raises :exc:`ValueError` if *signalnum* is invalid.");

#define SIGNAL_STRSIGNAL_METHODDEF    \
    {"strsignal", (PyCFunction)signal_strsignal, METH_O, signal_strsignal__doc__},

static PyObject *
signal_strsignal_impl(PyObject *module, int signalnum);

static PyObject *
signal_strsignal(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int signalnum;

    signalnum = _PyLong_AsInt(arg);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_strsignal_impl(module, signalnum);

exit:
    return return_value;
}

#if defined(HAVE_SIGINTERRUPT)

PyDoc_STRVAR(signal_siginterrupt__doc__,
"siginterrupt($module, signalnum, flag, /)\n"
"--\n"
"\n"
"Change system call restart behaviour.\n"
"\n"
"If flag is False, system calls will be restarted when interrupted by\n"
"signal sig, else system calls will be interrupted.");

#define SIGNAL_SIGINTERRUPT_METHODDEF    \
    {"siginterrupt", _PyCFunction_CAST(signal_siginterrupt), METH_FASTCALL, signal_siginterrupt__doc__},

static PyObject *
signal_siginterrupt_impl(PyObject *module, int signalnum, int flag);

static PyObject *
signal_siginterrupt(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int signalnum;
    int flag;

    if (!_PyArg_CheckPositional("siginterrupt", nargs, 2, 2)) {
        goto exit;
    }
    signalnum = _PyLong_AsInt(args[0]);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    flag = _PyLong_AsInt(args[1]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_siginterrupt_impl(module, signalnum, flag);

exit:
    return return_value;
}

#endif /* defined(HAVE_SIGINTERRUPT) */

#if defined(HAVE_SETITIMER)

PyDoc_STRVAR(signal_setitimer__doc__,
"setitimer($module, which, seconds, interval=0.0, /)\n"
"--\n"
"\n"
"Sets given itimer (one of ITIMER_REAL, ITIMER_VIRTUAL or ITIMER_PROF).\n"
"\n"
"The timer will fire after value seconds and after that every interval seconds.\n"
"The itimer can be cleared by setting seconds to zero.\n"
"\n"
"Returns old values as a tuple: (delay, interval).");

#define SIGNAL_SETITIMER_METHODDEF    \
    {"setitimer", _PyCFunction_CAST(signal_setitimer), METH_FASTCALL, signal_setitimer__doc__},

static PyObject *
signal_setitimer_impl(PyObject *module, int which, PyObject *seconds,
                      PyObject *interval);

static PyObject *
signal_setitimer(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int which;
    PyObject *seconds;
    PyObject *interval = NULL;

    if (!_PyArg_CheckPositional("setitimer", nargs, 2, 3)) {
        goto exit;
    }
    which = _PyLong_AsInt(args[0]);
    if (which == -1 && PyErr_Occurred()) {
        goto exit;
    }
    seconds = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    interval = args[2];
skip_optional:
    return_value = signal_setitimer_impl(module, which, seconds, interval);

exit:
    return return_value;
}

#endif /* defined(HAVE_SETITIMER) */

#if defined(HAVE_GETITIMER)

PyDoc_STRVAR(signal_getitimer__doc__,
"getitimer($module, which, /)\n"
"--\n"
"\n"
"Returns current value of given itimer.");

#define SIGNAL_GETITIMER_METHODDEF    \
    {"getitimer", (PyCFunction)signal_getitimer, METH_O, signal_getitimer__doc__},

static PyObject *
signal_getitimer_impl(PyObject *module, int which);

static PyObject *
signal_getitimer(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int which;

    which = _PyLong_AsInt(arg);
    if (which == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_getitimer_impl(module, which);

exit:
    return return_value;
}

#endif /* defined(HAVE_GETITIMER) */

#if defined(HAVE_SIGSET_T) && defined(PYPTHREAD_SIGMASK)

PyDoc_STRVAR(signal_pthread_sigmask__doc__,
"pthread_sigmask($module, how, mask, /)\n"
"--\n"
"\n"
"Fetch and/or change the signal mask of the calling thread.");

#define SIGNAL_PTHREAD_SIGMASK_METHODDEF    \
    {"pthread_sigmask", _PyCFunction_CAST(signal_pthread_sigmask), METH_FASTCALL, signal_pthread_sigmask__doc__},

static PyObject *
signal_pthread_sigmask_impl(PyObject *module, int how, sigset_t mask);

static PyObject *
signal_pthread_sigmask(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int how;
    sigset_t mask;

    if (!_PyArg_CheckPositional("pthread_sigmask", nargs, 2, 2)) {
        goto exit;
    }
    how = _PyLong_AsInt(args[0]);
    if (how == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!_Py_Sigset_Converter(args[1], &mask)) {
        goto exit;
    }
    return_value = signal_pthread_sigmask_impl(module, how, mask);

exit:
    return return_value;
}

#endif /* defined(HAVE_SIGSET_T) && defined(PYPTHREAD_SIGMASK) */

#if defined(HAVE_SIGSET_T) && defined(HAVE_SIGPENDING)

PyDoc_STRVAR(signal_sigpending__doc__,
"sigpending($module, /)\n"
"--\n"
"\n"
"Examine pending signals.\n"
"\n"
"Returns a set of signal numbers that are pending for delivery to\n"
"the calling thread.");

#define SIGNAL_SIGPENDING_METHODDEF    \
    {"sigpending", (PyCFunction)signal_sigpending, METH_NOARGS, signal_sigpending__doc__},

static PyObject *
signal_sigpending_impl(PyObject *module);

static PyObject *
signal_sigpending(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return signal_sigpending_impl(module);
}

#endif /* defined(HAVE_SIGSET_T) && defined(HAVE_SIGPENDING) */

#if defined(HAVE_SIGSET_T) && defined(HAVE_SIGWAIT)

PyDoc_STRVAR(signal_sigwait__doc__,
"sigwait($module, sigset, /)\n"
"--\n"
"\n"
"Wait for a signal.\n"
"\n"
"Suspend execution of the calling thread until the delivery of one of the\n"
"signals specified in the signal set sigset.  The function accepts the signal\n"
"and returns the signal number.");

#define SIGNAL_SIGWAIT_METHODDEF    \
    {"sigwait", (PyCFunction)signal_sigwait, METH_O, signal_sigwait__doc__},

static PyObject *
signal_sigwait_impl(PyObject *module, sigset_t sigset);

static PyObject *
signal_sigwait(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    sigset_t sigset;

    if (!_Py_Sigset_Converter(arg, &sigset)) {
        goto exit;
    }
    return_value = signal_sigwait_impl(module, sigset);

exit:
    return return_value;
}

#endif /* defined(HAVE_SIGSET_T) && defined(HAVE_SIGWAIT) */

#if ((defined(HAVE_SIGFILLSET) && defined(HAVE_SIGSET_T)) || defined(MS_WINDOWS))

PyDoc_STRVAR(signal_valid_signals__doc__,
"valid_signals($module, /)\n"
"--\n"
"\n"
"Return a set of valid signal numbers on this platform.\n"
"\n"
"The signal numbers returned by this function can be safely passed to\n"
"functions like `pthread_sigmask`.");

#define SIGNAL_VALID_SIGNALS_METHODDEF    \
    {"valid_signals", (PyCFunction)signal_valid_signals, METH_NOARGS, signal_valid_signals__doc__},

static PyObject *
signal_valid_signals_impl(PyObject *module);

static PyObject *
signal_valid_signals(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return signal_valid_signals_impl(module);
}

#endif /* ((defined(HAVE_SIGFILLSET) && defined(HAVE_SIGSET_T)) || defined(MS_WINDOWS)) */

#if defined(HAVE_SIGSET_T) && defined(HAVE_SIGWAITINFO)

PyDoc_STRVAR(signal_sigwaitinfo__doc__,
"sigwaitinfo($module, sigset, /)\n"
"--\n"
"\n"
"Wait synchronously until one of the signals in *sigset* is delivered.\n"
"\n"
"Returns a struct_siginfo containing information about the signal.");

#define SIGNAL_SIGWAITINFO_METHODDEF    \
    {"sigwaitinfo", (PyCFunction)signal_sigwaitinfo, METH_O, signal_sigwaitinfo__doc__},

static PyObject *
signal_sigwaitinfo_impl(PyObject *module, sigset_t sigset);

static PyObject *
signal_sigwaitinfo(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    sigset_t sigset;

    if (!_Py_Sigset_Converter(arg, &sigset)) {
        goto exit;
    }
    return_value = signal_sigwaitinfo_impl(module, sigset);

exit:
    return return_value;
}

#endif /* defined(HAVE_SIGSET_T) && defined(HAVE_SIGWAITINFO) */

#if defined(HAVE_SIGSET_T) && defined(HAVE_SIGTIMEDWAIT)

PyDoc_STRVAR(signal_sigtimedwait__doc__,
"sigtimedwait($module, sigset, timeout, /)\n"
"--\n"
"\n"
"Like sigwaitinfo(), but with a timeout.\n"
"\n"
"The timeout is specified in seconds, with floating point numbers allowed.");

#define SIGNAL_SIGTIMEDWAIT_METHODDEF    \
    {"sigtimedwait", _PyCFunction_CAST(signal_sigtimedwait), METH_FASTCALL, signal_sigtimedwait__doc__},

static PyObject *
signal_sigtimedwait_impl(PyObject *module, sigset_t sigset,
                         PyObject *timeout_obj);

static PyObject *
signal_sigtimedwait(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    sigset_t sigset;
    PyObject *timeout_obj;

    if (!_PyArg_CheckPositional("sigtimedwait", nargs, 2, 2)) {
        goto exit;
    }
    if (!_Py_Sigset_Converter(args[0], &sigset)) {
        goto exit;
    }
    timeout_obj = args[1];
    return_value = signal_sigtimedwait_impl(module, sigset, timeout_obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_SIGSET_T) && defined(HAVE_SIGTIMEDWAIT) */

#if defined(HAVE_PTHREAD_KILL)

PyDoc_STRVAR(signal_pthread_kill__doc__,
"pthread_kill($module, thread_id, signalnum, /)\n"
"--\n"
"\n"
"Send a signal to a thread.");

#define SIGNAL_PTHREAD_KILL_METHODDEF    \
    {"pthread_kill", _PyCFunction_CAST(signal_pthread_kill), METH_FASTCALL, signal_pthread_kill__doc__},

static PyObject *
signal_pthread_kill_impl(PyObject *module, unsigned long thread_id,
                         int signalnum);

static PyObject *
signal_pthread_kill(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned long thread_id;
    int signalnum;

    if (!_PyArg_CheckPositional("pthread_kill", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyLong_Check(args[0])) {
        _PyArg_BadArgument("pthread_kill", "argument 1", "int", args[0]);
        goto exit;
    }
    thread_id = PyLong_AsUnsignedLongMask(args[0]);
    signalnum = _PyLong_AsInt(args[1]);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = signal_pthread_kill_impl(module, thread_id, signalnum);

exit:
    return return_value;
}

#endif /* defined(HAVE_PTHREAD_KILL) */

#if (defined(__linux__) && defined(__NR_pidfd_send_signal))

PyDoc_STRVAR(signal_pidfd_send_signal__doc__,
"pidfd_send_signal($module, pidfd, signalnum, siginfo=None, flags=0, /)\n"
"--\n"
"\n"
"Send a signal to a process referred to by a pid file descriptor.");

#define SIGNAL_PIDFD_SEND_SIGNAL_METHODDEF    \
    {"pidfd_send_signal", _PyCFunction_CAST(signal_pidfd_send_signal), METH_FASTCALL, signal_pidfd_send_signal__doc__},

static PyObject *
signal_pidfd_send_signal_impl(PyObject *module, int pidfd, int signalnum,
                              PyObject *siginfo, int flags);

static PyObject *
signal_pidfd_send_signal(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int pidfd;
    int signalnum;
    PyObject *siginfo = Py_None;
    int flags = 0;

    if (!_PyArg_CheckPositional("pidfd_send_signal", nargs, 2, 4)) {
        goto exit;
    }
    pidfd = _PyLong_AsInt(args[0]);
    if (pidfd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    signalnum = _PyLong_AsInt(args[1]);
    if (signalnum == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    siginfo = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    flags = _PyLong_AsInt(args[3]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = signal_pidfd_send_signal_impl(module, pidfd, signalnum, siginfo, flags);

exit:
    return return_value;
}

#endif /* (defined(__linux__) && defined(__NR_pidfd_send_signal)) */

#ifndef SIGNAL_ALARM_METHODDEF
    #define SIGNAL_ALARM_METHODDEF
#endif /* !defined(SIGNAL_ALARM_METHODDEF) */

#ifndef SIGNAL_PAUSE_METHODDEF
    #define SIGNAL_PAUSE_METHODDEF
#endif /* !defined(SIGNAL_PAUSE_METHODDEF) */

#ifndef SIGNAL_SIGINTERRUPT_METHODDEF
    #define SIGNAL_SIGINTERRUPT_METHODDEF
#endif /* !defined(SIGNAL_SIGINTERRUPT_METHODDEF) */

#ifndef SIGNAL_SETITIMER_METHODDEF
    #define SIGNAL_SETITIMER_METHODDEF
#endif /* !defined(SIGNAL_SETITIMER_METHODDEF) */

#ifndef SIGNAL_GETITIMER_METHODDEF
    #define SIGNAL_GETITIMER_METHODDEF
#endif /* !defined(SIGNAL_GETITIMER_METHODDEF) */

#ifndef SIGNAL_PTHREAD_SIGMASK_METHODDEF
    #define SIGNAL_PTHREAD_SIGMASK_METHODDEF
#endif /* !defined(SIGNAL_PTHREAD_SIGMASK_METHODDEF) */

#ifndef SIGNAL_SIGPENDING_METHODDEF
    #define SIGNAL_SIGPENDING_METHODDEF
#endif /* !defined(SIGNAL_SIGPENDING_METHODDEF) */

#ifndef SIGNAL_SIGWAIT_METHODDEF
    #define SIGNAL_SIGWAIT_METHODDEF
#endif /* !defined(SIGNAL_SIGWAIT_METHODDEF) */

#ifndef SIGNAL_VALID_SIGNALS_METHODDEF
    #define SIGNAL_VALID_SIGNALS_METHODDEF
#endif /* !defined(SIGNAL_VALID_SIGNALS_METHODDEF) */

#ifndef SIGNAL_SIGWAITINFO_METHODDEF
    #define SIGNAL_SIGWAITINFO_METHODDEF
#endif /* !defined(SIGNAL_SIGWAITINFO_METHODDEF) */

#ifndef SIGNAL_SIGTIMEDWAIT_METHODDEF
    #define SIGNAL_SIGTIMEDWAIT_METHODDEF
#endif /* !defined(SIGNAL_SIGTIMEDWAIT_METHODDEF) */

#ifndef SIGNAL_PTHREAD_KILL_METHODDEF
    #define SIGNAL_PTHREAD_KILL_METHODDEF
#endif /* !defined(SIGNAL_PTHREAD_KILL_METHODDEF) */

#ifndef SIGNAL_PIDFD_SEND_SIGNAL_METHODDEF
    #define SIGNAL_PIDFD_SEND_SIGNAL_METHODDEF
#endif /* !defined(SIGNAL_PIDFD_SEND_SIGNAL_METHODDEF) */
/*[clinic end generated code: output=9b3f9f1ae2ac2b94 input=a9049054013a1b77]*/
