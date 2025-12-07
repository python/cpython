.. _profiling-module:

***************************************
:mod:`profiling` --- Python Profilers
***************************************

.. module:: profiling
   :synopsis: Python profiling tools for performance analysis.

**Source code:** :source:`Lib/profiling/`

--------------

.. versionadded:: 3.15

.. index::
   single: statistical profiling
   single: profiling, statistical
   single: deterministic profiling
   single: profiling, deterministic


Introduction to Profiling
=========================

A :dfn:`profile` is a set of statistics that describes how often and for how
long various parts of a program execute. These statistics help identify
performance bottlenecks and guide optimization efforts. Python provides two
fundamentally different approaches to collecting this information: statistical
sampling and deterministic tracing.

The :mod:`profiling` package organizes Python's built-in profiling tools under
a single namespace. It contains two submodules, each implementing a different
profiling methodology:

:mod:`profiling.sampling`
   A statistical profiler that periodically samples the call stack of running
   processes. It can attach to any Python process without requiring code
   modification, making it ideal for production debugging. The sampling
   approach introduces virtually no overhead to the profiled program.

:mod:`profiling.tracing`
   A deterministic profiler that traces every function call, return, and
   exception event. It provides exact call counts and timing information,
   making it suitable for development and testing where precision matters
   more than minimal overhead.

.. note::

   The profiler modules are designed to provide an execution profile for a
   given program, not for benchmarking purposes. For benchmarking, use the
   :mod:`timeit` module, which provides reasonably accurate timing
   measurements. This distinction is particularly important when comparing
   Python code against C code: deterministic profilers introduce overhead for
   Python code but not for C-level functions, which can skew comparisons.


.. _choosing-a-profiler:

Choosing a Profiler
===================

The choice between statistical sampling and deterministic tracing depends on
your specific use case. Each approach offers distinct advantages that make it
better suited to certain scenarios.

Statistical profiling excels when you need to understand performance
characteristics without affecting the program's behavior. Because the sampling
profiler reads process memory externally rather than instrumenting code, it
introduces virtually no overhead. This makes it the right choice for profiling
production systems, investigating intermittent performance issues, and
analyzing programs where timing accuracy is critical.

Deterministic profiling provides complete visibility into program execution.
Every function call is recorded with precise timing, giving you exact call
counts and the ability to trace the full call graph. This level of detail is
valuable during development when you need to understand exactly how code
executes, verify that optimizations reduce call counts, or identify unexpected
function calls.

The following table summarizes the key differences:

+--------------------+------------------------------+------------------------------+
| Feature            | Statistical Sampling         | Deterministic                |
|                    | (:mod:`profiling.sampling`)  | (:mod:`profiling.tracing`)   |
+====================+==============================+==============================+
| **Target**         | Running process              | Code you run                 |
+--------------------+------------------------------+------------------------------+
| **Overhead**       | Virtually none               | Moderate                     |
+--------------------+------------------------------+------------------------------+
| **Accuracy**       | Statistical estimate         | Exact call counts            |
+--------------------+------------------------------+------------------------------+
| **Setup**          | Attach to any PID            | Instrument code              |
+--------------------+------------------------------+------------------------------+
| **Use Case**       | Production debugging         | Development/testing          |
+--------------------+------------------------------+------------------------------+
| **Implementation** | C extension                  | C extension                  |
+--------------------+------------------------------+------------------------------+


When to Use Statistical Sampling
--------------------------------

The statistical profiler (:mod:`profiling.sampling`) is recommended when:

You need to profile a production system where any performance impact is
unacceptable. The sampling profiler runs in a separate process and reads the
target process memory without interrupting its execution.

You want to profile an already-running process without restarting it or
modifying its code. Simply provide the process ID and the profiler will attach
and begin collecting data.

You are investigating performance issues that might be affected by profiler
overhead. Since statistical profiling does not instrument the code, it captures
the program's natural behavior.

Your application uses multiple threads and you want to understand how work is
distributed across them. The sampling profiler can collect stack traces from
all threads simultaneously.


When to Use Deterministic Tracing
---------------------------------

The deterministic profiler (:mod:`profiling.tracing`) is recommended when:

You need exact call counts for every function. Statistical profiling provides
estimates based on sampling frequency, which may miss or undercount
short-lived function calls.

You want to trace the complete call graph and understand caller-callee
relationships. Deterministic profiling records every call, enabling detailed
analysis of how functions interact.

You are developing or testing code and can tolerate moderate overhead in
exchange for precise measurements. The overhead is acceptable for most
development workflows.

You need to measure time spent in specific functions accurately, including
functions that execute quickly. Deterministic profiling captures every
invocation rather than relying on statistical sampling.


Quick Start
===========

This section provides the minimal steps needed to start profiling. For complete
documentation, see the dedicated pages for each profiler.


Statistical Profiling
---------------------

To profile a running Python process, use the :mod:`profiling.sampling` module
with the process ID::

   python -m profiling.sampling 1234

This attaches to process 1234, samples its call stack for the default duration,
and prints a summary of where time was spent. No changes to the target process
are required.

For custom settings, specify the sampling interval (in microseconds) and
duration (in seconds)::

   python -m profiling.sampling -i 50 -d 30 1234


Deterministic Profiling
-----------------------

To profile a piece of code, use the :mod:`profiling.tracing` module::

   import profiling.tracing
   profiling.tracing.run('my_function()')

This executes the given code under the profiler and prints a summary showing
function call counts and timing. For profiling a script from the command line::

   python -m profiling.tracing myscript.py


.. _profile-output:

Understanding Profile Output
============================

Both profilers produce output showing function-level statistics. The
deterministic profiler output looks like this::

         214 function calls (207 primitive calls) in 0.002 seconds

   Ordered by: cumulative time

   ncalls  tottime  percall  cumtime  percall filename:lineno(function)
        1    0.000    0.000    0.002    0.002 {built-in method builtins.exec}
        1    0.000    0.000    0.001    0.001 <string>:1(<module>)
        1    0.000    0.000    0.001    0.001 __init__.py:250(compile)

The first line indicates that 214 calls were monitored, of which 207 were
:dfn:`primitive` (not induced via recursion). The column headings are:

ncalls
   The number of calls to this function. When two numbers appear separated by
   a slash (for example, ``3/1``), the function recursed: the first number is
   the total calls and the second is the primitive (non-recursive) calls.

tottime
   The total time spent in this function alone, excluding time spent in
   functions it called.

percall
   The quotient of ``tottime`` divided by ``ncalls``.

cumtime
   The cumulative time spent in this function and all functions it called.
   This figure is accurate even for recursive functions.

percall
   The quotient of ``cumtime`` divided by primitive calls.

filename:lineno(function)
   The location and name of the function.


Legacy Compatibility
====================

For backward compatibility, the ``cProfile`` module remains available as an
alias to :mod:`profiling.tracing`. Existing code using ``import cProfile`` will
continue to work without modification in all future Python versions.

.. deprecated:: 3.15

   The pure Python :mod:`profile` module is deprecated and will be removed in
   Python 3.17. Use :mod:`profiling.tracing` (or its alias ``cProfile``)
   instead. See :mod:`profile` for migration guidance.


.. seealso::

   :mod:`profiling.tracing`
      Deterministic tracing profiler for development and testing.

   :mod:`profiling.sampling`
      Statistical sampling profiler for production debugging.

   :mod:`pstats`
      Statistics analysis and formatting for profile data.

   :mod:`timeit`
      Module for measuring execution time of small code snippets.


.. rubric:: Submodules

.. toctree::
   :maxdepth: 1

   profiling-tracing.rst
   profiling-sampling.rst
