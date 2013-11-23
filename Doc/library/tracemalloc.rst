:mod:`tracemalloc` --- Trace memory allocations
===============================================

.. module:: tracemalloc
   :synopsis: Trace memory allocations.

The tracemalloc module is a debug tool to trace memory blocks allocated by
Python. It provides the following information:

* Traceback where an object was allocated
* Statistics on allocated memory blocks per filename and per line number:
  total size, number and average size of allocated memory blocks
* Compute the differences between two snapshots to detect memory leaks

To trace most memory blocks allocated by Python, the module should be started
as early as possible by setting the :envvar:`PYTHONTRACEMALLOC` environment
variable to ``1``, or by using :option:`-X` ``tracemalloc`` command line
option. The :func:`tracemalloc.start` function can be called at runtime to
start tracing Python memory allocations.

By default, a trace of an allocated memory block only stores the most recent
frame (1 frame). To store 25 frames at startup: set the
:envvar:`PYTHONTRACEMALLOC` environment variable to ``25``, or use the
:option:`-X` ``tracemalloc=25`` command line option.

.. versionadded:: 3.4


Examples
========

Display the top 10
------------------

Display the 10 files allocating the most memory::

    import tracemalloc

    tracemalloc.start()

    # ... run your application ...

    snapshot = tracemalloc.take_snapshot()
    top_stats = snapshot.statistics('lineno')

    print("[ Top 10 ]")
    for stat in top_stats[:10]:
        print(stat)


Example of output of the Python test suite::

    [ Top 10 ]
    <frozen importlib._bootstrap>:716: size=4855 KiB, count=39328, average=126 B
    <frozen importlib._bootstrap>:284: size=521 KiB, count=3199, average=167 B
    /usr/lib/python3.4/collections/__init__.py:368: size=244 KiB, count=2315, average=108 B
    /usr/lib/python3.4/unittest/case.py:381: size=185 KiB, count=779, average=243 B
    /usr/lib/python3.4/unittest/case.py:402: size=154 KiB, count=378, average=416 B
    /usr/lib/python3.4/abc.py:133: size=88.7 KiB, count=347, average=262 B
    <frozen importlib._bootstrap>:1446: size=70.4 KiB, count=911, average=79 B
    <frozen importlib._bootstrap>:1454: size=52.0 KiB, count=25, average=2131 B
    <string>:5: size=49.7 KiB, count=148, average=344 B
    /usr/lib/python3.4/sysconfig.py:411: size=48.0 KiB, count=1, average=48.0 KiB

We can see that Python loaded ``4.8 MiB`` data (bytecode and constants) from
modules and that the :mod:`collections` module allocated ``244 KiB`` to build
:class:`~collections.namedtuple` types.

See :meth:`Snapshot.statistics` for more options.


Compute differences
-------------------

Take two snapshots and display the differences::

    import tracemalloc
    tracemalloc.start()
    # ... start your application ...

    snapshot1 = tracemalloc.take_snapshot()
    # ... call the function leaking memory ...
    snapshot2 = tracemalloc.take_snapshot()

    top_stats = snapshot2.compare_to(snapshot1, 'lineno')

    print("[ Top 10 differences ]")
    for stat in top_stats[:10]:
        print(stat)

Example of output before/after running some tests of the Python test suite::

    [ Top 10 differences ]
    <frozen importlib._bootstrap>:716: size=8173 KiB (+4428 KiB), count=71332 (+39369), average=117 B
    /usr/lib/python3.4/linecache.py:127: size=940 KiB (+940 KiB), count=8106 (+8106), average=119 B
    /usr/lib/python3.4/unittest/case.py:571: size=298 KiB (+298 KiB), count=589 (+589), average=519 B
    <frozen importlib._bootstrap>:284: size=1005 KiB (+166 KiB), count=7423 (+1526), average=139 B
    /usr/lib/python3.4/mimetypes.py:217: size=112 KiB (+112 KiB), count=1334 (+1334), average=86 B
    /usr/lib/python3.4/http/server.py:848: size=96.0 KiB (+96.0 KiB), count=1 (+1), average=96.0 KiB
    /usr/lib/python3.4/inspect.py:1465: size=83.5 KiB (+83.5 KiB), count=109 (+109), average=784 B
    /usr/lib/python3.4/unittest/mock.py:491: size=77.7 KiB (+77.7 KiB), count=143 (+143), average=557 B
    /usr/lib/python3.4/urllib/parse.py:476: size=71.8 KiB (+71.8 KiB), count=969 (+969), average=76 B
    /usr/lib/python3.4/contextlib.py:38: size=67.2 KiB (+67.2 KiB), count=126 (+126), average=546 B

We can see that Python loaded ``4.4 MiB`` of new data (bytecode and constants)
from modules (on of total of ``8.2 MiB``) and that the :mod:`linecache` module
cached ``940 KiB`` of Python source code to format tracebacks.

If the system has little free memory, snapshots can be written on disk using
the :meth:`Snapshot.dump` method to analyze the snapshot offline. Then use the
:meth:`Snapshot.load` method reload the snapshot.


Get the traceback of a memory block
-----------------------------------

Code to display the traceback of the biggest memory block::

    import linecache
    import tracemalloc

    # Store 25 frames
    tracemalloc.start(25)

    # ... run your application ...

    snapshot = tracemalloc.take_snapshot()
    top_stats = snapshot.statistics('traceback')

    # pick the biggest memory block
    stat = top_stats[0]
    print("%s memory blocks: %.1f KiB" % (stat.count, stat.size / 1024))
    for frame in stat.traceback:
        print('  File "%s", line %s' % (frame.filename, frame.lineno))
        line = linecache.getline(frame.filename, frame.lineno)
        line = line.strip()
        if line:
            print('    ' + line)

Example of output of the Python test suite (traceback limited to 25 frames)::

    903 memory blocks: 870.1 KiB
      File "<frozen importlib._bootstrap>", line 716
      File "<frozen importlib._bootstrap>", line 1036
      File "<frozen importlib._bootstrap>", line 934
      File "<frozen importlib._bootstrap>", line 1068
      File "<frozen importlib._bootstrap>", line 619
      File "<frozen importlib._bootstrap>", line 1581
      File "<frozen importlib._bootstrap>", line 1614
      File "/usr/lib/python3.4/doctest.py", line 101
        import pdb
      File "<frozen importlib._bootstrap>", line 284
      File "<frozen importlib._bootstrap>", line 938
      File "<frozen importlib._bootstrap>", line 1068
      File "<frozen importlib._bootstrap>", line 619
      File "<frozen importlib._bootstrap>", line 1581
      File "<frozen importlib._bootstrap>", line 1614
      File "/usr/lib/python3.4/test/support/__init__.py", line 1728
        import doctest
      File "/usr/lib/python3.4/test/test_pickletools.py", line 21
        support.run_doctest(pickletools)
      File "/usr/lib/python3.4/test/regrtest.py", line 1276
        test_runner()
      File "/usr/lib/python3.4/test/regrtest.py", line 976
        display_failure=not verbose)
      File "/usr/lib/python3.4/test/regrtest.py", line 761
        match_tests=ns.match_tests)
      File "/usr/lib/python3.4/test/regrtest.py", line 1563
        main()
      File "/usr/lib/python3.4/test/__main__.py", line 3
        regrtest.main_in_temp_cwd()
      File "/usr/lib/python3.4/runpy.py", line 73
        exec(code, run_globals)
      File "/usr/lib/python3.4/runpy.py", line 160
        "__main__", fname, loader, pkg_name)

We can see that most memory was allocated in the :mod:`importlib` module to
load data (bytecode and constants) from modules: ``870 KiB``. The traceback is
where the :mod:`importlib` loaded data for the the last time: on the ``import
pdb`` line of the :mod:`doctest` module. The traceback may change if a new
module is loaded.


Pretty top
----------

Code to display the 10 lines allocating the most memory with a pretty output,
ignoring ``<frozen importlib._bootstrap>`` and ``<unknown>`` files::

    import os
    import tracemalloc

    def display_top(snapshot, group_by='lineno', limit=10):
        snapshot = snapshot.filter_traces((
            tracemalloc.Filter(False, "<frozen importlib._bootstrap>"),
            tracemalloc.Filter(False, "<unknown>"),
        ))
        top_stats = snapshot.statistics(group_by)

        print("Top %s lines" % limit)
        for index, stat in enumerate(top_stats[:limit], 1):
            frame = stat.traceback[0]
            # replace "/path/to/module/file.py" with "module/file.py"
            filename = os.sep.join(frame.filename.split(os.sep)[-2:])
            print("#%s: %s:%s: %.1f KiB"
                  % (index, filename, frame.lineno,
                     stat.size / 1024))

        other = top_stats[limit:]
        if other:
            size = sum(stat.size for stat in other)
            print("%s other: %.1f KiB" % (len(other), size / 1024))
        total = sum(stat.size for stat in top_stats)
        print("Total allocated size: %.1f KiB" % (total / 1024))

    tracemalloc.start()

    # ... run your application ...

    snapshot = tracemalloc.take_snapshot()
    display_top(snapshot, 10)

Example of output of the Python test suite::

    2013-11-08 14:16:58.149320: Top 10 lines
    #1: collections/__init__.py:368: 291.9 KiB
    #2: Lib/doctest.py:1291: 200.2 KiB
    #3: unittest/case.py:571: 160.3 KiB
    #4: Lib/abc.py:133: 99.8 KiB
    #5: urllib/parse.py:476: 71.8 KiB
    #6: <string>:5: 62.7 KiB
    #7: Lib/base64.py:140: 59.8 KiB
    #8: Lib/_weakrefset.py:37: 51.8 KiB
    #9: collections/__init__.py:362: 50.6 KiB
    #10: test/test_site.py:56: 48.0 KiB
    7496 other: 4161.9 KiB
    Total allocated size: 5258.8 KiB

See :meth:`Snapshot.statistics` for more options.


API
===

Functions
---------

.. function:: clear_traces()

   Clear traces of memory blocks allocated by Python.

   See also :func:`stop`.


.. function:: get_object_traceback(obj)

   Get the traceback where the Python object *obj* was allocated.
   Return a :class:`Traceback` instance, or ``None`` if the :mod:`tracemalloc`
   module is not tracing memory allocations or did not trace the allocation of
   the object.

   See also :func:`gc.get_referrers` and :func:`sys.getsizeof` functions.


.. function:: get_traceback_limit()

   Get the maximum number of frames stored in the traceback of a trace.

   The :mod:`tracemalloc` module must be tracing memory allocations to
   get the limit, otherwise an exception is raised.

   The limit is set by the :func:`start` function.


.. function:: get_traced_memory()

   Get the current size and maximum size of memory blocks traced by the
   :mod:`tracemalloc` module as a tuple: ``(size: int, max_size: int)``.


.. function:: get_tracemalloc_memory()

   Get the memory usage in bytes of the :mod:`tracemalloc` module used to store
   traces of memory blocks.
   Return an :class:`int`.


.. function:: is_tracing()

    ``True`` if the :mod:`tracemalloc` module is tracing Python memory
    allocations, ``False`` otherwise.

    See also :func:`start` and :func:`stop` functions.


.. function:: start(nframe: int=1)

   Start tracing Python memory allocations: install hooks on Python memory
   allocators. Collected tracebacks of traces will be limited to *nframe*
   frames. By default, a trace of a memory block only stores the most recent
   frame: the limit is ``1``. *nframe* must be greater or equal to ``1``.

   Storing more than ``1`` frame is only useful to compute statistics grouped
   by ``'traceback'`` or to compute cumulative statistics: see the
   :meth:`Snapshot.compare_to` and :meth:`Snapshot.statistics` methods.

   Storing more frames increases the memory and CPU overhead of the
   :mod:`tracemalloc` module. Use the :func:`get_tracemalloc_memory` function
   to measure how much memory is used by the :mod:`tracemalloc` module.

   The :envvar:`PYTHONTRACEMALLOC` environment variable
   (``PYTHONTRACEMALLOC=NFRAME``) and the :option:`-X` ``tracemalloc=NFRAME``
   command line option can be used to start tracing at startup.

   See also :func:`stop`, :func:`is_tracing` and :func:`get_traceback_limit`
   functions.


.. function:: stop()

   Stop tracing Python memory allocations: uninstall hooks on Python memory
   allocators. Clear also traces of memory blocks allocated by Python

   Call :func:`take_snapshot` function to take a snapshot of traces before
   clearing them.

   See also :func:`start` and :func:`is_tracing` functions.


.. function:: take_snapshot()

   Take a snapshot of traces of memory blocks allocated by Python. Return a new
   :class:`Snapshot` instance.

   The snapshot does not include memory blocks allocated before the
   :mod:`tracemalloc` module started to trace memory allocations.

   Tracebacks of traces are limited to :func:`get_traceback_limit` frames. Use
   the *nframe* parameter of the :func:`start` function to store more frames.

   The :mod:`tracemalloc` module must be tracing memory allocations to take a
   snapshot, see the the :func:`start` function.

   See also the :func:`get_object_traceback` function.


Filter
------

.. class:: Filter(inclusive: bool, filename_pattern: str, lineno: int=None, all_frames: bool=False)

   Filter on traces of memory blocks.

   See the :func:`fnmatch.fnmatch` function for the syntax of
   *filename_pattern*. The ``'.pyc'`` and ``'.pyo'`` file extensions are
   replaced with ``'.py'``.

   Examples:

   * ``Filter(True, subprocess.__file__)`` only includes traces of the
     :mod:`subprocess` module
   * ``Filter(False, tracemalloc.__file__)`` excludes traces of the
     :mod:`tracemalloc` module
   * ``Filter(False, "<unknown>")`` excludes empty tracebacks

   .. attribute:: inclusive

      If *inclusive* is ``True`` (include), only trace memory blocks allocated
      in a file with a name matching :attr:`filename_pattern` at line number
      :attr:`lineno`.

      If *inclusive* is ``False`` (exclude), ignore memory blocks allocated in
      a file with a name matching :attr:`filename_pattern` at line number
      :attr:`lineno`.

   .. attribute:: lineno

      Line number (``int``) of the filter. If *lineno* is ``None``, the filter
      matches any line number.

   .. attribute:: filename_pattern

      Filename pattern of the filter (``str``).

   .. attribute:: all_frames

      If *all_frames* is ``True``, all frames of the traceback are checked. If
      *all_frames* is ``False``, only the most recent frame is checked.

      This attribute is ignored if the traceback limit is less than ``2``.  See
      the :func:`get_traceback_limit` function and
      :attr:`Snapshot.traceback_limit` attribute.


Frame
-----

.. class:: Frame

   Frame of a traceback.

   The :class:`Traceback` class is a sequence of :class:`Frame` instances.

   .. attribute:: filename

      Filename (``str``).

   .. attribute:: lineno

      Line number (``int``).


Snapshot
--------

.. class:: Snapshot

   Snapshot of traces of memory blocks allocated by Python.

   The :func:`take_snapshot` function creates a snapshot instance.

   .. method:: compare_to(old_snapshot: Snapshot, group_by: str, cumulative: bool=False)

      Compute the differences with an old snapshot. Get statistics as a sorted
      list of :class:`StatisticDiff` instances grouped by *group_by*.

      See the :meth:`statistics` method for *group_by* and *cumulative*
      parameters.

      The result is sorted from the biggest to the smallest by: absolute value
      of :attr:`StatisticDiff.size_diff`, :attr:`StatisticDiff.size`, absolute
      value of :attr:`StatisticDiff.count_diff`, :attr:`Statistic.count` and
      then by :attr:`StatisticDiff.traceback`.


   .. method:: dump(filename)

      Write the snapshot into a file.

      Use :meth:`load` to reload the snapshot.


   .. method:: filter_traces(filters)

      Create a new :class:`Snapshot` instance with a filtered :attr:`traces`
      sequence, *filters* is a list of :class:`Filter` instances.  If *filters*
      is an empty list, return a new :class:`Snapshot` instance with a copy of
      the traces.

      All inclusive filters are applied at once, a trace is ignored if no
      inclusive filters match it. A trace is ignored if at least one exclusive
      filter matchs it.


   .. classmethod:: load(filename)

      Load a snapshot from a file.

      See also :meth:`dump`.


   .. method:: statistics(group_by: str, cumulative: bool=False)

      Get statistics as a sorted list of :class:`Statistic` instances grouped
      by *group_by*:

      =====================  ========================
      group_by               description
      =====================  ========================
      ``'filename'``         filename
      ``'lineno'``           filename and line number
      ``'traceback'``        traceback
      =====================  ========================

      If *cumulative* is ``True``, cumulate size and count of memory blocks of
      all frames of the traceback of a trace, not only the most recent frame.
      The cumulative mode can only be used with *group_by* equals to
      ``'filename'`` and ``'lineno'`` and :attr:`traceback_limit` greater than
      ``1``.

      The result is sorted from the biggest to the smallest by:
      :attr:`Statistic.size`, :attr:`Statistic.count` and then by
      :attr:`Statistic.traceback`.


   .. attribute:: traceback_limit

      Maximum number of frames stored in the traceback of :attr:`traces`:
      result of the :func:`get_traceback_limit` when the snapshot was taken.

   .. attribute:: traces

      Traces of all memory blocks allocated by Python: sequence of
      :class:`Trace` instances.

      The sequence has an undefined order. Use the :meth:`Snapshot.statistics`
      method to get a sorted list of statistics.


Statistic
---------

.. class:: Statistic

   Statistic on memory allocations.

   :func:`Snapshot.statistics` returns a list of :class:`Statistic` instances.

   See also the :class:`StatisticDiff` class.

   .. attribute:: count

      Number of memory blocks (``int``).

   .. attribute:: size

      Total size of memory blocks in bytes (``int``).

   .. attribute:: traceback

      Traceback where the memory block was allocated, :class:`Traceback`
      instance.


StatisticDiff
-------------

.. class:: StatisticDiff

   Statistic difference on memory allocations between an old and a new
   :class:`Snapshot` instance.

   :func:`Snapshot.compare_to` returns a list of :class:`StatisticDiff`
   instances. See also the :class:`Statistic` class.

   .. attribute:: count

      Number of memory blocks in the new snapshot (``int``): ``0`` if
      the memory blocks have been released in the new snapshot.

   .. attribute:: count_diff

      Difference of number of memory blocks between the old and the new
      snapshots (``int``): ``0`` if the memory blocks have been allocated in
      the new snapshot.

   .. attribute:: size

      Total size of memory blocks in bytes in the new snapshot (``int``):
      ``0`` if the memory blocks have been released in the new snapshot.

   .. attribute:: size_diff

      Difference of total size of memory blocks in bytes between the old and
      the new snapshots (``int``): ``0`` if the memory blocks have been
      allocated in the new snapshot.

   .. attribute:: traceback

      Traceback where the memory blocks were allocated, :class:`Traceback`
      instance.


Trace
-----

.. class:: Trace

   Trace of a memory block.

   The :attr:`Snapshot.traces` attribute is a sequence of :class:`Trace`
   instances.

   .. attribute:: size

      Size of the memory block in bytes (``int``).

   .. attribute:: traceback

      Traceback where the memory block was allocated, :class:`Traceback`
      instance.


Traceback
---------

.. class:: Traceback

   Sequence of :class:`Frame` instances sorted from the most recent frame to
   the oldest frame.

   A traceback contains at least ``1`` frame. If the ``tracemalloc`` module
   failed to get a frame, the filename ``"<unknown>"`` at line number ``0`` is
   used.

   When a snapshot is taken, tracebacks of traces are limited to
   :func:`get_traceback_limit` frames. See the :func:`take_snapshot` function.

   The :attr:`Trace.traceback` attribute is an instance of :class:`Traceback`
   instance.


