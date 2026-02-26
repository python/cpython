.. highlight:: c

.. _synchronization:

Synchronization primitives
==========================

The C-API provides a basic mutual exclusion lock.

.. c:type:: PyMutex

   A mutual exclusion lock.  The :c:type:`!PyMutex` should be initialized to
   zero to represent the unlocked state.  For example::

      PyMutex mutex = {0};

   Instances of :c:type:`!PyMutex` should not be copied or moved.  Both the
   contents and address of a :c:type:`!PyMutex` are meaningful, and it must
   remain at a fixed, writable location in memory.

   .. note::

      A :c:type:`!PyMutex` currently occupies one byte, but the size should be
      considered unstable.  The size may change in future Python releases
      without a deprecation period.

   .. versionadded:: 3.13

.. c:function:: void PyMutex_Lock(PyMutex *m)

   Lock mutex *m*.  If another thread has already locked it, the calling
   thread will block until the mutex is unlocked.  While blocked, the thread
   will temporarily detach the :term:`thread state <attached thread state>` if one exists.

   .. versionadded:: 3.13

.. c:function:: void PyMutex_Unlock(PyMutex *m)

   Unlock mutex *m*. The mutex must be locked --- otherwise, the function will
   issue a fatal error.

   .. versionadded:: 3.13

.. c:function:: int PyMutex_IsLocked(PyMutex *m)

   Returns non-zero if the mutex *m* is currently locked, zero otherwise.

   .. note::

      This function is intended for use in assertions and debugging only and
      should not be used to make concurrency control decisions, as the lock
      state may change immediately after the check.

   .. versionadded:: 3.14

.. _python-critical-section-api:

Python critical section API
---------------------------

The critical section API provides a deadlock avoidance layer on top of
per-object locks for :term:`free-threaded <free threading>` CPython.  They are
intended to replace reliance on the :term:`global interpreter lock`, and are
no-ops in versions of Python with the global interpreter lock.

Critical sections are intended to be used for custom types implemented
in C-API extensions. They should generally not be used with built-in types like
:class:`list` and :class:`dict` because their public C-APIs
already use critical sections internally, with the notable
exception of :c:func:`PyDict_Next`, which requires critical section
to be acquired externally.

Critical sections avoid deadlocks by implicitly suspending active critical
sections, hence, they do not provide exclusive access such as provided by
traditional locks like :c:type:`PyMutex`.  When a critical section is started,
the per-object lock for the object is acquired. If the code executed inside the
critical section calls C-API functions then it can suspend the critical section thereby
releasing the per-object lock, so other threads can acquire the per-object lock
for the same object.

Variants that accept :c:type:`PyMutex` pointers rather than Python objects are also
available. Use these variants to start a critical section in a situation where
there is no :c:type:`PyObject` -- for example, when working with a C type that
does not extend or wrap :c:type:`PyObject` but still needs to call into the C
API in a manner that might lead to deadlocks.

The functions and structs used by the macros are exposed for cases
where C macros are not available. They should only be used as in the
given macro expansions. Note that the sizes and contents of the structures may
change in future Python versions.

.. note::

   Operations that need to lock two objects at once must use
   :c:macro:`Py_BEGIN_CRITICAL_SECTION2`.  You *cannot* use nested critical
   sections to lock more than one object at once, because the inner critical
   section may suspend the outer critical sections.  This API does not provide
   a way to lock more than two objects at once.

Example usage::

   static PyObject *
   set_field(MyObject *self, PyObject *value)
   {
      Py_BEGIN_CRITICAL_SECTION(self);
      Py_SETREF(self->field, Py_XNewRef(value));
      Py_END_CRITICAL_SECTION();
      Py_RETURN_NONE;
   }

In the above example, :c:macro:`Py_SETREF` calls :c:macro:`Py_DECREF`, which
can call arbitrary code through an object's deallocation function.  The critical
section API avoids potential deadlocks due to reentrancy and lock ordering
by allowing the runtime to temporarily suspend the critical section if the
code triggered by the finalizer blocks and calls :c:func:`PyEval_SaveThread`.

.. c:macro:: Py_BEGIN_CRITICAL_SECTION(op)

   Acquires the per-object lock for the object *op* and begins a
   critical section.

   In the free-threaded build, this macro expands to::

      {
          PyCriticalSection _py_cs;
          PyCriticalSection_Begin(&_py_cs, (PyObject*)(op))

   In the default build, this macro expands to ``{``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION_MUTEX(m)

   Locks the mutex *m* and begins a critical section.

   In the free-threaded build, this macro expands to::

     {
          PyCriticalSection _py_cs;
          PyCriticalSection_BeginMutex(&_py_cs, m)

   Note that unlike :c:macro:`Py_BEGIN_CRITICAL_SECTION`, there is no cast for
   the argument of the macro - it must be a :c:type:`PyMutex` pointer.

   On the default build, this macro expands to ``{``.

   .. versionadded:: 3.14

.. c:macro:: Py_END_CRITICAL_SECTION()

   Ends the critical section and releases the per-object lock.

   In the free-threaded build, this macro expands to::

          PyCriticalSection_End(&_py_cs);
      }

   In the default build, this macro expands to ``}``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION2(a, b)

   Acquires the per-object locks for the objects *a* and *b* and begins a
   critical section.  The locks are acquired in a consistent order (lowest
   address first) to avoid lock ordering deadlocks.

   In the free-threaded build, this macro expands to::

      {
          PyCriticalSection2 _py_cs2;
          PyCriticalSection2_Begin(&_py_cs2, (PyObject*)(a), (PyObject*)(b))

   In the default build, this macro expands to ``{``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION2_MUTEX(m1, m2)

   Locks the mutexes *m1* and *m2* and begins a critical section.

   In the free-threaded build, this macro expands to::

     {
          PyCriticalSection2 _py_cs2;
          PyCriticalSection2_BeginMutex(&_py_cs2, m1, m2)

   Note that unlike :c:macro:`Py_BEGIN_CRITICAL_SECTION2`, there is no cast for
   the arguments of the macro - they must be :c:type:`PyMutex` pointers.

   On the default build, this macro expands to ``{``.

   .. versionadded:: 3.14

.. c:macro:: Py_END_CRITICAL_SECTION2()

   Ends the critical section and releases the per-object locks.

   In the free-threaded build, this macro expands to::

          PyCriticalSection2_End(&_py_cs2);
      }

   In the default build, this macro expands to ``}``.

   .. versionadded:: 3.13


Legacy locking APIs
-------------------

These APIs are obsolete since Python 3.13 with the introduction of
:c:type:`PyMutex`.

.. versionchanged:: 3.15
   These APIs are now a simple wrapper around ``PyMutex``.


.. c:type:: PyThread_type_lock

   A pointer to a mutual exclusion lock.


.. c:type:: PyLockStatus

   The result of acquiring a lock with a timeout.

   .. c:namespace:: NULL

   .. c:enumerator:: PY_LOCK_FAILURE

      Failed to acquire the lock.

   .. c:enumerator:: PY_LOCK_ACQUIRED

      The lock was successfully acquired.

   .. c:enumerator:: PY_LOCK_INTR

      The lock was interrupted by a signal.


.. c:function:: PyThread_type_lock PyThread_allocate_lock(void)

   Allocate a new lock.

   On success, this function returns a lock; on failure, this
   function returns ``0`` without an exception set.

   The caller does not need to hold an :term:`attached thread state`.

   .. versionchanged:: 3.15
      This function now always uses :c:type:`PyMutex`. In prior versions, this
      would use a lock provided by the operating system.


.. c:function:: void PyThread_free_lock(PyThread_type_lock lock)

   Destroy *lock*. The lock should not be held by any thread when calling
   this.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: PyLockStatus PyThread_acquire_lock_timed(PyThread_type_lock lock, long long microseconds, int intr_flag)

   Acquire *lock* with a timeout.

   This will wait for *microseconds* microseconds to acquire the lock. If the
   timeout expires, this function returns :c:enumerator:`PY_LOCK_FAILURE`.
   If *microseconds* is ``-1``, this will wait indefinitely until the lock has
   been released.

   If *intr_flag* is ``1``, acquiring the lock may be interrupted by a signal,
   in which case this function returns :c:enumerator:`PY_LOCK_INTR`. Upon
   interruption, it's generally expected that the caller makes a call to
   :c:func:`Py_MakePendingCalls` to propagate an exception to Python code.

   If the lock is successfully acquired, this function returns
   :c:enumerator:`PY_LOCK_ACQUIRED`.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)

   Acquire *lock*.

   If *waitflag* is ``1`` and another thread currently holds the lock, this
   function will wait until the lock can be acquired and will always return
   ``1``.

   If *waitflag* is ``0`` and another thread holds the lock, this function will
   not wait and instead return ``0``. If the lock is not held by any other
   thread, then this function will acquire it and return ``1``.

   Unlike :c:func:`PyThread_acquire_lock_timed`, acquiring the lock cannot be
   interrupted by a signal.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: int PyThread_release_lock(PyThread_type_lock lock)

   Release *lock*. If *lock* is not held, then this function issues a
   fatal error.

   The caller does not need to hold an :term:`attached thread state`.
