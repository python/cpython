.. _multi-threading-howto:

**************************
Multi-threaded programming
**************************

A :term:`thread` is an independent flow of execution inside a program.  Every
Python program has at least one thread, the *main thread*, and a program
that never starts another is *single-threaded*.  A *multi-threaded* program
starts more threads, either directly with the :mod:`threading` module or
indirectly through functionality provided by a library, such as
:class:`concurrent.futures.ThreadPoolExecutor`.  Each thread runs its own
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

Threads can also divide work between CPU cores.  In the default build of
CPython, :term:`parallelism` between threads is only possible while an
operation releases the :term:`global interpreter lock` (GIL), as blocking I/O
and many functions implemented in :term:`native code` do.  In the
:term:`free-threaded build` the GIL is disabled, and threads can also run
Python code in parallel, assuming some other lock or coordination does not
hinder parallelism.

All the threads in a program share the same modules and globals. Objects can
also be shared between threads, allowing multiple threads to simultaneously
access the the same data directly. This can avoid the costs that
:mod:`multiprocessing` pays to start processes and to copy data between
processes.

Sharing objects between threads also introduces the possibility of bugs that
cannot occur in single-threaded programs.  An :mod:`asyncio` task only
gives up control at an ``await``, but a thread can be interrupted at any point
so that another can run, and two threads can even be using the same objects at
the same instant.  Code that is correct when one thread runs it can therefore
give wrong answers when several threads run it, and it may only do so
occasionally.  Such a bug is called a :term:`race condition`.

Code that behaves correctly when several threads use it concurrently is called
thread-safe. The rest of the guide explains how to recognize code that is not
thread-safe and how to fix it. After reading the guide, you should be able to
answer these questions:

- Should my program use threads or a different form of concurrency?
- Why does my threaded program give different results from run to run and how
  do I make my program deterministic?
- Which objects can my program safely share between threads?
- What are my options for protecting shared data, and what does each option cost?
- How do I coordinate work between the threads in my program?

This HOWTO describes several strategies for making code thread-safe. Working
with threads requires a trade-off between safety, performance and code
simplicity. Each section describes the costs of a strategy as well as its
benefits. Hopefully you can use the examples below to write and update
real-world Python code to use threads or fix issues seen when executing in
multiple threads.

.. TODO: adjust the list of questions to match the sections once they are
   written.

.. TODO: link glossary entries on first use.  Beyond the ones linked above,
   the glossary has: atomic operation, concurrent modification, data race,
   deadlock, global interpreter lock, global state, immutable, lock-free,
   mutable, per-object lock, reentrant, synchronization primitive.

The advice applies both to the default build of CPython and to the
:term:`free-threaded build`.  Places where the two builds
behave differently are called out where they matter.

.. seealso::

   :ref:`freethreading-python-howto`
      Describes the free-threaded build itself.

   :ref:`threadsafety`
      The reference for which operations on built-in types are safe to use
      from several threads.

   :mod:`threading`, :mod:`queue` and :mod:`concurrent.futures`
      The reference documentation for the modules used in this guide.

.. TODO: decide on one callout style for build differences (for example a
   note that starts "On the free-threaded build") and use it throughout.


Is threading the right tool?
============================

.. TODO: keep this to a paragraph or two.
   - Who this guide is for.
   - Short comparison with asyncio, multiprocessing and
     concurrent.interpreters, with links rather than detail.
   - asyncio and threads both keep a program going while something waits.
     asyncio needs code written with async and await; threads work with
     ordinary blocking functions, and asyncio itself uses threads for
     those (asyncio.to_thread(), loop.run_in_executor()).
   - The price of not needing async and await: an asyncio task switches
     only at an await, while a thread can be preempted anywhere and
     threads can run simultaneously on the same objects.
   - Compared with multiprocessing, threads avoid the cost of starting
     processes and of copying data between them.  Do not describe sharing
     as free: see the note on contention under "Fine-grained locks".
   - Build difference: under the GIL, threads run in parallel only while an
     operation releases the GIL, which blocking I/O and much native code
     do.  On the free-threaded build, Python code runs in parallel too.


What goes wrong
===============

.. TODO:
   - Motivating example: a counter.  Show the serial version giving a
     consistent result, then an unsynchronized threaded version giving
     results that vary from run to run.
   - The example has to fail on both builds.  A bare ``counter += 1`` loop
     almost never loses updates under the GIL since 3.10, because thread
     switches only happen at backward jumps and calls.  Use a check-then-act
     with a call in the middle, or use the difference to show that the GIL
     hides races without removing them.
   - Define "data race" and "race condition" and keep them distinct.
     Link the glossary entries for both, and for "atomic operation".
   - Compound operations: check-then-act and read-modify-write.
   - What built-in types guarantee: single operations are safe, sequences of
     them are not.  Link to the thread safety reference.
   - Wording: reserve "guarantee" for documented behaviour on both builds.
     Anything else that happens to hold under the GIL is not to be relied on.


Running threads
===============

.. TODO: keep this short; the threading and concurrent.futures references
   cover the API.
   - concurrent.futures.ThreadPoolExecutor first, threading.Thread second.
   - Waiting for threads: join() and futures.
   - Exceptions: threading.excepthook compared with exceptions carried by
     futures.
   - Stopping a thread: there is no way to kill one, so cancellation is
     cooperative (threading.Event).
   - Daemon threads and interpreter shutdown.


Strategies for sharing data
===========================

The strategies below are ordered from the one to reach for first to the one
to reach for last.  Each fixes the same running example, a global dictionary
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
   - What matters is shared mutation.  Work that is partitioned so that no
     two threads touch the same data is safe by construction.
   - Example: filling an image where each thread fills a disjoint band.
     Standard library only (bytearray or list), using executor.map().
   - Per-thread results merged at the end.  This also fixes the counter.
   - threading.local and contextvars for per-thread state; when to prefer
     which.
   - Cost: the work has to be restructured, and per-thread copies of a cache
     multiply memory use and repeat work.
   - Build difference: sys.flags.thread_inherit_context defaults differ, so
     whether a new thread sees the caller's context variables depends on the
     build.
   - Build difference: the partitioned example is safe on both builds.
     Under the GIL it is only faster if the work on each piece releases the
     GIL.  On the free-threaded build, pure-Python work speeds up too.


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
     copy, then rebind the shared reference.  Distinguish from the lazy-copy
     optimization of the same name (fork, pandas).
   - Readers take one snapshot of the shared reference and use only that.
   - Without a writer lock, the last writer wins and concurrent updates are
     lost.  That is fine for memoization and wrong for most other state.
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
   - Sharing is not free even without locks.  Build difference: on the
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
