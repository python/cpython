:mod:`!_thread` --- Low-level threading API
===========================================

.. module:: _thread
   :synopsis: Low-level threading API.

.. index::
   single: light-weight processes
   single: processes, light-weight
   single: binary semaphores
   single: semaphores, binary

--------------

This module provides low-level primitives for working with multiple threads
(also called :dfn:`light-weight processes` or :dfn:`tasks`) --- multiple threads of
control sharing their global data space.  For synchronization, simple locks
(also called :dfn:`mutexes` or :dfn:`binary semaphores`) are provided.
The :mod:`threading` module provides an easier to use and higher-level
threading API built on top of this module.

.. index::
   single: pthreads
   pair: threads; POSIX

.. versionchanged:: 3.7
   This module used to be optional, it is now always available.

This module defines the following constants and functions:

.. exception:: error

   Raised on thread-specific errors.

   .. versionchanged:: 3.3
      This is now a synonym of the built-in :exc:`RuntimeError`.


.. data:: LockType

   This is the type of lock objects.


.. function:: start_new_thread(function, args[, kwargs])

   Start a new thread and return its identifier.  The thread executes the
   function *function* with the argument list *args* (which must be a tuple).
   The optional *kwargs* argument specifies a dictionary of keyword arguments.

   When the function returns, the thread silently exits.

   When the function terminates with an unhandled exception,
   :func:`sys.unraisablehook` is called to handle the exception. The *object*
   attribute of the hook argument is *function*. By default, a stack trace is
   printed and then the thread exits (but other threads continue to run).

   When the function raises a :exc:`SystemExit` exception, it is silently
   ignored.

   .. audit-event:: _thread.start_new_thread function,args,kwargs start_new_thread

   .. versionchanged:: 3.8
      :func:`sys.unraisablehook` is now used to handle unhandled exceptions.


.. function:: interrupt_main(signum=signal.SIGINT, /)

   Simulate the effect of a signal arriving in the main thread.
   A thread can use this function to interrupt the main thread, though
   there is no guarantee that the interruption will happen immediately.

   If given, *signum* is the number of the signal to simulate.
   If *signum* is not given, :const:`signal.SIGINT` is simulated.

   If the given signal isn't handled by Python (it was set to
   :const:`signal.SIG_DFL` or :const:`signal.SIG_IGN`), this function does
   nothing.

   .. versionchanged:: 3.10
      The *signum* argument is added to customize the signal number.

   .. note::
      This does not emit the corresponding signal but schedules a call to
      the associated handler (if it exists).
      If you want to truly emit the signal, use :func:`signal.raise_signal`.


.. function:: exit()

   Raise the :exc:`SystemExit` exception.  When not caught, this will cause the
   thread to exit silently.

..
   function:: exit_prog(status)

      Exit all threads and report the value of the integer argument
      *status* as the exit status of the entire program.
      **Caveat:** code in pending :keyword:`finally` clauses, in this thread
      or in other threads, is not executed.


.. function:: allocate_lock()

   Return a new lock object.  Methods of locks are described below.  The lock is
   initially unlocked.


.. function:: get_ident()

   Return the 'thread identifier' of the current thread.  This is a nonzero
   integer.  Its value has no direct meaning; it is intended as a magic cookie to
   be used e.g. to index a dictionary of thread-specific data.  Thread identifiers
   may be recycled when a thread exits and another thread is created.


.. function:: get_native_id()

   Return the native integral Thread ID of the current thread assigned by the kernel.
   This is a non-negative integer.
   Its value may be used to uniquely identify this particular thread system-wide
   (until the thread terminates, after which the value may be recycled by the OS).

   .. availability:: Windows, FreeBSD, Linux, macOS, OpenBSD, NetBSD, AIX, DragonFlyBSD, GNU/kFreeBSD.

   .. versionadded:: 3.8

   .. versionchanged:: 3.13
      Added support for GNU/kFreeBSD.


.. function:: stack_size([size])

   Return the thread stack size used when creating new threads.  The optional
   *size* argument specifies the stack size to be used for subsequently created
   threads, and must be 0 (use platform or configured default) or a positive
   integer value of at least 32,768 (32 KiB). If *size* is not specified,
   0 is used.  If changing the thread stack size is
   unsupported, a :exc:`RuntimeError` is raised.  If the specified stack size is
   invalid, a :exc:`ValueError` is raised and the stack size is unmodified.  32 KiB
   is currently the minimum supported stack size value to guarantee sufficient
   stack space for the interpreter itself.  Note that some platforms may have
   particular restrictions on values for the stack size, such as requiring a
   minimum stack size > 32 KiB or requiring allocation in multiples of the system
   memory page size - platform documentation should be referred to for more
   information (4 KiB pages are common; using multiples of 4096 for the stack size is
   the suggested approach in the absence of more specific information).

   .. availability:: Windows, pthreads.

      Unix platforms with POSIX threads support.


.. data:: TIMEOUT_MAX

   The maximum value allowed for the *timeout* parameter of
   :meth:`Lock.acquire <threading.Lock.acquire>`. Specifying a timeout greater
   than this value will raise an :exc:`OverflowError`.

   .. versionadded:: 3.2


Lock objects have the following methods:


.. method:: lock.acquire(blocking=True, timeout=-1)

   Without any optional argument, this method acquires the lock unconditionally, if
   necessary waiting until it is released by another thread (only one thread at a
   time can acquire a lock --- that's their reason for existence).

   If the *blocking* argument is present, the action depends on its
   value: if it is false, the lock is only acquired if it can be acquired
   immediately without waiting, while if it is true, the lock is acquired
   unconditionally as above.

   If the floating-point *timeout* argument is present and positive, it
   specifies the maximum wait time in seconds before returning.  A negative
   *timeout* argument specifies an unbounded wait.  You cannot specify
   a *timeout* if *blocking* is false.

   The return value is ``True`` if the lock is acquired successfully,
   ``False`` if not.

   .. versionchanged:: 3.2
      The *timeout* parameter is new.

   .. versionchanged:: 3.2
      Lock acquires can now be interrupted by signals on POSIX.

   .. versionchanged:: 3.14
      Lock acquires can now be interrupted by signals on Windows.


.. method:: lock.release()

   Releases the lock.  The lock must have been acquired earlier, but not
   necessarily by the same thread.


.. method:: lock.locked()

   Return the status of the lock: ``True`` if it has been acquired by some thread,
   ``False`` if not.

In addition to these methods, lock objects can also be used via the
:keyword:`with` statement, e.g.::

   import _thread

   a_lock = _thread.allocate_lock()

   with a_lock:
       print("a_lock is locked while this executes")

**Caveats:**

.. index:: pair: module; signal

* Interrupts always go to the main thread (the :exc:`KeyboardInterrupt`
  exception will be received by that thread.)

* Calling :func:`sys.exit` or raising the :exc:`SystemExit` exception is
  equivalent to calling :func:`_thread.exit`.

* When the main thread exits, it is system defined whether the other threads
  survive.  On most systems, they are killed without executing
  :keyword:`try` ... :keyword:`finally` clauses or executing object
  destructors.

