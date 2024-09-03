.. _concurrency-howto:

*****************
Concurrency HOWTO
*****************

There are many outstanding resources, both online and in print,
that would do an excellent job of introducing you to concurrency.
This howto document builds on those by walking you through how
to apply that knowledge using Python.

Python supports the following concurrency models directly:

* free-threading (stdlib, C-API)
* isolated threads, AKA CSP/actor model (stdlib*, C-API)
* coroutines, AKA async/await (language, stdlib, C-API)
* multi-processing (stdlib)
* distributed, e.g. SMP (stdlib)

In this document, we'll look at how to take advantage of this
concurrency support.  The overall focus is on the following:

* understanding the supported concurrency models
* factors to consider when designing a concurrent solution
* key concurrency primitives
* high-level, app-oriented practical examples

.. XXX Add a summary and section about key concurrency patterns

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


Python Concurrency Models
=========================

As mentioned, there are essentially five concurrency models that
Python supports directly:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - model
     - Python API
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
     - | :pypi:`dask`
       | (`multiprocessing <multiprocessing-distributed_>`_)
     - multiprocessing across multiple computers

After we look at some comparisons of the concurrency models,
we'll briefly talk about critical caveats for specific models.

Comparison tables
-----------------

The following tables provide a detailed look with side-by-side comparisons.
We'll also compare them at a high level in
`a later section <concurrency-pick-a-model_>`_.

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

   * - model
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

.. XXX "human-friendly"

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
     - research
     - curriculum
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

There are tradeoffs to each, whether in performance or complexity.
Free-threading probably has the most notoriety and the most examples,
but is also has the most pitfalls (see `concurrency-downsides`_ below).
Isolated threads have few of those pitfalls but are less familiar.
Multiprocessing and distributed are likewise isolated, but less
efficient, which can have a larger negative impact at smaller scales.
Async can be straightforward, but may cascade throughout a code base
and doesn't necessarily give you parallelism.

free-threading
^^^^^^^^^^^^^^

Python directly supports use of physical threads through the
:mod:`threading` module.

* minimal conceptual indirection: closely tied to low-level physical threads
* the most direct route to taking advantage of multi-core parallelism

The main downside to using threads is that each one shares the full
memory of the process with all the others.  That exposes programs
to a significant risk of `races <concurrency-downsides_>`_.

The other potential problem with using threads is that the conceptual
model has no inherent synchronization, so it can be hard to follow
what is going on in the program at any given moment.  That is
especially challenging for testing and debugging.

.. _python-gil:

The Global Interpreter Lock (GIL)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While physical threads are the direct route to multi-core parallelism,
Python's threads have always had an extra wrinkle that gets in the way:
the :term:`global interpreter lock` (GIL).

The :term:`!GIL` is very efficient tool for keeping the Python
implementation simple, which is an important constraint for the project.
In fact, it protects Python's maintainers and users from a large
category of concurrency problems that one must normally face when
threads are involved.

The big tradeoff is that the bytecode interpreter, which executes your
Python code, only runs while holding the :term:`!GIL`.  That means only
one thread can be running Python code at a time.  Threads will take
short turns, so none have to wait too long, but it still prevents
any actual parallelism.

At the same time, the Python runtime (and extension modules) can
release the :term:`!GIL` when the thread is going to be doing something
unrelated to Python, particularly something slow or long,
like a blocking IO operation.

There is also an ongoing effort to eliminate the :term:`!GIL`:
:pep:`630`.  Any attempt to remove the :term:`!GIL` necessarily involves
some slowdown to single-threaded performance and extra maintenance
burden to the Python project and extension module maintainers.
However, there is sufficient interest in unlocking full multi-core
parallelism to justify the current experiment.


Isolated Threads (CSP/Actor Model)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

There's a major alternative to free-threading, both for multi-core
parallelism and for a simpler conceptual model: use multiple interpreters.

Python's major implementation, CPython, has for decades supported
running with multiple independent copies of the Python runtime
("interpreter") in a single process.  However, these interpreters
weren't completely isolated from one another; most importantly they
shared the one :term:`!GIL`.  Over several years a lot of work went
into improving the isolation between interpreters, culminating in
no longer sharing a single :term:`!GIL`.

Besides unlocking full multi-core parallelism, the isolation between
interpreters means that, from a conceptual level, concurrency can be
simpler.  An interpreter encapsulates all of Python's runtime state,
including things like :data:`sys.modules`.  By default, interpreters
mostly don't share any data (including objects) at all.  Anything that
gets shared is done on a strictly opt-in basis.  That means programmers
don't need to worry about possible `races <concurrency-downsides_>`_
with *any* data in the program.  They only need to worry about data
that was explicitly shared.

Using multiple interpreters is fairly straight-forward:

1. create a new interpreter
2. switch the current thread to use that interpreter
3. call :func:`exec`, but targeting the new interpreter
4. switch back

Note that no threads were involved.  That's because running in an
interpreter happens relative to the current thread.  New threads
aren't implicitly involved.  They can be added in explicitly though.
Why?  For multi-core parallelism.

If you want multi-core parallelism, run a different interpreter in each
thread.  Their isolation means that each can run unblocked in that
thread.

.. _python-stdlib-interpreters:

A Stdlib Module for Using Multiple Interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

While use of multiple interpreters has been part of Python's C-API
for decades, the feature hasn't been exposed to Python code through
the stdlib.  :pep:`734` proposes changing that by adding a new
:mod:`!interpreters` module.

In the meantime, an implementation of that PEP is available for
Python 3.13+ on PyPI: :pypi:`interpreters-pep-734`.

.. _python-interpreters-overhead:

Improving Performance for Multiple Interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The long effort to improve on Python's implementation of multiple
interpreters focused on isolation and stability.  There was very little
done to improve performance.  This has the most impact on:

* how much memory each interpreter uses
  (i.e. how many can run at the same time)
* how long it takes to create a new interpreter

As the work on isolation wraps up, improvements will shift to focus
on performance and memory usage.  Thus the overhead associated with
using multiple interpreters will drastically decrease over time.

Coroutines (Async/Await)
^^^^^^^^^^^^^^^^^^^^^^^^

The use of :term:`coroutines <coroutine>` for concurrency has been
around a long time and has grown in popularity in the software world,
particularly with the addition of ``async/await`` syntax in
various languages.

Python has supported coroutines to some degree since the beginning.
The best example is :pypi:`twisted`, which has provided this concurrency
model for decades.  For most of that time :pypi:`!twisted` did it
primarily through callbacks and a form of "promises"/"futures".

Explicit support for coroutines in Python really started with the
introduction of :term:`generators <generator>` in Python 2.2
(:pep:`255`).  In Python 2.5 (:pep:`342`), :term:`!generators` were
tweaked to explicitly support use as coroutines.  That went a step
further in Python 3.3 with the addition of ``yield from`` (:pep:`380`)
and the :mod:`asyncio` module (:pep:`3156`).  Finally, in Python 3.5
(:pep:`492`), we got dedicated ``async/await`` syntax
and :ref:`a dedicated protocol <coroutine-protocol>`
for :term:`!coroutine` objects.

There are three main pieces to using coroutines:

* coroutines (non-blocking, yield control instead)
* an event loop (schedules coroutines)
* coroutine wrappers around blocking operations

A :term:`coroutine function` looks *almost* the same as a regular
function.  It is a non-blocking function that *cooperatively* yields
control of the program to other coroutines, which in turn yield control
back (eventually).  At those points of synchronization,
coroutines often provide data to one another.

The event loop is what keeps track of which coroutines have yielded
control and which should get control next.

Generally a coroutine needs to avoid doing anything that takes very long
before yielding control back to the event loop.  Any blocking operation
in a coroutine, like waiting on a socket, has to be implemented in a way
that only waits a little while, yields, and then waits again, etc. until
ready.  The alternative is to wrap the blocking operation/function
in some sort of "future" coroutine that yields until the blocking
operation completes.  The event loop can also fill that role
to an extent.

In addition to support for coroutines in the language, Python's stdlib
provides the :mod:`asyncio` module, which includes:

* an event loop
* a number of useful coroutines
* a variety of helpful APIs that build on coroutines and the event loop

One of the main challenges with using coroutines is that they do not
normally mix well with non-coroutines.  As a result, ``async/await``
can be contagious, requiring surrounding code to be async.  This can
lead to having the same thing implemented twice, once normal and once
async, with significant code duplication.

Multi-processing
^^^^^^^^^^^^^^^^

The stdlib :mod:`multiprocessing` module, which has been around many
years, provides an API for using multiple processes for concurrency.
Furthermore, processes are always isolated, so you have many of the
same benefits of using multiple interpreters, including multi-core
parallelism.

There are some obstacles however.  First of all, using multiple
processes has a higher overhead than operating in a single process,
sometimes significantly higher.  This applies in just about every
dimension of overhead.  Secondly, the :mod:`multiprocessing` module's
API is substantially larger and more complex that what we use for
threads and multiple interpreters.  Finally, there are some scaling
issues with using multiple processes, related both to the performance
overhead and to how the operating system assigns resources like
file handles.

The similarity with :class:`threading.Thread` is intentional.
On top of that, the :mod:`multiprocessing` module provides an extensive
API to address a variety of needs, including machinery for inter-process
shared memory.  Also note that that API can be used for threads and
(eventually) interpreters using different backends.

Distributed
^^^^^^^^^^^

When it comes to concurrency at scale, through distributed concurrency,
one of the best examples is :pypi:`dask`.

.. _multiprocessing-distributed:

Using multiprocessing for distributed computing
-----------------------------------------------

...





.. _concurrency-races:

Data races
----------

The first category relates to mutable data shared between threads:
a data race is where one thread writes to memory at a time when another
thread is expecting the value to be unchanged, invalidating its logic.
Similarly, two threads could write to the same memory location at the
same time, either corrupting the data there or invalidating
the expectations of one of the threads.

In each case, the non-deterministic scheduling of threads means it is
both hard to reproduce races and to track down where a race happened.
These qualities much these bugs especially frustrating
and worth diligently avoiding.

Races are possible when the concurrency approach is subject
to parallel execution or to non-deterministic switching.
(This excludes coroutines, which rely on cooperative multitasking.)
When all memory is possibly shared, as is the case with free-threading,
then all memory is at risk.

Dealing with data races is often managed using locks (AKA mutexes),
at a low level, and thread-safe types and APIs at a high level.
Depending on the programming language, the complexity is sometimes
mitigated somewhat by the compiler and runtime.  There are even
libraries and frameworks that help abstract away the complexity
to an extent.  On top of that, there are tools that can help identify
potential races via static analysis.  Unfortunately, none of these aids
is foolproof and the risk of hitting a race is always looming.



.. _concurrency-downsides:

What are the downsides?
-----------------------

The main challenge when using concurrency is the (potential) extra
complexity.  This complexity comes from the effect of multiple logical
threads running at the same time and interacting with each other.
In practice, this falls into two categories: data races and tracing
relative execution.  Both are a form of "spooky action at a distance" [#f1]_
(meaning something changes unexpectedly in one place due to unknown
changes somewhere else).

The first category relates to mutable data shared between threads:
a data race is where one thread writes to memory at a time when another
thread is expecting the value to be unchanged, invalidating its logic.
Similarly, two threads could write to the same memory location at the
same time, either corrupting the data there or invalidating
the expectations of one of the threads.

In each case, the non-deterministic scheduling of threads means it is
both hard to reproduce races and to track down where a race happened.
These qualities much these bugs especially frustrating
and worth diligently avoiding.

Races are possible when the concurrency approach is subject
to parallel execution or to non-deterministic switching.
(This excludes coroutines, which rely on cooperative multitasking.)
When all memory is possibly shared, as is the case with free-threading,
then all memory is at risk.

Dealing with data races is often managed using locks (AKA mutexes),
at a low level, and thread-safe types and APIs at a high level.
Depending on the programming language, the complexity is sometimes
mitigated somewhat by the compiler and runtime.  There are even
libraries and frameworks that help abstract away the complexity
to an extent.  On top of that, there are tools that can help identify
potential races via static analysis.  Unfortunately, none of these aids
is foolproof and the risk of hitting a race is always looming.

.. XXX mention reentrancy?

The second category of complexity is the problem of tracing the execution
of one logical thread relative to another.  This is especially relevant
for error handling, when an error in the one thread is exposed in the
other.  This applies equally to threads that start other threads as to
concurrency models that use callbacks.  Knowing where the failing thread
was started is valuable when debugging, as is knowing where a callback
was registered.




high-level APIs
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

How can concurrency help?
-------------------------

Here are the benefits concurrency can bring to the table:

* 


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
-------------------------

...

.. _concurrency-identify-tasks:

Identifying the logical tasks in your program
------------------------------------------------

...

.. _concurrency-characteristics:

The concurrency characteristics of your program
------------------------------------------------

...

.. _concurrency-pick-a-model:

Picking a concurrency model
---------------------------

...

free-threading:

* main value: efficient multi-core
* main costs: races & conceptual overhead

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




* are there libraries that can take care of the concurrency parts?



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


What problems can concurrency help solve?
-----------------------------------------


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


Python Concurrency Primitives
=============================

...


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

This a basic Python implementation of the linux ``grep`` tool.
From a concurrency standpoint, each file is processed in its own
logical thread.

Here's the non-concurrent app code that all the implementations share:

.. raw:: html

   <details>
   <summary>(expand)</summary>

.. literalinclude:: ../includes/concurrency.py
   :start-after: [start-grep-common]
   :end-before: [end-grep-common]
   :dedent:
   :linenos:

.. raw:: html

   </details>

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
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-grep-sequential]
          :end-before: [end-grep-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-grep-threads]
          :end-before: [end-grep-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-grep-subinterpreters]
          :end-before: [end-grep-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-grep-async]
          :end-before: [end-grep-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-grep-multiprocessing]
          :end-before: [end-grep-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

For threads, multiprocessing, and
`multiple interpreters * <python-stdlib-interpreters_>`_,
you can also use :mod:`concurrent.futures`:

.. raw:: html

   <details>
   <summary>(expand)</summary>

.. literalinclude:: ../includes/concurrency.py
   :start-after: [start-grep-cf-threads]
   :end-before: [end-grep-cf-threads]
   :dedent:
   :linenos:

.. raw:: html

   </details>

.. raw:: html

   <br/>

Workload 2: ...
---------------

# ...

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
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w2-sequential]
          :end-before: [end-w2-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w2-threads]
          :end-before: [end-w2-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w2-subinterpreters]
          :end-before: [end-w2-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w2-async]
          :end-before: [end-w2-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w2-multiprocessing]
          :end-before: [end-w2-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

.. raw:: html

   <br/>

Workload 3: ...
---------------

# ...

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
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w3-sequential]
          :end-before: [end-w3-sequential]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w3-threads]
          :end-before: [end-w3-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w3-subinterpreters]
          :end-before: [end-w3-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w3-async]
          :end-before: [end-w3-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :start-after: [start-w3-multiprocessing]
          :end-before: [end-w3-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>


.. rubric:: Footnotes

.. [#f1] The phrase was originally said by Albert Einstein about
   quantum entanglement.
