.. _library-threadsafety:

**********************************************
Thread Safety Guarantees for Standard Library
**********************************************

This page documents thread-safety guarantees for standard library types in
Python's :term:`free-threaded build`.  See :ref:`threadsafety` for the
guarantees provided by built-in types and for the terms used here.

For general guidance on writing thread-safe code in free-threaded Python, see
:ref:`freethreading-python-howto`.


.. currentmodule:: collections

.. _thread-safety-deque:

Thread safety for deque objects
===============================

The :func:`len` function is lock-free and :term:`atomic <atomic operation>`.

All other operations hold the :term:`per-object lock`.

Adding or removing an element at either end is
:term:`atomic <atomic operation>`.  For a bounded deque (one created with
*maxlen*), discarding an item from the opposite end is part of the same
atomic operation:

.. code-block::
   :class: good

   d.append(x)      # add to the right side
   d.appendleft(x)  # add to the left side
   d.pop()          # remove and return from the right side
   d.popleft()      # remove and return from the left side

Reading or writing a single element by index is also
:term:`atomic <atomic operation>`:

.. code-block::
   :class: good

   d[i]             # deque.__getitem__
   d[i] = x         # deque.__setitem__

The following operations modify the deque in place and are
:term:`atomic <atomic operation>`.  Other threads cannot observe
intermediate states:

.. code-block::
   :class: good

   del d[i]           # remove element at index
   d.insert(i, x)     # insert element at index
   d.rotate(n)        # rotate n steps
   d.reverse()        # reverse in place
   d.clear()          # remove all elements
   d *= n             # repeat in place

The following operations return new objects and appear
:term:`atomic <atomic operation>` to other threads.  For subclasses of
:class:`deque`, :meth:`~deque.copy` calls the subclass constructor,
which iterates over the deque:

.. code-block::
   :class: good

   d.copy()          # returns a shallow copy of the deque
   copy.copy(d)      # same as d.copy()
   d * n             # repeats d n times into a new deque

The following operations compare elements using :meth:`~object.__eq__`,
which can execute arbitrary Python code.  During such comparisons, the
deque may be modified by another thread.  If that happens, the operation
raises :exc:`RuntimeError` (or :exc:`IndexError` for
:meth:`~deque.remove`) rather than returning a result based on
inconsistent state.  For built-in types like :class:`str`, :class:`int`,
and :class:`float`, that implement :meth:`~object.__eq__` in C, the
underlying lock is not released during comparisons and this is not a
concern:

.. code-block::
   :class: maybe

   x in d           # deque.__contains__
   d.count(x)       # count occurrences
   d.index(x)       # find first occurrence
   d.remove(x)      # remove first occurrence

:meth:`~deque.extend` and :meth:`~deque.extendleft`
are safe to call from multiple threads.  The deque is locked for the
duration of the call, but the *iterable* is not.  It is iterated using its
iterator, which can be concurrently modified by another thread.  If
iterating the *iterable* executes Python code (for example, a generator),
the lock may be released and other threads may observe a partially
extended deque.  The same applies to ``d += iterable`` and to the second
operand of ``d1 + d2``.

Iterating over a deque, and operations that are implemented by iterating
over it, are not atomic.  Each step of the iteration holds the lock, so a
deque iterator can be shared between threads.  If the deque is modified
while it is being iterated, the iterator raises :exc:`RuntimeError`.  This
applies to the following operations:

.. code-block::
   :class: maybe

   for item in d: ...   # iteration
   reversed(d)          # reverse iteration
   list(d)              # conversion to other containers
   d == other           # comparisons with another deque
   repr(d)              # string representation
   pickle.dumps(d)      # pickling

Operations that involve multiple accesses are never atomic:

.. code-block::
   :class: bad

   # NOT atomic: check-then-act
   if d:
       item = d.popleft()  # another thread may have emptied d

   # NOT atomic: read-modify-write
   d[0] = d[0] + 1

To avoid time-of-check to time-of-use (TOCTOU) issues, call the
method directly and handle the exception:

.. code-block::
   :class: good

   try:
       item = d.popleft()
   except IndexError:
       pass  # deque was empty

Because :meth:`~deque.append` and :meth:`~deque.popleft` are atomic, a
deque can be used as a simple FIFO queue shared between threads.  If
consumers need to wait for items to become available, use
:class:`queue.Queue` instead.
