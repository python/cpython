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


.. _threadsafety-levels:

Thread safety levels
====================

The C API documentation uses the following levels to describe the thread
safety guarantees of each function. The levels are listed from least to
most safe.

.. _threadsafety-level-incompatible:

Incompatible
------------

A function or operation that cannot be made safe for concurrent use even
with external synchronization. Incompatible code typically accesses
global state in an unsynchronized way and must only be called from a single
thread throughout the program's lifetime.

Example: a function that modifies process-wide state such as signal handlers
or environment variables, where concurrent calls from any threads, even with
external locking, can conflict with the runtime or other libraries.

.. _threadsafety-level-compatible:

Compatible
----------

A function or operation that is safe to call from multiple threads
*provided* the caller supplies appropriate external synchronization, for
example by holding a :term:`lock` for the duration of each call. Without
such synchronization, concurrent calls may produce :term:`race conditions
<race condition>` or :term:`data races <data race>`.

Example: a function that reads from or writes to an object whose internal
state is not protected by a lock. Callers must ensure that no two threads
access the same object at the same time.

.. _threadsafety-level-distinct:

Safe on distinct objects
------------------------

A function or operation that is safe to call from multiple threads without
external synchronization, as long as each thread operates on a **different**
object. Two threads may call the function at the same time, but they must
not pass the same object (or objects that share underlying state) as
arguments.

Example: a function that modifies fields of a struct using non-atomic
writes. Two threads can each call the function on their own struct
instance safely, but concurrent calls on the *same* instance require
external synchronization.

.. _threadsafety-level-shared:

Safe on shared objects
----------------------

A function or operation that is safe for concurrent use on the **same**
object. The implementation uses internal synchronization (such as
:term:`per-object locks <per-object lock>` or
:ref:`critical sections <python-critical-section-api>`) to protect shared
mutable state, so callers do not need to supply their own locking.

Example: :c:func:`PyList_GetItemRef` can be called from multiple threads on the
same :c:type:`PyListObject` - it uses internal synchronization to serialize
access.

.. _threadsafety-level-atomic:

Atomic
------

A function or operation that appears :term:`atomic <atomic operation>` with
respect to other threads - it executes instantaneously from the perspective
of other threads. This is the strongest form of thread safety.

Example: :c:func:`PyMutex_IsLocked` performs an atomic read of the mutex
state and can be called from any thread at any time.


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


.. _thread-safety-set:

Thread safety for set objects
==============================

The :func:`len` function is lock-free and :term:`atomic <atomic operation>`.

The following read operation is lock-free. It does not block concurrent
modifications and may observe intermediate states from operations that
hold the per-object lock:

.. code-block::
   :class: good

   elem in s    # set.__contains__

This operation may compare elements using :meth:`~object.__eq__`, which can
execute arbitrary Python code. During such comparisons, the set may be
modified by another thread. For built-in types like :class:`str`,
:class:`int`, and :class:`float`, :meth:`!__eq__` does not release the
underlying lock during comparisons and this is not a concern.

All other operations from here on hold the per-object lock.

Adding or removing a single element is safe to call from multiple threads
and will not corrupt the set:

.. code-block::
   :class: good

   s.add(elem)      # add element
   s.remove(elem)   # remove element, raise if missing
   s.discard(elem)  # remove element if present
   s.pop()          # remove and return arbitrary element

These operations also compare elements, so the same :meth:`~object.__eq__`
considerations as above apply.

The :meth:`~set.copy` method returns a new object and holds the per-object lock
for the duration so that it is always atomic.

The :meth:`~set.clear` method holds the lock for its duration. Other
threads cannot observe elements being removed.

The following operations only accept :class:`set` or :class:`frozenset`
as operands and always lock both objects:

.. code-block::
   :class: good

   s |= other                   # other must be set/frozenset
   s &= other                   # other must be set/frozenset
   s -= other                   # other must be set/frozenset
   s ^= other                   # other must be set/frozenset
   s & other                    # other must be set/frozenset
   s | other                    # other must be set/frozenset
   s - other                    # other must be set/frozenset
   s ^ other                    # other must be set/frozenset

:meth:`set.update`, :meth:`set.union`, :meth:`set.intersection` and
:meth:`set.difference` can take multiple iterables as arguments. They all
iterate through all the passed iterables and do the following:

   * :meth:`set.update` and :meth:`set.union` lock both objects only when
     the other operand is a :class:`set`, :class:`frozenset`, or :class:`dict`.
   * :meth:`set.intersection` and :meth:`set.difference` always try to lock
     all objects.

:meth:`set.symmetric_difference` tries to lock both objects.

The update variants of the above methods also have some differences between
them:

   * :meth:`set.difference_update` and :meth:`set.intersection_update` try
     to lock all objects one-by-one.
   * :meth:`set.symmetric_difference_update` only locks the arguments if it is
     of type :class:`set`, :class:`frozenset`, or :class:`dict`.

The following methods always try to lock both objects:

.. code-block::
   :class: good

   s.isdisjoint(other)          # both locked
   s.issubset(other)            # both locked
   s.issuperset(other)          # both locked

Operations that involve multiple accesses, as well as iteration, are never
atomic:

.. code-block::
   :class: bad

   # NOT atomic: check-then-act
   if elem in s:
         s.remove(elem)

   # NOT thread-safe: iteration while modifying
   for elem in s:
         process(elem)  # another thread may modify s

Consider external synchronization when sharing :class:`set` instances
across threads.  See :ref:`freethreading-python-howto` for more information.


.. _thread-safety-bytearray:

Thread safety for bytearray objects
===================================

   The :func:`len` function is lock-free and :term:`atomic <atomic operation>`.

   Concatenation and comparisons use the buffer protocol, which prevents
   resizing but does not hold the per-object lock. These operations may
   observe intermediate states from concurrent modifications:

   .. code-block::
      :class: maybe

      ba + other    # may observe concurrent writes
      ba == other   # may observe concurrent writes
      ba < other    # may observe concurrent writes

   All other operations from here on hold the per-object lock.

   Reading a single element or slice is safe to call from multiple threads:

   .. code-block::
      :class: good

      ba[i]        # bytearray.__getitem__
      ba[i:j]      # slice

   The following operations are safe to call from multiple threads and will
   not corrupt the bytearray:

   .. code-block::
      :class: good

      ba[i] = x         # write single byte
      ba[i:j] = values  # write slice
      ba.append(x)      # append single byte
      ba.extend(other)  # extend with iterable
      ba.insert(i, x)   # insert single byte
      ba.pop()          # remove and return last byte
      ba.pop(i)         # remove and return byte at index
      ba.remove(x)      # remove first occurrence
      ba.reverse()      # reverse in place
      ba.clear()        # remove all bytes

   Slice assignment locks both objects when *values* is a :class:`bytearray`:

   .. code-block::
      :class: good

      ba[i:j] = other_bytearray  # both locked

   The following operations return new objects and hold the per-object lock
   for the duration:

   .. code-block::
      :class: good

      ba.copy()     # returns a shallow copy
      ba * n        # repeat into new bytearray

   The membership test holds the lock for its duration:

   .. code-block::
      :class: good

      x in ba       # bytearray.__contains__

   All other bytearray methods (such as :meth:`~bytearray.find`,
   :meth:`~bytearray.replace`, :meth:`~bytearray.split`,
   :meth:`~bytearray.decode`, etc.) hold the per-object lock for their
   duration.

   Operations that involve multiple accesses, as well as iteration, are never
   atomic:

   .. code-block::
      :class: bad

      # NOT atomic: check-then-act
      if x in ba:
          ba.remove(x)

      # NOT thread-safe: iteration while modifying
      for byte in ba:
          process(byte)  # another thread may modify ba

   To safely iterate over a bytearray that may be modified by another
   thread, iterate over a copy:

   .. code-block::
      :class: good

      # Make a copy to iterate safely
      for byte in ba.copy():
          process(byte)

   Consider external synchronization when sharing :class:`bytearray` instances
   across threads.  See :ref:`freethreading-python-howto` for more information.


.. _thread-safety-memoryview:

Thread safety for memoryview objects
====================================

:class:`memoryview` objects provide access to the internal data of an
underlying object without copying. Thread safety depends on both the
memoryview itself and the underlying buffer exporter.

The memoryview implementation uses atomic operations to track its own
exports in the :term:`free-threaded build`. Creating and
releasing a memoryview are thread-safe. Attribute access (e.g.,
:attr:`~memoryview.shape`, :attr:`~memoryview.format`) reads fields that
are immutable for the lifetime of the memoryview, so concurrent reads
are safe as long as the memoryview has not been released.

However, the actual data accessed through the memoryview is owned by the
underlying object. Concurrent access to this data is only safe if the
underlying object supports it:

* For immutable objects like :class:`bytes`, concurrent reads through
  multiple memoryviews are safe.

* For mutable objects like :class:`bytearray`, reading and writing the
  same memory region from multiple threads without external
  synchronization is not safe and may result in data corruption.
  Note that even read-only memoryviews of mutable objects do not
  prevent data races if the underlying object is modified from
  another thread.

.. code-block::
   :class: bad

   # NOT safe: concurrent writes to the same buffer
   data = bytearray(1000)
   view = memoryview(data)
   # Thread 1: view[0:500] = b'x' * 500
   # Thread 2: view[0:500] = b'y' * 500

.. code-block::
   :class: good

   # Safe: use a lock for concurrent access
   import threading
   lock = threading.Lock()
   data = bytearray(1000)
   view = memoryview(data)

   with lock:
       view[0:500] = b'x' * 500

Resizing or reallocating the underlying object (such as calling
:meth:`bytearray.resize`) while a memoryview is exported raises
:exc:`BufferError`. This is enforced regardless of threading.

In the :term:`free-threaded build`, :meth:`~memoryview.release` is
thread-safe. If another thread has an in-flight operation on the memoryview
when it is released, the underlying buffer may remain exported until that
operation completes. Restrictions on the exporter, such as prohibiting
resizing, remain in effect until then.

For read-heavy workloads that share an underlying object across threads,
creating a separate memoryview of that object in each thread may improve
performance.


.. _threadsafety-memory-ordering:

Memory ordering
===============

.. impl-detail::

   The guarantees in this section apply to CPython's free-threaded build.
   The default build, which serializes execution with the :term:`GIL`,
   also satisfies them.  They are not a memory model for other Python
   implementations, and they apply to third-party extension types only if
   those types document them.

The sections above describe which operations are atomic.  Memory ordering
is a separate question: when does a change made by one thread become
visible to another?  Without a rule for that, a thread that receives an
object from another thread could see it partly initialized, or could see
stale values of other variables.

CPython provides *release-acquire* ordering for object references stored in
shared locations.  In short: if a thread loads a reference that another
thread stored, it sees everything that thread did before the store.  The
rest of this section makes that precise.


Definitions
-----------

Memory action
   A single load or store of a shared location, or a synchronization action
   such as acquiring a lock or starting a thread.  One Python statement may
   perform several memory actions.  ``x.a += 1`` loads ``x.a``, adds, and
   stores ``x.a``; another thread's actions can come between the load and
   the store.

Shared location
   Storage that holds a reference to an object and can be reached by more
   than one thread: list items, dict keys and values, set elements, instance
   attributes (including attributes in :attr:`~object.__slots__`), class
   attributes, closure cells, and module globals.  Scalar attributes
   implemented by C member descriptors are shared locations too.

   The length and other structural state of a container, the contents of
   buffers such as :class:`bytearray`, and local variables are not shared
   locations.

Reads-from
   Every load reads from exactly one store to the same location, or from
   its initial value.  Deleting an item or attribute is a store.

:term:`Sequenced-before <sequenced-before>`
   The order of memory actions within one thread.  This is the evaluation
   order defined in :ref:`evalorder`, together with the order in which
   statements execute.

:term:`Synchronizes-with <synchronizes-with>`
   An ordering edge between memory actions in different threads.  The
   edges CPython provides are listed in the rules below.

:term:`Happens-before <happens-before>`
   The transitive closure of sequenced-before and synchronizes-with.  If
   action A happens-before action B, the effects of A are visible to B.
   Running earlier in time does not by itself make A happen-before B.
   Happens-before has no cycles: no action happens-before itself.

Coherence
   The stores to one location occur in a single total order that is
   consistent with happens-before.  As a result:

   * if a store happens-before a load of the same location, the load reads
     from that store or a later one, never an older value;
   * if one load of a location happens-before another, the second reads
     from the same store as the first or a later one, so values do not go
     backwards;
   * a load never reads from a store that it happens-before;
   * a load never returns a value that was not stored.

   Coherence applies to every load, including the observations of
   container structure described in rule 2.


Rules
-----

1. **Publication.**  A store of an object reference to a shared location
   synchronizes-with every load that reads from that store.  A thread that
   obtains an object from a shared location therefore sees it fully
   initialized, and also sees everything else the storing thread did
   before the store.

   A load here is an operation that hands the stored object to the thread:
   reading a variable or attribute, ``lst[i]``, ``d[k]``, a
   :meth:`~dict.get` that finds the key, an iterator producing the item,
   or :meth:`~list.pop` returning it.

   The rule applies to storage locations, not syntax.  The default
   attribute machinery (:meth:`object.__setattr__`,
   :meth:`object.__getattribute__`, type and module attribute access,
   member descriptors) uses the shared locations above, so ordinary
   attribute access is covered.  A custom :meth:`~object.__setattr__`,
   :meth:`~object.__getattr__`, :meth:`~object.__getattribute__`,
   descriptor, property, or module ``__getattr__`` provides whatever its
   body does.  For example, if it stores into a dict, the guarantee
   follows from the dict store.

2. **Container structure is not a synchronization point.**  An operation
   that reports on the shape of a container, rather than returning one of
   its elements, creates no synchronizes-with edge.  This includes:

   * :func:`len`, truth testing, and :meth:`~object.__len__`;
   * ``in`` and ``not in``, and comparing containers;
   * a failed lookup: :exc:`KeyError`, :exc:`IndexError`,
     :exc:`AttributeError`, :exc:`NameError`, the default returned by
     :meth:`~dict.get` or :func:`getattr`, or :func:`hasattr` returning
     false;
   * an iterator finishing.

   These are still loads, so coherence applies.  A thread that is already
   ordered after a mutation (through a lock, :meth:`~threading.Thread.join`,
   a queue, or a reference load under rule 1) sees it: the length includes
   the appended item, the lookup finds the key, the deleted item is gone.
   Without such ordering the observation may be stale, and it says nothing
   about other locations.  The length of a container and its elements are
   separate state: a thread may see a new length before it can see the
   element counted by it, or the reverse.

3. **Synchronization primitives.**  The following are synchronizes-with
   edges:

   * releasing a lock, and the next successful acquisition of it
     (:class:`~threading.Lock`, :class:`~threading.RLock`, and the locks
     used by :class:`~threading.Condition` and
     :class:`~threading.Semaphore`);
   * :meth:`threading.Thread.start`, and the first action of the new
     thread;
   * the last action of a thread, and a :meth:`~threading.Thread.join`
     that returns because the thread finished, or an
     :meth:`~threading.Thread.is_alive` that returns false;
   * :meth:`threading.Event.set`, and a :meth:`~threading.Event.wait` or
     :meth:`~threading.Event.is_set` that returns true;
   * :meth:`threading.Condition.notify` or
     :meth:`~threading.Condition.notify_all`, and the return of a
     :meth:`~threading.Condition.wait` that it wakes;
   * :meth:`queue.Queue.put`, and the :meth:`~queue.Queue.get` that
     returns that item;
   * completing a :class:`concurrent.futures.Future`, and a
     :meth:`~concurrent.futures.Future.result`,
     :meth:`~concurrent.futures.Future.exception`, or true
     :meth:`~concurrent.futures.Future.done` on it;
   * dropping the last reference to an object, and its
     :meth:`~object.__del__` method and :mod:`weakref` callbacks, in
     whichever thread they run;
   * similar edges for the other primitives in :mod:`threading`,
     :mod:`queue` and :mod:`concurrent.futures`.

   A ``join()`` or ``wait()`` that times out creates no edge.

4. **Single mutating operations are atomic.**  The mutating operations
   documented as atomic in the sections above (such as
   :meth:`list.append`, :meth:`list.pop`, :meth:`dict.setdefault`,
   :meth:`dict.pop`, ``del d[k]``, :meth:`set.add` and
   :meth:`set.discard`) take effect in a single total order per object.
   That order is consistent with happens-before, and each operation sees
   the effects of every operation before it in the order and none after
   it.  So *N* concurrent appends add *N* items, and of *N* concurrent
   ``d.pop(k)`` calls, exactly one returns the value.

   This order covers only the mutating operations on one object.  It is
   not a synchronizes-with edge: two threads that append to the same list
   are not ordered with respect to anything else by doing so.  Readers
   that do not take the object's lock are not part of the order; they get
   coherence and rule 1.  Where an operation calls Python code, such as
   :meth:`~object.__hash__` or :meth:`~object.__eq__`, other operations on
   the container may run in between, as documented above.

5. **Visibility.**  A store becomes visible in finite time: a thread that
   keeps loading a location eventually reads from that store or a later
   one.  The interpreter does not hoist a load of a shared location out of
   a loop or serve it from a cache indefinitely.

6. **What is not guaranteed.**

   * Sequences of operations are not atomic.  Each load and store is a
     separate memory action, so ``x.a += 1`` or
     ``d[k] = d.get(k, 0) + 1`` can lose updates.
   * There is no single order of memory actions across independent
     objects (*sequential consistency*).  Two threads that each store to
     one object and then load from another may both see the old value.
     Two threads may disagree about the order of two stores made by two
     other threads.
   * Readers that do not take the object's lock may see intermediate
     states of mutations that change several elements, as documented for
     each type above.
   * An iterator shared between threads may produce duplicate items, skip
     items, and produce items after another thread got
     :exc:`StopIteration`.
   * Buffer contents, such as bytes written through a :class:`memoryview`,
     :class:`bytearray`, :mod:`array`, :mod:`mmap` or :mod:`ctypes`
     object, are not covered by rule 1.  They are visible to a thread that
     is ordered after the write some other way, but reading them creates
     no ordering.
   * Another thread's local variables, reached through
     :attr:`frame.f_locals` or :func:`sys._current_frames`.

   Use a lock or another synchronization primitive when any of these
   matter.


Examples
--------

Handing over an object through a shared container works, because the
consumer loads the reference the producer stored (rule 1):

.. code-block::
   :class: good

   # Producer thread
   data = {"answer": 42}
   shared["result"] = data

   # Consumer thread
   result = shared.get("result")
   if result is not None:
       assert result["answer"] == 42

A flag works for the same reason.  Suppose ``data`` and ``ready`` are
module globals, ``ready`` is initially false, and nothing else modifies
them:

.. code-block::
   :class: good

   # Thread 1
   data = 42
   ready = True

   # Thread 2
   while not ready:
       pass
   assert data == 42

In thread 1, the store to ``data`` is sequenced before the store to
``ready``.  Thread 2 eventually loads the new value of ``ready`` (rule 5),
and that load synchronizes-with the store (rule 1).  So the store to
``data`` happens-before thread 2's load of ``data``, which by coherence
returns 42.  (A busy loop wastes CPU time; :class:`threading.Event` is the
better tool for this.)

Polling the length of a container does not work, because ``len()`` creates
no ordering (rule 2):

.. code-block::
   :class: bad

   # Worker threads
   totals[i] = compute()
   done.append(i)

   # Main thread
   while len(done) < N:
       time.sleep(0.01)
   use(totals)  # may see stale items in totals

The main thread never loads a reference that a worker stored, so it is not
ordered after the workers' stores to ``totals``.  Use
:meth:`~threading.Thread.join`, a :class:`~threading.Event`, a
:class:`~queue.Queue`, or a lock instead.  The same applies to loops such as
``while not items: ...`` and ``while key not in d: ...``.

For the same reason, ``lst[len(lst) - 1]`` without other synchronization
can raise :exc:`IndexError` or return an older item even if the list is
only ever appended to.


Litmus tests
------------

Each test below is a small program with two or more threads, followed by a
question: can the program end with a particular result?  The answer says
whether the rules above allow that result, and why.

Unless stated otherwise, ``x``, ``y`` and ``flag`` are module globals (or
attributes of one shared object) that start as ``0``; ``lst`` is a shared
list that starts empty; ``d`` is a shared dict that starts empty; ``m`` is
a shared :class:`threading.Lock`.  Names starting with ``r`` are local
variables of the thread that assigns them.  Results are examined after all
threads have finished.

Every Python value is an object, so ``x = 1`` stores a reference to the
object ``1``, and rule 1 applies to it like any other reference.

"Yes" means the rules permit the result.  It does not mean the result is
common: some permitted results appear only on some hardware, such as
ARM64, and only rarely.  The default build, which runs one thread at a
time, can show only two of them: check-then-act and lost update.  Code
that relies on the others not happening is correct only on that build.


Message passing and publication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

**Message passing.**  Can this program end with ``r1 == 1`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = y
   y = 1              r2 = x

No.  If ``r1 == 1``, thread 2's load of ``y`` read from thread 1's store, so
the store synchronizes-with the load (rule 1).  The store to ``x`` is
sequenced before the store to ``y``, so it happens-before thread 2's load
of ``x``, which by coherence reads ``1``.

**Initialization.**  ``s.obj`` starts as ``None`` and ``C`` is a class
whose instances have no attribute ``v`` until one is set.  Can thread 2
raise :exc:`AttributeError`?

.. code-block:: text

   # Thread 1         # Thread 2
   o = C()            r1 = s.obj
   o.v = 1            if r1 is not None:
   s.obj = o              r2 = r1.v

No.  A thread that loads a reference sees everything the storing thread did
before the store, including the initialization of the object.

**Message passing through a dict.**  Can this program end with ``r1 == 1``
and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = d.get("k")
   d["k"] = 1         r2 = x

No.  A :meth:`~dict.get` that finds the key loads the stored reference
(rule 1), so this is the same as message passing.

**Message passing through iteration.**  Can thread 2 set ``r2`` to ``0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              for r1 in lst:
   lst.append(1)          r2 = x

No.  The loop body runs only if the iterator produces the appended item,
and producing it is a load of the reference thread 1 stored (rule 1).


Container structure
^^^^^^^^^^^^^^^^^^^

**Message passing through a length.**  Can this program end with
``r1 == 1`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = len(lst)
   lst.append(1)      r2 = x

Yes.  :func:`len` creates no synchronizes-with edge (rule 2), so thread 2
is not ordered after thread 1's store to ``x``.

**Message passing through membership.**  Can this program end with
``r1 == True`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = "k" in d
   d["k"] = 1         r2 = x

Yes.  ``in`` reports on the structure of the dict and does not load the
stored value (rule 2).

**Message passing through deletion.**  Here ``d`` starts as ``{"k": 1}``.
Can this program end with ``r1 is None`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = d.get("k")
   del d["k"]         r2 = x

Yes.  A failed lookup, including the default returned by
:meth:`~dict.get`, creates no ordering (rule 2).

**Length, then element.**  Can thread 2 raise :exc:`IndexError`?

.. code-block:: text

   # Thread 1         # Thread 2
   lst.append(1)      r1 = len(lst)
                      if r1 > 0:
                          r2 = lst[r1 - 1]

Yes.  The length and the element are separate state.  Thread 2 can see the
new length before it can see the element.

**Length under a lock.**  Can this program end with ``r1 == 1`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              with m:
   with m:                r1 = len(lst)
       lst.append(1)  r2 = x

No.  If ``r1 == 1``, thread 2 acquired ``m`` after thread 1 released it,
and that is a synchronizes-with edge (rule 3).  Using the same lock on both
sides is what turns the length into a usable signal.


Coherence and visibility
^^^^^^^^^^^^^^^^^^^^^^^^

**Values do not go backwards.**  Can this program end with ``r1 == 1`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = x
                      r2 = x

No.  The second load is sequenced after the first, so by coherence it reads
from the same store or a later one.

**Stores to one location have one order.**  Can this program end with
``r1 == 2`` and ``r2 == 1``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = x
   x = 2              r2 = x

No.  The stores to ``x`` are in one order, ``1`` then ``2``, and thread 2's
loads cannot go backwards in that order.

**Coherence of lengths.**  Can this program end with ``r1 == 1`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   lst.append(1)      r1 = len(lst)
                      r2 = len(lst)

No.  :func:`len` creates no ordering with other locations, but it is still
a load, so coherence applies to it.

**Visibility.**  Can thread 2 loop forever?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              while x == 0:
                          pass

No.  Thread 1's store becomes visible to thread 2 in finite time (rule 5).


Causality
^^^^^^^^^

**Write-to-read causality.**  Can this program end with ``r1 == 1``,
``r2 == 1`` and ``r3 == 0``?

.. code-block:: text

   # Thread 1     # Thread 2     # Thread 3
   x = 1          r1 = x         r2 = y
                  y = 1          r3 = x

No.  Happens-before is transitive.  Thread 1's store to ``x``
happens-before thread 2's store to ``y``, which happens-before thread 3's
load of ``x``.

**Load buffering.**  Can this program end with ``r1 == 1`` and ``r2 == 1``?

.. code-block:: text

   # Thread 1         # Thread 2
   r1 = x             r2 = y
   y = 1              x = 1

No.  If ``r1 == 1``, thread 2's store to ``x`` happens-before thread 1's
store to ``y``.  Thread 2's load of ``y`` happens-before its own store to
``x``, so ``r2 == 1`` would need a load to read from a store that happens
after it.


No single order
^^^^^^^^^^^^^^^

**Store buffering.**  Can this program end with ``r1 == 0`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              y = 1
   r1 = y             r2 = x

Yes.  Each thread stores to one location and then loads another.  The rules
give no order between the two threads here, so each load may read the
initial value (rule 6).  In any single interleaving of the four statements
one of the loads would see ``1``, so this result shows that there is no
such interleaving.

**Store buffering with a lock.**  Can this program end with ``r1 == 0`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   with m:            with m:
       x = 1              y = 1
       r1 = y             r2 = x

No.  The two critical sections run one after the other.  The second to
acquire ``m`` is ordered after the first, and sees its store.

**Independent reads of independent writes (IRIW).**  Can this program end
with ``r1 == 1``, ``r2 == 0``, ``r3 == 1`` and ``r4 == 0``?

.. code-block:: text

   # Thread 1   # Thread 2   # Thread 3   # Thread 4
   x = 1        y = 1        r1 = x       r3 = y
                             r2 = y       r4 = x

Yes.  Thread 3 sees the store to ``x`` first and thread 4 sees the store to
``y`` first.  Stores to independent locations are not seen in one global
order (rule 6).


Atomic operations
^^^^^^^^^^^^^^^^^

**Concurrent appends.**  Can ``lst`` end up with one item?

.. code-block:: text

   # Thread 1         # Thread 2
   lst.append(1)      lst.append(2)

No.  :meth:`list.append` is atomic (rule 4), so both appends take effect
and ``lst`` ends with two items, in either order.

**Concurrent pops.**  Here ``d`` starts as ``{"k": 1}``.  Can this program
end with ``r1 == 1`` and ``r2 == 1``?

.. code-block:: text

   # Thread 1               # Thread 2
   r1 = d.pop("k", None)    r2 = d.pop("k", None)

No.  The two pops take effect in some order.  The first removes the key and
returns ``1``; the second sees the key gone and returns ``None``.

**Concurrent setdefault.**  Can this program end with ``r1 != r2``?

.. code-block:: text

   # Thread 1                    # Thread 2
   r1 = d.setdefault("k", 1)     r2 = d.setdefault("k", 2)

No.  Whichever call takes effect second finds the value stored by the
first and returns it.

**Check, then act.**  Can this program end with ``r1 == 1`` and ``r2 == 2``?

.. code-block:: text

   # Thread 1               # Thread 2
   if "k" not in d:         if "k" not in d:
       d["k"] = 1               d["k"] = 2
   r1 = d["k"]              r2 = d["k"]

Yes.  Each statement is a separate operation.  Both threads can find the key
missing before either stores it.  Use :meth:`~dict.setdefault` or a lock.

**Lost update.**  Can ``x`` end up as ``1``?

.. code-block:: text

   # Thread 1         # Thread 2
   x += 1             x += 1

Yes.  ``x += 1`` is a load, an addition and a store.  Both threads can load
``0`` before either stores.  With ``with m: x += 1`` in both threads, ``x``
always ends as ``2``.


Synchronization primitives
^^^^^^^^^^^^^^^^^^^^^^^^^^

**Thread start.**  Can ``r1 == 0``?

.. code-block:: text

   # Main thread      # Thread t
   x = 1              r1 = x
   t.start()

No.  :meth:`~threading.Thread.start` synchronizes-with the first action of
the new thread (rule 3).

**Thread join.**  Can ``r1 == 0``?

.. code-block:: text

   # Main thread      # Thread t
   t.start()          x = 1
   t.join()
   r1 = x

No.  The last action of a thread synchronizes-with the return of
:meth:`~threading.Thread.join` (rule 3).  The same holds if the main thread
waits with ``while t.is_alive(): pass`` instead.

**Event.**  ``e`` is a :class:`threading.Event`.  Can this program end with
``r1 == True`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = e.is_set()
   e.set()            r2 = x

No.  :meth:`~threading.Event.set` synchronizes-with an
:meth:`~threading.Event.is_set` that returns true (rule 3).  The same holds
for a :meth:`~threading.Event.wait` that returns true.

**Queue.**  ``q`` is a :class:`queue.Queue`.  Can ``r1 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              q.get()
   q.put(None)        r1 = x

No.  :meth:`~queue.Queue.put` synchronizes-with the
:meth:`~queue.Queue.get` that returns the item (rule 3).


Buffers
^^^^^^^

In these tests ``buf`` is a shared ``bytearray(1)``.

**Buffer published by a reference.**  Can this program end with
``r1 == 1`` and ``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   buf[0] = 1         r1 = flag
   flag = 1           r2 = buf[0]

No.  The load of ``flag`` synchronizes-with the store (rule 1), so the
write to the buffer happens-before the read.

**Buffer used as a flag.**  Can this program end with ``r1 == 1`` and
``r2 == 0``?

.. code-block:: text

   # Thread 1         # Thread 2
   x = 1              r1 = buf[0]
   buf[0] = 1         r2 = x

Yes.  Reading buffer contents creates no ordering (rule 6).
