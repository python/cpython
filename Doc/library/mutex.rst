
:mod:`mutex` --- Mutual exclusion support
=========================================

.. module:: mutex
   :synopsis: Lock and queue for mutual exclusion.
   :deprecated:

.. deprecated::
   The :mod:`mutex` module has been removed in Python 3.0.

.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`mutex` module defines a class that allows mutual-exclusion via
acquiring and releasing locks. It does not require (or imply)
:mod:`threading` or multi-tasking, though it could be useful for those
purposes.

The :mod:`mutex` module defines the following class:


.. class:: mutex()

   Create a new (unlocked) mutex.

   A mutex has two pieces of state --- a "locked" bit and a queue. When the mutex
   is not locked, the queue is empty. Otherwise, the queue contains zero or more
   ``(function, argument)`` pairs representing functions (or methods) waiting to
   acquire the lock. When the mutex is unlocked while the queue is not empty, the
   first queue entry is removed and its  ``function(argument)`` pair called,
   implying it now has the lock.

   Of course, no multi-threading is implied -- hence the funny interface for
   :meth:`lock`, where a function is called once the lock is acquired.


.. _mutex-objects:

Mutex Objects
-------------

:class:`mutex` objects have following methods:


.. method:: mutex.test()

   Check whether the mutex is locked.


.. method:: mutex.testandset()

   "Atomic" test-and-set, grab the lock if it is not set, and return ``True``,
   otherwise, return ``False``.


.. method:: mutex.lock(function, argument)

   Execute ``function(argument)``, unless the mutex is locked. In the case it is
   locked, place the function and argument on the queue. See :meth:`unlock` for
   explanation of when ``function(argument)`` is executed in that case.


.. method:: mutex.unlock()

   Unlock the mutex if queue is empty, otherwise execute the first element in the
   queue.

