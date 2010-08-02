:mod:`trace` --- Trace or track Python statement execution
==========================================================

.. module:: trace
   :synopsis: Trace or track Python statement execution.


The :mod:`trace` module allows you to trace program execution, generate
annotated statement coverage listings, print caller/callee relationships and
list functions executed during a program run.  It can be used in another program
or from the command line.


.. _trace-cli:

Command Line Usage
------------------

The :mod:`trace` module can be invoked from the command line.  It can be as
simple as ::

   python -m trace --count -C . somefile.py ...

The above will execute :file:`somefile.py` and generate annotated listings of all
Python modules imported during the execution into the current directory.

Meta-options
^^^^^^^^^^^^

``--help``

   Display usage and exit.

``--version``

   Display the version of the module and exit.

Main options
^^^^^^^^^^^^

The ``--listfuncs`` option is mutually exclusive with the ``--trace`` and
``--count`` options . When ``--listfuncs`` is provided, neither ``--counts``
nor ``--trace`` are accepted, and vice versa.

``--count, -c``

   Produce a set of annotated listing files upon program completion that shows
   how many times each statement was executed.
   See also ``--coverdir``, ``--file``, ``--no-report`` below.

``--trace, -t``

   Display lines as they are executed.

``--listfuncs, -l``

   Display the functions executed by running the program.

``--report, -r``

   Produce an annotated list from an earlier program run that used the ``--count``
   and ``--file`` option. Do not execute any code.

``--trackcalls, -T``

   Display the calling relationships exposed by running the program.

Modifiers
^^^^^^^^^

``--file=<file>, -f``

   Name of a file to accumulate counts over several tracing runs. Should be used
   with the ``--count`` option.

``--coverdir=<dir>, -C``

   Directory where the report files go. The coverage report for
   ``package.module`` is written to file ``dir/package/module.cover``.

``--missing, -m``

   When generating annotated listings, mark lines which were not executed with
   '``>>>>>>``'.

``--summary, -s``

   When using ``--count`` or ``--report``, write a brief summary to
   stdout for each file processed.

``--no-report, -R``

   Do not generate annotated listings.  This is useful if you intend to make
   several runs with ``--count`` then produce a single set of annotated
   listings at the end.

``--timing, -g``

   Prefix each line with the time since the program started. Only used while
   tracing.

Filters
^^^^^^^

These options may be repeated multiple times.

``--ignore-module=<mod>``

   Accepts comma separated list of module names. Ignore each of the named
   modules and its submodules (if it is a package).

``--ignore-dir=<dir>``

   Ignore all modules and packages in the named directory and subdirectories
   (multiple directories can be joined by ``os.pathsep``).

.. _trace-api:

Programming Interface
---------------------


.. class:: Trace(count=1, trace=1, countfuncs=0, countcallers=0, ignoremods=(), ignoredirs=(), infile=None, outfile=None, timing=False)

   Create an object to trace execution of a single statement or expression. All
   parameters are optional.  *count* enables counting of line numbers. *trace*
   enables line execution tracing.  *countfuncs* enables listing of the functions
   called during the run.  *countcallers* enables call relationship tracking.
   *ignoremods* is a list of modules or packages to ignore.  *ignoredirs* is a list
   of directories whose modules or packages should be ignored.  *infile* is the
   name of the file from which to read stored count information.  *outfile* is
   the name of the file in which to write updated count information. *timing*
   enables a timestamp relative to when tracing was started to be displayed.


.. method:: Trace.run(cmd)

   Run *cmd* under control of the :class:`Trace` object with the current tracing parameters.
   *cmd* must be a string or code object, suitable for passing into :func:`exec`.


.. method:: Trace.runctx(cmd, globals=None, locals=None)

   Run *cmd* under control of the :class:`Trace` object with the current tracing parameters
   in the defined global and local environments.  If not defined, *globals* and
   *locals* default to empty dictionaries.


.. method:: Trace.runfunc(func, *args, **kwds)

   Call *func* with the given arguments under control of the :class:`Trace` object
   with the current tracing parameters.

.. method:: Trace.results()

   Return a :class:`CoverageResults` object that contains the cumulative results
   of all previous calls to ``run``, ``runctx`` and ``runfunc`` for the given
   :class:`Trace` instance. Does not reset the accumulated trace results.

.. class:: CoverageResults

   A container for coverage results, created by :meth:`Trace.results`. Should not
   be created directly by the user.

.. method:: CoverageResults.update(other)

   Merge in data from another :class:`CoverageResults` object.

.. method:: CoverageResults.write_results(show_missing=True, summary=False, coverdir=None)

   Write coverage results. Set *show_missing* to show lines that had no hits.
   Set *summary* to include in the output the coverage summary per module. *coverdir*
   specifies the directory into which the coverage result files will be output.
   If ``None``, the results for each source file are placed in its directory.

..

A simple example demonstrating the use of the programming interface::

   import sys
   import trace

   # create a Trace object, telling it what to ignore, and whether to
   # do tracing or line-counting or both.
   tracer = trace.Trace(
       ignoredirs=[sys.prefix, sys.exec_prefix],
       trace=0,
       count=1)

   # run the new command using the given tracer
   tracer.run('main()')

   # make a report, placing output in /tmp
   r = tracer.results()
   r.write_results(show_missing=True, coverdir="/tmp")

