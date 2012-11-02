:mod:`timeit` --- Measure execution time of small code snippets
===============================================================

.. module:: timeit
   :synopsis: Measure the execution time of small code snippets.


.. index::
   single: Benchmarking
   single: Performance

**Source code:** :source:`Lib/timeit.py`

--------------

This module provides a simple way to time small bits of Python code. It has both
a :ref:`command-line-interface` as well as a :ref:`callable <python-interface>`
one.  It avoids a number of common traps for measuring execution times.
See also Tim Peters' introduction to the "Algorithms" chapter in the *Python
Cookbook*, published by O'Reilly.


Basic Examples
--------------

The following example shows how the :ref:`command-line-interface`
can be used to compare three different expressions:

.. code-block:: sh

   $ python -m timeit '"-".join(str(n) for n in range(100))'
   10000 loops, best of 3: 40.3 usec per loop
   $ python -m timeit '"-".join([str(n) for n in range(100)])'
   10000 loops, best of 3: 33.4 usec per loop
   $ python -m timeit '"-".join(map(str, range(100)))'
   10000 loops, best of 3: 25.2 usec per loop

This can be achieved from the :ref:`python-interface` with::

   >>> import timeit
   >>> timeit.timeit('"-".join(str(n) for n in range(100))', number=10000)
   0.8187260627746582
   >>> timeit.timeit('"-".join([str(n) for n in range(100)])', number=10000)
   0.7288308143615723
   >>> timeit.timeit('"-".join(map(str, range(100)))', number=10000)
   0.5858950614929199

Note however that :mod:`timeit` will automatically determine the number of
repetitions only when the command-line interface is used.  In the
:ref:`timeit-examples` section you can find more advanced examples.


.. _python-interface:

Python Interface
----------------

The module defines three convenience functions and a public class:


.. function:: timeit(stmt='pass', setup='pass', timer=<default timer>, number=1000000)

   Create a :class:`Timer` instance with the given statement, *setup* code and
   *timer* function and run its :meth:`.timeit` method with *number* executions.


.. function:: repeat(stmt='pass', setup='pass', timer=<default timer>, repeat=3, number=1000000)

   Create a :class:`Timer` instance with the given statement, *setup* code and
   *timer* function and run its :meth:`.repeat` method with the given *repeat*
   count and *number* executions.


.. function:: default_timer()

   The default timer, which is always :func:`time.perf_counter`.

   .. versionchanged:: 3.3
      :func:`time.perf_counter` is now the default timer.


.. class:: Timer(stmt='pass', setup='pass', timer=<timer function>)

   Class for timing execution speed of small code snippets.

   The constructor takes a statement to be timed, an additional statement used
   for setup, and a timer function.  Both statements default to ``'pass'``;
   the timer function is platform-dependent (see the module doc string).
   *stmt* and *setup* may also contain multiple statements separated by ``;``
   or newlines, as long as they don't contain multi-line string literals.

   To measure the execution time of the first statement, use the :meth:`.timeit`
   method.  The :meth:`.repeat` method is a convenience to call :meth:`.timeit`
   multiple times and return a list of results.

   The *stmt* and *setup* parameters can also take objects that are callable
   without arguments.  This will embed calls to them in a timer function that
   will then be executed by :meth:`.timeit`.  Note that the timing overhead is a
   little larger in this case because of the extra function calls.


   .. method:: Timer.timeit(number=1000000)

      Time *number* executions of the main statement.  This executes the setup
      statement once, and then returns the time it takes to execute the main
      statement a number of times, measured in seconds as a float.
      The argument is the number of times through the loop, defaulting to one
      million.  The main statement, the setup statement and the timer function
      to be used are passed to the constructor.

      .. note::

         By default, :meth:`.timeit` temporarily turns off :term:`garbage
         collection` during the timing.  The advantage of this approach is that
         it makes independent timings more comparable.  This disadvantage is
         that GC may be an important component of the performance of the
         function being measured.  If so, GC can be re-enabled as the first
         statement in the *setup* string.  For example::

            timeit.Timer('for i in range(10): oct(i)', 'gc.enable()').timeit()


   .. method:: Timer.repeat(repeat=3, number=1000000)

      Call :meth:`.timeit` a few times.

      This is a convenience function that calls the :meth:`.timeit` repeatedly,
      returning a list of results.  The first argument specifies how many times
      to call :meth:`.timeit`.  The second argument specifies the *number*
      argument for :meth:`.timeit`.

      .. note::

         It's tempting to calculate mean and standard deviation from the result
         vector and report these.  However, this is not very useful.
         In a typical case, the lowest value gives a lower bound for how fast
         your machine can run the given code snippet; higher values in the
         result vector are typically not caused by variability in Python's
         speed, but by other processes interfering with your timing accuracy.
         So the :func:`min` of the result is probably the only number you
         should be interested in.  After that, you should look at the entire
         vector and apply common sense rather than statistics.


   .. method:: Timer.print_exc(file=None)

      Helper to print a traceback from the timed code.

      Typical use::

         t = Timer(...)       # outside the try/except
         try:
             t.timeit(...)    # or t.repeat(...)
         except Exception:
             t.print_exc()

      The advantage over the standard traceback is that source lines in the
      compiled template will be displayed.  The optional *file* argument directs
      where the traceback is sent; it defaults to :data:`sys.stderr`.


.. _command-line-interface:

Command-Line Interface
----------------------

When called as a program from the command line, the following form is used::

   python -m timeit [-n N] [-r N] [-s S] [-t] [-c] [-h] [statement ...]

Where the following options are understood:

.. program:: timeit

.. cmdoption:: -n N, --number=N

   how many times to execute 'statement'

.. cmdoption:: -r N, --repeat=N

   how many times to repeat the timer (default 3)

.. cmdoption:: -s S, --setup=S

   statement to be executed once initially (default ``pass``)

.. cmdoption:: -p, --process

   measure process time, not wallclock time, using :func:`time.process_time`
   instead of :func:`time.perf_counter`, which is the default

   .. versionadded:: 3.3

.. cmdoption:: -t, --time

   use :func:`time.time` (deprecated)

.. cmdoption:: -c, --clock

   use :func:`time.clock` (deprecated)

.. cmdoption:: -v, --verbose

   print raw timing results; repeat for more digits precision

.. cmdoption:: -h, --help

   print a short usage message and exit

A multi-line statement may be given by specifying each line as a separate
statement argument; indented lines are possible by enclosing an argument in
quotes and using leading spaces.  Multiple :option:`-s` options are treated
similarly.

If :option:`-n` is not given, a suitable number of loops is calculated by trying
successive powers of 10 until the total time is at least 0.2 seconds.

:func:`default_timer` measurements can be affected by other programs running on
the same machine, so the best thing to do when accurate timing is necessary is
to repeat the timing a few times and use the best time.  The :option:`-r`
option is good for this; the default of 3 repetitions is probably enough in
most cases.  You can use :func:`time.process_time` to measure CPU time.

.. note::

   There is a certain baseline overhead associated with executing a pass statement.
   The code here doesn't try to hide it, but you should be aware of it.  The
   baseline overhead can be measured by invoking the program without arguments,
   and it might differ between Python versions.


.. _timeit-examples:

Examples
--------

It is possible to provide a setup statement that is executed only once at the beginning:

.. code-block:: sh

   $ python -m timeit -s 'text = "sample string"; char = "g"'  'char in text'
   10000000 loops, best of 3: 0.0877 usec per loop
   $ python -m timeit -s 'text = "sample string"; char = "g"'  'text.find(char)'
   1000000 loops, best of 3: 0.342 usec per loop

::

   >>> import timeit
   >>> timeit.timeit('char in text', setup='text = "sample string"; char = "g"')
   0.41440500499993504
   >>> timeit.timeit('text.find(char)', setup='text = "sample string"; char = "g"')
   1.7246671520006203

The same can be done using the :class:`Timer` class and its methods::

   >>> import timeit
   >>> t = timeit.Timer('char in text', setup='text = "sample string"; char = "g"')
   >>> t.timeit()
   0.3955516149999312
   >>> t.repeat()
   [0.40193588800002544, 0.3960157959998014, 0.39594301399984033]


The following examples show how to time expressions that contain multiple lines.
Here we compare the cost of using :func:`hasattr` vs. :keyword:`try`/:keyword:`except`
to test for missing and present object attributes:

.. code-block:: sh

   $ python -m timeit 'try:' '  str.__bool__' 'except AttributeError:' '  pass'
   100000 loops, best of 3: 15.7 usec per loop
   $ python -m timeit 'if hasattr(str, "__bool__"): pass'
   100000 loops, best of 3: 4.26 usec per loop

   $ python -m timeit 'try:' '  int.__bool__' 'except AttributeError:' '  pass'
   1000000 loops, best of 3: 1.43 usec per loop
   $ python -m timeit 'if hasattr(int, "__bool__"): pass'
   100000 loops, best of 3: 2.23 usec per loop

::

   >>> import timeit
   >>> # attribute is missing
   >>> s = """\
   ... try:
   ...     str.__bool__
   ... except AttributeError:
   ...     pass
   ... """
   >>> timeit.timeit(stmt=s, number=100000)
   0.9138244460009446
   >>> s = "if hasattr(str, '__bool__'): pass"
   >>> timeit.timeit(stmt=s, number=100000)
   0.5829014980008651
   >>>
   >>> # attribute is present
   >>> s = """\
   ... try:
   ...     int.__bool__
   ... except AttributeError:
   ...     pass
   ... """
   >>> timeit.timeit(stmt=s, number=100000)
   0.04215312199994514
   >>> s = "if hasattr(int, '__bool__'): pass"
   >>> timeit.timeit(stmt=s, number=100000)
   0.08588060699912603


To give the :mod:`timeit` module access to functions you define, you can pass a
*setup* parameter which contains an import statement::

   def test():
       """Stupid test function"""
       L = [i for i in range(100)]

   if __name__ == '__main__':
       import timeit
       print(timeit.timeit("test()", setup="from __main__ import test"))
