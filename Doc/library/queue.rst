:mod:`queue` --- A synchronized queue class
===========================================

.. module:: queue
   :synopsis: A synchronized queue class.


The :mod:`queue` module implements multi-producer, multi-consumer queues.
It is especially useful in threaded programming when information must be
exchanged safely between multiple threads.  The :class:`Queue` class in this
module implements all the required locking semantics.  It depends on the
availability of thread support in Python; see the :mod:`threading`
module.

Implements three types of queue whose only difference is the order that
the entries are retrieved.  In a FIFO queue, the first tasks added are
the first retrieved. In a LIFO queue, the most recently added entry is
the first retrieved (operating like a stack).  With a priority queue,
the entries are kept sorted (using the :mod:`heapq` module) and the
lowest valued entry is retrieved first.


The :mod:`queue` module defines the following classes and exceptions:

.. class:: Queue(maxsize=0)

   Constructor for a FIFO queue.  *maxsize* is an integer that sets the upperbound
   limit on the number of items that can be placed in the queue.  Insertion will
   block once this size has been reached, until queue items are consumed.  If
   *maxsize* is less than or equal to zero, the queue size is infinite.

.. class:: LifoQueue(maxsize=0)

   Constructor for a LIFO queue.  *maxsize* is an integer that sets the upperbound
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


.. exception:: Empty

   Exception raised when non-blocking :meth:`get` (or :meth:`get_nowait`) is called
   on a :class:`Queue` object which is empty.


.. exception:: Full

   Exception raised when non-blocking :meth:`put` (or :meth:`put_nowait`) is called
   on a :class:`Queue` object which is full.

.. seealso::

   :class:`collections.deque` is an alternative implementation of unbounded
   queues with fast atomic :func:`append` and :func:`popleft` operations that
   do not require locking.


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

   Put *item* into the queue. If optional args *block* is true and *timeout* is
   None (the default), block if necessary until a free slot is available. If
   *timeout* is a positive number, it blocks at most *timeout* seconds and raises
   the :exc:`Full` exception if no free slot was available within that time.
   Otherwise (*block* is false), put an item on the queue if a free slot is
   immediately available, else raise the :exc:`Full` exception (*timeout* is
   ignored in that case).


.. method:: Queue.put_nowait(item)

   Equivalent to ``put(item, False)``.


.. method:: Queue.get(block=True, timeout=None)

   Remove and return an item from the queue. If optional args *block* is true and
   *timeout* is None (the default), block if necessary until an item is available.
   If *timeout* is a positive number, it blocks at most *timeout* seconds and
   raises the :exc:`Empty` exception if no item was available within that time.
   Otherwise (*block* is false), return an item if one is immediately available,
   else raise the :exc:`Empty` exception (*timeout* is ignored in that case).


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
           do_work(item)
           q.task_done()

   q = Queue()
   for i in range(num_worker_threads):
        t = Thread(target=worker)
        t.daemon = True
        t.start()

   for item in source():
       q.put(item)

   q.join()       # block until all tasks are done


.. seealso::

   Class :class:`multiprocessing.Queue`
      A queue class for use in a multi-processing (rather than multi-threading)
      context.

   Latest version of the :source:`queue module Python source code
   <Lib/queue.py>`
