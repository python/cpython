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
* :class:`Barrier`


---------


Lock
====

.. class:: Lock()

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

.. class:: Event()

   An event object.  Not thread-safe.

   An asyncio event can be used to notify multiple asyncio tasks
   that some event has happened.

   An Event object manages an internal flag that can be set to *true*
   with the :meth:`~Event.set` method and reset to *false* with the
   :meth:`clear` method.  The :meth:`~Event.wait` method blocks until the
   flag is set to *true*.  The flag is set to *false* initially.

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

.. class:: Condition(lock=None)

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

.. class:: Semaphore(value=1)

   A Semaphore object.  Not thread-safe.

   A semaphore manages an internal counter which is decremented by each
   :meth:`acquire` call and incremented by each :meth:`release` call.
   The counter can never go below zero; when :meth:`acquire` finds
   that it is zero, it blocks, waiting until some task calls
   :meth:`release`.

   The optional *value* argument gives the initial value for the
   internal counter (``1`` by default). If the given value is
   less than ``0`` a :exc:`ValueError` is raised.

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

.. class:: BoundedSemaphore(value=1)

   A bounded semaphore object.  Not thread-safe.

   Bounded Semaphore is a version of :class:`Semaphore` that raises
   a :exc:`ValueError` in :meth:`~Semaphore.release` if it
   increases the internal counter above the initial *value*.


Barrier
=======

   .. versionadded:: 3.10

   A barrier object.  Not thread-safe.

   This class, a clone of :class:`threading.Barrier`, provides a simple synchronization
   primitive for use by a fixed number of tasks that need to wait for each other.
   Each of the task tries to pass the barrier by calling the :meth:`~Barrier.wait` method
   and will block until all of the tasks have made their :meth:`~Barrier.wait` calls.
   At this point, the tasks are released simultaneously.

   The barrier can be reused any number of times for the same number of tasks.

   .. _asyncio_example_barrier:

   Example::

       import asyncio
       import os
       import random

       DELAYS = (1.0, 4.0, 2.0, 0.5)

       def job_done():
          msg = "ACTION: All files are removed"
          print(f'{os.linesep}{msg:-^80s}{os.linesep}')

       async def remove_file(barrier, filename):
          print(f'Remove "{filename}" ...')

          # remove filename
          await asyncio.sleep(random.choice(DELAYS)) # simulate operation
          print(f'\t"{filename}" is removed')

          return await barrier.wait()

       async def remove_folder(barrier, folder):
          print(f'Waiting for remove all files from folder "{folder}" ...')

          # wait for all removed files
          await barrier.wait()

          # remove folder
          await asyncio.sleep(1.0) # simulates operation
          print(f'Folder "{folder}" is now removed ...')

          return -1

       async def main(folder, files, nb_files):
          # Create a Barrier object with parties equal to nb_files+1
          # corresponding to the number of all removing items,
          # and an action as a print message.
          barrier = asyncio.Barrier(nb_files+1, action=job_done)

          # Add remove_file task for each filename.
          tasks = [asyncio.create_task(remove_file(barrier, f)) for f in files]

          # Add remove_folder task for the folder.
          tasks.append(asyncio.create_task(remove_folder(barrier, folder)))

          # Wait until folder+filenames are removed.
          await asyncio.gather(*tasks)

       asyncio.run(main("Datas", [f"file{i+1:02d}.txt" for i in range(10)], 10))

.. class:: Barrier(parties, action=None)

   Create a barrier object for *parties* number of tasks.  A callable *action*, when
   provided, is called once at the begining of draining.

   .. coroutinemethod:: wait()

      Pass the barrier.  When all the tasks party to the barrier have called
      this function, they are all released simultaneously.

      The return value is an integer in the range 0 to *parties* -- 1, different
      for each task.  This can be used to select a task to do some special
      housekeeping, e.g.::

         ...
          file_position = await barrier.wait()
          if file_position == 0:
             # Only one tasks needs to print this
             print(f"I have the position #{file_position}")

      If an *action* was provided to the constructor, the last calling task of :meth:`wait` method will
      have called it prior to being released.  Should this call raise an error,
      the barrier is put into the broken state.

      This method may raise a :class:`BrokenBarrierError` exception if the
      barrier is broken or reset while a task is waiting.

   .. method:: reset()

      Return the barrier to the default, empty state.  Any tasks waiting on it
      will receive the :class:`BrokenBarrierError` exception.

      If a barrier is broken it may be better to just leave it and create a new one.

   .. method:: abort()

      Put the barrier into a broken state.  This causes any active or future
      calls to :meth:`wait` to fail with the :class:`BrokenBarrierError`.
      Use this for example if one of the taks needs to abort, to avoid infinite waiting tasks.

   .. attribute:: parties

      The number of tasks required to pass the barrier.

   .. attribute:: n_waiting

      The number of tasks currently waiting in the barrier.

   .. attribute:: broken

      A boolean that is ``True`` if the barrier is in the broken state.


.. exception:: BrokenBarrierError

   This exception, a subclass of :exc:`RuntimeError`, is raised when the
   :class:`Barrier` object is reset or broken.

---------


.. versionchanged:: 3.9

   Acquiring a lock using ``await lock`` or ``yield from lock`` and/or
   :keyword:`with` statement (``with await lock``, ``with (yield from
   lock)``) was removed.  Use ``async with lock`` instead.
