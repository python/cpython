.. _multi-threading-howto:

**************************
Multi-threaded programming
**************************

A :term:`thread` is an independent flow of execution inside a program. Every
Python program has at least one thread, the *main thread*, and a program
that never starts another is *single-threaded*. A *multi-threaded* program
starts more threads, either directly with the :mod:`threading` module or
indirectly through functionality provided by a library, such as
:class:`concurrent.futures.ThreadPoolExecutor`. Each thread runs its own
function :term:`concurrently <concurrency>` with the others.

Using threads allows a program's execution to proceed when it is necessary to
do work that might take a long time to complete or might never complete. For
example, a GUI application must remain responsive to user input and must not
appear to freeze because it is doing other work and cannot check for user
input. When a function makes its caller wait for an arbitrarily long time, it
is called a *blocking call*. Reading a file and querying a database are
examples of blocking calls. :mod:`asyncio` does the same job for code written
with ``async`` and ``await``; threads do it for ordinary functions that may
block.

Threads can also divide work between CPU cores. In the default build of
CPython, :term:`parallelism` between threads is only possible while an
operation releases the :term:`global interpreter lock` (GIL), as blocking I/O
and many functions implemented in :term:`native code` do. In the
:term:`free-threaded build`, there is no GIL and threads can run
Python code in parallel.

All the threads in a program share the same modules and globals. Objects can
also be shared between threads, allowing multiple threads to simultaneously
access the same data directly. Sharing objects can avoid the costs that
:mod:`multiprocessing` pays to start processes and to copy data between
processes.

Sharing objects between threads also introduces the possibility of bugs that
cannot occur in single-threaded programs. Tasks running on one asyncio event
loop only switch at an await, but a thread can be interrupted at any point so
that another can run, and two threads can even be using the same objects at the
same instant. Code that is correct in single-threaded use can therefore give
wrong answers in multi-threaded use, and it may only do so occasionally. Such a
bug is called a :term:`race condition`.

APIs that behave correctly when several threads use functions, objects, and
types provided the API concurrently are called :term:`thread-safe`. The rest of
this HOWTO explains how to recognize code that is not thread-safe and how to fix
it. After reading this HOWTO, you should be able to answer these questions:

- Should my program use threads or a different form of concurrency?
- Why does my threaded program give different results from run to run and how
  do I make my program deterministic?
- Which objects can my program safely share between threads?
- What are my options for protecting shared data, and what does each option
  cost?
- How do I coordinate work between the threads in my program?

This HOWTO is for Python programmers who are new to writing multi-threaded
code. It assumes you know Python's built-in types and how to write functions and
classes, and it does not assume any experience with threads.

Below, we describe several strategies for making Python code
thread-safe. Working with threads requires a trade-off between safety,
performance and code simplicity. Each section describes the costs of a strategy
as well as its benefits. Hopefully you can use the examples below to write and
update real-world Python code to use threads or fix issues seen when executing
in multiple threads.

.. TODO: link glossary entries on first use. Beyond the ones linked above,
   the glossary has: atomic operation, concurrent modification, data race,
   deadlock, global interpreter lock, global state, immutable, lock-free,
   mutable, per-object lock, reentrant, synchronization primitive.

The advice applies both to the default build of CPython and to the
:term:`free-threaded build`. Places where the two builds
behave differently are marked with a box titled "Free-threaded build".

.. seealso::

   :ref:`freethreading-python-howto`
      Describes the free-threaded build itself.

   :ref:`threadsafety`
      The reference for which operations on built-in types are safe to use
      from several threads.

   :mod:`threading`, :mod:`queue` and :mod:`concurrent.futures`
      The reference documentation for the modules used in this HOWTO.

.. TODO: every "Build difference" item in the notes below becomes a titled
   admonition, placed where the difference matters. Use the same title
   every time and describe both builds inside the box when the difference
   is about the default build:

      .. admonition:: Free-threaded build

         Concurrent calls to ``next()`` on one iterator can skip or
         duplicate items.


Understanding Race Conditions
=============================

.. TODO:
   - Motivating example: a counter. Show the serial version giving a
     consistent result, then an unsynchronized threaded version giving
     results that vary from run to run.
   - The example has to fail on both builds. A bare ``counter += 1`` loop
     almost never loses updates under the GIL since 3.10, because thread
     switches only happen at backward jumps and calls. Use a check-then-act
     with a call in the middle, or use the difference to show that the GIL
     hides races without removing them.
   - Define "data race" and "race condition" and keep them distinct.
     Link the glossary entries for both, and for "atomic operation".
   - Compound operations: check-then-act and read-modify-write.
   - What built-in types guarantee: single operations are safe, sequences of
     them are not. Link to the thread safety reference.
   - Wording: reserve "guarantee" for documented behaviour on both builds.
     Anything else that happens to hold under the GIL is not to be relied on.


Running threads
===============

The standard library has several interfaces for running a code in another
thread. The table below states, for each of a set of common tasks, the best
tool to use for that task.

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Task you want to perform
     - The best tool for the task
   * - Run a function in another thread and use its return value
     - :meth:`ThreadPoolExecutor.submit() <concurrent.futures.Executor.submit>`,
       which returns a :class:`~concurrent.futures.Future`
   * - Run a function once for each item in a collection
     - :meth:`ThreadPoolExecutor.map() <concurrent.futures.Executor.map>`
   * - Run a function in a thread with low-level control over the thread
     - :class:`threading.Thread`
   * - Call a blocking function from :mod:`asyncio` code
     - :func:`asyncio.to_thread`
   * - Wait for threads to finish
     - A :keyword:`with` block around a
       :class:`~concurrent.futures.ThreadPoolExecutor`, or
       :meth:`Thread.join() <threading.Thread.join>`
   * - Handle an exception raised in another thread
     - Call :meth:`~concurrent.futures.Future.result` on the
       :class:`~concurrent.futures.Future` that
       :meth:`ThreadPoolExecutor.submit()
       <concurrent.futures.Executor.submit>` returned.

Stopping a long-running thread is tricky. Python has no way to stop a thread
from outside, and a thread stopped at an arbitrary point could leave a lock
held or shared data half updated. Cancelling a thread therefore requires
coordination, for example by having the thread periodically check a
:class:`threading.Event` and return when the event is set.


Strategies for sharing data
===========================

The strategies below are ordered from the one to reach for first to the one
to reach for last. Each fixes the same running example, a global dictionary
used as a cache, so that the costs can be compared.

.. TODO: introduce the running example: a module-level dict cache filled by
   an expensive function, called from several threads.

.. TODO: every rating in this table is a placeholder judgment and needs
   review.

.. list-table:: Trade-offs between the strategies
   :header-rows: 1
   :widths: 18 11 11 11 11 38

   * - Strategy
     - Hard to get wrong
     - Scales when reads dominate
     - Scales when writes dominate
     - Simple code
     - Main cost
   * - :ref:`Don't share <multi-threading-dont-share>`
     - High
     - High
     - High
     - Medium
     - The work has to partition cleanly
   * - :ref:`Share immutable objects <multi-threading-immutable>`
     - High
     - High
     - Low
     - Medium
     - Every update makes a copy
   * - :ref:`Hand off ownership <multi-threading-handoff>`
     - Medium
     - Medium
     - Medium
     - Medium
     - Queue overhead; the program becomes a pipeline
   * - :ref:`Copy-on-write <multi-threading-copy-on-write>`
     - Medium
     - High
     - Low
     - Low
     - Copies on every write; updates can be lost without a writer lock
   * - :ref:`One coarse lock <multi-threading-coarse-lock>`
     - Medium
     - Low
     - Low
     - High
     - Threads take turns
   * - :ref:`Fine-grained locks <multi-threading-fine-grained-locks>`
     - Low
     - High
     - Medium
     - Low
     - Deadlock risk; hard to verify


.. _multi-threading-dont-share:

Don't share
-----------

.. TODO:
   - What matters is shared mutation. Work that is partitioned so that no
     two threads touch the same data is safe by construction.
   - Example: filling an image where each thread fills a disjoint band.
     Standard library only (bytearray or list), using executor.map().
   - Per-thread results merged at the end. This also fixes the counter.
   - threading.local and contextvars for per-thread state; when to prefer
     which.
   - Cost: the work has to be restructured, and per-thread copies of a cache
     multiply memory use and repeat work.
   - Build difference: sys.flags.thread_inherit_context defaults differ, so
     whether a new thread sees the caller's context variables depends on the
     build.
   - Build difference: the partitioned example is safe on both builds.
     Under the GIL it is only faster if the work on each piece releases the
     GIL. On the free-threaded build, pure-Python work speeds up too.


.. _multi-threading-immutable:

Share immutable objects
-----------------------

.. TODO:
   - tuple, frozenset, frozendict, frozen dataclasses.
   - Immutability is shallow: a tuple of lists is still shared mutable state.
   - Cost: every update builds a new object.


.. _multi-threading-handoff:

Hand off ownership
------------------

.. TODO:
   - Only one thread owns an object at a time; queue.Queue transfers it.
   - Producer-consumer example, including Queue.shutdown().
   - Cost: queue overhead, and the program has to be shaped as a pipeline.


.. _multi-threading-copy-on-write:

Copy-on-write
-------------

.. TODO:
   - Define on first use: never mutate the shared object; copy it, modify the
     copy, then rebind the shared reference. Distinguish from the lazy-copy
     optimization of the same name (fork, pandas).
   - Readers take one snapshot of the shared reference and use only that.
   - Without a writer lock, the last writer wins and concurrent updates are
     lost. That is fine for memoization and wrong for most other state.
     Adding a lock around writers fixes it and keeps readers lock-free.
   - Cost: each write copies the whole structure.


.. _multi-threading-coarse-lock:

One coarse lock
---------------

.. TODO:
   - threading.Lock around every access, with the "with" statement.
   - A lock only protects data if every access takes it.
   - Checking again after acquiring the lock; one-time initialization.
   - Cost: threads take turns, so there is no parallel speedup inside the
     lock.


.. _multi-threading-fine-grained-locks:

Fine-grained locks
------------------

.. TODO:
   - One lock per object or per shard of the data.
   - Cost: more locks mean more ways to deadlock and code that is harder to
     verify.
   - Removing locks does not guarantee scaling.
   - Sharing is not free even without locks. Build difference: on the
     free-threaded build, threads that use the same objects contend on
     their reference counts, which limits scaling.


Coordinating threads
====================

.. TODO: a table from the need to the primitive, with little or no prose:
   - wait until something has happened: threading.Event
   - wait until a condition on shared state holds: threading.Condition
   - limit how many threads do something at once: threading.Semaphore
   - start several threads at the same moment: threading.Barrier
   - pass work between threads: queue.Queue
