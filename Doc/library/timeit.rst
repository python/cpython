
:mod:`timeit` --- Measure execution time of small code snippets
===============================================================

.. module:: timeit
   :synopsis: Measure the execution time of small code snippets.


.. index::
   single: Benchmarking
   single: Performance

This module provides a simple way to time small bits of Python code. It has both
command line as well as callable interfaces.  It avoids a number of common traps
for measuring execution times.  See also Tim Peters' introduction to the
"Algorithms" chapter in the Python Cookbook, published by O'Reilly.

The module defines the following public class:


.. class:: Timer([stmt='pass' [, setup='pass' [, timer=<timer function>]]])

   Class for timing execution speed of small code snippets.

   The constructor takes a statement to be timed, an additional statement used for
   setup, and a timer function.  Both statements default to ``'pass'``; the timer
   function is platform-dependent (see the module doc string).  The statements may
   contain newlines, as long as they don't contain multi-line string literals.

   To measure the execution time of the first statement, use the :meth:`timeit`
   method.  The :meth:`repeat` method is a convenience to call :meth:`timeit`
   multiple times and return a list of results.

   The *stmt* and *setup* parameters can also take objects that are callable
   without arguments. This will embed calls to them in a timer function that
   will then be executed by :meth:`timeit`.  Note that the timing overhead is a
   little larger in this case because of the extra function calls.


.. method:: Timer.print_exc([file=None])

   Helper to print a traceback from the timed code.

   Typical use::

      t = Timer(...)       # outside the try/except
      try:
          t.timeit(...)    # or t.repeat(...)
      except:
          t.print_exc()

   The advantage over the standard traceback is that source lines in the compiled
   template will be displayed. The optional *file* argument directs where the
   traceback is sent; it defaults to ``sys.stderr``.


.. method:: Timer.repeat([repeat=3 [, number=1000000]])

   Call :meth:`timeit` a few times.

   This is a convenience function that calls the :meth:`timeit` repeatedly,
   returning a list of results.  The first argument specifies how many times to
   call :meth:`timeit`.  The second argument specifies the *number* argument for
   :func:`timeit`.

   .. note::

      It's tempting to calculate mean and standard deviation from the result vector
      and report these.  However, this is not very useful.  In a typical case, the
      lowest value gives a lower bound for how fast your machine can run the given
      code snippet; higher values in the result vector are typically not caused by
      variability in Python's speed, but by other processes interfering with your
      timing accuracy.  So the :func:`min` of the result is probably the only number
      you should be interested in.  After that, you should look at the entire vector
      and apply common sense rather than statistics.


.. method:: Timer.timeit([number=1000000])

   Time *number* executions of the main statement. This executes the setup
   statement once, and then returns the time it takes to execute the main statement
   a number of times, measured in seconds as a float.  The argument is the number
   of times through the loop, defaulting to one million.  The main statement, the
   setup statement and the timer function to be used are passed to the constructor.

   .. note::

      By default, :meth:`timeit` temporarily turns off garbage collection during the
      timing.  The advantage of this approach is that it makes independent timings
      more comparable.  This disadvantage is that GC may be an important component of
      the performance of the function being measured.  If so, GC can be re-enabled as
      the first statement in the *setup* string.  For example::

         timeit.Timer('for i in range(10): oct(i)', 'gc.enable()').timeit()

Starting with version 2.6, the module also defines two convenience functions:


.. function:: repeat(stmt[, setup[, timer[, repeat=3 [, number=1000000]]]])

   Create a :class:`Timer` instance with the given statement, setup code and timer
   function and run its :meth:`repeat` method with the given repeat count and
   *number* executions.


.. function:: timeit(stmt[, setup[, timer[, number=1000000]]])

   Create a :class:`Timer` instance with the given statement, setup code and timer
   function and run its :meth:`timeit` method with *number* executions.


Command Line Interface
----------------------

When called as a program from the command line, the following form is used::

   python -m timeit [-n N] [-r N] [-s S] [-t] [-c] [-h] [statement ...]

where the following options are understood:

-n N/:option:`--number=N`
   how many times to execute 'statement'

-r N/:option:`--repeat=N`
   how many times to repeat the timer (default 3)

-s S/:option:`--setup=S`
   statement to be executed once initially (default ``'pass'``)

-t/:option:`--time`
   use :func:`time.time` (default on all platforms but Windows)

-c/:option:`--clock`
   use :func:`time.clock` (default on Windows)

-v/:option:`--verbose`
   print raw timing results; repeat for more digits precision

-h/:option:`--help`
   print a short usage message and exit

A multi-line statement may be given by specifying each line as a separate
statement argument; indented lines are possible by enclosing an argument in
quotes and using leading spaces.  Multiple :option:`-s` options are treated
similarly.

If :option:`-n` is not given, a suitable number of loops is calculated by trying
successive powers of 10 until the total time is at least 0.2 seconds.

The default timer function is platform dependent.  On Windows,
:func:`time.clock` has microsecond granularity but :func:`time.time`'s
granularity is 1/60th of a second; on Unix, :func:`time.clock` has 1/100th of a
second granularity and :func:`time.time` is much more precise.  On either
platform, the default timer functions measure wall clock time, not the CPU time.
This means that other processes running on the same computer may interfere with
the timing.  The best thing to do when accurate timing is necessary is to repeat
the timing a few times and use the best time.  The :option:`-r` option is good
for this; the default of 3 repetitions is probably enough in most cases.  On
Unix, you can use :func:`time.clock` to measure CPU time.

.. note::

   There is a certain baseline overhead associated with executing a pass statement.
   The code here doesn't try to hide it, but you should be aware of it.  The
   baseline overhead can be measured by invoking the program without arguments.

The baseline overhead differs between Python versions!  Also, to fairly compare
older Python versions to Python 2.3, you may want to use Python's :option:`-O`
option for the older versions to avoid timing ``SET_LINENO`` instructions.


Examples
--------

Here are two example sessions (one using the command line, one using the module
interface) that compare the cost of using :func:`hasattr` vs.
:keyword:`try`/:keyword:`except` to test for missing and present object
attributes. ::

   % timeit.py 'try:' '  str.__bool__' 'except AttributeError:' '  pass'
   100000 loops, best of 3: 15.7 usec per loop
   % timeit.py 'if hasattr(str, "__bool__"): pass'
   100000 loops, best of 3: 4.26 usec per loop
   % timeit.py 'try:' '  int.__bool__' 'except AttributeError:' '  pass'
   1000000 loops, best of 3: 1.43 usec per loop
   % timeit.py 'if hasattr(int, "__bool__"): pass'
   100000 loops, best of 3: 2.23 usec per loop

::

   >>> import timeit
   >>> s = """\
   ... try:
   ...     str.__bool__
   ... except AttributeError:
   ...     pass
   ... """
   >>> t = timeit.Timer(stmt=s)
   >>> print "%.2f usec/pass" % (1000000 * t.timeit(number=100000)/100000)
   17.09 usec/pass
   >>> s = """\
   ... if hasattr(str, '__bool__'): pass
   ... """
   >>> t = timeit.Timer(stmt=s)
   >>> print "%.2f usec/pass" % (1000000 * t.timeit(number=100000)/100000)
   4.85 usec/pass
   >>> s = """\
   ... try:
   ...     int.__bool__
   ... except AttributeError:
   ...     pass
   ... """
   >>> t = timeit.Timer(stmt=s)
   >>> print "%.2f usec/pass" % (1000000 * t.timeit(number=100000)/100000)
   1.97 usec/pass
   >>> s = """\
   ... if hasattr(int, '__bool__'): pass
   ... """
   >>> t = timeit.Timer(stmt=s)
   >>> print "%.2f usec/pass" % (1000000 * t.timeit(number=100000)/100000)
   3.15 usec/pass

To give the :mod:`timeit` module access to functions you define, you can pass a
``setup`` parameter which contains an import statement::

   def test():
       "Stupid test function"
       L = []
       for i in range(100):
           L.append(i)

   if __name__=='__main__':
       from timeit import Timer
       t = Timer("test()", "from __main__ import test")
       print t.timeit()

