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
* async/await (:mod:`asycio`)

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
notoriety and the most examples, but is also the most likely to cause
you pain.
Isolated threads have few of the downsides but are less familiar.
Multiprocessing and distributed are less efficient at smaller scales.
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

What are the downsides?
-----------------------

The main challenge when using concurrency is the (potential) extra
complexity.  This complexity comes from the effect of multiple logical
threads running at the same time and interacting with each other.
In practice, this falls into two categories: data races and tracing
relative execution.  Both are a form of "spooky action at a distance".

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

Free-threading
--------------

The stdlib :mod:`threading` module ...

...

Isolated Threads (CSP/Actor Model)
----------------------------------

The future stdlib :mod:`!interpreters` module ...

...

Multi-processing
----------------

The stdlib :mod:`multiprocessing` module ...

...

Distributed
-----------

The popular :mod:`!dask` module ...

...

Async/Await
-----------

The stdlib :mod:`asyncio` module ...

...

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


Python Concurrency Workloads
============================

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

Using :mod:`concurrent.futures`:

.. raw:: html

   <details>
   <summary>(expand)</summary>

.. literalinclude:: ../includes/concurrency.py
   :name: concurrency-workload-1-concurrent-futures-thread
   :start-after: [start-w1-concurrent-futures-thread]
   :end-before: [end-w1-concurrent-futures-thread]
   :dedent:
   :linenos:

.. raw:: html

   </details>

Workload 2
----------

...
