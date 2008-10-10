:mod:`thread` --- Multiple threads of control
=============================================

.. module:: thread
   :synopsis: Create multiple threads of control within one interpreter.

.. note::
   The :mod:`thread` module has been renamed to :mod:`_thread` in Python 3.0.
   The :term:`2to3` tool will automatically adapt imports when converting your
   sources to 3.0; however, you should consider using the high-level
   :mod:`threading` module instead.


.. index::
   single: light-weight processes
   single: processes, light-weight
   single: binary semaphores
   single: semaphores, binary

This module provides low-level primitives for working with multiple threads
(also called :dfn:`light-weight processes` or :dfn:`tasks`) --- multiple threads of
control sharing their global data space.  For synchronization, simple locks
(also called :dfn:`mutexes` or :dfn:`binary semaphores`) are provided.
The :mod:`threading` module provides an easier to use and higher-level
threading API built on top of this module.

.. index::
   single: pthreads
   pair: threads; POSIX

The module is optional.  It is supported on Windows, Linux, SGI IRIX, Solaris
2.x, as well as on systems that have a POSIX thread (a.k.a. "pthread")
implementation.  For systems lacking the :mod:`thread` module, the
:mod:`dummy_thread` module is available. It duplicates this module's interface
and can be used as a drop-in replacement.

It defines the following constant and functions:


.. exception:: error

   Raised on thread-specific errors.


.. data:: LockType

   This is the type of lock objects.


.. function:: start_new_thread(function, args[, kwargs])

   Start a new thread and return its identifier.  The thread executes the function
   *function* with the argument list *args* (which must be a tuple).  The optional
   *kwargs* argument specifies a dictionary of keyword arguments. When the function
   returns, the thread silently exits.  When the function terminates with an
   unhandled exception, a stack trace is printed and then the thread exits (but
   other threads continue to run).


.. function:: interrupt_main()

   Raise a :exc:`KeyboardInterrupt` exception in the main thread.  A subthread can
   use this function to interrupt the main thread.

   .. versionadded:: 2.3


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


.. function:: stack_size([size])

   Return the thread stack size used when creating new threads.  The optional
   *size* argument specifies the stack size to be used for subsequently created
   threads, and must be 0 (use platform or configured default) or a positive
   integer value of at least 32,768 (32kB). If changing the thread stack size is
   unsupported, the :exc:`error` exception is raised.  If the specified stack size is
   invalid, a :exc:`ValueError` is raised and the stack size is unmodified.  32kB
   is currently the minimum supported stack size value to guarantee sufficient
   stack space for the interpreter itself.  Note that some platforms may have
   particular restrictions on values for the stack size, such as requiring a
   minimum stack size > 32kB or requiring allocation in multiples of the system
   memory page size - platform documentation should be referred to for more
   information (4kB pages are common; using multiples of 4096 for the stack size is
   the suggested approach in the absence of more specific information).
   Availability: Windows, systems with POSIX threads.

   .. versionadded:: 2.5

Lock objects have the following methods:


.. method:: lock.acquire([waitflag])

   Without the optional argument, this method acquires the lock unconditionally, if
   necessary waiting until it is released by another thread (only one thread at a
   time can acquire a lock --- that's their reason for existence).  If the integer
   *waitflag* argument is present, the action depends on its value: if it is zero,
   the lock is only acquired if it can be acquired immediately without waiting,
   while if it is nonzero, the lock is acquired unconditionally as before.  The
   return value is ``True`` if the lock is acquired successfully, ``False`` if not.


.. method:: lock.release()

   Releases the lock.  The lock must have been acquired earlier, but not
   necessarily by the same thread.


.. method:: lock.locked()

   Return the status of the lock: ``True`` if it has been acquired by some thread,
   ``False`` if not.

In addition to these methods, lock objects can also be used via the
:keyword:`with` statement, e.g.::

   import thread

   a_lock = thread.allocate_lock()

   with a_lock:
       print "a_lock is locked while this executes"

**Caveats:**

  .. index:: module: signal

* Threads interact strangely with interrupts: the :exc:`KeyboardInterrupt`
  exception will be received by an arbitrary thread.  (When the :mod:`signal`
  module is available, interrupts always go to the main thread.)

* Calling :func:`sys.exit` or raising the :exc:`SystemExit` exception is
  equivalent to calling :func:`exit`.

* Not all built-in functions that may block waiting for I/O allow other threads
  to run.  (The most popular ones (:func:`time.sleep`, :meth:`file.read`,
  :func:`select.select`) work as expected.)

* It is not possible to interrupt the :meth:`acquire` method on a lock --- the
  :exc:`KeyboardInterrupt` exception will happen after the lock has been acquired.

  .. index:: pair: threads; IRIX

* When the main thread exits, it is system defined whether the other threads
  survive.  On SGI IRIX using the native thread implementation, they survive.  On
  most other systems, they are killed without executing :keyword:`try` ...
  :keyword:`finally` clauses or executing object destructors.

* When the main thread exits, it does not do any of its usual cleanup (except
  that :keyword:`try` ... :keyword:`finally` clauses are honored), and the
  standard I/O files are not flushed.

