.. _threadsafety:

************************
Thread Safety Guarantees
************************

This page documents thread-safety guarantees for built-in types in Python's
free-threaded build. The guarantees described here apply when using Python with
the :term:`GIL` disabled (free-threaded mode). When the GIL is enabled, most
operations are implicitly serialized.

For general guidance on writing thread-safe code in free-threaded Python, see
:ref:`freethreading-python-howto`.


.. _thread-safety-list:

Thread safety for list objects
==============================

Reading a single element from a :class:`list` is
:term:`atomic <atomic operation>`:

.. code-block::
   :class: good

   lst[i]   # list.__getitem__

The following methods traverse the list and use :term:`atomic <atomic operation>`
reads of each item to perform their function. That means that they may
return results affected by concurrent modifications:

.. code-block::
   :class: maybe

   item in lst
   lst.index(item)
   lst.count(item)

All of the above operations avoid acquiring :term:`per-object locks
<per-object lock>`. They do not block concurrent modifications. Other
operations that hold a lock will not block these from observing intermediate
states.

All other operations from here on block using the :term:`per-object lock`.

Writing a single item via ``lst[i] = x`` is safe to call from multiple
threads and will not corrupt the list.

The following operations return new objects and appear
:term:`atomic <atomic operation>` to other threads:

.. code-block::
   :class: good

   lst1 + lst2    # concatenates two lists into a new list
   x * lst        # repeats lst x times into a new list
   lst.copy()     # returns a shallow copy of the list

The following methods that only operate on a single element with no shifting
required are :term:`atomic <atomic operation>`:

.. code-block::
   :class: good

   lst.append(x)  # append to the end of the list, no shifting required
   lst.pop()      # pop element from the end of the list, no shifting required

The :meth:`~list.clear` method is also :term:`atomic <atomic operation>`.
Other threads cannot observe elements being removed.

The :meth:`~list.sort` method is not :term:`atomic <atomic operation>`.
Other threads cannot observe intermediate states during sorting, but the
list appears empty for the duration of the sort.

The following operations may allow :term:`lock-free` operations to observe
intermediate states since they modify multiple elements in place:

.. code-block::
   :class: maybe

   lst.insert(idx, item)  # shifts elements
   lst.pop(idx)           # idx not at the end of the list, shifts elements
   lst *= x               # copies elements in place

The :meth:`~list.remove` method may allow concurrent modifications since
element comparison may execute arbitrary Python code (via
:meth:`~object.__eq__`).

:meth:`~list.extend` is safe to call from multiple threads.  However, its
guarantees depend on the iterable passed to it. If it is a :class:`list`, a
:class:`tuple`, a :class:`set`, a :class:`frozenset`, a :class:`dict` or a
:ref:`dictionary view object <dict-views>` (but not their subclasses), the
``extend`` operation is safe from concurrent modifications to the iterable.
Otherwise, an iterator is created which can be concurrently modified by
another thread.  The same applies to inplace concatenation of a list with
other iterables when using ``lst += iterable``.

Similarly, assigning to a list slice with ``lst[i:j] = iterable`` is safe
to call from multiple threads, but ``iterable`` is only locked when it is
also a :class:`list` (but not its subclasses).

Operations that involve multiple accesses, as well as iteration, are never
atomic. For example:

.. code-block::
   :class: bad

   # NOT atomic: read-modify-write
   lst[i] = lst[i] + 1

   # NOT atomic: check-then-act
   if lst:
       item = lst.pop()

   # NOT thread-safe: iteration while modifying
   for item in lst:
       process(item)  # another thread may modify lst

Consider external synchronization when sharing :class:`list` instances
across threads.


.. _thread-safety-dict:

Thread safety for dict objects
==============================

Creating a dictionary with the :class:`dict` constructor is atomic when the
argument to it is a :class:`dict` or a :class:`tuple`. When using the
:meth:`dict.fromkeys` method, dictionary creation is atomic when the
argument is a :class:`dict`, :class:`tuple`, :class:`set` or
:class:`frozenset`.

The following operations and functions are :term:`lock-free` and
:term:`atomic <atomic operation>`.

.. code-block::
   :class: good

   d[key]       # dict.__getitem__
   d.get(key)   # dict.get
   key in d     # dict.__contains__
   len(d)       # dict.__len__

All other operations from here on hold the :term:`per-object lock`.

Writing or removing a single item is safe to call from multiple threads
and will not corrupt the dictionary:

.. code-block::
   :class: good

   d[key] = value        # write
   del d[key]            # delete
   d.pop(key)            # remove and return
   d.popitem()           # remove and return last item
   d.setdefault(key, v)  # insert if missing

These operations may compare keys using :meth:`~object.__eq__`, which can
execute arbitrary Python code. During such comparisons, the dictionary may
be modified by another thread. For built-in types like :class:`str`,
:class:`int`, and :class:`float`, that implement :meth:`~object.__eq__` in C,
the underlying lock is not released during comparisons and this is not a
concern.

The following operations return new objects and hold the :term:`per-object lock`
for the duration of the operation:

.. code-block::
   :class: good

   d.copy()      # returns a shallow copy of the dictionary
   d | other     # merges two dicts into a new dict
   d.keys()      # returns a new dict_keys view object
   d.values()    # returns a new dict_values view object
   d.items()     # returns a new dict_items view object

The :meth:`~dict.clear` method holds the lock for its duration. Other
threads cannot observe elements being removed.

The following operations lock both dictionaries. For :meth:`~dict.update`
and ``|=``, this applies only when the other operand is a :class:`dict`
that uses the standard dict iterator (but not subclasses that override
iteration). For equality comparison, this applies to :class:`dict` and
its subclasses:

.. code-block::
   :class: good

   d.update(other_dict)  # both locked when other_dict is a dict
   d |= other_dict       # both locked when other_dict is a dict
   d == other_dict       # both locked for dict and subclasses

All comparison operations also compare values using :meth:`~object.__eq__`,
so for non-built-in types the lock may be released during comparison.

:meth:`~dict.fromkeys` locks both the new dictionary and the iterable
when the iterable is exactly a :class:`dict`, :class:`set`, or
:class:`frozenset` (not subclasses):

.. code-block::
   :class: good

   dict.fromkeys(a_dict)      # locks both
   dict.fromkeys(a_set)       # locks both
   dict.fromkeys(a_frozenset) # locks both

When updating from a non-dict iterable, only the target dictionary is
locked. The iterable may be concurrently modified by another thread:

.. code-block::
   :class: maybe

   d.update(iterable)        # iterable is not a dict: only d locked
   d |= iterable             # iterable is not a dict: only d locked
   dict.fromkeys(iterable)   # iterable is not a dict/set/frozenset: only result locked

Operations that involve multiple accesses, as well as iteration, are never
atomic:

.. code-block::
   :class: bad

   # NOT atomic: read-modify-write
   d[key] = d[key] + 1

   # NOT atomic: check-then-act (TOCTOU)
   if key in d:
       del d[key]

   # NOT thread-safe: iteration while modifying
   for key, value in d.items():
       process(key)  # another thread may modify d

To avoid time-of-check to time-of-use (TOCTOU) issues, use atomic
operations or handle exceptions:

.. code-block::
   :class: good

   # Use pop() with default instead of check-then-delete
   d.pop(key, None)

   # Or handle the exception
   try:
       del d[key]
   except KeyError:
       pass

To safely iterate over a dictionary that may be modified by another
thread, iterate over a copy:

.. code-block::
   :class: good

   # Make a copy to iterate safely
   for key, value in d.copy().items():
       process(key)

Consider external synchronization when sharing :class:`dict` instances
across threads.
