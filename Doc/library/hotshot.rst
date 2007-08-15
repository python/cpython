
:mod:`hotshot` --- High performance logging profiler
====================================================

.. module:: hotshot
   :synopsis: High performance logging profiler, mostly written in C.
.. moduleauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Anthony Baxter <anthony@interlink.com.au>


.. versionadded:: 2.2

This module provides a nicer interface to the :mod:`_hotshot` C module. Hotshot
is a replacement for the existing :mod:`profile` module. As it's written mostly
in C, it should result in a much smaller performance impact than the existing
:mod:`profile` module.

.. note::

   The :mod:`hotshot` module focuses on minimizing the overhead while profiling, at
   the expense of long data post-processing times. For common usages it is
   recommended to use :mod:`cProfile` instead. :mod:`hotshot` is not maintained and
   might be removed from the standard library in the future.

.. versionchanged:: 2.5
   the results should be more meaningful than in the past: the timing core
   contained a critical bug.

.. warning::

   The :mod:`hotshot` profiler does not yet work well with threads. It is useful to
   use an unthreaded script to run the profiler over the code you're interested in
   measuring if at all possible.


.. class:: Profile(logfile[, lineevents[, linetimings]])

   The profiler object. The argument *logfile* is the name of a log file to use for
   logged profile data. The argument *lineevents* specifies whether to generate
   events for every source line, or just on function call/return. It defaults to
   ``0`` (only log function call/return). The argument *linetimings* specifies
   whether to record timing information. It defaults to ``1`` (store timing
   information).


.. _hotshot-objects:

Profile Objects
---------------

Profile objects have the following methods:


.. method:: Profile.addinfo(key, value)

   Add an arbitrary labelled value to the profile output.


.. method:: Profile.close()

   Close the logfile and terminate the profiler.


.. method:: Profile.fileno()

   Return the file descriptor of the profiler's log file.


.. method:: Profile.run(cmd)

   Profile an :func:`exec`\ -compatible string in the script environment. The
   globals from the :mod:`__main__` module are used as both the globals and locals
   for the script.


.. method:: Profile.runcall(func, *args, **keywords)

   Profile a single call of a callable. Additional positional and keyword arguments
   may be passed along; the result of the call is returned, and exceptions are
   allowed to propagate cleanly, while ensuring that profiling is disabled on the
   way out.


.. method:: Profile.runctx(cmd, globals, locals)

   Profile an :func:`exec`\ -compatible string in a specific environment. The
   string is compiled before profiling begins.


.. method:: Profile.start()

   Start the profiler.


.. method:: Profile.stop()

   Stop the profiler.


Using hotshot data
------------------

.. module:: hotshot.stats
   :synopsis: Statistical analysis for Hotshot


.. versionadded:: 2.2

This module loads hotshot profiling data into the standard :mod:`pstats` Stats
objects.


.. function:: load(filename)

   Load hotshot data from *filename*. Returns an instance of the
   :class:`pstats.Stats` class.


.. seealso::

   Module :mod:`profile`
      The :mod:`profile` module's :class:`Stats` class


.. _hotshot-example:

Example Usage
-------------

Note that this example runs the python "benchmark" pystones.  It can take some
time to run, and will produce large output files. ::

   >>> import hotshot, hotshot.stats, test.pystone
   >>> prof = hotshot.Profile("stones.prof")
   >>> benchtime, stones = prof.runcall(test.pystone.pystones)
   >>> prof.close()
   >>> stats = hotshot.stats.load("stones.prof")
   >>> stats.strip_dirs()
   >>> stats.sort_stats('time', 'calls')
   >>> stats.print_stats(20)
            850004 function calls in 10.090 CPU seconds

      Ordered by: internal time, call count

      ncalls  tottime  percall  cumtime  percall filename:lineno(function)
           1    3.295    3.295   10.090   10.090 pystone.py:79(Proc0)
      150000    1.315    0.000    1.315    0.000 pystone.py:203(Proc7)
       50000    1.313    0.000    1.463    0.000 pystone.py:229(Func2)
    .
    .
    .

