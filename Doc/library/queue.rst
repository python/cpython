:mod:`queue` --- A synchronized queue class
===========================================

.. module:: queue
   :synopsis: A synchronized queue class.

**Source code:** :source:`Lib/queue.py`

--------------

The :mod:`queue` module implements multi-producer, multi-consumer queues.
It is especially useful in threaded programming when information must be
exchanged safely between multiple threads.  The :class:`Queue` class in this
module implements all the required locking semantics.

The module implements three types of queue, which differ only in the order in
which the entries are retrieved.  In a :abbr:`FIFO (first-in, first-out)`
queue, the first tasks added are the first retrieved. In a
:abbr:`LIFO (last-in, first-out)` queue, the most recently added entry is
the first retrieved (operating like a stack).  With a priority queue,
the entries are kept sorted (using the :mod:`heapq` module) and the
lowest valued entry is retrieved first.

Internally, those three types of queues use locks to temporarily block
competing threads; however, they are not designed to handle reentrancy
within a thread.

In addition, the module implements a "simple"
:abbr:`FIFO (first-in, first-out)` queue type where
specific implementations can provide additional guarantees
in exchange for the smaller functionality.

The :mod:`queue` module defines the following classes and exceptions:

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
   one returned by ``sorted(list(entries))[0]``).  A typical pattern for entries
   is a tuple in the form: ``(priority_number, data)``.

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


.. exception:: Closed

   Sub-exception of :exc:`Full` raised when :meth:`~Queue.put`
   (or :meth:`~Queue.put_nowait`) is called on a :class:`Queue`
   object which is closed.
   Also raised in a blocking :meth:`~Queue.put` when the :class:`Queue`
   object becomes closed while the method is blocking.


.. exception:: Exhausted

   Sub-exception of :exc:`Empty` raised when :meth:`~Queue.get`
   (or :meth:`~Queue.get_nowait`) is called on a :class:`Queue`
   object which is exhausted, i.e. closed and empty.
   Also raised in a blocking :meth:`~Queue.get` when the :class:`Queue`
   object becomes exhausted while the method is blocking.


.. _queueobjects:

Queue Objects
-------------

Queue objects (:class:`Queue`, :class:`LifoQueue`, or :class:`PriorityQueue`)
provide the public methods described below.


.. method:: Queue.qsize()

   Return the approximate size of the queue.  Note, qsize() > 0 doesn't
   guarantee that a subsequent :meth:`get` will not block, nor will
   qsize() < maxsize guarantee that :meth:`put` will not block.


.. method:: Queue.empty()

   Return ``True`` if the queue is empty, ``False`` otherwise.  If empty()
   returns ``True`` it doesn't guarantee that a subsequent call to :meth:`put`
   will not block.  Similarly, if empty() returns ``False`` it doesn't
   guarantee that a subsequent call to :meth:`get` will not block.


.. method:: Queue.full()

   Return ``True`` if the queue is full, ``False`` otherwise.  The queue is
   considered full if it currently has no free slots available for new items,
   either because its *maxsize* capacity is filled, or because it is closed.
   If full() returns ``True`` it doesn't guarantee that a subsequent call to
   :meth:`get` will not block.  Similarly, if full() returns ``False`` it
   doesn't guarantee that a subsequent call to :meth:`put` will not block.


.. method:: Queue.closed()

   Return ``True`` if the queue is closed, ``False`` otherwise.  The queue can
   only be closed by an explicit call to the :meth:`~Queue.close` method. If
   closed() returns ``True``, it is guaranteed that for the rest of the queue's
   lifetime both closed() and :meth:`full` will continue to return ``True``
   whereas meth:`put` and meth:`put_nowait` will always fail.


.. method:: Queue.exhausted()

   Return ``True`` if the queue is both closed and empty, ``False`` otherwise.
   If exhausted() returns ``True``, it is guaranteed that for the rest of the
   queue's lifetime all of exhausted(), meth:`closed`, meth:`empty` and
   meth:`full` will continue to return ``True`` whereas meth:`get`,
   meth:`get_nowait`, meth:`put` and meth:`put_nowait` will always fail.


.. method:: Queue.put(item, block=True, timeout=None)

   Put *item* into the queue.

   If optional args *block* is true and *timeout* is ``None`` (the default),
   block if necessary until a free slot is available or the queue is closed;
   in the latter case, raise a :exc:`Closed` exception.

   If *block* is true and *timeout* is a positive number, block at most
   *timeout* seconds and raise the :exc:`Full` exception if no free slot
   becomes available within that time, or the :exc:`Closed` exception if the
   queue gets closed before any slots are available.

   If *block* is false, ignore *timeout* and put an item on the
   queue if a free slot is immediately available, else raise the :exc:`Full` or
   :exc:`Closed` exception, depending on whether the queue is closed.
   (Note that :exc:`Closed` is a subclass of :exc:`Full`.)


.. method:: Queue.put_nowait(item)

   Equivalent to ``put(item, False)``.


.. method:: Queue.get(block=True, timeout=None)

   Remove and return an item from the queue.

   If optional args *block* is true and *timeout* is ``None`` (the default),
   block if necessary until an item is available or the queue is closed. If
   the queue is closed and no items are available, raise the :exc:`Exhausted`
   exception.

   If *block* is true and *timeout* is a positive number, block at most
   *timeout* seconds and raise the :exc:`Empty` exception if no item becomes
   available within that time, or the :exc:`Exhausted` exception if the queue
   gets closed before any items are available.

   If *block* is false, ignore *timeout* and return an item if one is
   immediately available, else raise the :exc:`Empty` or :exc:`Exhausted`
   exception, depending on whether the queue is closed.
   (Note that :exc:`Exhausted` is a subclass of :exc:`Empty`.)


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
   indicate that the item was retrieved and all work on it is complete. When the
   count of unfinished tasks drops to zero, :meth:`join` unblocks.


Example of how to wait for enqueued tasks to be completed::

    def worker():
        while True:
            item = q.get()
            if item is None:
                break
            do_work(item)
            q.task_done()

    q = queue.Queue()
    threads = []
    for i in range(num_worker_threads):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)

    for item in source():
        q.put(item)

    # block until all tasks are done
    q.join()

    # stop workers
    for i in range(num_worker_threads):
        q.put(None)
    for t in threads:
        t.join()


The :class:`Queue` also supports a *closing* protocol, which can be used to
cleanly coordinate completion or termination of work.


.. method:: Queue.close()

   Close the queue. Once closed, the queue does not accept new items and can
   not be reopened. Closing immediately aborts any currently-blocking
   :meth:`put` calls; if the queue is empty at the moment of closing, it
   also aborts any currently-blocking :meth:`get` calls.
   Repeated calls to close() after the first one do nothing.


The queue is typically closed by whichever part of the producer-consumer
structure is driving the workflow. A consumer could close the
queue to signal to producers that there is no interest in further items, a
producer could close it to signal to consumers that after the current
contents of the queue are retrieved there is no need to keep waiting for new
ones, or a third part of the system could close it to signal to both sides that
they should terminate communication without dropping any items that are already
in transport.

If multiple actors need to coordinate the conditions for closing
among themselves (e.g. if the queue is to be closed once multiple producers are
all finished), this can be resolved outside the queue itself, typically using
synchronization primitives such as :class:`~threading.Semaphore` or
:class:`~threading.Barrier`.


.. method:: Queue.__iter__()

   Iterate through (blocking) retrieval of items from the queue until it is
   closed. Each :func:`next` call on the iterator will retrieve one item if
   available, block until an item becomes available, or stop the iteration
   once the queue is exhausted (closed and empty). This enables simple
   consumption in the form of a plain ``for`` loop::

    for item in queue:
        process(item)

   The iteration support is intended for use with *close()*, because
   the iteration will never terminate of its own on a queue that doesn't get
   closed.
   It can still be used without explicit closing though, as long as the
   consumption code is able to decide on its own when to stop retrieving::

    for item in queue:
        process(item)
        if item.is_last():
            break


Note that the closing and task-tracking protocols are independent and have no
interactions. One is only about the transport of items, and the other is about
the completion of their processing.


SimpleQueue Objects
-------------------

:class:`SimpleQueue` objects provide the public methods described below.

.. method:: SimpleQueue.qsize()

   Return the approximate size of the queue.  Note, qsize() > 0 doesn't
   guarantee that a subsequent get() will not block.


.. method:: SimpleQueue.empty()

   Return ``True`` if the queue is empty, ``False`` otherwise. If empty()
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

   Equivalent to ``put(item)``, provided for compatibility with
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
   :meth:`~collections.deque.popleft` operations that do not require locking.
