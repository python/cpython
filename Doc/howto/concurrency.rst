:tocdepth: 3

.. XXX reference vs. tutorial?

.. _concurrency-howto:

*****************
Concurrency HOWTO
*****************

There are many outstanding resources, both online and in print,
that would do an excellent job of introducing you to concurrency.
This howto document builds on those by walking you through how
to apply that knowledge using Python.

Python supports the following concurrency models directly:

* **free-threading** (stdlib, C-API)
* **isolated threads**, *AKA CSP/actor model* (stdlib\*, C-API)
* **coroutines**, *AKA async/await* (language, stdlib, C-API)
* **multi-processing** (stdlib)
* **distributed**, *e.g. SMP* (stdlib)

In this document, we'll look at how to take advantage of this
concurrency support.  The overall focus is on the following:

* `understanding the supported concurrency models <concurrency-models_>`_
* `factors to consider when designing a concurrent solution <concurrency-design_>`_
* `key concurrency primitives <concurrency-primitives_>`_
* `high-level, app-oriented practical examples <concurrency-workload-examples_>`_

.. note::

   You should always make sure concurrency is the right tool for the job
   before you reach for it when solving your problem.  There are many
   cases where concurrency simply isn't applicable or will only
   complicate the solution.  In-depth discussion of this point
   is outside the scope of this document.

.. note::

   Free-threading is one of the oldest concurrency models, fundamental
   to operating systems, and widely supported in programming languages.
   However, it is generally considered perilous and not human-friendly.
   Other concurrency models have demonstrated better usability and
   newer programming languages typically avoid exposing threads directly.
   Take that into consideration before reaching for threads and look at
   the alternatives first.

.. note::

   Python supports other concurrency models indirectly through
   community-maintained PyPI packages.  One well-known example is
   :pypi:`dask`, which supports "distributed" computing.


Quick reference
===============

We'll be using the following terms and ideas throughout:

task (logical thread)
   | a cohesive *linear* sequence of abstract steps in a program;
   | effectively, a mini-program;
   | the logical equivalent of executed instructions corresponding to code;
   | also known as "logical process"

physical thread (OS thread)
   | where the actual code for a logical thread runs on the CPU (and operating system);
   | we avoid using using plain "thread" for this, to avoid ambiguity

Python thread
   | the Python runtime running in a physical thread
   | particularly the portion of the runtime state active in the physical thread
   | (see :class:`threading.Thread`)

concurrency (multitasking)
   | a program with multiple logical threads running simultaneously
   | (not necessarily in parallel)

parallelism (multi-core)
   running a program's multiple logical threads on multiple physical
   threads (CPU cores)

.. raw:: html

   <style>
       table.vert-aligned th
       {
           vertical-align: top;
       }

       table.vert-aligned td
       {
           vertical-align: top;
       }
   </style>

For convenience, here are the concurrency primitives we'll cover later:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - primitive
     - used with
     - purpose
   * - ...
     - ...
     - ...

Likewise, the high-level examples:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - workload (app)
     - per-request inputs
     - per-request outputs
     - *N* core tasks
     - core task
   * - `grep <Workload: grep_>`_
     - | *N* filenames (**stdin**)
       | file bytes x *N* (**disk**)
     - *M* matches (**stdout**)
     - 1+ per file
     - | **time**: ~ file size
       | **mem**: small
   * - `... <Workload 2: ..._>`_
     - ...
     - ...
     - ...
     - ...
   * - `... <Workload 3: ..._>`_
     - ...
     - ...
     - ...
     - ...

To help further compare the models, there are side-by-side examples
of each of those "apps":

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - workload (app)
     - side-by-side examples
   * - `grep <Workload: grep_>`_
     - `by concurrency models <concurrency-grep-side-by-side_>`_
   * - `... <Workload 2: ..._>`_
     - `by concurrency models <concurrency-example-2-side-by-side_>`_
   * - `... <Workload 3: ..._>`_
     - `by concurrency models <concurrency-example-3-side-by-side_>`_


.. _concurrency-models:

Python Concurrency Models
=========================

As mentioned, there are essentially five concurrency models that
Python supports directly:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - model
     - Python stdlib
     - description
   * - free threading
     - :mod:`threading`
     - using multiple physical threads in the same process,
       with no isolation between them
   * - | isolated threads
       | (multiple interpreters)
     - `interpreters <python-stdlib-interpreters_>`_
     - threads, often physical, with strict isolation
       between them (e.g. CSP and actor model)
   * - coroutines (async/await)
     - :mod:`asyncio`
     - switching between logical threads is explicitly controlled by each
   * - multi-processing
     - :mod:`multiprocessing`
     - using multiple isolated processes
   * - distributed
     - `multiprocessing <multiprocessing-distributed_>`_
     - multiprocessing across multiple computers

There are tradeoffs to each, whether in performance or complexity.
We'll take a look at those tradeoffs in detail
`later <concurrency-pick-a-model_>`_.

Before that, we'll review various comparisons of the concurrency models,
and we'll briefly talk about `critical caveats <Critical caveats_>`_
for specific models.

Comparison tables
-----------------

The following tables provide a detailed look with side-by-side comparisons.

key characteristics
^^^^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * -
     - scale
     - multi-core
     - `races <concurrency-races_>`_
     - overhead
   * - free-threading
     - small-medium
     - `yes* <python-gil_>`_
     - **yes**
     - very low
   * - multiple interpreters
     - small-medium
     - yes
     - limited
     - `low+ <python-interpreters-overhead_>`_
   * - coroutines
     - small-medium
     - **no**
     - no
     - low
   * - multi-processing
     - small
     - yes
     - limited
     - **medium**
   * - distributed
     - large
     - yes
     - limited
     - **medium**

overhead details
^^^^^^^^^^^^^^^^

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * -
     - memory
     - startup
     - cross-task
     - management
     - system
   * - free threading
     - very low
     - very low
     - none
     - very low
     - none
   * - multiple interpreters
     - `low* <python-interpreters-overhead_>`_
     - `medium* <python-interpreters-overhead_>`_
     - low
     - very low
     - none
   * - coroutines
     - low
     - low
     - none
     - low
     - none
   * - multi-processing
     - medium
     - medium
     - medium
     - medium
     - low
   * - distributed
     - medium+
     - medium+
     - medium-high
     - medium
     - low-medium

complexity
^^^^^^^^^^

.. TODO "human-friendly"

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * -
     - parallel
     - | shared
       | mem
     - | shared
       | I/O
     - | shared
       | env
     - | cross
       | thread
     - :abbr:`sync (synchronization between logical threads)`
     - :abbr:`tracking (how easy it is to keep track of where one logical thread is running relative to another, especially when one terminates)`
     - :abbr:`compat (compatibility with code not using this concurrency model)`
     - | extra
       | LOC
   * - free-threading
     - `yes* <python-gil_>`_
     - **all**
     - **all**
     - **yes**
     - **high**
     - **explicit**
     -
     - yes
     - low?
   * - multiple interpreters
     - yes
     - limited
     - **all**
     - **yes**
     - low
     - implicit
     - ???
     - yes
     - low?
   * - coroutines
     - **no**
     - all
     - all
     - yes
     - low-med?
     - implicit
     - ???
     - **no**
     - low-med
   * - multi-processing
     - yes
     - limited
     - no
     - no?
     - low
     - | implicit
       | +optional
     - ???
     - yes
     - low-med?
   * - distributed
     - yes
     - limited
     - no
     - no?
     - low
     - | implicit
       | +optional
     - ???
     - yes
     - medium?

exposure
^^^^^^^^

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * -
     - | academic
       | research
     - | academic
       | curriculum
     - industry
     - examples
     - | Python
       | history
   * - free-threading
     - very high
     - high
     - high
     - high
     - 0.9?
   * - | isolated threads
       | (multiple interpreters)
     - high
     - low?
     - low-medium?
     - low-medium?
     - `2.2 <python-stdlib-interpreters_>`_
   * - coroutines
     - medium-high?
     - medium?
     - medium?
     - medium-high?
     - 3.3-3.5 (2.2)
   * - multi-processing
     - ???
     - low?
     - low-medium?
     - low?
     - 2.6
   * - distributed
     - medium-high?
     - low?
     - medium?
     - medium?
     - n/a

Critical caveats
----------------

Here are some important details to consider, specific to individual
concurrency models in Python.

.. contents::
   :local:

.. _concurrency-races:

Data races and non-deterministic scheduling (free-threading)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The principal caveat for physical threads is that each thread
shares the *full* memory of the process with all its other threads.
Combined with their non-deterministic scheduling (and parallel
execution), threads expose programs to a significant risk of races.

The potential consequences of a race are data corruption and invalidated
expectations of data consistency.  In each case, the non-deterministic
scheduling of threads means it is both hard to reproduce races and to
track down where a race happened.  These qualities make these bugs
especially frustrating and worth diligently avoiding.

Python threads are light wrappers around physical threads and thus have
the same caveats.  The majority of data in a Python program is mutable
and *all* of the program's data is subject to potential modification
by any thread at any moment.  This requires extra effort, to synchronize
around reads and writes.  Furthermore, given the maximally-broad scope
of the data involved, it's difficult to be sure all possible races
have been dealt with, especially as a code base changes over time.

The other concurrency models essentially don't have this problem.
In the case of coroutines, explicit cooperative scheduling eliminates
the risk of a simultaneous read-write or write-write.  It also means
program logic can rely on memory consistency between synchronization
points (``await``).

With the remaining concurrency models, data is never shared between
logical threads unless done explicitly (typically at the existing
inherent points of synchronization).  By default that shared data is
either read-only or managed in a thread-safe way.  Most notably,
the opt-in sharing means the set of shared data to manage is
explicitly defined (and often small) instead of covering
*all* memory in the process.

.. _python-gil:

The Global Interpreter Lock (GIL)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While physical threads are the direct route to multi-core parallelism,
Python's threads have always had an extra wrinkle that gets in the way:
the :term:`global interpreter lock` (GIL).

The :term:`!GIL` is very efficient tool for keeping the Python
implementation simple, which is an important constraint for the project.
In fact, it protects Python's maintainers *and* users from a large
category of concurrency problems that one must normally face when
threads are involved.

The big tradeoff is that the bytecode interpreter, which executes your
Python code, only runs while holding the :term:`!GIL`.  That means only
one thread can be running Python code at a time.  Threads will take
short turns, so none have to wait too long, but it still prevents
any actual parallelism of CPU-bound code.

That said, the Python runtime (and extension modules) can release the
:term:`!GIL` when the thread is going to be doing something unrelated
to Python, particularly something slow or long,
like a blocking IO operation.

There is also an ongoing effort to eliminate the :term:`!GIL`:
:pep:`630`.  Any attempt to remove the :term:`!GIL` necessarily involves
some slowdown to single-threaded performance and extra maintenance
burden to the Python project and extension module maintainers.
However, there is sufficient interest in unlocking full multi-core
parallelism to justify the current experiment.

You can also move from free-threading to isolated threads using multiple
interpreters.  Each interpreter has has its own
:term:`GIL <global interpreter lock>`.  Thus, If you want multi-core
parallelism, run a different interpreter in each thread.  Their
isolation means that each can run unblocked in that thread.

Thread isolation and multiple interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As just noted, races effectively stop being a problem if the memory
used by each physical thread is effectively isolated from the others.
That isolation can also help with the other caveats related to
physical threads.  In Python you can get this isolation
by using multiple interpreters.

In this context, an "interpreter" represents nearly all the capability
and state of the Python runtime, for its C-API and to execute Python
code.  The full runtime supports multiple interpreters and includes
some state that all interpreters share.  Most importantly, the state
of each interpreter is effectively isolated from the others.

That isolation includes things like :data:`sys.modules`.  By default,
interpreters mostly don't share any data (including objects) at all.
Anything that gets shared is done on a strictly opt-in basis.  That
means programmers wouldn't need to worry about possible races with
*any* data in the program.  They would only need to worry about data
that was explicitly shared.

Interpreters themselves are not specific to any thread, but instead
each physical thread has (at most) one interpreter active at any given
moment.  Each interpreter can be associated in this way with any number
of threads.  Since each interpreter is isolated from the others,
any thread using one interpreter is thus isolated from threads
using any other interpreter.

Using multiple interpreters is fairly straight-forward:

1. create a new interpreter
2. switch the current thread to use that interpreter
3. call :func:`exec`, but targeting the new interpreter
4. switch back

Note that no threads were involved.  That's because running in an
interpreter happens relative to the current thread.  New threads
aren't implicitly involved.

Multi-processing and distributed computing provide similar isolation,
though with some tradeoffs.

.. _python-stdlib-interpreters:

A stdlib module for using multiple interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While use of multiple interpreters has been part of Python's C-API
for decades, the feature hasn't been exposed to Python code through
the stdlib.  :pep:`734` proposes changing that by adding a new
:mod:`!interpreters` module.

In the meantime, an implementation of that PEP is available for
Python 3.13+ on PyPI: :pypi:`interpreters-pep-734`.

.. _python-interpreters-overhead:

Improving performance for multiple interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The long effort to improve on Python's implementation of multiple
interpreters focused on isolation and stability.  There was very little
done to improve performance.  This has the most impact on:

* how much memory each interpreter uses
  (i.e. how many can run at the same time)
* how long it takes to create a new interpreter

It also impacts how efficiently data/objects can be passed between
interpreters, and how effectively objects can be shared.

As the work on isolation wraps up, improvements will shift to focus
on performance and memory usage.  Thus the overhead associated with
using multiple interpreters will drastically decrease over time.

Shared resources
^^^^^^^^^^^^^^^^

Aside from memory, all physical threads in a process share the
following resources:

* commandline arguments ("argv")
* env vars
* current working directory
* signals, IPC, etc.
* open I/O resources (file descriptors, sockets, etc.)

When relevant, these must be managed in a thread-safe way.

Tracing execution
^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   The other potential problem with using threads is that the conceptual
   model has no inherent synchronization, so it can be hard to follow
   what is going on in the program at any given moment.  That is
   especially challenging for testing and debugging.

   * "callback hell"
   * "where was this thread/coroutine started?"
   * composing a reliable sequential representation of the program?
   * "what happened (in order) leading up to this point?"

   Besides unlocking full multi-core parallelism, the isolation between
   interpreters means that, from a conceptual level, concurrency can be
   simpler.

   The second category of complexity is the problem of tracing the execution
   of one logical thread relative to another.  This is especially relevant
   for error handling, when an error in the one thread is exposed in the
   other.  This applies equally to threads that start other threads as to
   concurrency models that use callbacks.  Knowing where the failing thread
   was started is valuable when debugging, as is knowing where a callback
   was registered.

Coroutines are contagious
^^^^^^^^^^^^^^^^^^^^^^^^^

Coroutines can be an effective mechanism for letting a program's
non-blocking code run while simultaneously waiting for blocking code
to finish.  The tricky part is that the underlying machinery (the
:ref:`event loop <asyncio-event-loop>`) relies on each coroutine
explicitly yielding control at the appropriate moments.

Normal functions do not follow this pattern, so they cannot take
advantage of that cooperative scheduling to avoid blocking
the program.  Thus coroutines and non-coroutines don't mix well.
While there are tools for wrapping normal functions to act like
coroutines, they are often converted into coroutines instead.
At that point, if any non-async code relies on the function then
either you'll need to convert the other code a coroutine or you'll
need to keep the original non-async implementation around along
with the new, almost identical async one.

You can see how that can proliferate, leading to possible extra
maintenance/development costs.

Processes consume extra resources
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When using multi-processing for concurrency, keep in mind that the
operating system will assign a certain set of limited resources to each
process.  For example, each process has its own PID and handle to the
executable.  You can run only so many processes before you run out of
these resources.  Concurrency in a single process doesn't have this
problem, and a distributed program can work around it.

.. _multiprocessing-distributed:

Using multiprocessing for distributed computing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Not only does the :mod:`multiprocessing` module support concurrency
with multiple local processes, it can also support a distributed model
using remote computers.  That said, consider first looking into tools
that have been designed specifically for distributed computing,
like :pypi:`dask`.

Resilience to crashes
^^^^^^^^^^^^^^^^^^^^^

A process can crash if it does something it shouldn't, like try to
access memory outside what the OS has provided it.  If your program
is running in multiple processes (incl. distributed) then you can
more easily recover from a crash in any one process.  Recovering
from a crash when using free-threading, multiple interpreters, or
coroutines isn't nearly so easy.

High-level APIs
---------------

Also note that Python's stdlib provides various higher-level APIs
that support these concurrency models in various contexts:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * -
     - :mod:`concurrent.futures`
     - :mod:`socketserver`
     - :mod:`http.server`
   * - free-threading
     - :class:`yes <concurrent.futures.ThreadPoolExecutor>`
     - :class:`yes <socketserver.ThreadingMixIn>`
     - :class:`yes <http.server.ThreadingHTTPServer>`
   * - multiple interpreters
     - (`pending <python-stdlib-interpreters_>`_)
     -
     -
   * - coroutines
     - ???
     -
     -
   * - multi-processing
     - | :class:`yes <concurrent.futures.ProcessPoolExecutor>`
       | (:class:`similar <multiprocessing.pool.Pool>`)
     - :class:`yes <socketserver.ForkingMixIn>`
     -
   * - distributed
     - ???
     -
     -


.. _concurrency-design:

Designing A Program For Concurrency
===================================

Whether you are starting a new project using concurrency or refactoring
an existing one to use it, it's important to design for concurrency
before taking one more step.  Doing so will save you a lot of
headache later.

1. decide if your program *might* benefit from concurrency
2. `break down your *logical* program into distinct tasks <concurrency-identify-tasks_>`_
3. `determine which tasks could run at the same time <concurrency-identify-tasks_>`_
4. `identify the other concurrency-related characteristics of your program <concurrency-characteristics_>`_
5. `decide which concurrency model fits best <concurrency-pick-a-model_>`_
6. go for it!

At each step you should be continuously asking yourself if concurrency
is still a good fit for your program.

Some problems are obviously not solvable with concurrency.  Otherwise,
even if you *could* use concurrency, it might not provide much value.
Furthermore, even if it seems like it would provide meaningful value,
the additional costs in performance, complexity, or maintainability
might outweigh that benefit.

Thus, when you're thinking of solving a problem using concurrency,
it's crucial that you understand the problem well.

Getting started
---------------

How can concurrency help?
^^^^^^^^^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   Here are the benefits concurrency can bring to the table:

   * ...


   Primarily, concurrency can be helpful by making your program faster
   and more responsive (less latency), when possible.  In other words,
   you get better computational throughput.  That happens by enabling
   the following:

   * run on multiple CPU cores (parallelism)
   * keep blocking resources from blocking the whole program
   * make sure critical tasks have priority
   * make sure other tasks have a fair share of time
   * process results as they come, instead of waiting for them all

   Other possible benefits:

   * asynchronous events can be handled more cleanly
   * better efficiency using hardware resources
   * improved scalability

How can concurrency hurt?
^^^^^^^^^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   The main challenge when using concurrency is the (potential) extra
   complexity.  This complexity comes from the effect of multiple logical
   threads running at the same time and interacting with each other.
   In practice, this falls into two categories: data races and tracing
   relative execution.  Both are a form of "spooky action at a distance" [#f1]_
   (meaning something changes unexpectedly in one place due to unknown
   changes somewhere else).

Analyze your problem
--------------------

.. _concurrency-identify-tasks:

Identifying the logical tasks in your program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   At its most fundamental, concurrency means doing multiple things at once,
   from a strictly *logical* viewpoint.

   When a computer program runs, it executes a sequence of code
   in a given order.  If you were to trace the actual execution, you would
   still end up with a *linear* series of executed instructions that matches
   the code.  We call this sequence of code (and instructions) a logical
   "thread" of execution.

   Sometimes it makes sense to break up that sequence into smaller pieces,
   where some of them can run independently of others.  Thus the program
   then involves multiple logical threads.  This is also called
   "multitasking" and each logical thread a "task".

   One important observation is that most concurrent programs
   can be represented instead as a single task, with the code of the
   concurrent tasks merged into a single sequence.

.. _concurrency-characteristics:

The concurrency characteristics of your program
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   For a given workload, here are some characteristics that will help you
   understand the problem and, potentially, which concurrency model would
   be the best fit:

   * requests

     * frequency
     * expected latency for (at least partial) response

   * inputs per request

     * how many
     * size of each input

   * tasks (logical threads) per input

     * how many
     * variety vs. uniformity
     * compute per task: how much
     * data per task: how much and what kinds
     * I/O per task: how much and what kinds
     * tasks not tied to outputs

   * task interaction

     * how much and in what ways
     * what data is shared between tasks
     * how much blocking while waiting

   * outputs per request

     * how many
     * size pf each output
     * correlation to inputs

   To some extent the most critical factors can be compressed down to:

   * many inputs vs. 1 large divisible input
   * many outputs vs. combined output vs. matching large output
   * many short computations vs. fewer medium/long computations

   We could also break it down into quadrants::

      .     stream of tasks       queue of tasks
      C                       |
      P                       |
      U                       |
      -                       |
      b                       |
      o                       |
      u                       |
      n                       |
      d                       |
       -----------------------|-----------------------
      I                       |
      O                       |
      -                       |
      b                       |
      o                       |
      u                       |
      n                       |
      d                       |


   Aside from the concurrency model, the answers to the above can impact
   the following:

   * use of a worker pool
   * use of background tasks/threads

   In the context of the above characteristics, let's revisit the ways that
   concurrency can be helpful:

   * get work done faster

      * run more tasks at once (multi-core)

   * make the app feel more responsive

      * make sure critical tasks have priority
      * process results as they come, instead of waiting for them all
      * send payload to multiple targets before starting next task

   * use system resources more efficiently

      * keep slow parts from blocking fast parts
      * keep blocking resources from blocking the whole program
      * make sure other tasks have a fair share of time
      * task scheduling & resource usage optimization

   * scaling
   * handle asynchronous events

Other considerations
^^^^^^^^^^^^^^^^^^^^

TBD

.. TODO finish

   * are there libraries that can take care of the concurrency parts?
   * ...

.. _concurrency-pick-a-model:

Pick a concurrency model
------------------------

TBD

.. TODO finish

   As mentioned earlier, each concurrency model has its own set of tradeoffs.
   Free-threading probably has the most notoriety and the most examples,
   but is also has the most pitfalls (see `Critical caveats`_ above).
   Isolated threads have few of those pitfalls but are less familiar
   and at least a little less efficient.
   Multiprocessing and distributed are likewise isolated, but less
   efficient, which can have a larger negative impact at smaller scales.
   Async can be straightforward, but may cascade throughout a code base
   and doesn't necessarily give you parallelism.

   free-threading:

   * main value: efficient multi-core
   * main costs: races & conceptual overhead

   * minimal conceptual indirection: closely tied to low-level physical threads
   * the most direct route to taking advantage of multi-core parallelism


   A high-level look:

   .. list-table::
      :header-rows: 1
      :class: borderless vert-aligned
      :align: left

      * - model
        - pros
        - cons
      * - free threading
        - * very light-weight and efficient
          * wide-spread
          * can enable multi-core parallelism (`caveat: GIL <python-gil_>`_)
        - * all memory is shared, subject to races
          * some IO may have races (e.g. writing to stdout)
          * can be hard for humans to follow what's happening in different
            threads at any given point
      * - multiple interpreters (isolated threads)
        - * isolation eliminates nearly all races, by default
            (sharing is strictly opt-in)
          * synchronization is built in to cross-interpreter interaction
          * enables full multi-core parallelism of all Python code
        - * unfamiliar to many
          * less efficient than threads
          * (currently) limited in what data can be shared between
            interpreters
      * - coroutines (async/await)
        - * not subject to races
          * increasingly familiar to many; popular in newer languages
          * has a long history in Python (e.g. ``twisted``)
        - * async and non-async functions don't mix well,
            potentially leading to duplication of code
          * switching to async can require substantial cascading code churn
          * callbacks can make it difficult to follow program logic,
            making debugging harder
          * does not enable multi-core parallelism
      * - multiprocessing
        - * isolated (no races)
          * enables full multi-core parallelism of all Python code
        - * substantially less efficient than using a single process
          * can lead to exhaustion of system resources
            (e.g. file handles, PIDs)
          * API can be hard to use
      * - distributed
        - * isolated (no races)
          * fully parallel
          * facilitates massive scaling
        - * not necessarily a good fit for small-scale applications
          * often requires configuration


.. _concurrency-primitives:

Python Concurrency Primitives
=============================

TBD

.. TODO finish

   Dealing with data races is often managed using locks (AKA mutexes),
   at a low level, and thread-safe types and APIs at a high level.
   Depending on the programming language, the complexity is sometimes
   mitigated somewhat by the compiler and runtime.  There are even
   libraries and frameworks that help abstract away the complexity
   to an extent.  On top of that, there are tools that can help identify
   potential races via static analysis.  Unfortunately, none of these aids
   is foolproof and the risk of hitting a race is always looming.

   synchronization
   ---------------

   Additionally, concurrency often involves some degree of synchronization
   between the logical threads.  At the most basic conceptual level:
   one thread may wait for another to finish.

   shared resources
   ----------------

   Aside from code running at the same time, concurrency typically
   also involves some amount of resources shared between the concurrent
   tasks.  That may include memory, files, and sockets.

Group A
-------

primitive 1
^^^^^^^^^^^

TBD

.. TODO finish

Group B
-------

primitive 1
^^^^^^^^^^^

TBD

.. TODO finish


.. _concurrency-workload-examples:

Python Concurrency Workload Examples
====================================

Below we have a series of examples of how to implement the most
common Python workloads that take advantage of concurrency.
For each workload you will find an implementation for each of the
concurrency models.

The implementations are meant to accurately demonstrate how best
to solve the problem using the given concurrency model.  The examples
for the workload are presented side-by-side, for easier comparison.
The examples for threads, multiprocessing, and multiple interpreters
will use :mod:`concurrent.futures` when that is the better approach.
Performance comparisons are not included here.

Here's a summary of the examples, by workload:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - workload
     - req in
     - req out
     - *N* core tasks
     - core task
   * - `grep <Workload: grep_>`_
     - | *N* filenames (**stdin**)
       | file bytes x *N* (**disk**)
     - *M* matches (**stdout**)
     - 1+ per file
     - | **time**: ~ file size
       | **mem**: small
   * - `... <Workload 2: ..._>`_
     - ...
     - ...
     - ...
     - ...
   * - `... <Workload 3: ..._>`_
     - ...
     - ...
     - ...
     - ...

.. other examples:

   * <numeric> (scientific, finance, ML, matrices)
   * conway's game of life
   * raytracer
   * mandelbrot
   * find primes
   * compute factorials
   * <apply transforms>
   * <merge-sort on a bunch of dicts>

   * <backtracking>
   * <divide-and-conquer>
   * <map-reduce>

Also see:

* https://github.com/faster-cpython/ideas/wiki/Tables:-Workloads
* https://github.com/ericsnowcurrently/concurrency-benchmarks

.. note::

   Each example is implemented as a basic commandline tool, but can be
   easily adapted to run as a web service.

Workload: grep
--------------

.. _example-grep-full-code:

.. raw:: html

   <details>
   <summary>(expand for full example code)</summary>

.. literalinclude:: ../includes/concurrency/grep/__init__.py
   :caption:
   :linenos:

.. literalinclude:: ../includes/concurrency/grep/__main__.py
   :caption:
   :linenos:

.. literalinclude:: ../includes/concurrency/grep/_utils.py
   :caption:
   :linenos:

.. literalinclude:: ../includes/concurrency/grep/_implementations.py
   :caption:
   :linenos:

.. raw:: html

   </details>

This a basic Python implementation of the linux ``grep`` tool.
We read from one or more files and report about lines that match
(or don't match) the given regular expression.

This represents a workload involving a mix of moderate IO and CPU work.

Design and analysis
^^^^^^^^^^^^^^^^^^^

Design steps from `above <concurrency-design_>`_:

1. concurrency fits?

   Yes!  There is potentially a bunch of work happening at the same time,
   and we want results as fast as possible.

2. identify logical tasks

   At a high level, the application works like this:

   1. handle args (including compile regex)
   2. if recursive, walk tree to find filenames
   3. for each file, yield each match
   4. print each match
   5. exit with 0 if matched and 1 otherwise

   At step 3 we do the following for each file:

   a. open the file
   b. iterate over the lines
   c. apply the regex to each line
   d. yield each match
   e. close the file

3. select concurrent tasks

   Concurrent work happens at step 3.  Sub-steps a, b, and e are
   IO-intensive.  Sub-step c is CPU-intensive.  The simplest approach
   would be one concurrent worker per file.  Relative to a strictly
   sequential approach, there's extra complexity here in managing the
   workers, fanning out the work to them, and merging the results back
   into a single iterator.

   If we were worried about any particularly large file or sufficiently
   large regular expression, we could take things further.  That would
   involve splitting up step 3 even further by breaking the file into
   chunks that are divided up among multiple workers.  However, doing
   so would introduce extra complexity that might not pay for itself.

4. concurrency-related characteristics

   TBD

   .. TODO finish

5. pick best model

   TBD

   .. TODO finish

Here are additional key constraints and considerations:

* there's usually a limit to how many files can be open concurrently,
  so we'll have to be careful not to process too many at once
* the order of the yielded/printed matches must match the order of the
  requested files and the order of each files lines

High-level code
^^^^^^^^^^^^^^^

With the initial design and analysis done, let's move on to code.
We'll start with the high-level code corresponding to the application's
five top-level tasks we identified earlier.

Most of the high-level code has nothing to do with concurrency.
The parts that do are handled entirely by ``do_search()`` (highlighted),
which we'll look at in a moment.

.. literalinclude:: ../includes/concurrency/grep/__init__.py
   :start-after: [start-high-level]
   :end-before: [end-high-level]
   :linenos:
   :emphasize-lines: 3,10

Here's the script that wraps that code and does steps 4 and 5:

.. raw:: html

   <details>
   <summary>(expand)</summary>

.. literalinclude:: ../includes/concurrency/grep/__main__.py
   :linenos:

.. raw:: html

   </details>

As noted earlier, ``grep()`` itself isn't all that interesting
relative to concurrency.  Mostly it makes use of several basic helpers.
You can look at the implementation of the helpers
`above <example-grep-full-code_>`_.

One notable point is that the actual files are not opened until
we need to iterate over the lines.  For the most part, this is so we
can avoid dealing with passing an open file to a concurrency worker.
Instead we pass the filename, which is much simpler.

Concurrent Code
^^^^^^^^^^^^^^^

Now lets look at how concurrency actually fits in.  ``do_search()``
wraps the selected implementation.  The implementation, in turn,
is a callable that returns an iterator of ``Match`` objects.

Concurrency may be happening as long as that iterable hasn't been
exhausted.  That means it is happening more or less the entire time
we loop over the matches in ``render_matches()`` (in the script code
above).  We must factor that into our concurrent implementations.

With that in mind, lets look at the different concurrent implementations.

We'll actually start with a non-concurrent, sequential one:

.. literalinclude:: ../includes/concurrency/grep/_implementations.py
   :start-after: [start-impl-sequential]
   :end-before: [end-impl-sequential]
   :linenos:

``search_lines()`` is the sequential-search helper used by all the
implementations:

.. literalinclude:: ../includes/concurrency/grep/__init__.py
   :start-after: [start-search-lines]
   :end-before: [end-search-lines]
   :linenos:

Each yielded ``Match`` object is a namedtuple of
(filename, full line text, match text).
For inverted matches, ``match.match`` is not set.
For file-only matches, only ``match.filename`` is set.

Now let's look at the implementations for the various concurrency
models.  We'll start with the simplest: threads.

.. literalinclude:: ../includes/concurrency/grep/_implementations.py
   :start-after: [start-impl-threads-basic]
   :end-before: [end-impl-threads-basic]
   :linenos:

We can use that as a baseline for the other implementations.

Common pattern
^^^^^^^^^^^^^^

TBD

.. TODO finish

Here are some things we don't do but would be worth doing:

* stop iteration when requested (or for ``ctrl-C``)

.. _concurrency-grep-side-by-side:

Side-by-side
^^^^^^^^^^^^

Here's the implementations for the different concurrency models,
side-by-side for easy comparison:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - sequential
     - threads
     - multiple interpreters
     - coroutines
     - multiple processes
     - concurrent.futures
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-sequential]
          :end-before: [end-impl-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-threads-basic]
          :end-before: [end-impl-threads-basic]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-interpreters-basic]
          :end-before: [end-impl-interpreters-basic]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-asyncio-basic]
          :end-before: [end-impl-asyncio-basic]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-multiprocessing-basic]
          :end-before: [end-impl-multiprocessing-basic]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/grep/_implementations.py
          :start-after: [start-impl-cf-threads-basic]
          :end-before: [end-impl-cf-threads-basic]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

Model-specific details
^^^^^^^^^^^^^^^^^^^^^^

Here are some implementation-specific details we had to deal with.

threads:

* ...

interpreters:

* ...

multiprocessing:

* ...

asyncio:

* ...

concurrent.futures:

* ...

concurrent.futures
^^^^^^^^^^^^^^^^^^

For threads, multiprocessing, and
`multiple interpreters * <python-stdlib-interpreters_>`_,
you can also use :mod:`concurrent.futures`:

.. raw:: html

   <details>
   <summary>(expand)</summary>

.. literalinclude:: ../includes/concurrency/grep/_implementations.py
   :start-after: [start-impl-cf-threads-basic]
   :end-before: [end-impl-cf-threads-basic]
   :dedent:
   :linenos:

.. raw:: html

   </details>


.. raw:: html

   <br/>


Workload 2: ...
---------------

.. TODO include full code

TBD

.. TODO finish

Design and analysis
^^^^^^^^^^^^^^^^^^^

Design steps from `above <concurrency-design_>`_:

1. concurrency fits?

   TBD

   .. TODO finish

2. identify logical tasks

   TBD

   .. TODO finish

3. select concurrent tasks

   TBD

   .. TODO finish

4. concurrency-related characteristics

   TBD

   .. TODO finish

5. pick best model

   TBD

   .. TODO finish

Here are additional key constraints and considerations:

* ...

High-level code
^^^^^^^^^^^^^^^

# ...

.. _concurrency-example-2-side-by-side:

Side-by-side
^^^^^^^^^^^^

Here's the implementations for the different concurrency models,
side-by-side for easy comparison:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - sequential
     - threads
     - multiple interpreters
     - coroutines
     - multiple processes
     - concurrent.futures
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-sequential]
          :end-before: [end-w2-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-threads]
          :end-before: [end-w2-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-subinterpreters]
          :end-before: [end-w2-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-async]
          :end-before: [end-w2-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-multiprocessing]
          :end-before: [end-w2-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w2-cf]
          :end-before: [end-w2-cf]
          :dedent:
          :linenos:

       .. raw:: html

          </details>


.. raw:: html

   <br/>


Workload 3: ...
---------------

.. TODO include full code

TBD

.. TODO finish

Design and analysis
^^^^^^^^^^^^^^^^^^^

Design steps from `above <concurrency-design_>`_:

1. concurrency fits?

   TBD

   .. TODO finish

2. identify logical tasks

   TBD

   .. TODO finish

3. select concurrent tasks

   TBD

   .. TODO finish

4. concurrency-related characteristics

   TBD

   .. TODO finish

5. pick best model

   TBD

   .. TODO finish

Here are additional key constraints and considerations:

* ...

High-level code
^^^^^^^^^^^^^^^

# ...

.. _concurrency-example-3-side-by-side:

Side-by-side
^^^^^^^^^^^^

Here's the implementations for the different concurrency models,
side-by-side for easy comparison:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - sequential
     - threads
     - multiple interpreters
     - coroutines
     - multiple processes
     - concurrent.futures
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-sequential]
          :end-before: [end-w3-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-threads]
          :end-before: [end-w3-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-subinterpreters]
          :end-before: [end-w3-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-async]
          :end-before: [end-w3-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-multiprocessing]
          :end-before: [end-w3-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency/run-examples.py
          :start-after: [start-w3-cf]
          :end-before: [end-w3-cf]
          :dedent:
          :linenos:

       .. raw:: html

          </details>


.. XXX

   .. rubric:: Footnotes

   .. [#f1] The phrase was originally said by Albert Einstein about
      quantum entanglement.
