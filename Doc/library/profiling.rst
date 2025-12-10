.. highlight:: shell-session

.. _profiling-module:

***************************************
:mod:`profiling` --- Python profilers
***************************************

.. module:: profiling
   :synopsis: Python profiling tools for performance analysis.

.. versionadded:: 3.15

**Source code:** :source:`Lib/profiling/`

--------------

.. index::
   single: statistical profiling
   single: profiling, statistical
   single: deterministic profiling
   single: profiling, deterministic


Introduction to profiling
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
   A statistical profiler that periodically samples the call stack. Run scripts
   directly or attach to running processes by PID. Provides multiple output
   formats (flame graphs, heatmaps, Firefox Profiler), GIL analysis, GC tracking,
   and multiple profiling modes (wall-clock, CPU, GIL) with virtually no overhead.

:mod:`profiling.tracing`
   A deterministic profiler that traces every function call, return, and
   exception event. Provides exact call counts and precise timing information,
   capturing every invocation including very fast functions.

.. note::

   The profiler modules are designed to provide an execution profile for a
   given program, not for benchmarking purposes. For benchmarking, use the
   :mod:`timeit` module, which provides reasonably accurate timing
   measurements. This distinction is particularly important when comparing
   Python code against C code: deterministic profilers introduce overhead for
   Python code but not for C-level functions, which can skew comparisons.


.. _choosing-a-profiler:

Choosing a profiler
===================

For most performance analysis, use the statistical profiler
(:mod:`profiling.sampling`). It has minimal overhead, works for both development
and production, and provides rich visualization options including flamegraphs,
heatmaps, GIL analysis, and more.

Use the deterministic profiler (:mod:`profiling.tracing`) when you need **exact
call counts** and cannot afford to miss any function calls. Since it instruments
every function call and return, it will capture even very fast functions that
complete between sampling intervals. The tradeoff is higher overhead.

The following table summarizes the key differences:

+--------------------+------------------------------+------------------------------+
| Feature            | Statistical sampling         | Deterministic                |
|                    | (:mod:`profiling.sampling`)  | (:mod:`profiling.tracing`)   |
+====================+==============================+==============================+
| **Overhead**       | Virtually none               | Moderate                     |
+--------------------+------------------------------+------------------------------+
| **Accuracy**       | Statistical estimate         | Exact call counts            |
+--------------------+------------------------------+------------------------------+
| **Output formats** | pstats, flamegraph, heatmap, | pstats                       |
|                    | gecko, collapsed             |                              |
+--------------------+------------------------------+------------------------------+
| **Profiling modes**| Wall-clock, CPU, GIL         | Wall-clock                   |
+--------------------+------------------------------+------------------------------+
| **Special frames** | GC, native (C extensions)    | N/A                          |
+--------------------+------------------------------+------------------------------+
| **Attach to PID**  | Yes                          | No                           |
+--------------------+------------------------------+------------------------------+


When to use statistical sampling
--------------------------------

The statistical profiler (:mod:`profiling.sampling`) is recommended for most
performance analysis tasks. Use it the same way you would use
:mod:`profiling.tracing`::

   python -m profiling.sampling run script.py

One of the main strengths of the sampling profiler is its variety of output
formats. Beyond traditional pstats tables, it can generate interactive
flamegraphs that visualize call hierarchies, line-level source heatmaps that
show exactly where time is spent in your code, and Firefox Profiler output for
timeline-based analysis.

The profiler also provides insight into Python interpreter behavior that
deterministic profiling cannot capture. Use ``--mode gil`` to identify GIL
contention in multi-threaded code, ``--mode cpu`` to measure actual CPU time
excluding I/O waits, or inspect ``<GC>`` frames to understand garbage collection
overhead. The ``--native`` option reveals time spent in C extensions, helping
distinguish Python overhead from library performance.

For multi-threaded applications, the ``-a`` option samples all threads
simultaneously, showing how work is distributed. And for production debugging,
the ``attach`` command connects to any running Python process by PID without
requiring a restart or code changes.


When to use deterministic tracing
---------------------------------

The deterministic profiler (:mod:`profiling.tracing`) instruments every function
call and return. This approach has higher overhead than sampling, but guarantees
complete coverage of program execution.

The primary reason to choose deterministic tracing is when you need exact call
counts. Statistical profiling estimates frequency based on sampling, which may
undercount short-lived functions that complete between samples. If you need to
verify that an optimization actually reduced the number of function calls, or
if you want to trace the complete call graph to understand caller-callee
relationships, deterministic tracing is the right choice.

Deterministic tracing also excels at capturing functions that execute in
microseconds. Such functions may not appear frequently enough in statistical
samples, but deterministic tracing records every invocation regardless of
duration.


Quick start
===========

This section provides the minimal steps needed to start profiling. For complete
documentation, see the dedicated pages for each profiler.


Statistical profiling
---------------------

To profile a script, use the :mod:`profiling.sampling` module with the ``run``
command::

   python -m profiling.sampling run script.py
   python -m profiling.sampling run -m mypackage.module

This runs the script under the profiler and prints a summary of where time was
spent. For an interactive flamegraph::

   python -m profiling.sampling run --flamegraph script.py

To profile an already-running process, use the ``attach`` command with the
process ID::

   python -m profiling.sampling attach 1234

For custom settings, specify the sampling interval (in microseconds) and
duration (in seconds)::

   python -m profiling.sampling run -i 50 -d 30 script.py


Deterministic profiling
-----------------------

To profile a script from the command line::

   python -m profiling.tracing script.py

To profile a piece of code programmatically:

.. code-block:: python

   import profiling.tracing
   profiling.tracing.run('my_function()')

This executes the given code under the profiler and prints a summary showing
exact function call counts and timing.


.. _profile-output:

Understanding profile output
============================

Both profilers collect function-level statistics, though they present them in
different formats. The sampling profiler offers multiple visualizations
(flamegraphs, heatmaps, Firefox Profiler, pstats tables), while the
deterministic profiler produces pstats-compatible output. Regardless of format,
the underlying concepts are the same.

Key profiling concepts:

**Direct time** (also called *self time* or *tottime*)
   Time spent executing code in the function itself, excluding time spent in
   functions it called. High direct time indicates the function contains
   expensive operations.

**Cumulative time** (also called *total time* or *cumtime*)
   Time spent in the function and all functions it called. This measures the
   total cost of calling a function, including its entire call subtree.

**Call count** (also called *ncalls* or *samples*)
   How many times the function was called (deterministic) or sampled
   (statistical). In deterministic profiling, this is exact. In statistical
   profiling, it represents the number of times the function appeared in a
   stack sample.

**Primitive calls**
   Calls that are not induced by recursion. When a function recurses, the total
   call count includes recursive invocations, but primitive calls counts only
   the initial entry. Displayed as ``total/primitive`` (for example, ``3/1``
   means three total calls, one primitive).

**Caller/Callee relationships**
   Which functions called a given function (callers) and which functions it
   called (callees). Flamegraphs visualize this as nested rectangles; pstats
   can display it via the :meth:`~pstats.Stats.print_callers` and
   :meth:`~pstats.Stats.print_callees` methods.


Legacy compatibility
====================

For backward compatibility, the ``cProfile`` module remains available as an
alias to :mod:`profiling.tracing`. Existing code using ``import cProfile`` will
continue to work without modification in all future Python versions.

.. deprecated:: 3.15

   The pure Python :mod:`profile` module is deprecated and will be removed in
   Python 3.17. Use :mod:`profiling.tracing` (or its alias ``cProfile``)
   instead. See :mod:`profile` for migration guidance.


.. seealso::

   :mod:`profiling.sampling`
      Statistical sampling profiler with flamegraphs, heatmaps, and GIL analysis.
      Recommended for most users.

   :mod:`profiling.tracing`
      Deterministic tracing profiler for exact call counts.

   :mod:`pstats`
      Statistics analysis and formatting for profile data.

   :mod:`timeit`
      Module for measuring execution time of small code snippets.


.. rubric:: Submodules

.. toctree::
   :maxdepth: 1

   profiling.tracing.rst
   profiling.sampling.rst
