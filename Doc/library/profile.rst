
.. _profile:

********************
The Python Profilers
********************

.. sectionauthor:: James Roskind


.. index:: single: InfoSeek Corporation

Copyright © 1994, by InfoSeek Corporation, all rights reserved.

Written by James Roskind. [#]_

Permission to use, copy, modify, and distribute this Python software and its
associated documentation for any purpose (subject to the restriction in the
following sentence) without fee is hereby granted, provided that the above
copyright notice appears in all copies, and that both that copyright notice and
this permission notice appear in supporting documentation, and that the name of
InfoSeek not be used in advertising or publicity pertaining to distribution of
the software without specific, written prior permission.  This permission is
explicitly restricted to the copying and modification of the software to remain
in Python, compiled Python, or other languages (such as C) wherein the modified
or derived code is exclusively imported into a Python module.

INFOSEEK CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT
SHALL INFOSEEK CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

The profiler was written after only programming in Python for 3 weeks. As a
result, it is probably clumsy code, but I don't know for sure yet 'cause I'm a
beginner :-).  I did work hard to make the code run fast, so that profiling
would be a reasonable thing to do.  I tried not to repeat code fragments, but
I'm sure I did some stuff in really awkward ways at times.  Please send
suggestions for improvements to: jar@netscape.com.  I won't promise *any*
support.  ...but I'd appreciate the feedback.


.. _profiler-introduction:

Introduction to the profilers
=============================

.. index::
   single: deterministic profiling
   single: profiling, deterministic

A :dfn:`profiler` is a program that describes the run time performance of a
program, providing a variety of statistics.  This documentation describes the
profiler functionality provided in the modules :mod:`profile` and :mod:`pstats`.
This profiler provides :dfn:`deterministic profiling` of any Python programs.
It also provides a series of report generation tools to allow users to rapidly
examine the results of a profile operation.

The Python standard library provides three different profilers:

#. :mod:`profile`, a pure Python module, described in the sequel. Copyright ©
   1994, by InfoSeek Corporation.

#. :mod:`cProfile`, a module written in C, with a reasonable overhead that makes
   it suitable for profiling long-running programs. Based on :mod:`lsprof`,
   contributed by Brett Rosen and Ted Czotter.

#. :mod:`hotshot`, a C module focusing on minimizing the overhead while
   profiling, at the expense of long data post-processing times.

The :mod:`profile` and :mod:`cProfile` modules export the same interface, so
they are mostly interchangeables; :mod:`cProfile` has a much lower overhead but
is not so far as well-tested and might not be available on all systems.
:mod:`cProfile` is really a compatibility layer on top of the internal
:mod:`_lsprof` module.  The :mod:`hotshot` module is reserved to specialized
usages.

.. % \section{How Is This Profiler Different From The Old Profiler?}
.. % \nodename{Profiler Changes}
.. % 
.. % (This section is of historical importance only; the old profiler
.. % discussed here was last seen in Python 1.1.)
.. % 
.. % The big changes from old profiling module are that you get more
.. % information, and you pay less CPU time.  It's not a trade-off, it's a
.. % trade-up.
.. % 
.. % To be specific:
.. % 
.. % \begin{description}
.. % 
.. % \item[Bugs removed:]
.. % Local stack frame is no longer molested, execution time is now charged
.. % to correct functions.
.. % 
.. % \item[Accuracy increased:]
.. % Profiler execution time is no longer charged to user's code,
.. % calibration for platform is supported, file reads are not done \emph{by}
.. % profiler \emph{during} profiling (and charged to user's code!).
.. % 
.. % \item[Speed increased:]
.. % Overhead CPU cost was reduced by more than a factor of two (perhaps a
.. % factor of five), lightweight profiler module is all that must be
.. % loaded, and the report generating module (\module{pstats}) is not needed
.. % during profiling.
.. % 
.. % \item[Recursive functions support:]
.. % Cumulative times in recursive functions are correctly calculated;
.. % recursive entries are counted.
.. % 
.. % \item[Large growth in report generating UI:]
.. % Distinct profiles runs can be added together forming a comprehensive
.. % report; functions that import statistics take arbitrary lists of
.. % files; sorting criteria is now based on keywords (instead of 4 integer
.. % options); reports shows what functions were profiled as well as what
.. % profile file was referenced; output format has been improved.
.. % 
.. % \end{description}


.. _profile-instant:

Instant User's Manual
=====================

This section is provided for users that "don't want to read the manual." It
provides a very brief overview, and allows a user to rapidly perform profiling
on an existing application.

To profile an application with a main entry point of :func:`foo`, you would add
the following to your module::

   import cProfile
   cProfile.run('foo()')

(Use :mod:`profile` instead of :mod:`cProfile` if the latter is not available on
your system.)

The above action would cause :func:`foo` to be run, and a series of informative
lines (the profile) to be printed.  The above approach is most useful when
working with the interpreter.  If you would like to save the results of a
profile into a file for later examination, you can supply a file name as the
second argument to the :func:`run` function::

   import cProfile
   cProfile.run('foo()', 'fooprof')

The file :file:`cProfile.py` can also be invoked as a script to profile another
script.  For example::

   python -m cProfile myscript.py

:file:`cProfile.py` accepts two optional arguments on the command line::

   cProfile.py [-o output_file] [-s sort_order]

:option:`-s` only applies to standard output (:option:`-o` is not supplied).
Look in the :class:`Stats` documentation for valid sort values.

When you wish to review the profile, you should use the methods in the
:mod:`pstats` module.  Typically you would load the statistics data as follows::

   import pstats
   p = pstats.Stats('fooprof')

The class :class:`Stats` (the above code just created an instance of this class)
has a variety of methods for manipulating and printing the data that was just
read into ``p``.  When you ran :func:`cProfile.run` above, what was printed was
the result of three method calls::

   p.strip_dirs().sort_stats(-1).print_stats()

The first method removed the extraneous path from all the module names. The
second method sorted all the entries according to the standard module/line/name
string that is printed. The third method printed out all the statistics.  You
might try the following sort calls:

.. % (this is to comply with the semantics of the old profiler).

::

   p.sort_stats('name')
   p.print_stats()

The first call will actually sort the list by function name, and the second call
will print out the statistics.  The following are some interesting calls to
experiment with::

   p.sort_stats('cumulative').print_stats(10)

This sorts the profile by cumulative time in a function, and then only prints
the ten most significant lines.  If you want to understand what algorithms are
taking time, the above line is what you would use.

If you were looking to see what functions were looping a lot, and taking a lot
of time, you would do::

   p.sort_stats('time').print_stats(10)

to sort according to time spent within each function, and then print the
statistics for the top ten functions.

You might also try::

   p.sort_stats('file').print_stats('__init__')

This will sort all the statistics by file name, and then print out statistics
for only the class init methods (since they are spelled with ``__init__`` in
them).  As one final example, you could try::

   p.sort_stats('time', 'cum').print_stats(.5, 'init')

This line sorts statistics with a primary key of time, and a secondary key of
cumulative time, and then prints out some of the statistics. To be specific, the
list is first culled down to 50% (re: ``.5``) of its original size, then only
lines containing ``init`` are maintained, and that sub-sub-list is printed.

If you wondered what functions called the above functions, you could now (``p``
is still sorted according to the last criteria) do::

   p.print_callers(.5, 'init')

and you would get a list of callers for each of the listed functions.

If you want more functionality, you're going to have to read the manual, or
guess what the following functions do::

   p.print_callees()
   p.add('fooprof')

Invoked as a script, the :mod:`pstats` module is a statistics browser for
reading and examining profile dumps.  It has a simple line-oriented interface
(implemented using :mod:`cmd`) and interactive help.


.. _deterministic-profiling:

What Is Deterministic Profiling?
================================

:dfn:`Deterministic profiling` is meant to reflect the fact that all *function
call*, *function return*, and *exception* events are monitored, and precise
timings are made for the intervals between these events (during which time the
user's code is executing).  In contrast, :dfn:`statistical profiling` (which is
not done by this module) randomly samples the effective instruction pointer, and
deduces where time is being spent.  The latter technique traditionally involves
less overhead (as the code does not need to be instrumented), but provides only
relative indications of where time is being spent.

In Python, since there is an interpreter active during execution, the presence
of instrumented code is not required to do deterministic profiling.  Python
automatically provides a :dfn:`hook` (optional callback) for each event.  In
addition, the interpreted nature of Python tends to add so much overhead to
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


Reference Manual -- :mod:`profile` and :mod:`cProfile`
======================================================

.. module:: cProfile
   :synopsis: Python profiler


The primary entry point for the profiler is the global function
:func:`profile.run` (resp. :func:`cProfile.run`). It is typically used to create
any profile information.  The reports are formatted and printed using methods of
the class :class:`pstats.Stats`.  The following is a description of all of these
standard entry points and functions.  For a more in-depth view of some of the
code, consider reading the later section on Profiler Extensions, which includes
discussion of how to derive "better" profilers from the classes presented, or
reading the source code for these modules.


.. function:: run(command[, filename])

   This function takes a single argument that can be passed to the :func:`exec`
   function, and an optional file name.  In all cases this routine attempts to
   :func:`exec` its first argument, and gather profiling statistics from the
   execution. If no file name is present, then this function automatically
   prints a simple profiling report, sorted by the standard name string
   (file/line/function-name) that is presented in each line.  The following is a
   typical output from such a call::

            2706 function calls (2004 primitive calls) in 4.504 CPU seconds

      Ordered by: standard name

      ncalls  tottime  percall  cumtime  percall filename:lineno(function)
           2    0.006    0.003    0.953    0.477 pobject.py:75(save_objects)
        43/3    0.533    0.012    0.749    0.250 pobject.py:99(evaluate)
       ...

   The first line indicates that 2706 calls were monitored.  Of those calls, 2004
   were :dfn:`primitive`.  We define :dfn:`primitive` to mean that the call was not
   induced via recursion. The next line: ``Ordered by: standard name``, indicates
   that the text string in the far right column was used to sort the output. The
   column headings include:

   ncalls 
      for the number of calls,

   tottime 
      for the total time spent in the given function (and excluding time made in calls
      to sub-functions),

   percall 
      is the quotient of ``tottime`` divided by ``ncalls``

   cumtime 
      is the total time spent in this and all subfunctions (from invocation till
      exit). This figure is accurate *even* for recursive functions.

   percall 
      is the quotient of ``cumtime`` divided by primitive calls

   filename:lineno(function) 
      provides the respective data of each function

   When there are two numbers in the first column (for example, ``43/3``), then the
   latter is the number of primitive calls, and the former is the actual number of
   calls.  Note that when the function does not recurse, these two values are the
   same, and only the single figure is printed.


.. function:: runctx(command, globals, locals[, filename])

   This function is similar to :func:`run`, with added arguments to supply the
   globals and locals dictionaries for the *command* string.

Analysis of the profiler data is done using the :class:`Stats` class.

.. note::

   The :class:`Stats` class is defined in the :mod:`pstats` module.


.. module:: pstats
   :synopsis: Statistics object for use with the profiler.


.. class:: Stats(filename[, stream=sys.stdout[, ...]])

   This class constructor creates an instance of a "statistics object" from a
   *filename* (or set of filenames).  :class:`Stats` objects are manipulated by
   methods, in order to print useful reports.  You may specify an alternate output
   stream by giving the keyword argument, ``stream``.

   The file selected by the above constructor must have been created by the
   corresponding version of :mod:`profile` or :mod:`cProfile`.  To be specific,
   there is *no* file compatibility guaranteed with future versions of this
   profiler, and there is no compatibility with files produced by other profilers.
   If several files are provided, all the statistics for identical functions will
   be coalesced, so that an overall view of several processes can be considered in
   a single report.  If additional files need to be combined with data in an
   existing :class:`Stats` object, the :meth:`add` method can be used.


.. _profile-stats:

The :class:`Stats` Class
------------------------

:class:`Stats` objects have the following methods:


.. method:: Stats.strip_dirs()

   This method for the :class:`Stats` class removes all leading path information
   from file names.  It is very useful in reducing the size of the printout to fit
   within (close to) 80 columns.  This method modifies the object, and the stripped
   information is lost.  After performing a strip operation, the object is
   considered to have its entries in a "random" order, as it was just after object
   initialization and loading.  If :meth:`strip_dirs` causes two function names to
   be indistinguishable (they are on the same line of the same filename, and have
   the same function name), then the statistics for these two entries are
   accumulated into a single entry.


.. method:: Stats.add(filename[, ...])

   This method of the :class:`Stats` class accumulates additional profiling
   information into the current profiling object.  Its arguments should refer to
   filenames created by the corresponding version of :func:`profile.run` or
   :func:`cProfile.run`. Statistics for identically named (re: file, line, name)
   functions are automatically accumulated into single function statistics.


.. method:: Stats.dump_stats(filename)

   Save the data loaded into the :class:`Stats` object to a file named *filename*.
   The file is created if it does not exist, and is overwritten if it already
   exists.  This is equivalent to the method of the same name on the
   :class:`profile.Profile` and :class:`cProfile.Profile` classes.


.. method:: Stats.sort_stats(key[, ...])

   This method modifies the :class:`Stats` object by sorting it according to the
   supplied criteria.  The argument is typically a string identifying the basis of
   a sort (example: ``'time'`` or ``'name'``).

   When more than one key is provided, then additional keys are used as secondary
   criteria when there is equality in all keys selected before them.  For example,
   ``sort_stats('name', 'file')`` will sort all the entries according to their
   function name, and resolve all ties (identical function names) by sorting by
   file name.

   Abbreviations can be used for any key names, as long as the abbreviation is
   unambiguous.  The following are the keys currently defined:

   +------------------+----------------------+
   | Valid Arg        | Meaning              |
   +==================+======================+
   | ``'calls'``      | call count           |
   +------------------+----------------------+
   | ``'cumulative'`` | cumulative time      |
   +------------------+----------------------+
   | ``'file'``       | file name            |
   +------------------+----------------------+
   | ``'module'``     | file name            |
   +------------------+----------------------+
   | ``'pcalls'``     | primitive call count |
   +------------------+----------------------+
   | ``'line'``       | line number          |
   +------------------+----------------------+
   | ``'name'``       | function name        |
   +------------------+----------------------+
   | ``'nfl'``        | name/file/line       |
   +------------------+----------------------+
   | ``'stdname'``    | standard name        |
   +------------------+----------------------+
   | ``'time'``       | internal time        |
   +------------------+----------------------+

   Note that all sorts on statistics are in descending order (placing most time
   consuming items first), where as name, file, and line number searches are in
   ascending order (alphabetical). The subtle distinction between ``'nfl'`` and
   ``'stdname'`` is that the standard name is a sort of the name as printed, which
   means that the embedded line numbers get compared in an odd way.  For example,
   lines 3, 20, and 40 would (if the file names were the same) appear in the string
   order 20, 3 and 40.  In contrast, ``'nfl'`` does a numeric compare of the line
   numbers.  In fact, ``sort_stats('nfl')`` is the same as ``sort_stats('name',
   'file', 'line')``.

   For backward-compatibility reasons, the numeric arguments ``-1``, ``0``, ``1``,
   and ``2`` are permitted.  They are interpreted as ``'stdname'``, ``'calls'``,
   ``'time'``, and ``'cumulative'`` respectively.  If this old style format
   (numeric) is used, only one sort key (the numeric key) will be used, and
   additional arguments will be silently ignored.

   .. % For compatibility with the old profiler,


.. method:: Stats.reverse_order()

   This method for the :class:`Stats` class reverses the ordering of the basic list
   within the object.  Note that by default ascending vs descending order is
   properly selected based on the sort key of choice.

   .. % This method is provided primarily for
   .. % compatibility with the old profiler.


.. method:: Stats.print_stats([restriction, ...])

   This method for the :class:`Stats` class prints out a report as described in the
   :func:`profile.run` definition.

   The order of the printing is based on the last :meth:`sort_stats` operation done
   on the object (subject to caveats in :meth:`add` and :meth:`strip_dirs`).

   The arguments provided (if any) can be used to limit the list down to the
   significant entries.  Initially, the list is taken to be the complete set of
   profiled functions.  Each restriction is either an integer (to select a count of
   lines), or a decimal fraction between 0.0 and 1.0 inclusive (to select a
   percentage of lines), or a regular expression (to pattern match the standard
   name that is printed; as of Python 1.5b1, this uses the Perl-style regular
   expression syntax defined by the :mod:`re` module).  If several restrictions are
   provided, then they are applied sequentially.  For example::

      print_stats(.1, 'foo:')

   would first limit the printing to first 10% of list, and then only print
   functions that were part of filename :file:`.\*foo:`.  In contrast, the
   command::

      print_stats('foo:', .1)

   would limit the list to all functions having file names :file:`.\*foo:`, and
   then proceed to only print the first 10% of them.


.. method:: Stats.print_callers([restriction, ...])

   This method for the :class:`Stats` class prints a list of all functions that
   called each function in the profiled database.  The ordering is identical to
   that provided by :meth:`print_stats`, and the definition of the restricting
   argument is also identical.  Each caller is reported on its own line.  The
   format differs slightly depending on the profiler that produced the stats:

   * With :mod:`profile`, a number is shown in parentheses after each caller to
     show how many times this specific call was made.  For convenience, a second
     non-parenthesized number repeats the cumulative time spent in the function
     at the right.

   * With :mod:`cProfile`, each caller is preceeded by three numbers: the number of
     times this specific call was made, and the total and cumulative times spent in
     the current function while it was invoked by this specific caller.


.. method:: Stats.print_callees([restriction, ...])

   This method for the :class:`Stats` class prints a list of all function that were
   called by the indicated function.  Aside from this reversal of direction of
   calls (re: called vs was called by), the arguments and ordering are identical to
   the :meth:`print_callers` method.


.. _profile-limits:

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

The problem is more important with :mod:`profile` than with the lower-overhead
:mod:`cProfile`.  For this reason, :mod:`profile` provides a means of
calibrating itself for a given platform so that this error can be
probabilistically (on the average) removed. After the profiler is calibrated, it
will be more accurate (in a least square sense), but it will sometimes produce
negative numbers (when call counts are exceptionally low, and the gods of
probability work against you :-). )  Do *not* be alarmed by negative numbers in
the profile.  They should *only* appear if you have calibrated your profiler,
and the results are actually better than without calibration.


.. _profile-calibration:

Calibration
===========

The profiler of the :mod:`profile` module subtracts a constant from each event
handling time to compensate for the overhead of calling the time function, and
socking away the results.  By default, the constant is 0. The following
procedure can be used to obtain a better constant for a given platform (see
discussion in section Limitations above). ::

   import profile
   pr = profile.Profile()
   for i in range(5):
       print(pr.calibrate(10000))

The method executes the number of Python calls given by the argument, directly
and again under the profiler, measuring the time for both. It then computes the
hidden overhead per profiler event, and returns that as a float.  For example,
on an 800 MHz Pentium running Windows 2000, and using Python's time.clock() as
the timer, the magical number is about 12.5e-6.

The object of this exercise is to get a fairly consistent result. If your
computer is *very* fast, or your timer function has poor resolution, you might
have to pass 100000, or even 1000000, to get consistent results.

When you have a consistent answer, there are three ways you can use it: [#]_ ::

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


.. _profiler-extensions:

Extensions --- Deriving Better Profilers
========================================

The :class:`Profile` class of both modules, :mod:`profile` and :mod:`cProfile`,
were written so that derived classes could be developed to extend the profiler.
The details are not described here, as doing this successfully requires an
expert understanding of how the :class:`Profile` class works internally.  Study
the source code of the module carefully if you want to pursue this.

If all you want to do is change how current time is determined (for example, to
force use of wall-clock time or elapsed process time), pass the timing function
you want to the :class:`Profile` class constructor::

   pr = profile.Profile(your_time_func)

The resulting profiler will then call :func:`your_time_func`.

:class:`profile.Profile`
   :func:`your_time_func` should return a single number, or a list of numbers whose
   sum is the current time (like what :func:`os.times` returns).  If the function
   returns a single time number, or the list of returned numbers has length 2, then
   you will get an especially fast version of the dispatch routine.

   Be warned that you should calibrate the profiler class for the timer function
   that you choose.  For most machines, a timer that returns a lone integer value
   will provide the best results in terms of low overhead during profiling.
   (:func:`os.times` is *pretty* bad, as it returns a tuple of floating point
   values).  If you want to substitute a better timer in the cleanest fashion,
   derive a class and hardwire a replacement dispatch method that best handles your
   timer call, along with the appropriate calibration constant.

:class:`cProfile.Profile`
   :func:`your_time_func` should return a single number.  If it returns plain
   integers, you can also invoke the class constructor with a second argument
   specifying the real duration of one unit of time.  For example, if
   :func:`your_integer_time_func` returns times measured in thousands of seconds,
   you would constuct the :class:`Profile` instance as follows::

      pr = profile.Profile(your_integer_time_func, 0.001)

   As the :mod:`cProfile.Profile` class cannot be calibrated, custom timer
   functions should be used with care and should be as fast as possible.  For the
   best results with a custom timer, it might be necessary to hard-code it in the C
   source of the internal :mod:`_lsprof` module.

.. rubric:: Footnotes

.. [#] Updated and converted to LaTeX by Guido van Rossum. Further updated by Armin
   Rigo to integrate the documentation for the new :mod:`cProfile` module of Python
   2.5.

.. [#] Prior to Python 2.2, it was necessary to edit the profiler source code to embed
   the bias as a literal number.  You still can, but that method is no longer
   described, because no longer needed.

