.. _profile:

****************************************
:mod:`!profile` --- Pure Python profiler
****************************************

.. module:: profile
   :synopsis: Pure Python profiler (deprecated).
   :deprecated:

**Source code:** :source:`Lib/profile.py`

--------------

.. deprecated-removed:: 3.15 3.17

The :mod:`profile` module is deprecated and will be removed in Python 3.17.
Use :mod:`profiling.tracing` instead.

The :mod:`profile` module provides a pure Python implementation of a
deterministic profiler. While useful for understanding profiler internals or
extending profiler behavior through subclassing, its pure Python implementation
introduces significant overhead compared to the C-based :mod:`profiling.tracing`
module.

For most profiling tasks, use:

- :mod:`profiling.sampling` for production debugging with zero overhead
- :mod:`profiling.tracing` for development and testing


Migration
=========

Migrating from :mod:`profile` to :mod:`profiling.tracing` is straightforward.
The APIs are compatible::

   # Old (deprecated)
   import profile
   profile.run('my_function()')

   # New (recommended)
   import profiling.tracing
   profiling.tracing.run('my_function()')

For most code, replacing ``import profile`` with ``import profiling.tracing``
(and using ``profiling.tracing`` instead of ``profile`` throughout) provides
a straightforward migration path.

.. note::

   The ``cProfile`` module remains available as a backward-compatible alias
   to :mod:`profiling.tracing`. Existing code using ``import cProfile`` will
   continue to work without modification.


:mod:`!profile` and :mod:`!profiling.tracing` module reference
==============================================================

Both the :mod:`profile` and :mod:`profiling.tracing` modules provide the
following functions:

.. function:: run(command, filename=None, sort=-1)

   This function takes a single argument that can be passed to the :func:`exec`
   function, and an optional file name.  In all cases this routine executes::

      exec(command, __main__.__dict__, __main__.__dict__)

   and gathers profiling statistics from the execution. If no file name is
   present, then this function automatically creates a :class:`~pstats.Stats`
   instance and prints a simple profiling report. If the sort value is specified,
   it is passed to this :class:`~pstats.Stats` instance to control how the
   results are sorted.

.. function:: runctx(command, globals, locals, filename=None, sort=-1)

   This function is similar to :func:`run`, with added arguments to supply the
   globals and locals mappings for the *command* string. This routine
   executes::

      exec(command, globals, locals)

   and gathers profiling statistics as in the :func:`run` function above.

.. class:: Profile(timer=None, timeunit=0.0, subcalls=True, builtins=True)

   This class is normally only used if more precise control over profiling is
   needed than what the :func:`profiling.tracing.run` function provides.

   A custom timer can be supplied for measuring how long code takes to run via
   the *timer* argument. This must be a function that returns a single number
   representing the current time. If the number is an integer, the *timeunit*
   specifies a multiplier that specifies the duration of each unit of time. For
   example, if the timer returns times measured in thousands of seconds, the
   time unit would be ``.001``.

   Directly using the :class:`Profile` class allows formatting profile results
   without writing the profile data to a file::

      import profiling.tracing
      import pstats
      import io
      from pstats import SortKey

      pr = profiling.tracing.Profile()
      pr.enable()
      # ... do something ...
      pr.disable()
      s = io.StringIO()
      sortby = SortKey.CUMULATIVE
      ps = pstats.Stats(pr, stream=s).sort_stats(sortby)
      ps.print_stats()
      print(s.getvalue())

   The :class:`Profile` class can also be used as a context manager (supported
   only in :mod:`profiling.tracing`, not in the deprecated :mod:`profile`
   module; see :ref:`typecontextmanager`)::

      import profiling.tracing

      with profiling.tracing.Profile() as pr:
          # ... do something ...

          pr.print_stats()

   .. versionchanged:: 3.8
      Added context manager support.

   .. method:: enable()

      Start collecting profiling data. Only in :mod:`profiling.tracing`.

   .. method:: disable()

      Stop collecting profiling data. Only in :mod:`profiling.tracing`.

   .. method:: create_stats()

      Stop collecting profiling data and record the results internally
      as the current profile.

   .. method:: print_stats(sort=-1)

      Create a :class:`~pstats.Stats` object based on the current
      profile and print the results to stdout.

      The *sort* parameter specifies the sorting order of the displayed
      statistics. It accepts a single key or a tuple of keys to enable
      multi-level sorting, as in :meth:`pstats.Stats.sort_stats`.

      .. versionadded:: 3.13
         :meth:`~Profile.print_stats` now accepts a tuple of keys.

   .. method:: dump_stats(filename)

      Write the results of the current profile to *filename*.

   .. method:: run(cmd)

      Profile the cmd via :func:`exec`.

   .. method:: runctx(cmd, globals, locals)

      Profile the cmd via :func:`exec` with the specified global and
      local environment.

   .. method:: runcall(func, /, *args, **kwargs)

      Profile ``func(*args, **kwargs)``

Note that profiling will only work if the called command/function actually
returns.  If the interpreter is terminated (e.g. via a :func:`sys.exit` call
during the called command/function execution) no profiling results will be
printed.


Differences from :mod:`!profiling.tracing`
==========================================

The :mod:`profile` module differs from :mod:`profiling.tracing` in several
ways:

**Higher overhead.** The pure Python implementation is significantly slower
than the C implementation, making it unsuitable for profiling long-running
programs or performance-sensitive code.

**Calibration support.** The :mod:`profile` module supports calibration to
compensate for profiling overhead. This is not needed in :mod:`profiling.tracing`
because the C implementation has negligible overhead.

**Custom timers.** Both modules support custom timers, but :mod:`profile`
accepts timer functions that return tuples (like :func:`os.times`), while
:mod:`profiling.tracing` requires a function returning a single number.

**Subclassing.** The pure Python implementation is easier to subclass and
extend for custom profiling behavior.


.. _deterministic-profiling:

What is deterministic profiling?
================================

:dfn:`Deterministic profiling` is meant to reflect the fact that all *function
call*, *function return*, and *exception* events are monitored, and precise
timings are made for the intervals between these events (during which time the
user's code is executing).  In contrast, :dfn:`statistical profiling` (which is
provided by the :mod:`profiling.sampling` module) periodically samples the
effective instruction pointer, and deduces where time is being spent.  The
latter technique traditionally involves less overhead (as the code does not
need to be instrumented), but provides only relative indications of where time
is being spent.

In Python, since there is an interpreter active during execution, the presence
of instrumented code is not required in order to do deterministic profiling.
Python automatically provides a :dfn:`hook` (optional callback) for each event.
In addition, the interpreted nature of Python tends to add so much overhead to
execution, that deterministic profiling tends to only add small processing
overhead in typical applications.  The result is that deterministic profiling is
not that expensive, yet provides extensive run time statistics about the
execution of a Python program.

Call count statistics can be used to identify bugs in code (surprising counts),
and to identify possible inline-expansion points (high call counts).  Internal
time statistics can be used to identify "hot loops" that should be carefully
optimized.  Cumulative time statistics should be used to identify high level
errors in the selection of algorithms.  Note that the unusual handling of
cumulative times in this profiler allows statistics for recursive
implementations of algorithms to be directly compared to iterative
implementations.


.. _profile-limitations:

Limitations
===========

One limitation has to do with accuracy of timing information. There is a
fundamental problem with deterministic profilers involving accuracy.  The most
obvious restriction is that the underlying "clock" is only ticking at a rate
(typically) of about .001 seconds.  Hence no measurements will be more accurate
than the underlying clock.  If enough measurements are taken, then the "error"
will tend to average out. Unfortunately, removing this first error induces a
second source of error.

The second problem is that it "takes a while" from when an event is dispatched
until the profiler's call to get the time actually *gets* the state of the
clock.  Similarly, there is a certain lag when exiting the profiler event
handler from the time that the clock's value was obtained (and then squirreled
away), until the user's code is once again executing.  As a result, functions
that are called many times, or call many functions, will typically accumulate
this error. The error that accumulates in this fashion is typically less than
the accuracy of the clock (less than one clock tick), but it *can* accumulate
and become very significant.

The problem is more important with the deprecated :mod:`profile` module than
with the lower-overhead :mod:`profiling.tracing`.  For this reason,
:mod:`profile` provides a means of calibrating itself for a given platform so
that this error can be probabilistically (on the average) removed. After the
profiler is calibrated, it will be more accurate (in a least square sense), but
it will sometimes produce negative numbers (when call counts are exceptionally
low, and the gods of probability work against you :-). )  Do *not* be alarmed
by negative numbers in the profile.  They should *only* appear if you have
calibrated your profiler, and the results are actually better than without
calibration.


.. _profile-calibration:

Calibration
===========

The profiler of the :mod:`profile` module subtracts a constant from each event
handling time to compensate for the overhead of calling the time function, and
socking away the results.  By default, the constant is 0. The following
procedure can be used to obtain a better constant for a given platform (see
:ref:`profile-limitations`). ::

   import profile
   pr = profile.Profile()
   for i in range(5):
       print(pr.calibrate(10000))

The method executes the number of Python calls given by the argument, directly
and again under the profiler, measuring the time for both. It then computes the
hidden overhead per profiler event, and returns that as a float.  For example,
on a 1.8Ghz Intel Core i5 running macOS, and using Python's time.process_time() as
the timer, the magical number is about 4.04e-6.

The object of this exercise is to get a fairly consistent result. If your
computer is *very* fast, or your timer function has poor resolution, you might
have to pass 100000, or even 1000000, to get consistent results.

When you have a consistent answer, there are three ways you can use it::

   import profile

   # 1. Apply computed bias to all Profile instances created hereafter.
   profile.Profile.bias = your_computed_bias

   # 2. Apply computed bias to a specific Profile instance.
   pr = profile.Profile()
   pr.bias = your_computed_bias

   # 3. Specify computed bias in instance constructor.
   pr = profile.Profile(bias=your_computed_bias)

If you have a choice, you are better off choosing a smaller constant, and then
your results will "less often" show up as negative in profile statistics.


.. _profile-timers:

Using a custom timer
====================

If you want to change how current time is determined (for example, to force use
of wall-clock time or elapsed process time), pass the timing function you want
to the :class:`Profile` class constructor::

    pr = profile.Profile(your_time_func)

The resulting profiler will then call ``your_time_func``. Depending on whether
you are using :class:`profile.Profile` or :class:`profiling.tracing.Profile`,
``your_time_func``'s return value will be interpreted differently:

:class:`profile.Profile`
   ``your_time_func`` should return a single number, or a list of numbers whose
   sum is the current time (like what :func:`os.times` returns).  If the
   function returns a single time number, or the list of returned numbers has
   length 2, then you will get an especially fast version of the dispatch
   routine.

   Be warned that you should calibrate the profiler class for the timer function
   that you choose (see :ref:`profile-calibration`).  For most machines, a timer
   that returns a lone integer value will provide the best results in terms of
   low overhead during profiling.  (:func:`os.times` is *pretty* bad, as it
   returns a tuple of floating-point values).  If you want to substitute a
   better timer in the cleanest fashion, derive a class and hardwire a
   replacement dispatch method that best handles your timer call, along with the
   appropriate calibration constant.

:class:`profiling.tracing.Profile`
   ``your_time_func`` should return a single number.  If it returns integers,
   you can also invoke the class constructor with a second argument specifying
   the real duration of one unit of time.  For example, if
   ``your_integer_time_func`` returns times measured in thousands of seconds,
   you would construct the :class:`Profile` instance as follows::

      pr = profiling.tracing.Profile(your_integer_time_func, 0.001)

   As the :class:`profiling.tracing.Profile` class cannot be calibrated, custom
   timer functions should be used with care and should be as fast as possible.
   For the best results with a custom timer, it might be necessary to hard-code
   it in the C source of the internal :mod:`!_lsprof` module.

Python 3.3 adds several new functions in :mod:`time` that can be used to make
precise measurements of process or wall-clock time. For example, see
:func:`time.perf_counter`.


.. seealso::

   :mod:`profiling`
      Overview of Python profiling tools.

   :mod:`profiling.tracing`
      Recommended replacement for this module.

   :mod:`pstats`
      Statistical analysis and formatting for profile data.
