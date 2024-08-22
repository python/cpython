.. _concurrency-howto:

*****************
Concurrency HOWTO
*****************

Python is a language the accommodates a variety of programming styles,
from procedural to object-oriented to functional.  The same applies
to concurrency.  Here we'll look at how different concurrency models
look in Python, with an emphasis on practical workload-oriented examples.

The following Python concurrency models are covered:

* free-threading (:mod:`threading` and :mod:`concurrent.futures`)
* isolated threads, AKA CSP/actor model (:mod:`!interpreters`)
* multi-processing (:mod:`multiprocessing` and :mod:`concurrent.futures`)
* distributed, e.g. SMP (:mod:`!dask`)
* async/await (:mod:`asyncio`)

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
   See `the table below <concurrency-models-comparison_>`_.


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
Now let's see what they look like in Python.

We'll also `compare them below <concurrency-models-comparison_>`_.

Finally, we'll look at how `concurrent.futures <concurrent.futures_>`_
provides a high-level API for some of the concurrency models.

Free-threading
--------------

.. currentmodule:: threading

For free-threading we can use the stdlib :mod:`threading` module.

Here's a basic example of how it looks to use the threading module::

    import threading

    def task():
        # Do something.
        pass

    threads = []
    for _ in range(5):
        t = threading.Thread(target=task)
        t.start()
        threads.append(t)

    # Wait for all the threads to finish
    for t in threads:
        t.join()

Note that there are some limitations to the parallelism Python
can provide.  See :pep:`630`.

.. currentmodule:: None

Isolated Threads (CSP/Actor Model)
----------------------------------

The future stdlib :mod:`!interpreters` module supports isolated execution:

::

    import interpreters
    import threading

    script = """if True:
        # Do something.
        pass
        """

    def task():
        interp = interpreters.create()
        interp.exec(script)

    threads = []
    for _ in range(5):
        t = threading.Thread(target=task)
        t.start()
        threads.append(t)

    # Wait for all the subinterpreters to finish
    for t in threads:
        t.join()

Multi-processing
----------------

.. currentmodule:: multiprocessing

You can use the stdlib :mod:`multiprocessing` module:

::

    import multiprocessing

    def task()
        # Do something.
        pass

    procs = []
    for _ in range(5):
        p = multiprocessing.Process(target=task)
        p.start()
        procs.append(p)

    # Wait for all the subprocesses to finish
    for p in procs:
        p.join()

.. currentmodule:: None

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

.. _concurrency-models-comparison:

Comparison
----------

.. list-table::
   :header-rows: 1
   :class: borderless
   :align: left

   * - model
     - pros
     - cons
   * - threads
     - ...
     - ...
   * - multiple interpreters
     - ...
     - ...
   * - multiprocessing
     - ...
     - ...
   * - distributed
     - ...
     - ...
   * - async/await
     - ...
     - ...

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
using :meth:`executor.submit() <Executor.submit>`.
It returns a :class:`Future <Future>` object which tracks completion
and provides the result.  :class:`Future <!Future>` objects have a few
other tricks, like cancelation and completion callbacks, which we won't
cover here.  Likewise we won't cover the various uses of timeouts.

Here's an example of using :meth:`executor.submit() <!Executor.submit>`
and :meth:`Future.result() <Future.result>`:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-basic
   :start-after: [start-cf-basic]
   :end-before: [end-cf-basic]
   :dedent:
   :linenos:

You can use :meth:`executor.map() <Executor.map>` to call a function
multiple times and yield each result:

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

You can wait for an existing set of :class:`futures <!Future>`
using :func:`wait`
(and :func:`as_completed` and :meth:`executor.map() <Executor.map>`):

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-wait
   :start-after: [start-cf-wait]
   :end-before: [end-cf-wait]
   :dedent:
   :linenos:

You can use :func:`as_completed` to handle each :class:`future <!Future>`
as it completes:

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-cf-as-completed
   :start-after: [start-cf-as-completed]
   :end-before: [end-cf-as-completed]
   :dedent:
   :linenos:

In each case handling errors on a per-:class:`future <!Future>` basis
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

.. raw:: html

   <style>
       table.workload-example th
       {
           vertical-align: top;
       }

       table.workload-example td
       {
           vertical-align: top;
       }
   </style>

.. list-table::
   :header-rows: 1
   :class: borderless workload-example
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
