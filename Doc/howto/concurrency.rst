.. _concurrency-howto:

*****************
Concurrency HOWTO
*****************

Python is a language the accommodates a variety of programming styles,
from procedural to object-oriented to functional.  The same applies
to concurrency.  Here we'll look at how different concurrency models
look in Python, with an emphasis on practical workload-oriented examples.

The following concurrency models are covered:

* free-threading
* isolated threads, AKA CSP/actor model
* multi-processing
* distributed, e.g. SMP
* async/await

Each of these will be explained, with some simple examples.  The later
workload-oriented examples will be implemented using each,
for comparison, when possible.

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

For convenience, here's a summary comparng the concurrency models
in Python:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - `model <Concurrency Models_>`_
     - Python API
     - scale
     - `multi-core <What is parallelism?_>`_
     - `races <concurrency-downsides_>`_
     - overhead
     - `c.f <concurrent.futures_>`_
   * - free threading `(Python) <python-free-threading_>`_
     - :mod:`threading`
     - small-medium
     - `yes* <python-gil_>`_
     - **yes**
     - very low
     - yes
   * - isolated threads `(Python) <python-isolated-threads_>`_
     - `interpreters <python-stdlib-interpreters_>`_
     - small-medium
     - yes
     - no
     - `low+ <python-interpreters-overhead_>`_
     - `yes* <python-stdlib-interpreters_>`_
   * - multiprocessing `(Python) <python-multiprocessing_>`_
     - :mod:`multiprocessing`
     - small
     - yes
     - no
     - **medium**
     - yes
   * - distributed `(Python) <python-distributed_>`_
     - :mod:`!dask`
     - large
     - yes
     - no
     - **medium+**
     - no
   * - async/await `(Python) <python-async-await_>`_
     - :mod:`asyncio`
     - small-medium
     - **no**
     - no
     - low
     - no


All About Concurrency
=====================

What is concurrency?
--------------------

At its most fundamental, concurrency means doing multiple things at once,
from a strictly *logical* viewpoint.

When a computer program runs, it executes a sequence of code
in a given order.  If you were to trace the actual execution, you would
still end up with a *linear* series of executed intructions that matches
the code.  We call this sequence of code (and instructions) a logical
"thread" of execution.

Sometimes it makes sense to break up that sequence into smaller pieces,
where some of them can run independently of others.  Thus the program
then involves multiple logical threads.  This is also called
"multitasking" and each logical thread a "task".

As an example of splitting up the sequence, consider the following
abstract program with three pieces::

   prep
   do A
   do B

If both ``do A`` and ``do B`` only rely on ``prep`` having completed,
then we could rearrange the program in one of the following ways and
end up with the same result::

   prep   =   prep    prep   =   prep -----
   do B   =   do A    do B   =     |      |
   do A   =                  =   do A   do B

In the first alternative, we swap ``do A`` and ``do B``.  In the second
one we split the original program into two programs that we can run at
the same time.  In the third one, we run ``do A`` and ``do B`` at the
same time.  "At the same time" means concurrency.  It always involves
multiple logical threads.

Additionally, concurrency often involves some degree of synchronization
between the logical threads.  At the most basic conceptual level:
one thread may wait for another to finish.

Aside from code running at the same time, concurrency typically
also involves some amount of resources shared between the concurrent
tasks.  That may include memory, files, and sockets.

One important observation is that most concurrent programs
can be represented instead as a single task, with the code of the
concurrent tasks merged into a single sequence.

What is parallelism?
--------------------

Concurrency may happen in one of two ways.  The concurrent tasks may
share a single CPU, each running a little bit at a time, with the
operating system (or language runtime) taking care of the switching.
The other way is where each task runs on its own CPU, meaning they
are physically running at the same time, not just logically.

That second way is parallelism.

Modern CPUs are designed around parallelism, with multiple cores
and sometimes multiple execution pipelines per core.  The operating
system exposes physical CPU threads as OS threads and as processes.
A programming language (or runtime) may add additional layers of
abstraction on top of that.

Parallelism is where concurrent logical threads are running
on distinct physical threads across multiple cores,

Concurrency Models
------------------

The concept of concurrency has been a part of the study and practice
of computer software since very early on, in the 1950s and 1960s,
long before the wide-spread adotion of multi-core CPUs.  Clearly
its about more than just parallelism.

Over the decades, research and use of concurrency has led to a variety
of well defined abstract models, with different characteristics and
tradeoffs.  The application of the different theoretical concurrency
models can be categorized as follows:

================= ==========
free threads      using multiple physical threads in the same process,
                  with no isolation between them
isolated threads  threads, often physical, with strict isolation
                  between them (e.g. CSP and actor model)
multiprocessing   using multiple isolated processes
distributed       multiprocessing across multiple computers
async/await       using coroutines (AKA "cooperative multitasking")
================= ==========

(There are certainly others, but these are the focus here.)

There are tradeoffs to each.  Free-threading probably has the most
notoriety and the most examples, but is also has the most pitfalls
(see `concurrency-downsides`_ below).
Isolated threads have few of those pitfalls but are less familiar.
Multiprocessing and distributed are likewise isolated, but less
efficient, which can have a larger negative impact at smaller scales.
Async can be straightforward, but may cascade throughout a code base
and doesn't necessarily give you parallelism.

What problems can concurrency help solve?
-----------------------------------------

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
(This excludes "async/await", which relies on cooperative multitasking.)
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

Workloads
---------

In practice, concurrency is used in a wide variety of software.
Here's a not-comprehensive list:

======================= ===========
application             concurrency
======================= ===========
web server              handle simultaneous static requests, CGI requests
web browser             load multiple resources at once
database server         handle simultaneous requests
devops script           process multiple files at once
system logger           handle simultaneous logging requests
ATM network             handle multiple bank transactions at once
hacker toolkit          decode a passwd file with brute force
raytracer               compute RGB for each image pixel
machine learning        apply matrices on training data set
astrophysics            merge black hole data from multiple satelites and observatories
investing               combine thousands of industry data sources into a concise actionable analysis
MMO game server         handle login requests, handle client updates
game client             GUI, physics engine, handle server updates
audio transcoder        process chunks
engineering simultation calculate stress loads at vertices
molecular modeling      try many permutations
======================= ===========

It can be helpful to identify common characteristics by which we could
group concurrency workloads.  Here are some:

* number of logical threads
* main + workers vs. independent
* main + background
* how much computation, per thread
* how much blocking on other threads, per thread
* how much blocking IO, per thread
* number of external inputs
* number of external outputs
* how much data used, per thread
* how much data do logical threads share
* size of the data shared by threads

From our list of workloads above, we can observe some clustering:

* ...

Let's also revisit the ways concurrency can be helpful:

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

All of these things factor in to how concurrency should be applied for
a workload, or even if it should.


Python Concurrency Models
=========================

We've looked at concurrency and concurrency models generally.
Now let's see what each looks like in Python.
We'll also look at `concurrent.futures <concurrent.futures_>`_
provides a high-level API for some of the concurrency models.

Here's a summary:

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - model
     - Python API
     - scale
     - pros
     - cons
   * - free threading
     - :mod:`threading`
     - small-medium
     - * familiar to many
       * many examples available
       * can enable multi-core parallelism (`caveat: GIL <python-gil_>`_)
     - * all memory is subject to races
       * some IO may have races (e.g. writing to stdout)
       * can be hard for humans to follow what's happening in different
         threads at any given point
   * - multiple interpreters (isolated threads)
     - `interpreters <python-stdlib-interpreters_>`_
     - small-medium
     - * isolation eliminates nearly all races, by default
         (sharing is strictly opt-in)
       * synchronization is built in to cross-interpreter interaction
       * enables full multi-core parallelism of all Python code
     - * unfamiliar to many
       * less efficient than threads
       * (currently) limited in what data can be shared between
         interpreters
   * - multiprocessing
     - :mod:`multiprocessing`
     - small
     - * isolated (no races)
       * enables full multi-core parallelism of all Python code
     - * substantially less efficient than using a single process
       * can lead to exhaustion of system resources
         (e.g. file handles, PIDs)
       * API can be hard to use
   * - distributed
     - :mod:`!dask`
     - large
     - * isolated (no races)
       * fully parallel
       * facilitates massive scaling
     - * not necessarily a good fit for small-scale applications
       * often requires configuration
   * - async/await
     - :mod:`asyncio`
     - small-medium
     - * not subject to races
       * increasingly familiar to many; popular in newer languages
       * has a long history in Python (e.g. ``twisted``)
     - * async and non-async functions don't mix well,
         potentially leading to duplication of code
       * switching to async can require substantial cascading code churn
       * callbacks can make it difficult to follow program logic,
         making debugging harder
       * does not enable multi-core parallelism

Here's a comparison of the overhead of each model in Python:

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
   * - multiprocessing
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
   * - async/await
     - low
     - low
     - none
     - low
     - none

.. _python-free-threading:

Free-threading
--------------

.. currentmodule:: threading

Threads, through the :mod:`threading` module, have been the dominant
tool in Python concurrency for decades, which mirrors the generate state
of software in general.  Threads are very light-weight and efficient.
Most importantly, they are the most direct route to taking advantage
of multi-core parallelism (more an that in a moment).

The main downside to using threads is that each one shares the full
memory of the process with all the others.  That exposes programs
to a significant risk of `races <concurrency-downsides_>`_.

The other potential problem with using threads is that the conceptual
model has no inherent synchronization, so it can be hard to follow
what is going on in the program at any given moment.  That is
especially challenging for testing and debugging.

Using threads for concurrency boils down to:

1. create a thread object to run a function
2. start the thread
3. (optionally) wait for it to finish

Here's how that looks::

    import threading

    def task():
        # Do something.
        ...

    t = threading.Thread(target=task)
    t.start()

    # Do other stuff.

    t.join()

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

.. currentmodule:: None

.. _python-isolated-threads:

Isolated Threads (CSP/Actor Model)
----------------------------------

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

You can use the :mod:`!interpreters` module (more on that in a moment)
to do this::

    import interpreters

    script = """if True:
        # Do something.
        ...
        """

    interp = interpreters.create()
    interp.exec(script)

Note that no threads were involved.  That's because running in an
interpreter happens relative to the current thread.  New threads
aren't implicitly involved.  They can be added in explicitly though.
Why?  For multi-core parallelism.

If you want multi-core parallelism, run a different interpreter in each
thread.  Their isolation means that each can run unblocked in that
thread.

Here's the very explicit way to do that::

    import interpreters
    import threading

    script = """if True:
        # Do something.
        ...
        """

    def task():
        interp = interpreters.create()
        interp.exec(script)

    t = threading.Thread(target=task)
    t.start()

    # Do other stuff.

    t.join()

There's a convenience method too::

    import interpreters

    def task():
        # Do something.
        ...

    interp = interpreters.create()
    t = interp.call_in_thread(task)

    # Do other stuff.

    t.join()

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

.. _python-multiprocessing:

Multi-processing
----------------

.. currentmodule:: multiprocessing

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

Here's a very basic example::

    import multiprocessing

    def task()
        # Do something.
        pass

    p = multiprocessing.Process(target=task)
    p.start()

    # Do other stuff.

    p.join()

The similarity with :class:`threading.Thread` is intentional.
On top of that, the :mod:`multiprocessing` module provides an extensive
API to address a variety of needs, including machinery for inter-process
shared memory.  Also note that that API can be used for threads and
(eventually) interpreters using different backends.

.. currentmodule:: None

.. _python-distributed:

Distributed
-----------

The popular :mod:`!dask` module gives us distributed concurrency:

::

    from dask.distributed import LocalCluster

    def task()
        # Do something.
        pass

    client = LocalCluster().get_client()

    futures = []
    for _ in range(5):
        fut = client.submit(task)
        futures.append(fut)

    # Wait for all the tasks to finish.
    client.gather(futures)

.. _python-async-await:

Async/Await
-----------

.. currentmodule:: asyncio

The stdlib :mod:`asyncio` module provides an event loop you can use:

::

    import asyncio

    async def task():
        # Do something.
        pass

    coros = [task() for _ in range(5)]

    # Wait for all the coroutines to finish.
    await asyncio.gather(*coros)

.. currentmodule:: None

concurrent.futures
------------------

.. currentmodule:: concurrent.futures

:mod:`concurrent.futures` provides a high-level abstraction around
using concurrency in Python.

The :class:`Executor` base class is the focal point of the API.
It is implemented for threads as :class:`ThreadPoolExecutor`, wrapping
:class:`threading.Thread`.  It is implemented for subprocesses as
:class:`ProcessPoolExecutor`, wrapping :mod:`multiprocessing`.
It will be implemented for multiple interpreters as
:class:`!InterpreterPoolExecutor`.  Each implementation has some very
minor uniqueness that we'll look at in a moment.

.. note: :mod:`multiprocessing`, :mod:`asyncio`, and ``dask``
   provide similar APIs.  In the case of :mod:`!multiprocessing`,
   that API also supports thread and interpreter backends.

.. note: Generic examples in this section will use the thread-based
   implementation.  However, any of the other implementations
   can be simply substituted.

With an executor you can call a function asynchronously (in the background)
using :meth:`Executor.submit`.  It returns a :class:`Future` object
which tracks completion and provides the result.
:class:`!Future` objects have a few other tricks, like cancelation
and completion callbacks, which we won't cover here.
Likewise we won't cover the various uses of timeouts.

Here's an example of using :meth:`Executor.submit`
and :meth:`Future.result`:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-basic
   :start-after: [start-cf-basic]
   :end-before: [end-cf-basic]
   :dedent:
   :linenos:

You can use :meth:`Executor.map` to call a function multiple times
and yield each result:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-map-1
   :start-after: [start-cf-map-1]
   :end-before: [end-cf-map-1]
   :dedent:
   :linenos:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-map-2
   :start-after: [start-cf-map-2]
   :end-before: [end-cf-map-2]
   :dedent:
   :linenos:

You can wait for an existing set of :class:`!Future` objects using
:func:`wait` (and :func:`as_completed` and :meth:`Executor.map`):

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-wait
   :start-after: [start-cf-wait]
   :end-before: [end-cf-wait]
   :dedent:
   :linenos:

You can use :func:`as_completed` to handle each :class:`!Future`
as it completes:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-as-completed
   :start-after: [start-cf-as-completed]
   :end-before: [end-cf-as-completed]
   :dedent:
   :linenos:

In each case handling errors on a per-:class:`!Future` basis
is straightforward:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-error-result-1
   :start-after: [start-cf-error-result-1]
   :end-before: [end-cf-error-result-1]
   :dedent:
   :linenos:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-error-result-2
   :start-after: [start-cf-error-result-2]
   :end-before: [end-cf-error-result-2]
   :dedent:
   :linenos:

As promised, here's a look at what is unique to each of the
:class:`Executor` implementations.


:class:`ThreadPoolExecutor`:

* ...

:class:`ProcessPoolExecutor`:

* ...

:class:`!InterpreterPoolExecutor`:

* ...

.. currentmodule:: None


Python Concurrency Workload Examples
====================================

Below we have a series of examples of how to implement the most
common Python workloads that take advantage of concurrency.

...

also see:

* https://github.com/faster-cpython/ideas/wiki/Tables:-Workloads
* https://github.com/ericsnowcurrently/concurrency-benchmarks


Workload 1
----------

# ...

.. list-table::
   :header-rows: 1
   :class: borderless vert-aligned
   :align: left

   * - threads
     - multiple interpreters
     - async/await
     - multiple processes
     - SMP
   * - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :name: concurrency-workload-1-threads
          :start-after: [start-w1-threads]
          :end-before: [end-w1-threads]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :name: concurrency-workload-1-subinterpreters
          :start-after: [start-w1-subinterpreters]
          :end-before: [end-w1-subinterpreters]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :name: concurrency-workload-1-async
          :start-after: [start-w1-async]
          :end-before: [end-w1-async]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :name: concurrency-workload-1-multiprocessing
          :start-after: [start-w1-multiprocessing]
          :end-before: [end-w1-multiprocessing]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

     - .. raw:: html

          <details>
          <summary>(expand)</summary>

       .. literalinclude:: ../includes/concurrency.py
          :name: concurrency-workload-1-smp
          :start-after: [start-w1-smp]
          :end-before: [end-w1-smp]
          :dedent:
          :linenos:

       .. raw:: html

          </details>

Workload 2
----------

...


.. rubric:: Footnotes

.. [#f1] The phrase was originally said by Albert Einstein about
   quantum entanglement.
