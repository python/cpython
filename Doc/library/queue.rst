:mod:`!queue` --- A synchronized queue class
============================================

.. module:: queue
   :synopsis: A synchronized queue class.

**Source code:** :source:`Lib/queue.py`

--------------

The :mod:`!queue` module implements multi-producer, multi-consumer queues.
It is especially useful in threaded programming when information must be
exchanged safely between multiple threads.  The :class:`Queue` class in this
module implements all the required locking semantics.

The module implements three types of queue, which differ only in the order in
which the entries are retrieved.  In a :abbr:`FIFO (first-in, first-out)`
queue, the first tasks added are the first retrieved.  In a
:abbr:`LIFO (last-in, first-out)` queue, the most recently added entry is
the first retrieved (operating like a stack).  With a priority queue,
the entries are kept sorted (using the :mod:`heapq` module) and the
lowest valued entry is retrieved first.

Internally, those three types of queues use locks to temporarily block
competing threads; however, they are not designed to handle reentrancy
within a thread.

In addition, the module implements a "simple"
:abbr:`FIFO (first-in, first-out)` queue type, :class:`SimpleQueue`, whose
specific implementation provides additional guarantees
in exchange for the smaller functionality.

The :mod:`!queue` module defines the following classes and exceptions:

.. class:: Queue(maxsize=0)

   Constructor for a :abbr:`FIFO (first-in, first-out)` queue.  *maxsize* is
   an integer that sets the upperbound
   limit on the number of items that can be placed in the queue.  Insertion will
   block once this size has been reached, until queue items are consumed.  If
   *maxsize* is less than or equal to zero, the queue size is infinite.

.. class:: LifoQueue(maxsize=0)

   Constructor for a :abbr:`LIFO (last-in, first-out)` queue.  *maxsize* is
   an integer that sets the upperbound
   limit on the number of items that can be placed in the queue.  Insertion will
   block once this size has been reached, until queue items are consumed.  If
   *maxsize* is less than or equal to zero, the queue size is infinite.


.. class:: PriorityQueue(maxsize=0)

   Constructor for a priority queue.  *maxsize* is an integer that sets the upperbound
   limit on the number of items that can be placed in the queue.  Insertion will
   block once this size has been reached, until queue items are consumed.  If
   *maxsize* is less than or equal to zero, the queue size is infinite.

   The lowest valued entries are retrieved first (the lowest valued entry is the
   one that would be returned by ``min(entries)``).  A typical pattern for
   entries is a tuple in the form: ``(priority_number, data)``.

   If the *data* elements are not comparable, the data can be wrapped in a class
   that ignores the data item and only compares the priority number::

        from dataclasses import dataclass, field
        from typing import Any

        @dataclass(order=True)
        class PrioritizedItem:
            priority: int
            item: Any=field(compare=False)

.. class:: SimpleQueue()

   Constructor for an unbounded :abbr:`FIFO (first-in, first-out)` queue.
   Simple queues lack advanced functionality such as task tracking.

   .. versionadded:: 3.7


.. exception:: Empty

   Exception raised when non-blocking :meth:`~Queue.get` (or
   :meth:`~Queue.get_nowait`) is called
   on a :class:`Queue` object which is empty.


.. exception:: Full

   Exception raised when non-blocking :meth:`~Queue.put` (or
   :meth:`~Queue.put_nowait`) is called
   on a :class:`Queue` object which is full.


.. exception:: ShutDown

   Exception raised when :meth:`~Queue.put` or :meth:`~Queue.get` is called on
   a :class:`Queue` object which has been shut down.

   .. versionadded:: 3.13


.. _queueobjects:

Queue Objects
-------------

Queue objects (:class:`Queue`, :class:`LifoQueue`, or :class:`PriorityQueue`)
provide the public methods described below.


.. method:: Queue.qsize()

   Return the approximate size of the queue.  Note, qsize() > 0 doesn't
   guarantee that a subsequent get() will not block, nor will qsize() < maxsize
   guarantee that put() will not block.


.. method:: Queue.empty()

   Return ``True`` if the queue is empty, ``False`` otherwise.  If empty()
   returns ``True`` it doesn't guarantee that a subsequent call to put()
   will not block.  Similarly, if empty() returns ``False`` it doesn't
   guarantee that a subsequent call to get() will not block.


.. method:: Queue.full()

   Return ``True`` if the queue is full, ``False`` otherwise.  If full()
   returns ``True`` it doesn't guarantee that a subsequent call to get()
   will not block.  Similarly, if full() returns ``False`` it doesn't
   guarantee that a subsequent call to put() will not block.


.. method:: Queue.put(item, block=True, timeout=None)

   Put *item* into the queue.  If optional args *block* is true and *timeout* is
   ``None`` (the default), block if necessary until a free slot is available.  If
   *timeout* is a positive number, it blocks at most *timeout* seconds and raises
   the :exc:`Full` exception if no free slot was available within that time.
   Otherwise (*block* is false), put an item on the queue if a free slot is
   immediately available, else raise the :exc:`Full` exception (*timeout* is
   ignored in that case).

   Raises :exc:`ShutDown` if the queue has been shut down.


.. method:: Queue.put_nowait(item)

   Equivalent to ``put(item, block=False)``.


.. method:: Queue.get(block=True, timeout=None)

   Remove and return an item from the queue.  If optional args *block* is true and
   *timeout* is ``None`` (the default), block if necessary until an item is available.
   If *timeout* is a positive number, it blocks at most *timeout* seconds and
   raises the :exc:`Empty` exception if no item was available within that time.
   Otherwise (*block* is false), return an item if one is immediately available,
   else raise the :exc:`Empty` exception (*timeout* is ignored in that case).

   Prior to 3.0 on POSIX systems, and for all versions on Windows, if
   *block* is true and *timeout* is ``None``, this operation goes into
   an uninterruptible wait on an underlying lock.  This means that no exceptions
   can occur, and in particular a SIGINT will not trigger a :exc:`KeyboardInterrupt`.

   Raises :exc:`ShutDown` if the queue has been shut down and is empty, or if
   the queue has been shut down immediately.


.. method:: Queue.get_nowait()

   Equivalent to ``get(False)``.

Two methods are offered to support tracking whether enqueued tasks have been
fully processed by daemon consumer threads.


.. method:: Queue.task_done()

   Indicate that a formerly enqueued task is complete.  Used by queue consumer
   threads.  For each :meth:`get` used to fetch a task, a subsequent call to
   :meth:`task_done` tells the queue that the processing on the task is complete.

   If a :meth:`join` is currently blocking, it will resume when all items have been
   processed (meaning that a :meth:`task_done` call was received for every item
   that had been :meth:`put` into the queue).

   Raises a :exc:`ValueError` if called more times than there were items placed in
   the queue.


.. method:: Queue.join()

   Blocks until all items in the queue have been gotten and processed.

   The count of unfinished tasks goes up whenever an item is added to the queue.
   The count goes down whenever a consumer thread calls :meth:`task_done` to
   indicate that the item was retrieved and all work on it is complete.  When the
   count of unfinished tasks drops to zero, :meth:`join` unblocks.


Waiting for task completion
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Example of how to wait for enqueued tasks to be completed::

    import threading
    import queue

    q = queue.Queue()

    def worker():
        while True:
            item = q.get()
            print(f'Working on {item}')
            print(f'Finished {item}')
            q.task_done()

    # Turn-on the worker thread.
    threading.Thread(target=worker, daemon=True).start()

    # Send thirty task requests to the worker.
    for item in range(30):
        q.put(item)

    # Block until all tasks are done.
    q.join()
    print('All work completed')


Terminating queues
^^^^^^^^^^^^^^^^^^

When no longer needed, :class:`Queue` objects can be wound down
until empty or terminated immediately with a hard shutdown.

.. method:: Queue.shutdown(immediate=False)

   Put a :class:`Queue` instance into a shutdown mode.

   The queue can no longer grow.
   Future calls to :meth:`~Queue.put` raise :exc:`ShutDown`.
   Currently blocked callers of :meth:`~Queue.put` will be unblocked
   and will raise :exc:`ShutDown` in the formerly blocked thread.

   If *immediate* is false (the default), the queue can be wound
   down normally with :meth:`~Queue.get` calls to extract tasks
   that have already been loaded.

   And if :meth:`~Queue.task_done` is called for each remaining task, a
   pending :meth:`~Queue.join` will be unblocked normally.

   Once the queue is empty, future calls to :meth:`~Queue.get` will
   raise :exc:`ShutDown`.

   If *immediate* is true, the queue is terminated immediately.
   The queue is drained to be completely empty and the count
   of unfinished tasks is reduced by the number of tasks drained.
   If unfinished tasks is zero, callers of :meth:`~Queue.join`
   are unblocked.  Also, blocked callers of :meth:`~Queue.get`
   are unblocked and will raise :exc:`ShutDown` because the
   queue is empty.

   Use caution when using :meth:`~Queue.join` with *immediate* set
   to true. This unblocks the join even when no work has been done
   on the tasks, violating the usual invariant for joining a queue.

   .. versionadded:: 3.13


SimpleQueue Objects
-------------------

:class:`SimpleQueue` objects provide the public methods described below.

.. method:: SimpleQueue.qsize()

   Return the approximate size of the queue.  Note, qsize() > 0 doesn't
   guarantee that a subsequent get() will not block.


.. method:: SimpleQueue.empty()

   Return ``True`` if the queue is empty, ``False`` otherwise.  If empty()
   returns ``False`` it doesn't guarantee that a subsequent call to get()
   will not block.


.. method:: SimpleQueue.put(item, block=True, timeout=None)

   Put *item* into the queue.  The method never blocks and always succeeds
   (except for potential low-level errors such as failure to allocate memory).
   The optional args *block* and *timeout* are ignored and only provided
   for compatibility with :meth:`Queue.put`.

   .. impl-detail::
      This method has a C implementation which is reentrant.  That is, a
      ``put()`` or ``get()`` call can be interrupted by another ``put()``
      call in the same thread without deadlocking or corrupting internal
      state inside the queue.  This makes it appropriate for use in
      destructors such as ``__del__`` methods or :mod:`weakref` callbacks.


.. method:: SimpleQueue.put_nowait(item)

   Equivalent to ``put(item, block=False)``, provided for compatibility with
   :meth:`Queue.put_nowait`.


.. method:: SimpleQueue.get(block=True, timeout=None)

   Remove and return an item from the queue.  If optional args *block* is true and
   *timeout* is ``None`` (the default), block if necessary until an item is available.
   If *timeout* is a positive number, it blocks at most *timeout* seconds and
   raises the :exc:`Empty` exception if no item was available within that time.
   Otherwise (*block* is false), return an item if one is immediately available,
   else raise the :exc:`Empty` exception (*timeout* is ignored in that case).


.. method:: SimpleQueue.get_nowait()

   Equivalent to ``get(False)``.


.. seealso::

   Class :class:`multiprocessing.Queue`
      A queue class for use in a multi-processing (rather than multi-threading)
      context.

   :class:`collections.deque` is an alternative implementation of unbounded
   queues with fast atomic :meth:`~collections.deque.append` and
   :meth:`~collections.deque.popleft` operations that do not require locking
   and also support indexing.
