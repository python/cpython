.. _concurrency-howto:

*****************
Concurrency HOWTO
*****************

Python is a language the accommodates a variety of programming styles,
from procedural to object-oriented to functional.  The same applies
to concurrency.  Here we'll look at how different concurrency models
look in Python, with an emphasis on practical workload-oriented examples.

The following Python concurrency models are covered:

* threads (:mod:`threading` and :mod:`concurrent.futures`)
* multi-processing (:mod:`multiprocessing` and :mod:`concurrent.futures`)
* async/await
* CSP/actor model (:mod:`!interpreters`)
* distributed (e.g. SMP)

Each of these will be explained, with some simple examples.  The later
workload-oriented examples will be implemented using each,
for comparison, when possible.

.. note::

   You should always make sure concurrency is the right tool for the job
   before you reach for it when solving your problem.  There are many
   cases where concurrency simply isn't applicable or will only
   complicate the solution.  In-depth discussion of this point
   is outside the scope of this document.


All About Concurrency
=====================

What is concurrency?
--------------------

At its most fundamental, concurrency means doing multiple things at once,
from a strictly *logical* viewpoint.

When a computer program runs, it executes a sequence of code in order.
Sometimes it makes sense to break up that sequence into smaller pieces,
where some of them can run independently of others.

For example, consider the following program with three pieces::

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
same time.  "At the same time" means concurrency.

Concurrency often involves some degree of synchronization between
the tasks.  At the most basic conceptual level: one task may wait
for another to finish.

In addition to code running at the same time, concurrency typically
also involves some amount of resources shared between the concurrent
tasks.  That may include memory, files, and sockets.

What is parallelism?
--------------------

Concurrency may happen in one of two ways.  The concurrent tasks may
share a single CPU, each running a little bit at a time, with the
operating system (or language runtime) taking care of the switching.
The other way is where each task runs on its own CPU, meaning they
are physically running at the same time, not just logically.

That second way is parallelism.

What problems can concurrency help solve?
-----------------------------------------

Primarily, concurrency can be helpful by making your program faster
and more responsive (less latency), when possible.  In other words,
you get better computational throughput.  That happens by enabling
the following:

* run on multiple CPU cores (parallelism)
* keep blocking resources from blocking the whole program
* make sure critical tasks have priority
* process results as they come, instead of waiting for them all

Other possible benefits:

* asynchronous events can be handled more cleanly
* better efficiency using hardware resources
* improved scalability

What are the downsides?
-----------------------

The main challenge when using concurrency is the extra complexity.

.. XXX

* races on shared resources
* error handling
* ...

The operating system, along with some libraries and frameworks,
can help mitigate the extra complexity.  So can the concurrency
model you use, which we'll talk about a little later..

Workloads
---------

We've looked at what you can do with concurrency from a high level.
Now let's look at some concrete examples.


...


Concurrency Models
------------------

The concept of concurrency has been a part of the study and practice
of computer software since the 1950s and 1960s, with various innovations
since then.  The application of the different theoretical concurrency
models can be categorized as follows:

* free threads - using multiple threads in the same process,
  with no isolation between them
* isolated threads - threads with strict isolation between them
  (e.g. CSP and actor model)
* multiprocessing - using multiple isolated processes
* distributed - multiprocessing across multiple computers
* async/await - using coroutines (AKA cooperative multitasking)

(There are certainly others, but these are the focus here.)

There are tradeoffs to each.  Free-threading probably has the most
notoriety and the most examples, but is also the most likely to cause
you pain.
Isolated threads have few of the downsides but are less familiar.
Multiprocessing and distributed are less efficient at smaller scales.
Async can be straightforward, but may cascade throughout a code base,
doesn't necessarily give you parallelism.


Python Concurrency Models
=========================

We've looked at concurrency and concurrency models generally.
Now let's see what they look like in Python.

Free-threading
--------------

The stdlib :mod:`threading` module ...

...

Multi-processing
----------------

...

Async/Await
-----------

...

Isolated Threads (CSP/Actor Model)
----------------------------------

...

Distributed
-----------

...


Python Concurrency Workloads
============================

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
