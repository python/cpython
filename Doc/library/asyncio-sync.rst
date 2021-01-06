.. currentmodule:: asyncio

.. _asyncio-sync:

==========================
Synchronization Primitives
==========================

**Source code:** :source:`Lib/asyncio/locks.py`

-----------------------------------------------

asyncio synchronization primitives are designed to be similar to
those of the :mod:`threading` module with two important caveats:

* asyncio primitives are not thread-safe, therefore they should not
  be used for OS thread synchronization (use :mod:`threading` for
  that);

* methods of these synchronization primitives do not accept the *timeout*
  argument; use the :func:`asyncio.wait_for` function to perform
  operations with timeouts.

asyncio has the following basic synchronization primitives:

* :class:`Lock`
* :class:`Event`
* :class:`Condition`
* :class:`Semaphore`
* :class:`BoundedSemaphore`


---------


Lock
====

.. class:: Lock(\*, loop=None)

   Implements a mutex lock for asyncio tasks.  Not thread-safe.

   An asyncio lock can be used to guarantee exclusive access to a
   shared resource.

   The preferred way to use a Lock is an :keyword:`async with`
   statement::

       lock = asyncio.Lock()

       # ... later
       async with lock:
           # access shared state

   which is equivalent to::

       lock = asyncio.Lock()

       # ... later
       await lock.acquire()
       try:
           # access shared state
       finally:
           lock.release()

   .. deprecated-removed:: 3.8 3.10
      The *loop* parameter.

   .. coroutinemethod:: acquire()

      Acquire the lock.

      This method waits until the lock is *unlocked*, sets it to
      *locked* and returns ``True``.

      When more than one coroutine is blocked in :meth:`acquire`
      waiting for the lock to be unlocked, only one coroutine
      eventually proceeds.

      Acquiring a lock is *fair*: the coroutine that proceeds will be
      the first coroutine that started waiting on the lock.

   .. method:: release()

      Release the lock.

      When the lock is *locked*, reset it to *unlocked* and return.

      If the lock is *unlocked*, a :exc:`RuntimeError` is raised.

   .. method:: locked()

      Return ``True`` if the lock is *locked*.


Event
=====

.. class:: Event(\*, loop=None)

   An event object.  Not thread-safe.

   An asyncio event can be used to notify multiple asyncio tasks
   that some event has happened.

   An Event object manages an internal flag that can be set to *true*
   with the :meth:`~Event.set` method and reset to *false* with the
   :meth:`clear` method.  The :meth:`~Event.wait` method blocks until the
   flag is set to *true*.  The flag is set to *false* initially.


   .. deprecated-removed:: 3.8 3.10
      The *loop* parameter.

   .. _asyncio_example_sync_event:

   Example::

      async def waiter(event):
          print('waiting for it ...')
          await event.wait()
          print('... got it!')

      async def main():
          # Create an Event object.
          event = asyncio.Event()

          # Spawn a Task to wait until 'event' is set.
          waiter_task = asyncio.create_task(waiter(event))

          # Sleep for 1 second and set the event.
          await asyncio.sleep(1)
          event.set()

          # Wait until the waiter task is finished.
          await waiter_task

      asyncio.run(main())

   .. coroutinemethod:: wait()

      Wait until the event is set.

      If the event is set, return ``True`` immediately.
      Otherwise block until another task calls :meth:`~Event.set`.

   .. method:: set()

      Set the event.

      All tasks waiting for event to be set will be immediately
      awakened.

   .. method:: clear()

      Clear (unset) the event.

      Tasks awaiting on :meth:`~Event.wait` will now block until the
      :meth:`~Event.set` method is called again.

   .. method:: is_set()

      Return ``True`` if the event is set.


Condition
=========

.. class:: Condition(lock=None, \*, loop=None)

   A Condition object.  Not thread-safe.

   An asyncio condition primitive can be used by a task to wait for
   some event to happen and then get exclusive access to a shared
   resource.

   In essence, a Condition object combines the functionality
   of an :class:`Event` and a :class:`Lock`.  It is possible to have
   multiple Condition objects share one Lock, which allows coordinating
   exclusive access to a shared resource between different tasks
   interested in particular states of that shared resource.

   The optional *lock* argument must be a :class:`Lock` object or
   ``None``.  In the latter case a new Lock object is created
   automatically.


   .. deprecated-removed:: 3.8 3.10
      The *loop* parameter.

   The preferred way to use a Condition is an :keyword:`async with`
   statement::

       cond = asyncio.Condition()

       # ... later
       async with cond:
           await cond.wait()

   which is equivalent to::

       cond = asyncio.Condition()

       # ... later
       await cond.acquire()
       try:
           await cond.wait()
       finally:
           cond.release()

   .. coroutinemethod:: acquire()

      Acquire the underlying lock.

      This method waits until the underlying lock is *unlocked*,
      sets it to *locked* and returns ``True``.

   .. method:: notify(n=1)

      Wake up at most *n* tasks (1 by default) waiting on this
      condition.  The method is no-op if no tasks are waiting.

      The lock must be acquired before this method is called and
      released shortly after.  If called with an *unlocked* lock
      a :exc:`RuntimeError` error is raised.

   .. method:: locked()

      Return ``True`` if the underlying lock is acquired.

   .. method:: notify_all()

      Wake up all tasks waiting on this condition.

      This method acts like :meth:`notify`, but wakes up all waiting
      tasks.

      The lock must be acquired before this method is called and
      released shortly after.  If called with an *unlocked* lock
      a :exc:`RuntimeError` error is raised.

   .. method:: release()

      Release the underlying lock.

      When invoked on an unlocked lock, a :exc:`RuntimeError` is
      raised.

   .. coroutinemethod:: wait()

      Wait until notified.

      If the calling task has not acquired the lock when this method is
      called, a :exc:`RuntimeError` is raised.

      This method releases the underlying lock, and then blocks until
      it is awakened by a :meth:`notify` or :meth:`notify_all` call.
      Once awakened, the Condition re-acquires its lock and this method
      returns ``True``.

   .. coroutinemethod:: wait_for(predicate)

      Wait until a predicate becomes *true*.

      The predicate must be a callable which result will be
      interpreted as a boolean value.  The final value is the
      return value.


Semaphore
=========

.. class:: Semaphore(value=1, \*, loop=None)

   A Semaphore object.  Not thread-safe.

   A semaphore manages an internal counter which is decremented by each
   :meth:`acquire` call and incremented by each :meth:`release` call.
   The counter can never go below zero; when :meth:`acquire` finds
   that it is zero, it blocks, waiting until some task calls
   :meth:`release`.

   The optional *value* argument gives the initial value for the
   internal counter (``1`` by default). If the given value is
   less than ``0`` a :exc:`ValueError` is raised.


   .. deprecated-removed:: 3.8 3.10
      The *loop* parameter.

   The preferred way to use a Semaphore is an :keyword:`async with`
   statement::

       sem = asyncio.Semaphore(10)

       # ... later
       async with sem:
           # work with shared resource

   which is equivalent to::

       sem = asyncio.Semaphore(10)

       # ... later
       await sem.acquire()
       try:
           # work with shared resource
       finally:
           sem.release()

   .. coroutinemethod:: acquire()

      Acquire a semaphore.

      If the internal counter is greater than zero, decrement
      it by one and return ``True`` immediately.  If it is zero, wait
      until a :meth:`release` is called and return ``True``.

   .. method:: locked()

      Returns ``True`` if semaphore can not be acquired immediately.

   .. method:: release()

      Release a semaphore, incrementing the internal counter by one.
      Can wake up a task waiting to acquire the semaphore.

      Unlike :class:`BoundedSemaphore`, :class:`Semaphore` allows
      making more ``release()`` calls than ``acquire()`` calls.


BoundedSemaphore
================

.. class:: BoundedSemaphore(value=1, \*, loop=None)

   A bounded semaphore object.  Not thread-safe.

   Bounded Semaphore is a version of :class:`Semaphore` that raises
   a :exc:`ValueError` in :meth:`~Semaphore.release` if it
   increases the internal counter above the initial *value*.


   .. deprecated-removed:: 3.8 3.10
      The *loop* parameter.

---------


.. versionchanged:: 3.9

   Acquiring a lock using ``await lock`` or ``yield from lock`` and/or
   :keyword:`with` statement (``with await lock``, ``with (yield from
   lock)``) was removed.  Use ``async with lock`` instead.
