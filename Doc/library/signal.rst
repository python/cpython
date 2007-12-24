
:mod:`signal` --- Set handlers for asynchronous events
======================================================

.. module:: signal
   :synopsis: Set handlers for asynchronous events.


This module provides mechanisms to use signal handlers in Python. Some general
rules for working with signals and their handlers:

* A handler for a particular signal, once set, remains installed until it is
  explicitly reset (Python emulates the BSD style interface regardless of the
  underlying implementation), with the exception of the handler for
  :const:`SIGCHLD`, which follows the underlying implementation.

* There is no way to "block" signals temporarily from critical sections (since
  this is not supported by all Unix flavors).

* Although Python signal handlers are called asynchronously as far as the Python
  user is concerned, they can only occur between the "atomic" instructions of the
  Python interpreter.  This means that signals arriving during long calculations
  implemented purely in C (such as regular expression matches on large bodies of
  text) may be delayed for an arbitrary amount of time.

* When a signal arrives during an I/O operation, it is possible that the I/O
  operation raises an exception after the signal handler returns. This is
  dependent on the underlying Unix system's semantics regarding interrupted system
  calls.

* Because the C signal handler always returns, it makes little sense to catch
  synchronous errors like :const:`SIGFPE` or :const:`SIGSEGV`.

* Python installs a small number of signal handlers by default: :const:`SIGPIPE`
  is ignored (so write errors on pipes and sockets can be reported as ordinary
  Python exceptions) and :const:`SIGINT` is translated into a
  :exc:`KeyboardInterrupt` exception.  All of these can be overridden.

* Some care must be taken if both signals and threads are used in the same
  program.  The fundamental thing to remember in using signals and threads
  simultaneously is: always perform :func:`signal` operations in the main thread
  of execution.  Any thread can perform an :func:`alarm`, :func:`getsignal`, or
  :func:`pause`; only the main thread can set a new signal handler, and the main
  thread will be the only one to receive signals (this is enforced by the Python
  :mod:`signal` module, even if the underlying thread implementation supports
  sending signals to individual threads).  This means that signals can't be used
  as a means of inter-thread communication.  Use locks instead.

The variables defined in the :mod:`signal` module are:


.. data:: SIG_DFL

   This is one of two standard signal handling options; it will simply perform the
   default function for the signal.  For example, on most systems the default
   action for :const:`SIGQUIT` is to dump core and exit, while the default action
   for :const:`SIGCLD` is to simply ignore it.


.. data:: SIG_IGN

   This is another standard signal handler, which will simply ignore the given
   signal.


.. data:: SIG*

   All the signal numbers are defined symbolically.  For example, the hangup signal
   is defined as :const:`signal.SIGHUP`; the variable names are identical to the
   names used in C programs, as found in ``<signal.h>``. The Unix man page for
   ':cfunc:`signal`' lists the existing signals (on some systems this is
   :manpage:`signal(2)`, on others the list is in :manpage:`signal(7)`). Note that
   not all systems define the same set of signal names; only those names defined by
   the system are defined by this module.


.. data:: NSIG

   One more than the number of the highest signal number.

The :mod:`signal` module defines the following functions:


.. function:: alarm(time)

   If *time* is non-zero, this function requests that a :const:`SIGALRM` signal be
   sent to the process in *time* seconds. Any previously scheduled alarm is
   canceled (only one alarm can be scheduled at any time).  The returned value is
   then the number of seconds before any previously set alarm was to have been
   delivered. If *time* is zero, no alarm is scheduled, and any scheduled alarm is
   canceled.  If the return value is zero, no alarm is currently scheduled.  (See
   the Unix man page :manpage:`alarm(2)`.) Availability: Unix.


.. function:: getsignal(signalnum)

   Return the current signal handler for the signal *signalnum*. The returned value
   may be a callable Python object, or one of the special values
   :const:`signal.SIG_IGN`, :const:`signal.SIG_DFL` or :const:`None`.  Here,
   :const:`signal.SIG_IGN` means that the signal was previously ignored,
   :const:`signal.SIG_DFL` means that the default way of handling the signal was
   previously in use, and ``None`` means that the previous signal handler was not
   installed from Python.


.. function:: pause()

   Cause the process to sleep until a signal is received; the appropriate handler
   will then be called.  Returns nothing.  Not on Windows. (See the Unix man page
   :manpage:`signal(2)`.)


.. function:: set_wakeup_fd(fd)

   Set the wakeup fd to *fd*.  When a signal is received, a ``'\0'`` byte is
   written to the fd.  This can be used by a library to wakeup a poll or select
   call, allowing the signal to be fully processed.

   The old wakeup fd is returned.  *fd* must be non-blocking.  It is up to the
   library to remove any bytes before calling poll or select again.

   When threads are enabled, this function can only be called from the main thread;
   attempting to call it from other threads will cause a :exc:`ValueError`
   exception to be raised.


.. function:: signal(signalnum, handler)

   Set the handler for signal *signalnum* to the function *handler*.  *handler* can
   be a callable Python object taking two arguments (see below), or one of the
   special values :const:`signal.SIG_IGN` or :const:`signal.SIG_DFL`.  The previous
   signal handler will be returned (see the description of :func:`getsignal`
   above).  (See the Unix man page :manpage:`signal(2)`.)

   When threads are enabled, this function can only be called from the main thread;
   attempting to call it from other threads will cause a :exc:`ValueError`
   exception to be raised.

   The *handler* is called with two arguments: the signal number and the current
   stack frame (``None`` or a frame object; for a description of frame objects, see
   the reference manual section on the standard type hierarchy or see the attribute
   descriptions in the :mod:`inspect` module).


.. _signal-example:

Example
-------

Here is a minimal example program. It uses the :func:`alarm` function to limit
the time spent waiting to open a file; this is useful if the file is for a
serial device that may not be turned on, which would normally cause the
:func:`os.open` to hang indefinitely.  The solution is to set a 5-second alarm
before opening the file; if the operation takes too long, the alarm signal will
be sent, and the handler raises an exception. ::

   import signal, os

   def handler(signum, frame):
       print('Signal handler called with signal', signum)
       raise IOError("Couldn't open device!")

   # Set the signal handler and a 5-second alarm
   signal.signal(signal.SIGALRM, handler)
   signal.alarm(5)

   # This open() may hang indefinitely
   fd = os.open('/dev/ttyS0', os.O_RDWR)  

   signal.alarm(0)          # Disable the alarm

