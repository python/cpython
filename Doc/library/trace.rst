:mod:`trace` --- Trace or track Python statement execution
==========================================================

.. module:: trace
   :synopsis: Trace or track Python statement execution.

**Source code:** :source:`Lib/trace.py`

--------------

The :mod:`trace` module allows you to trace program execution, generate
annotated statement coverage listings, print caller/callee relationships and
list functions executed during a program run.  It can be used in another program
or from the command line.

.. _trace-cli:

Command-Line Usage
------------------

The :mod:`trace` module can be invoked from the command line.  It can be as
simple as ::

   python -m trace --count -C . somefile.py ...

The above will execute :file:`somefile.py` and generate annotated listings of
all Python modules imported during the execution into the current directory.

.. program:: trace

.. cmdoption:: --help

   Display usage and exit.

.. cmdoption:: --version

   Display the version of the module and exit.

Main options
^^^^^^^^^^^^

At least one of the following options must be specified when invoking
:mod:`trace`.  The :option:`--listfuncs <-l>` option is mutually exclusive with
the :option:`--trace <-t>` and :option:`--counts <-c>` options. When
:option:`--listfuncs <-l>` is provided, neither :option:`--counts <-c>` nor
:option:`--trace <-t>` are accepted, and vice versa.

.. program:: trace

.. cmdoption:: -c, --count

   Produce a set of annotated listing files upon program completion that shows
   how many times each statement was executed.  See also
   :option:`--coverdir <-C>`, :option:`--file <-f>` and
   :option:`--no-report <-R>` below.

.. cmdoption:: -t, --trace

   Display lines as they are executed.

.. cmdoption:: -l, --listfuncs

   Display the functions executed by running the program.

.. cmdoption:: -r, --report

   Produce an annotated list from an earlier program run that used the
   :option:`--count <-c>` and :option:`--file <-f>` option.  This does not
   execute any code.

.. cmdoption:: -T, --trackcalls

   Display the calling relationships exposed by running the program.

Modifiers
^^^^^^^^^

.. program:: trace

.. cmdoption:: -f, --file=<file>

   Name of a file to accumulate counts over several tracing runs.  Should be
   used with the :option:`--count <-c>` option.

.. cmdoption:: -C, --coverdir=<dir>

   Directory where the report files go.  The coverage report for
   ``package.module`` is written to file :file:`{dir}/{package}/{module}.cover`.

.. cmdoption:: -m, --missing

   When generating annotated listings, mark lines which were not executed with
   ``>>>>>>``.

.. cmdoption:: -s, --summary

   When using :option:`--count <-c>` or :option:`--report <-r>`, write a brief
   summary to stdout for each file processed.

.. cmdoption:: -R, --no-report

   Do not generate annotated listings.  This is useful if you intend to make
   several runs with :option:`--count <-c>`, and then produce a single set of
   annotated listings at the end.

.. cmdoption:: -g, --timing

   Prefix each line with the time since the program started.  Only used while
   tracing.

Filters
^^^^^^^

These options may be repeated multiple times.

.. program:: trace

.. cmdoption:: --ignore-module=<mod>

   Ignore each of the given module names and its submodules (if it is a
   package).  The argument can be a list of names separated by a comma.

.. cmdoption:: --ignore-dir=<dir>

   Ignore all modules and packages in the named directory and subdirectories.
   The argument can be a list of directories separated by :data:`os.pathsep`.

.. _trace-api:

Programmatic Interface
----------------------

.. class:: Trace(count=1, trace=1, countfuncs=0, countcallers=0, ignoremods=(),\
                 ignoredirs=(), infile=None, outfile=None, timing=False)

   Create an object to trace execution of a single statement or expression.  All
   parameters are optional.  *count* enables counting of line numbers.  *trace*
   enables line execution tracing.  *countfuncs* enables listing of the
   functions called during the run.  *countcallers* enables call relationship
   tracking.  *ignoremods* is a list of modules or packages to ignore.
   *ignoredirs* is a list of directories whose modules or packages should be
   ignored.  *infile* is the name of the file from which to read stored count
   information.  *outfile* is the name of the file in which to write updated
   count information.  *timing* enables a timestamp relative to when tracing was
   started to be displayed.

    .. method:: run(cmd)

       Execute the command and gather statistics from the execution with
       the current tracing parameters.  *cmd* must be a string or code object,
       suitable for passing into :func:`exec`.

    .. method:: runctx(cmd, globals=None, locals=None)

       Execute the command and gather statistics from the execution with the
       current tracing parameters, in the defined global and local
       environments.  If not defined, *globals* and *locals* default to empty
       dictionaries.

    .. method:: runfunc(func, *args, **kwds)

       Call *func* with the given arguments under control of the :class:`Trace`
       object with the current tracing parameters.

    .. method:: results()

       Return a :class:`CoverageResults` object that contains the cumulative
       results of all previous calls to ``run``, ``runctx`` and ``runfunc``
       for the given :class:`Trace` instance.  Does not reset the accumulated
       trace results.

.. class:: CoverageResults

   A container for coverage results, created by :meth:`Trace.results`.  Should
   not be created directly by the user.

    .. method:: update(other)

       Merge in data from another :class:`CoverageResults` object.

    .. method:: write_results(show_missing=True, summary=False, coverdir=None)

       Write coverage results.  Set *show_missing* to show lines that had no
       hits.  Set *summary* to include in the output the coverage summary per
       module.  *coverdir* specifies the directory into which the coverage
       result files will be output.  If ``None``, the results for each source
       file are placed in its directory.

A simple example demonstrating the use of the programmatic interface::

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

   # make a report, placing output in the current directory
   r = tracer.results()
   r.write_results(show_missing=True, coverdir=".")

