
:mod:`trace` --- Trace or track Python statement execution
==========================================================

.. module:: trace
   :synopsis: Trace or track Python statement execution.


The :mod:`trace` module allows you to trace program execution, generate
annotated statement coverage listings, print caller/callee relationships and
list functions executed during a program run.  It can be used in another program
or from the command line.

.. seealso::

   Latest version of the `trace module Python source code
   <http://svn.python.org/view/python/branches/release27-maint/Lib/trace.py?view=markup>`_

.. _trace-cli:

Command Line Usage
------------------

The :mod:`trace` module can be invoked from the command line.  It can be as
simple as ::

   python -m trace --count somefile.py ...

The above will generate annotated listings of all Python modules imported during
the execution of :file:`somefile.py`.

The following command-line arguments are supported:

:option:`--trace`, :option:`-t`
   Display lines as they are executed.

:option:`--count`, :option:`-c`
   Produce a set of  annotated listing files upon program completion that shows how
   many times each statement was executed.

:option:`--report`, :option:`-r`
   Produce an annotated list from an earlier program run that used the
   :option:`--count` and :option:`--file` arguments.

:option:`--no-report`, :option:`-R`
   Do not generate annotated listings.  This is useful if you intend to make
   several runs with :option:`--count` then produce a single set of annotated
   listings at the end.

:option:`--listfuncs`, :option:`-l`
   List the functions executed by running the program.

:option:`--trackcalls`, :option:`-T`
   Generate calling relationships exposed by running the program.

:option:`--file`, :option:`-f`
   Name a file containing (or to contain) counts.

:option:`--coverdir`, :option:`-C`
   Name a directory in which to save annotated listing files.

:option:`--missing`, :option:`-m`
   When generating annotated listings, mark lines which were not executed with
   '``>>>>>>``'.

:option:`--summary`, :option:`-s`
   When using :option:`--count` or :option:`--report`, write a brief summary to
   stdout for each file processed.

:option:`--ignore-module`
   Accepts comma separated list of module names. Ignore each of the named
   module and its submodules (if it is a package).  May be given
   multiple times.

:option:`--ignore-dir`
   Ignore all modules and packages in the named directory and subdirectories
   (multiple directories can be joined by os.pathsep).  May be given multiple
   times.


.. _trace-api:

Programming Interface
---------------------


.. class:: Trace([count=1[, trace=1[, countfuncs=0[, countcallers=0[, ignoremods=()[, ignoredirs=()[, infile=None[, outfile=None[, timing=False]]]]]]]]])

   Create an object to trace execution of a single statement or expression. All
   parameters are optional.  *count* enables counting of line numbers. *trace*
   enables line execution tracing.  *countfuncs* enables listing of the functions
   called during the run.  *countcallers* enables call relationship tracking.
   *ignoremods* is a list of modules or packages to ignore.  *ignoredirs* is a list
   of directories whose modules or packages should be ignored.  *infile* is the
   file from which to read stored count information.  *outfile* is a file in which
   to write updated count information. *timing* enables a timestamp relative
   to when tracing was started to be displayed.


.. method:: Trace.run(cmd)

   Run *cmd* under control of the Trace object with the current tracing parameters.


.. method:: Trace.runctx(cmd[, globals=None[, locals=None]])

   Run *cmd* under control of the Trace object with the current tracing parameters
   in the defined global and local environments.  If not defined, *globals* and
   *locals* default to empty dictionaries.


.. method:: Trace.runfunc(func, *args, **kwds)

   Call *func* with the given arguments under control of the :class:`Trace` object
   with the current tracing parameters.

This is a simple example showing the use of this module::

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

