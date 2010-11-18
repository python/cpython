:mod:`threading` --- Higher-level threading interface
=====================================================

.. module:: threading
   :synopsis: Higher-level threading interface.


This module constructs higher-level threading interfaces on top of the lower
level :mod:`_thread` module.  See also the :mod:`queue` module.

The :mod:`dummy_threading` module is provided for situations where
:mod:`threading` cannot be used because :mod:`_thread` is missing.

.. note::

   While they are not listed below, the ``camelCase`` names used for some
   methods and functions in this module in the Python 2.x series are still
   supported by this module.

.. seealso::

   Latest version of the :source:`threading module Python source code
   <Lib/threading.py>`

This module defines the following functions and objects:


.. function:: active_count()

   Return the number of :class:`Thread` objects currently alive.  The returned
   count is equal to the length of the list returned by :func:`.enumerate`.


.. function:: Condition()
   :noindex:

   A factory function that returns a new condition variable object. A condition
   variable allows one or more threads to wait until they are notified by another
   thread.

   See :ref:`condition-objects`.


.. function:: current_thread()

   Return the current :class:`Thread` object, corresponding to the caller's thread
   of control.  If the caller's thread of control was not created through the
   :mod:`threading` module, a dummy thread object with limited functionality is
   returned.


.. function:: enumerate()

   Return a list of all :class:`Thread` objects currently alive.  The list
   includes daemonic threads, dummy thread objects created by
   :func:`current_thread`, and the main thread.  It excludes terminated threads
   and threads that have not yet been started.


.. function:: Event()
   :noindex:

   A factory function that returns a new event object.  An event manages a flag
   that can be set to true with the :meth:`~Event.set` method and reset to false
   with the :meth:`clear` method.  The :meth:`wait` method blocks until the flag
   is true.

   See :ref:`event-objects`.


.. class:: local

   A class that represents thread-local data.  Thread-local data are data whose
   values are thread specific.  To manage thread-local data, just create an
   instance of :class:`local` (or a subclass) and store attributes on it::

      mydata = threading.local()
      mydata.x = 1

   The instance's values will be different for separate threads.

   For more details and extensive examples, see the documentation string of the
   :mod:`_threading_local` module.


.. function:: Lock()

   A factory function that returns a new primitive lock object.  Once a thread has
   acquired it, subsequent attempts to acquire it block, until it is released; any
   thread may release it.

   See :ref:`lock-objects`.


.. function:: RLock()

   A factory function that returns a new reentrant lock object. A reentrant lock
   must be released by the thread that acquired it. Once a thread has acquired a
   reentrant lock, the same thread may acquire it again without blocking; the
   thread must release it once for each time it has acquired it.

   See :ref:`rlock-objects`.


.. function:: Semaphore(value=1)
   :noindex:

   A factory function that returns a new semaphore object.  A semaphore manages a
   counter representing the number of :meth:`release` calls minus the number of
   :meth:`acquire` calls, plus an initial value. The :meth:`acquire` method blocks
   if necessary until it can return without making the counter negative.  If not
   given, *value* defaults to 1.

   See :ref:`semaphore-objects`.


.. function:: BoundedSemaphore(value=1)

   A factory function that returns a new bounded semaphore object.  A bounded
   semaphore checks to make sure its current value doesn't exceed its initial
   value.  If it does, :exc:`ValueError` is raised. In most situations semaphores
   are used to guard resources with limited capacity.  If the semaphore is released
   too many times it's a sign of a bug.  If not given, *value* defaults to 1.


.. class:: Thread
   :noindex:

   A class that represents a thread of control.  This class can be safely
   subclassed in a limited fashion.

   See :ref:`thread-objects`.


.. class:: Timer
   :noindex:

   A thread that executes a function after a specified interval has passed.

   See :ref:`timer-objects`.


.. function:: settrace(func)

   .. index:: single: trace function

   Set a trace function for all threads started from the :mod:`threading` module.
   The *func* will be passed to  :func:`sys.settrace` for each thread, before its
   :meth:`run` method is called.


.. function:: setprofile(func)

   .. index:: single: profile function

   Set a profile function for all threads started from the :mod:`threading` module.
   The *func* will be passed to  :func:`sys.setprofile` for each thread, before its
   :meth:`run` method is called.


.. function:: stack_size([size])

   Return the thread stack size used when creating new threads.  The optional
   *size* argument specifies the stack size to be used for subsequently created
   threads, and must be 0 (use platform or configured default) or a positive
   integer value of at least 32,768 (32kB). If changing the thread stack size is
   unsupported, a :exc:`ThreadError` is raised.  If the specified stack size is
   invalid, a :exc:`ValueError` is raised and the stack size is unmodified.  32kB
   is currently the minimum supported stack size value to guarantee sufficient
   stack space for the interpreter itself.  Note that some platforms may have
   particular restrictions on values for the stack size, such as requiring a
   minimum stack size > 32kB or requiring allocation in multiples of the system
   memory page size - platform documentation should be referred to for more
   information (4kB pages are common; using multiples of 4096 for the stack size is
   the suggested approach in the absence of more specific information).
   Availability: Windows, systems with POSIX threads.


This module also defines the following constant:

.. data:: TIMEOUT_MAX

   The maximum value allowed for the *timeout* parameter of blocking functions
   (:meth:`Lock.acquire`, :meth:`RLock.acquire`, :meth:`Condition.wait`, etc.).
   Specifying a timeout greater than this value will raise an
   :exc:`OverflowError`.

   .. versionadded:: 3.2


Detailed interfaces for the objects are documented below.

The design of this module is loosely based on Java's threading model. However,
where Java makes locks and condition variables basic behavior of every object,
they are separate objects in Python.  Python's :class:`Thread` class supports a
subset of the behavior of Java's Thread class; currently, there are no
priorities, no thread groups, and threads cannot be destroyed, stopped,
suspended, resumed, or interrupted.  The static methods of Java's Thread class,
when implemented, are mapped to module-level functions.

All of the methods described below are executed atomically.


.. _thread-objects:

Thread Objects
--------------

This class represents an activity that is run in a separate thread of control.
There are two ways to specify the activity: by passing a callable object to the
constructor, or by overriding the :meth:`run` method in a subclass.  No other
methods (except for the constructor) should be overridden in a subclass.  In
other words,  *only*  override the :meth:`__init__` and :meth:`run` methods of
this class.

Once a thread object is created, its activity must be started by calling the
thread's :meth:`start` method.  This invokes the :meth:`run` method in a
separate thread of control.

Once the thread's activity is started, the thread is considered 'alive'. It
stops being alive when its :meth:`run` method terminates -- either normally, or
by raising an unhandled exception.  The :meth:`is_alive` method tests whether the
thread is alive.

Other threads can call a thread's :meth:`join` method.  This blocks the calling
thread until the thread whose :meth:`join` method is called is terminated.

A thread has a name.  The name can be passed to the constructor, and read or
changed through the :attr:`name` attribute.

A thread can be flagged as a "daemon thread".  The significance of this flag is
that the entire Python program exits when only daemon threads are left.  The
initial value is inherited from the creating thread.  The flag can be set
through the :attr:`daemon` property.

There is a "main thread" object; this corresponds to the initial thread of
control in the Python program.  It is not a daemon thread.

There is the possibility that "dummy thread objects" are created. These are
thread objects corresponding to "alien threads", which are threads of control
started outside the threading module, such as directly from C code.  Dummy
thread objects have limited functionality; they are always considered alive and
daemonic, and cannot be :meth:`join`\ ed.  They are never deleted, since it is
impossible to detect the termination of alien threads.


.. class:: Thread(group=None, target=None, name=None, args=(), kwargs={})

   This constructor should always be called with keyword arguments.  Arguments
   are:

   *group* should be ``None``; reserved for future extension when a
   :class:`ThreadGroup` class is implemented.

   *target* is the callable object to be invoked by the :meth:`run` method.
   Defaults to ``None``, meaning nothing is called.

   *name* is the thread name.  By default, a unique name is constructed of the
   form "Thread-*N*" where *N* is a small decimal number.

   *args* is the argument tuple for the target invocation.  Defaults to ``()``.

   *kwargs* is a dictionary of keyword arguments for the target invocation.
   Defaults to ``{}``.

   If the subclass overrides the constructor, it must make sure to invoke the
   base class constructor (``Thread.__init__()``) before doing anything else to
   the thread.

   .. method:: start()

      Start the thread's activity.

      It must be called at most once per thread object.  It arranges for the
      object's :meth:`run` method to be invoked in a separate thread of control.

      This method will raise a :exc:`RuntimeException` if called more than once
      on the same thread object.

   .. method:: run()

      Method representing the thread's activity.

      You may override this method in a subclass.  The standard :meth:`run`
      method invokes the callable object passed to the object's constructor as
      the *target* argument, if any, with sequential and keyword arguments taken
      from the *args* and *kwargs* arguments, respectively.

   .. method:: join(timeout=None)

      Wait until the thread terminates. This blocks the calling thread until the
      thread whose :meth:`join` method is called terminates -- either normally
      or through an unhandled exception -- or until the optional timeout occurs.

      When the *timeout* argument is present and not ``None``, it should be a
      floating point number specifying a timeout for the operation in seconds
      (or fractions thereof). As :meth:`join` always returns ``None``, you must
      call :meth:`is_alive` after :meth:`join` to decide whether a timeout
      happened -- if the thread is still alive, the :meth:`join` call timed out.

      When the *timeout* argument is not present or ``None``, the operation will
      block until the thread terminates.

      A thread can be :meth:`join`\ ed many times.

      :meth:`join` raises a :exc:`RuntimeError` if an attempt is made to join
      the current thread as that would cause a deadlock. It is also an error to
      :meth:`join` a thread before it has been started and attempts to do so
      raises the same exception.

   .. attribute:: name

      A string used for identification purposes only. It has no semantics.
      Multiple threads may be given the same name.  The initial name is set by
      the constructor.

   .. method:: getName()
               setName()

      Old getter/setter API for :attr:`~Thread.name`; use it directly as a
      property instead.

   .. attribute:: ident

      The 'thread identifier' of this thread or ``None`` if the thread has not
      been started.  This is a nonzero integer.  See the
      :func:`thread.get_ident()` function.  Thread identifiers may be recycled
      when a thread exits and another thread is created.  The identifier is
      available even after the thread has exited.

   .. method:: is_alive()

      Return whether the thread is alive.

      This method returns ``True`` just before the :meth:`run` method starts
      until just after the :meth:`run` method terminates.  The module function
      :func:`.enumerate` returns a list of all alive threads.

   .. attribute:: daemon

      A boolean value indicating whether this thread is a daemon thread (True)
      or not (False).  This must be set before :meth:`start` is called,
      otherwise :exc:`RuntimeError` is raised.  Its initial value is inherited
      from the creating thread; the main thread is not a daemon thread and
      therefore all threads created in the main thread default to :attr:`daemon`
      = ``False``.

      The entire Python program exits when no alive non-daemon threads are left.

   .. method:: isDaemon()
               setDaemon()

      Old getter/setter API for :attr:`~Thread.daemon`; use it directly as a
      property instead.


.. _lock-objects:

Lock Objects
------------

A primitive lock is a synchronization primitive that is not owned by a
particular thread when locked.  In Python, it is currently the lowest level
synchronization primitive available, implemented directly by the :mod:`_thread`
extension module.

A primitive lock is in one of two states, "locked" or "unlocked". It is created
in the unlocked state.  It has two basic methods, :meth:`acquire` and
:meth:`release`.  When the state is unlocked, :meth:`acquire` changes the state
to locked and returns immediately.  When the state is locked, :meth:`acquire`
blocks until a call to :meth:`release` in another thread changes it to unlocked,
then the :meth:`acquire` call resets it to locked and returns.  The
:meth:`release` method should only be called in the locked state; it changes the
state to unlocked and returns immediately. If an attempt is made to release an
unlocked lock, a :exc:`RuntimeError` will be raised.

When more than one thread is blocked in :meth:`acquire` waiting for the state to
turn to unlocked, only one thread proceeds when a :meth:`release` call resets
the state to unlocked; which one of the waiting threads proceeds is not defined,
and may vary across implementations.

All methods are executed atomically.


.. method:: Lock.acquire(blocking=True, timeout=-1)

   Acquire a lock, blocking or non-blocking.

   When invoked without arguments, block until the lock is unlocked, then set it to
   locked, and return true.

   When invoked with the *blocking* argument set to true, do the same thing as when
   called without arguments, and return true.

   When invoked with the *blocking* argument set to false, do not block.  If a call
   without an argument would block, return false immediately; otherwise, do the
   same thing as when called without arguments, and return true.

   When invoked with the floating-point *timeout* argument set to a positive
   value, block for at most the number of seconds specified by *timeout*
   and as long as the lock cannot be acquired.  A negative *timeout* argument
   specifies an unbounded wait.  It is forbidden to specify a *timeout*
   when *blocking* is false.

   The return value is ``True`` if the lock is acquired successfully,
   ``False`` if not (for example if the *timeout* expired).

   .. versionchanged:: 3.2
      The *timeout* parameter is new.


.. method:: Lock.release()

   Release a lock.

   When the lock is locked, reset it to unlocked, and return.  If any other threads
   are blocked waiting for the lock to become unlocked, allow exactly one of them
   to proceed.

   Do not call this method when the lock is unlocked.

   There is no return value.


.. _rlock-objects:

RLock Objects
-------------

A reentrant lock is a synchronization primitive that may be acquired multiple
times by the same thread.  Internally, it uses the concepts of "owning thread"
and "recursion level" in addition to the locked/unlocked state used by primitive
locks.  In the locked state, some thread owns the lock; in the unlocked state,
no thread owns it.

To lock the lock, a thread calls its :meth:`acquire` method; this returns once
the thread owns the lock.  To unlock the lock, a thread calls its
:meth:`release` method. :meth:`acquire`/:meth:`release` call pairs may be
nested; only the final :meth:`release` (the :meth:`release` of the outermost
pair) resets the lock to unlocked and allows another thread blocked in
:meth:`acquire` to proceed.


.. method:: RLock.acquire(blocking=True, timeout=-1)

   Acquire a lock, blocking or non-blocking.

   When invoked without arguments: if this thread already owns the lock, increment
   the recursion level by one, and return immediately.  Otherwise, if another
   thread owns the lock, block until the lock is unlocked.  Once the lock is
   unlocked (not owned by any thread), then grab ownership, set the recursion level
   to one, and return.  If more than one thread is blocked waiting until the lock
   is unlocked, only one at a time will be able to grab ownership of the lock.
   There is no return value in this case.

   When invoked with the *blocking* argument set to true, do the same thing as when
   called without arguments, and return true.

   When invoked with the *blocking* argument set to false, do not block.  If a call
   without an argument would block, return false immediately; otherwise, do the
   same thing as when called without arguments, and return true.

   When invoked with the floating-point *timeout* argument set to a positive
   value, block for at most the number of seconds specified by *timeout*
   and as long as the lock cannot be acquired.  Return true if the lock has
   been acquired, false if the timeout has elapsed.

   .. versionchanged:: 3.2
      The *timeout* parameter is new.


.. method:: RLock.release()

   Release a lock, decrementing the recursion level.  If after the decrement it is
   zero, reset the lock to unlocked (not owned by any thread), and if any other
   threads are blocked waiting for the lock to become unlocked, allow exactly one
   of them to proceed.  If after the decrement the recursion level is still
   nonzero, the lock remains locked and owned by the calling thread.

   Only call this method when the calling thread owns the lock. A
   :exc:`RuntimeError` is raised if this method is called when the lock is
   unlocked.

   There is no return value.


.. _condition-objects:

Condition Objects
-----------------

A condition variable is always associated with some kind of lock; this can be
passed in or one will be created by default.  (Passing one in is useful when
several condition variables must share the same lock.)

A condition variable has :meth:`acquire` and :meth:`release` methods that call
the corresponding methods of the associated lock. It also has a :meth:`wait`
method, and :meth:`notify` and :meth:`notify_all` methods.  These three must only
be called when the calling thread has acquired the lock, otherwise a
:exc:`RuntimeError` is raised.

The :meth:`wait` method releases the lock, and then blocks until it is awakened
by a :meth:`notify` or :meth:`notify_all` call for the same condition variable in
another thread.  Once awakened, it re-acquires the lock and returns.  It is also
possible to specify a timeout.

The :meth:`notify` method wakes up one of the threads waiting for the condition
variable, if any are waiting.  The :meth:`notify_all` method wakes up all threads
waiting for the condition variable.

Note: the :meth:`notify` and :meth:`notify_all` methods don't release the lock;
this means that the thread or threads awakened will not return from their
:meth:`wait` call immediately, but only when the thread that called
:meth:`notify` or :meth:`notify_all` finally relinquishes ownership of the lock.

Tip: the typical programming style using condition variables uses the lock to
synchronize access to some shared state; threads that are interested in a
particular change of state call :meth:`wait` repeatedly until they see the
desired state, while threads that modify the state call :meth:`notify` or
:meth:`notify_all` when they change the state in such a way that it could
possibly be a desired state for one of the waiters.  For example, the following
code is a generic producer-consumer situation with unlimited buffer capacity::

   # Consume one item
   cv.acquire()
   while not an_item_is_available():
       cv.wait()
   get_an_available_item()
   cv.release()

   # Produce one item
   cv.acquire()
   make_an_item_available()
   cv.notify()
   cv.release()

To choose between :meth:`notify` and :meth:`notify_all`, consider whether one
state change can be interesting for only one or several waiting threads.  E.g.
in a typical producer-consumer situation, adding one item to the buffer only
needs to wake up one consumer thread.

Note:  Condition variables can be, depending on the implementation, subject
to both spurious wakeups (when :meth:`wait` returns without a :meth:`notify`
call) and stolen wakeups (when another thread acquires the lock before the
awoken thread.)  For this reason, it is always necessary to verify the state
the thread is waiting for when :meth:`wait` returns and optionally repeat
the call as often as necessary.


.. class:: Condition(lock=None)

   If the *lock* argument is given and not ``None``, it must be a :class:`Lock`
   or :class:`RLock` object, and it is used as the underlying lock.  Otherwise,
   a new :class:`RLock` object is created and used as the underlying lock.

   .. method:: acquire(*args)

      Acquire the underlying lock. This method calls the corresponding method on
      the underlying lock; the return value is whatever that method returns.

   .. method:: release()

      Release the underlying lock. This method calls the corresponding method on
      the underlying lock; there is no return value.

   .. method:: wait(timeout=None)

      Wait until notified or until a timeout occurs. If the calling thread has
      not acquired the lock when this method is called, a :exc:`RuntimeError` is
      raised.

      This method releases the underlying lock, and then blocks until it is
      awakened by a :meth:`notify` or :meth:`notify_all` call for the same
      condition variable in another thread, or until the optional timeout
      occurs.  Once awakened or timed out, it re-acquires the lock and returns.

      When the *timeout* argument is present and not ``None``, it should be a
      floating point number specifying a timeout for the operation in seconds
      (or fractions thereof).

      When the underlying lock is an :class:`RLock`, it is not released using
      its :meth:`release` method, since this may not actually unlock the lock
      when it was acquired multiple times recursively.  Instead, an internal
      interface of the :class:`RLock` class is used, which really unlocks it
      even when it has been recursively acquired several times. Another internal
      interface is then used to restore the recursion level when the lock is
      reacquired.

      The return value is ``True`` unless a given *timeout* expired, in which
      case it is ``False``.

      .. versionchanged:: 3.2
         Previously, the method always returned ``None``.

   .. method:: wait_for(predicate, timeout=None)

      Wait until a condition evaluates to True.  *predicate* should be a
      callable which result will be interpreted as a boolean value.
      A *timeout* may be provided giving the maximum time to wait.

      This utility method may call :meth:`wait` repeatedly until the predicate
      is satisfied, or until a timeout occurs. The return value is
      the last return value of the predicate and will evaluate to
      ``False`` if the method timed out.

      Ignoring the timeout feature, calling this method is roughly equivalent to
      writing::

        while not predicate():
            cv.wait()

      Therefore, the same rules apply as with :meth:`wait`: The lock must be
      held when called and is re-aquired on return.  The predicate is evaluated
      with the lock held.

      Using this method, the consumer example above can be written thus::

         with cv:
             cv.wait_for(an_item_is_available)
             get_an_available_item()

      .. versionadded:: 3.2

   .. method:: notify()

      Wake up a thread waiting on this condition, if any.  If the calling thread
      has not acquired the lock when this method is called, a
      :exc:`RuntimeError` is raised.

      This method wakes up one of the threads waiting for the condition
      variable, if any are waiting; it is a no-op if no threads are waiting.

      The current implementation wakes up exactly one thread, if any are
      waiting.  However, it's not safe to rely on this behavior.  A future,
      optimized implementation may occasionally wake up more than one thread.

      Note: the awakened thread does not actually return from its :meth:`wait`
      call until it can reacquire the lock.  Since :meth:`notify` does not
      release the lock, its caller should.

   .. method:: notify_all()

      Wake up all threads waiting on this condition.  This method acts like
      :meth:`notify`, but wakes up all waiting threads instead of one. If the
      calling thread has not acquired the lock when this method is called, a
      :exc:`RuntimeError` is raised.


.. _semaphore-objects:

Semaphore Objects
-----------------

This is one of the oldest synchronization primitives in the history of computer
science, invented by the early Dutch computer scientist Edsger W. Dijkstra (he
used :meth:`P` and :meth:`V` instead of :meth:`acquire` and :meth:`release`).

A semaphore manages an internal counter which is decremented by each
:meth:`acquire` call and incremented by each :meth:`release` call.  The counter
can never go below zero; when :meth:`acquire` finds that it is zero, it blocks,
waiting until some other thread calls :meth:`release`.


.. class:: Semaphore(value=1)

   The optional argument gives the initial *value* for the internal counter; it
   defaults to ``1``. If the *value* given is less than 0, :exc:`ValueError` is
   raised.

   .. method:: acquire(blocking=True, timeout=None)

      Acquire a semaphore.

      When invoked without arguments: if the internal counter is larger than
      zero on entry, decrement it by one and return immediately.  If it is zero
      on entry, block, waiting until some other thread has called
      :meth:`release` to make it larger than zero.  This is done with proper
      interlocking so that if multiple :meth:`acquire` calls are blocked,
      :meth:`release` will wake exactly one of them up.  The implementation may
      pick one at random, so the order in which blocked threads are awakened
      should not be relied on.  Returns true (or blocks indefinitely).

      When invoked with *blocking* set to false, do not block.  If a call
      without an argument would block, return false immediately; otherwise,
      do the same thing as when called without arguments, and return true.

      When invoked with a *timeout* other than None, it will block for at
      most *timeout* seconds.  If acquire does not complete successfully in
      that interval, return false.  Return true otherwise.

      .. versionchanged:: 3.2
         The *timeout* parameter is new.

   .. method:: release()

      Release a semaphore, incrementing the internal counter by one.  When it
      was zero on entry and another thread is waiting for it to become larger
      than zero again, wake up that thread.


.. _semaphore-examples:

:class:`Semaphore` Example
^^^^^^^^^^^^^^^^^^^^^^^^^^

Semaphores are often used to guard resources with limited capacity, for example,
a database server.  In any situation where the size of the resource size is
fixed, you should use a bounded semaphore.  Before spawning any worker threads,
your main thread would initialize the semaphore::

   maxconnections = 5
   ...
   pool_sema = BoundedSemaphore(value=maxconnections)

Once spawned, worker threads call the semaphore's acquire and release methods
when they need to connect to the server::

   pool_sema.acquire()
   conn = connectdb()
   ... use connection ...
   conn.close()
   pool_sema.release()

The use of a bounded semaphore reduces the chance that a programming error which
causes the semaphore to be released more than it's acquired will go undetected.


.. _event-objects:

Event Objects
-------------

This is one of the simplest mechanisms for communication between threads: one
thread signals an event and other threads wait for it.

An event object manages an internal flag that can be set to true with the
:meth:`~Event.set` method and reset to false with the :meth:`clear` method.  The
:meth:`wait` method blocks until the flag is true.


.. class:: Event()

   The internal flag is initially false.

   .. method:: is_set()

      Return true if and only if the internal flag is true.

   .. method:: set()

      Set the internal flag to true. All threads waiting for it to become true
      are awakened. Threads that call :meth:`wait` once the flag is true will
      not block at all.

   .. method:: clear()

      Reset the internal flag to false. Subsequently, threads calling
      :meth:`wait` will block until :meth:`.set` is called to set the internal
      flag to true again.

   .. method:: wait(timeout=None)

      Block until the internal flag is true.  If the internal flag is true on
      entry, return immediately.  Otherwise, block until another thread calls
      :meth:`set` to set the flag to true, or until the optional timeout occurs.

      When the timeout argument is present and not ``None``, it should be a
      floating point number specifying a timeout for the operation in seconds
      (or fractions thereof).

      This method returns the internal flag on exit, so it will always return
      ``True`` except if a timeout is given and the operation times out.

      .. versionchanged:: 3.1
         Previously, the method always returned ``None``.


.. _timer-objects:

Timer Objects
-------------

This class represents an action that should be run only after a certain amount
of time has passed --- a timer.  :class:`Timer` is a subclass of :class:`Thread`
and as such also functions as an example of creating custom threads.

Timers are started, as with threads, by calling their :meth:`start` method.  The
timer can be stopped (before its action has begun) by calling the :meth:`cancel`
method.  The interval the timer will wait before executing its action may not be
exactly the same as the interval specified by the user.

For example::

   def hello():
       print("hello, world")

   t = Timer(30.0, hello)
   t.start() # after 30 seconds, "hello, world" will be printed


.. class:: Timer(interval, function, args=[], kwargs={})

   Create a timer that will run *function* with arguments *args* and  keyword
   arguments *kwargs*, after *interval* seconds have passed.

   .. method:: cancel()

      Stop the timer, and cancel the execution of the timer's action.  This will
      only work if the timer is still in its waiting stage.


Barrier Objects
---------------

.. versionadded:: 3.2

This class provides a simple synchronization primitive for use by a fixed number
of threads that need to wait for each other.  Each of the threads tries to pass
the barrier by calling the :meth:`wait` method and will block until all of the
threads have made the call.  At this points, the threads are released
simultanously.

The barrier can be reused any number of times for the same number of threads.

As an example, here is a simple way to synchronize a client and server thread::

   b = Barrier(2, timeout=5)

   def server():
       start_server()
       b.wait()
       while True:
           connection = accept_connection()
           process_server_connection(connection)

   def client():
       b.wait()
       while True:
           connection = make_connection()
           process_client_connection(connection)


.. class:: Barrier(parties, action=None, timeout=None)

   Create a barrier object for *parties* number of threads.  An *action*, when
   provided, is a callable to be called by one of the threads when they are
   released.  *timeout* is the default timeout value if none is specified for
   the :meth:`wait` method.

   .. method:: wait(timeout=None)

      Pass the barrier.  When all the threads party to the barrier have called
      this function, they are all released simultaneously.  If a *timeout* is
      provided, is is used in preference to any that was supplied to the class
      constructor.

      The return value is an integer in the range 0 to *parties* -- 1, different
      for each thrad.  This can be used to select a thread to do some special
      housekeeping, e.g.::

         i = barrier.wait()
         if i == 0:
             # Only one thread needs to print this
             print("passed the barrier")

      If an *action* was provided to the constructor, one of the threads will
      have called it prior to being released.  Should this call raise an error,
      the barrier is put into the broken state.

      If the call times out, the barrier is put into the broken state.

      This method may raise a :class:`BrokenBarrierError` exception if the
      barrier is broken or reset while a thread is waiting.

   .. method:: reset()

      Return the barrier to the default, empty state.  Any threads waiting on it
      will receive the :class:`BrokenBarrierError` exception.

      Note that using this function may can require some external
      synchronization if there are other threads whose state is unknown.  If a
      barrier is broken it may be better to just leave it and create a new one.

   .. method:: abort()

      Put the barrier into a broken state.  This causes any active or future
      calls to :meth:`wait` to fail with the :class:`BrokenBarrierError`.  Use
      this for example if one of the needs to abort, to avoid deadlocking the
      application.

      It may be preferable to simply create the barrier with a sensible
      *timeout* value to automatically guard against one of the threads going
      awry.

   .. attribute:: parties

      The number of threads required to pass the barrier.

   .. attribute:: n_waiting

      The number of threads currently waiting in the barrier.

   .. attribute:: broken

      A boolean that is ``True`` if the barrier is in the broken state.


.. exception:: BrokenBarrierError

   This exception, a subclass of :exc:`RuntimeError`, is raised when the
   :class:`Barrier` object is reset or broken.


.. _with-locks:

Using locks, conditions, and semaphores in the :keyword:`with` statement
------------------------------------------------------------------------

All of the objects provided by this module that have :meth:`acquire` and
:meth:`release` methods can be used as context managers for a :keyword:`with`
statement.  The :meth:`acquire` method will be called when the block is entered,
and :meth:`release` will be called when the block is exited.

Currently, :class:`Lock`, :class:`RLock`, :class:`Condition`,
:class:`Semaphore`, and :class:`BoundedSemaphore` objects may be used as
:keyword:`with` statement context managers.  For example::

   import threading

   some_rlock = threading.RLock()

   with some_rlock:
       print("some_rlock is locked while this executes")


.. _threaded-imports:

Importing in threaded code
--------------------------

While the import machinery is thread-safe, there are two key restrictions on
threaded imports due to inherent limitations in the way that thread-safety is
provided:

* Firstly, other than in the main module, an import should not have the
  side effect of spawning a new thread and then waiting for that thread in
  any way. Failing to abide by this restriction can lead to a deadlock if
  the spawned thread directly or indirectly attempts to import a module.
* Secondly, all import attempts must be completed before the interpreter
  starts shutting itself down. This can be most easily achieved by only
  performing imports from non-daemon threads created through the threading
  module. Daemon threads and threads created directly with the thread
  module will require some other form of synchronization to ensure they do
  not attempt imports after system shutdown has commenced. Failure to
  abide by this restriction will lead to intermittent exceptions and
  crashes during interpreter shutdown (as the late imports attempt to
  access machinery which is no longer in a valid state).
