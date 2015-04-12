.. currentmodule:: asyncio

Queues
======

Queues:

* :class:`Queue`
* :class:`PriorityQueue`
* :class:`LifoQueue`

asyncio queue API was designed to be close to classes of the :mod:`queue`
module (:class:`~queue.Queue`, :class:`~queue.PriorityQueue`,
:class:`~queue.LifoQueue`), but it has no *timeout* parameter. The
:func:`asyncio.wait_for` function can be used to cancel a task after a timeout.

Queue
-----

.. class:: Queue(maxsize=0, \*, loop=None)

   A queue, useful for coordinating producer and consumer coroutines.

   If *maxsize* is less than or equal to zero, the queue size is infinite. If
   it is an integer greater than ``0``, then ``yield from put()`` will block
   when the queue reaches *maxsize*, until an item is removed by :meth:`get`.

   Unlike the standard library :mod:`queue`, you can reliably know this Queue's
   size with :meth:`qsize`, since your single-threaded asyncio application won't
   be interrupted between calling :meth:`qsize` and doing an operation on the
   Queue.

   This class is :ref:`not thread safe <asyncio-multithreading>`.

   .. versionchanged:: 3.4.4
      New :meth:`join` and :meth:`task_done` methods.

   .. method:: empty()

      Return ``True`` if the queue is empty, ``False`` otherwise.

   .. method:: full()

      Return ``True`` if there are :attr:`maxsize` items in the queue.

      .. note::

         If the Queue was initialized with ``maxsize=0`` (the default), then
         :meth:`full()` is never ``True``.

   .. coroutinemethod:: get()

      Remove and return an item from the queue. If queue is empty, wait until
      an item is available.

      This method is a :ref:`coroutine <coroutine>`.

      .. seealso::

         The :meth:`empty` method.

   .. method:: get_nowait()

      Remove and return an item from the queue.

      Return an item if one is immediately available, else raise
      :exc:`QueueEmpty`.

   .. coroutinemethod:: join()

      Block until all items in the queue have been gotten and processed.

      The count of unfinished tasks goes up whenever an item is added to the
      queue. The count goes down whenever a consumer thread calls
      :meth:`task_done` to indicate that the item was retrieved and all work on
      it is complete.  When the count of unfinished tasks drops to zero,
      :meth:`join` unblocks.

      This method is a :ref:`coroutine <coroutine>`.

      .. versionadded:: 3.4.4

   .. coroutinemethod:: put(item)

      Put an item into the queue. If the queue is full, wait until a free slot
      is available before adding item.

      This method is a :ref:`coroutine <coroutine>`.

      .. seealso::

         The :meth:`full` method.

   .. method:: put_nowait(item)

      Put an item into the queue without blocking.

      If no free slot is immediately available, raise :exc:`QueueFull`.

   .. method:: qsize()

      Number of items in the queue.

   .. method:: task_done()

      Indicate that a formerly enqueued task is complete.

      Used by queue consumers. For each :meth:`~Queue.get` used to fetch a task, a
      subsequent call to :meth:`task_done` tells the queue that the processing
      on the task is complete.

      If a :meth:`join` is currently blocking, it will resume when all items
      have been processed (meaning that a :meth:`task_done` call was received
      for every item that had been :meth:`~Queue.put` into the queue).

      Raises :exc:`ValueError` if called more times than there were items
      placed in the queue.

      .. versionadded:: 3.4.4

   .. attribute:: maxsize

      Number of items allowed in the queue.


PriorityQueue
-------------

.. class:: PriorityQueue

   A subclass of :class:`Queue`; retrieves entries in priority order (lowest
   first).

   Entries are typically tuples of the form: (priority number, data).


LifoQueue
---------

.. class:: LifoQueue

    A subclass of :class:`Queue` that retrieves most recently added entries
    first.


Exceptions
^^^^^^^^^^

.. exception:: QueueEmpty

   Exception raised when the :meth:`~Queue.get_nowait` method is called on a
   :class:`Queue` object which is empty.


.. exception:: QueueFull

   Exception raised when the :meth:`~Queue.put_nowait` method is called on a
   :class:`Queue` object which is full.
