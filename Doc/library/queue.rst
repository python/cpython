
:mod:`Queue` --- A synchronized queue class
===========================================

.. module:: Queue
   :synopsis: A synchronized queue class.


The :mod:`Queue` module implements a multi-producer, multi-consumer FIFO queue.
It is especially useful in threaded programming when information must be
exchanged safely between multiple threads.  The :class:`Queue` class in this
module implements all the required locking semantics.  It depends on the
availability of thread support in Python; see the :mod:`threading`
module.

The :mod:`Queue` module defines the following class and exception:


.. class:: Queue(maxsize)

   Constructor for the class.  *maxsize* is an integer that sets the upperbound
   limit on the number of items that can be placed in the queue.  Insertion will
   block once this size has been reached, until queue items are consumed.  If
   *maxsize* is less than or equal to zero, the queue size is infinite.


.. exception:: Empty

   Exception raised when non-blocking :meth:`get` (or :meth:`get_nowait`) is called
   on a :class:`Queue` object which is empty.


.. exception:: Full

   Exception raised when non-blocking :meth:`put` (or :meth:`put_nowait`) is called
   on a :class:`Queue` object which is full.


.. _queueobjects:

Queue Objects
-------------

Class :class:`Queue` implements queue objects and has the methods described
below.  This class can be derived from in order to implement other queue
organizations (e.g. stack) but the inheritable interface is not described here.
See the source code for details.  The public methods are:


.. method:: Queue.qsize()

   Return the approximate size of the queue.  Because of multithreading semantics,
   this number is not reliable.


.. method:: Queue.empty()

   Return ``True`` if the queue is empty, ``False`` otherwise. Because of
   multithreading semantics, this is not reliable.


.. method:: Queue.full()

   Return ``True`` if the queue is full, ``False`` otherwise. Because of
   multithreading semantics, this is not reliable.


.. method:: Queue.put(item[, block[, timeout]])

   Put *item* into the queue. If optional args *block* is true and *timeout* is
   None (the default), block if necessary until a free slot is available. If
   *timeout* is a positive number, it blocks at most *timeout* seconds and raises
   the :exc:`Full` exception if no free slot was available within that time.
   Otherwise (*block* is false), put an item on the queue if a free slot is
   immediately available, else raise the :exc:`Full` exception (*timeout* is
   ignored in that case).


.. method:: Queue.put_nowait(item)

   Equivalent to ``put(item, False)``.


.. method:: Queue.get([block[, timeout]])

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
   count of unfinished tasks drops to zero, join() unblocks.


Example of how to wait for enqueued tasks to be completed::

   def worker(): 
       while True: 
           item = q.get() 
           do_work(item) 
           q.task_done() 

   q = Queue() 
   for i in range(num_worker_threads): 
        t = Thread(target=worker)
        t.setDaemon(True)
        t.start() 

   for item in source():
       q.put(item) 

   q.join()       # block until all tasks are done

