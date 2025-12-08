.. highlight:: shell-session

.. _profiling-sampling:

***************************************************
:mod:`profiling.sampling` --- Statistical profiler
***************************************************

.. module:: profiling.sampling
   :synopsis: Statistical sampling profiler for Python processes.

.. versionadded:: 3.15

**Source code:** :source:`Lib/profiling/sampling/`

--------------

.. image:: tachyon-logo.png
   :alt: Tachyon logo
   :align: center
   :width: 300px

The :mod:`profiling.sampling` module, named **Tachyon**, provides statistical
profiling of Python programs through periodic stack sampling. The profiler can
run scripts directly or attach to any running Python process without requiring
code changes or restarts. Because sampling occurs externally to the target
process, overhead is virtually zero, making this profiler suitable for both
development and production environments.


What is statistical profiling?
==============================

Statistical profiling builds a picture of program behavior by periodically
capturing snapshots of the call stack. Rather than instrumenting every function
call and return as deterministic profilers do, the sampling profiler reads the
call stack at regular intervals to record what code is currently running.

This approach rests on a simple principle: functions that consume significant
CPU time will appear frequently in the collected samples. By gathering thousands
of samples over a profiling session, the profiler constructs an accurate
statistical estimate of where time is spent. The more samples collected, the
more precise this estimate becomes.

The key difference from :mod:`profiling.tracing` is how measurement happens.
A tracing profiler instruments your code, recording every function call and
return. This provides exact call counts and precise timing but adds overhead
to every function call. A sampling profiler, by contrast, observes the program
from outside at fixed intervals without modifying its execution. Think of the
difference like this: tracing is like having someone follow you and write down
every step you take, while sampling is like taking photographs every second
and inferring your path from those snapshots.

This external observation model is what makes sampling profiling practical for
production use. The profiled program runs at full speed because there is no
instrumentation code running inside it, and the target process is never stopped
or paused during sampling---the profiler reads the call stack directly from the
process's memory while it continues to run. You can attach to a live server,
collect data, and detach without the application ever knowing it was observed.
The trade-off is that very short-lived functions may be missed if they happen
to complete between samples.

Statistical profiling excels at answering the question, "Where is my program
spending time?" It reveals hotspots and bottlenecks in production code where
deterministic profiling overhead would be unacceptable. For exact call counts
and complete call graphs, use :mod:`profiling.tracing` instead.


Quick examples
==============

Profile a script and see the results immediately::

   python -m profiling.sampling run script.py

Profile a module with arguments::

   python -m profiling.sampling run -m mypackage.module arg1 arg2

Generate an interactive flame graph::

   python -m profiling.sampling run --flamegraph -o profile.html script.py

Attach to a running process by PID::

   python -m profiling.sampling attach 12345

Use live mode for real-time monitoring (press ``q`` to quit)::

   python -m profiling.sampling run --live script.py

Profile for 60 seconds with a faster sampling rate::

   python -m profiling.sampling run -d 60 -i 50 script.py

Generate a line-by-line heatmap::

   python -m profiling.sampling run --heatmap script.py


Commands
========

The profiler operates through two subcommands that determine how to obtain
the target process.


The ``run`` command
-------------------

The ``run`` command launches a Python script or module and profiles it from
startup::

   python -m profiling.sampling run script.py
   python -m profiling.sampling run -m mypackage.module

When profiling a script, the profiler starts the target in a subprocess,
waits for it to initialize, then begins collecting samples. The ``-m`` flag
indicates that the target should be run as a module (equivalent to
``python -m``). Arguments after the target are passed through to the
profiled program::

   python -m profiling.sampling run script.py --config settings.yaml


The ``attach`` command
----------------------

The ``attach`` command connects to an already-running Python process by its
process ID::

   python -m profiling.sampling attach 12345

This command is particularly valuable for investigating performance issues in
production systems. The target process requires no modification and need not
be restarted. The profiler attaches, collects samples for the specified
duration, then detaches and produces output.

On most systems, attaching to another process requires appropriate permissions.
On Linux, you may need to run as root or adjust the ``ptrace_scope`` setting.


Sampling configuration
======================

Before exploring the various output formats and visualization options, it is
important to understand how to configure the sampling process itself. The
profiler offers several options that control how frequently samples are
collected, how long profiling runs, which threads are observed, and what
additional context is captured in each sample.

The default configuration works well for most use cases:

.. list-table::
   :header-rows: 1
   :widths: 25 75

   * - Option
     - Default behavior
   * - ``--interval`` / ``-i``
     - 100 µs between samples (~10,000 samples/sec)
   * - ``--duration`` / ``-d``
     - Profile for 10 seconds
   * - ``--all-threads`` / ``-a``
     - Sample main thread only
   * - ``--native``
     - No ``<native>`` frames (C code time attributed to caller)
   * - ``--no-gc``
     - Include ``<GC>`` frames when garbage collection is active
   * - ``--mode``
     - Wall-clock mode (all samples recorded)
   * - ``--realtime-stats``
     - No live statistics display during profiling


Sampling interval and duration
------------------------------

The two most fundamental parameters are the sampling interval and duration.
Together, these determine how many samples will be collected during a profiling
session.

The ``--interval`` option (``-i``) sets the time between samples in
microseconds. The default is 100 microseconds, which produces approximately
10,000 samples per second::

   python -m profiling.sampling run -i 50 script.py

Lower intervals capture more samples and provide finer-grained data at the
cost of slightly higher profiler CPU usage. Higher intervals reduce profiler
overhead but may miss short-lived functions. For most applications, the
default interval provides a good balance between accuracy and overhead.

The ``--duration`` option (``-d``) sets how long to profile in seconds. The
default is 10 seconds::

   python -m profiling.sampling run -d 60 script.py

Longer durations collect more samples and produce more statistically reliable
results, especially for code paths that execute infrequently. When profiling
a program that runs for a fixed time, you may want to set the duration to
match or exceed the expected runtime.


Thread selection
----------------

Python programs often use multiple threads, whether explicitly through the
:mod:`threading` module or implicitly through libraries that manage thread
pools.

By default, the profiler samples only the main thread. The ``--all-threads``
option (``-a``) enables sampling of all threads in the process::

   python -m profiling.sampling run -a script.py

Multi-thread profiling reveals how work is distributed across threads and can
identify threads that are blocked or starved. Each thread's samples are
combined in the output, with the ability to filter by thread in some formats.
This option is particularly useful when investigating concurrency issues or
when work is distributed across a thread pool.


Special frames
--------------

The profiler can inject artificial frames into the captured stacks to provide
additional context about what the interpreter is doing at the moment each
sample is taken. These synthetic frames help distinguish different types of
execution that would otherwise be invisible.

The ``--native`` option adds ``<native>`` frames to indicate when Python has
called into C code (extension modules, built-in functions, or the interpreter
itself)::

   python -m profiling.sampling run --native script.py

These frames help distinguish time spent in Python code versus time spent in
native libraries. Without this option, native code execution appears as time
in the Python function that made the call. This is useful when optimizing
code that makes heavy use of C extensions like NumPy or database drivers.

By default, the profiler includes ``<GC>`` frames when garbage collection is
active. The ``--no-gc`` option suppresses these frames::

   python -m profiling.sampling run --no-gc script.py

GC frames help identify programs where garbage collection consumes significant
time, which may indicate memory allocation patterns worth optimizing. If you
see substantial time in ``<GC>`` frames, consider investigating object
allocation rates or using object pooling.


Real-time statistics
--------------------

The ``--realtime-stats`` option displays sampling rate statistics during
profiling::

   python -m profiling.sampling run --realtime-stats script.py

This shows the actual achieved sampling rate, which may be lower than requested
if the profiler cannot keep up. The statistics help verify that profiling is
working correctly and that sufficient samples are being collected. See
:ref:`sampling-efficiency` for details on interpreting these metrics.


.. _sampling-efficiency:

Sampling efficiency
-------------------

Sampling efficiency metrics help assess the quality of the collected data.
These metrics appear in the profiler's terminal output and in the flame graph
sidebar.

**Sampling efficiency** is the percentage of sample attempts that succeeded.
Each sample attempt reads the target process's call stack from memory. An
attempt can fail if the process is in an inconsistent state at the moment of
reading, such as during a context switch or while the interpreter is updating
its internal structures. A low efficiency may indicate that the profiler could
not keep up with the requested sampling rate, often due to system load or an
overly aggressive interval setting.

**Missed samples** is the percentage of expected samples that were not
collected. Based on the configured interval and duration, the profiler expects
to collect a certain number of samples. Some samples may be missed if the
profiler falls behind schedule, for example when the system is under heavy
load. A small percentage of missed samples is normal and does not significantly
affect the statistical accuracy of the profile.

Both metrics are informational. Even with some failed attempts or missed
samples, the profile remains statistically valid as long as enough samples
were collected. The profiler reports the actual number of samples captured,
which you can use to judge whether the data is sufficient for your analysis.


Profiling modes
===============

The sampling profiler supports three modes that control which samples are
recorded. The mode determines what the profile measures: total elapsed time,
CPU execution time, or time spent holding the global interpreter lock.


Wall-clock mode
---------------

Wall-clock mode (``--mode=wall``) captures all samples regardless of what the
thread is doing. This is the default mode and provides a complete picture of
where time passes during program execution::

   python -m profiling.sampling run --mode=wall script.py

In wall-clock mode, samples are recorded whether the thread is actively
executing Python code, waiting for I/O, blocked on a lock, or sleeping.
This makes wall-clock profiling ideal for understanding the overall time
distribution in your program, including time spent waiting.

If your program spends significant time in I/O operations, network calls, or
sleep, wall-clock mode will show these waits as time attributed to the calling
function. This is often exactly what you want when optimizing end-to-end
latency.


CPU mode
--------

CPU mode (``--mode=cpu``) records samples only when the thread is actually
executing on a CPU core::

   python -m profiling.sampling run --mode=cpu script.py

Samples taken while the thread is sleeping, blocked on I/O, or waiting for
a lock are discarded. The resulting profile shows where CPU cycles are consumed,
filtering out idle time.

CPU mode is useful when you want to focus on computational hotspots without
being distracted by I/O waits. If your program alternates between computation
and network calls, CPU mode reveals which computational sections are most
expensive.


GIL mode
--------

GIL mode (``--mode=gil``) records samples only when the thread holds Python's
global interpreter lock::

   python -m profiling.sampling run --mode=gil script.py

This mode is specifically designed for analyzing GIL contention in
multi-threaded Python programs. Since only one thread can hold the GIL at a
time, a profile showing GIL-holding time reveals which code is preventing
other threads from running Python bytecode.

GIL mode helps answer questions like "which functions are monopolizing the
GIL?" and "why are my other threads starving?" It is less useful for
single-threaded programs or for code that releases the GIL (such as many
NumPy operations or I/O calls).


Output formats
==============

The profiler produces output in several formats, each suited to different
analysis workflows. The format is selected with a command-line flag, and
output goes to stdout, a file, or a directory depending on the format.


pstats format
-------------

The pstats format (``--pstats``) produces a text table similar to what
deterministic profilers generate. This is the default output format::

   python -m profiling.sampling run script.py
   python -m profiling.sampling run --pstats script.py

Output appears on stdout by default::

   Profile Stats (Mode: wall):
        nsamples  sample%    tottime (ms)  cumul%   cumtime (ms)  filename:lineno(function)
          234/892    11.7%       234.00     44.6%       892.00    server.py:145(handle_request)
          156/156     7.8%       156.00      7.8%       156.00    <built-in>:0(socket.recv)
           98/421     4.9%        98.00     21.1%       421.00    parser.py:67(parse_message)

The columns show sampling counts and estimated times. The ``nsamples`` column
displays two numbers: direct samples (when the function was at the top of the
stack, actively executing) and cumulative samples (when the function appeared
anywhere on the stack). The percentages and times derive from these counts
and the total profiling duration. Time units are selected automatically based
on the data: seconds, milliseconds, or microseconds.

The output includes a legend explaining each column and a summary of
interesting functions that highlights:

- **Hot spots**: functions with high direct/cumulative sample ratio (time spent
  directly executing rather than waiting for callees)
- **Indirect calls**: functions appearing frequently on the stack via other
  callers
- **Call magnification**: functions where cumulative samples far exceed direct
  samples, indicating significant time in nested calls

Use ``--no-summary`` to suppress both the legend and summary sections.

To save pstats output to a file instead of stdout::

   python -m profiling.sampling run -o profile.txt script.py

The pstats format supports several options for controlling the display.
The ``--sort`` option determines the column used for ordering results::

   python -m profiling.sampling run --sort=tottime script.py
   python -m profiling.sampling run --sort=cumtime script.py
   python -m profiling.sampling run --sort=nsamples script.py

The ``--limit`` option restricts output to the top N entries::

   python -m profiling.sampling run --limit=30 script.py

The ``--no-summary`` option suppresses the header summary that precedes the
statistics table.


Collapsed stacks format
-----------------------

Collapsed stacks format (``--collapsed``) produces one line per unique call
stack, with a count of how many times that stack was sampled::

   python -m profiling.sampling run --collapsed script.py

The output looks like:

.. code-block:: text

   main;process_data;parse_json;decode_utf8 42
   main;process_data;parse_json 156
   main;handle_request;send_response 89

Each line contains semicolon-separated function names representing the call
stack from bottom to top, followed by a space and the sample count. This
format is designed for compatibility with external flame graph tools,
particularly Brendan Gregg's ``flamegraph.pl`` script.

To generate a flame graph from collapsed stacks::

   python -m profiling.sampling run --collapsed script.py > stacks.txt
   flamegraph.pl stacks.txt > profile.svg

The resulting SVG can be viewed in any web browser and provides an interactive
visualization where you can click to zoom into specific call paths.


Flame graph format
------------------

Flame graph format (``--flamegraph``) produces a self-contained HTML file with
an interactive flame graph visualization::

   python -m profiling.sampling run --flamegraph script.py
   python -m profiling.sampling run --flamegraph -o profile.html script.py

If no output file is specified, the profiler generates a filename based on
the process ID (for example, ``flamegraph.12345.html``).

The generated HTML file requires no external dependencies and can be opened
directly in a web browser. The visualization displays call stacks as nested
rectangles, with width proportional to time spent. Hovering over a rectangle
shows details about that function including source code context, and clicking
zooms into that portion of the call tree.

The flame graph interface includes:

- A sidebar showing profile summary, thread statistics, sampling efficiency
  metrics (see :ref:`sampling-efficiency`), and top hotspot functions
- Search functionality supporting both function name matching and
  ``file.py:42`` line patterns
- Per-thread filtering via dropdown
- Dark/light theme toggle (preference saved across sessions)
- SVG export for saving the current view

Flame graphs are particularly effective for identifying deep call stacks and
understanding the hierarchical structure of time consumption. Wide rectangles
at the top indicate functions that consume significant time either directly
or through their callees.


Gecko format
------------

Gecko format (``--gecko``) produces JSON output compatible with the Firefox
Profiler::

   python -m profiling.sampling run --gecko script.py
   python -m profiling.sampling run --gecko -o profile.json script.py

The `Firefox Profiler <https://profiler.firefox.com>`__ is a sophisticated
web-based tool originally built for profiling Firefox itself. It provides
features beyond basic flame graphs, including a timeline view, call tree
exploration, and marker visualization. See the
`Firefox Profiler documentation <https://profiler.firefox.com/docs/#/>`__ for
detailed usage instructions.

To use the output, open the Firefox Profiler in your browser and load the
JSON file. The profiler runs entirely client-side, so your profiling data
never leaves your machine.

Gecko format automatically collects additional metadata about GIL state and
CPU activity, enabling analysis features specific to Python's threading model.
The profiler emits interval markers that appear as colored bands in the
Firefox Profiler timeline:

- **GIL markers**: show when threads hold or release the global interpreter lock
- **CPU markers**: show when threads are executing on CPU versus idle
- **Code type markers**: distinguish Python code from native (C extension) code
- **GC markers**: indicate garbage collection activity

For this reason, the ``--mode`` option is not available with Gecko format;
all relevant data is captured automatically.


Heatmap format
--------------

Heatmap format (``--heatmap``) generates an interactive HTML visualization
showing sample counts at the source line level::

   python -m profiling.sampling run --heatmap script.py
   python -m profiling.sampling run --heatmap -o my_heatmap script.py

Unlike other formats that produce a single file, heatmap output creates a
directory containing HTML files for each profiled source file. If no output
path is specified, the directory is named ``heatmap_PID``.

The heatmap visualization displays your source code with a color gradient
indicating how many samples were collected at each line. Hot lines (many
samples) appear in warm colors, while cold lines (few or no samples) appear
in cool colors. This view helps pinpoint exactly which lines of code are
responsible for time consumption.

The heatmap interface provides several interactive features:

- **Coloring modes**: toggle between "Self Time" (direct execution) and
  "Total Time" (cumulative, including time in called functions)
- **Cold code filtering**: show all lines or only lines with samples
- **Call graph navigation**: each line has buttons to navigate to callers
  (functions that called this line) and callees (functions called from here)
- **Scroll minimap**: a vertical overview showing the heat distribution across
  the entire file
- **Hierarchical index**: files organized by type (stdlib, site-packages,
  project) with aggregate sample counts per folder
- **Dark/light theme**: toggle with preference saved across sessions
- **Line linking**: click line numbers to create shareable URLs

Heatmaps are especially useful when you know which file contains a performance
issue but need to identify the specific lines. Many developers prefer this
format because it maps directly to their source code, making it easy to read
and navigate. For smaller scripts and focused analysis, heatmaps provide an
intuitive view that shows exactly where time is spent without requiring
interpretation of hierarchical visualizations.


Live mode
=========

Live mode (``--live``) provides a terminal-based real-time view of profiling
data, similar to the ``top`` command for system processes::

   python -m profiling.sampling run --live script.py
   python -m profiling.sampling attach --live 12345

The display updates continuously as new samples arrive, showing the current
hottest functions. This mode requires the :mod:`curses` module, which is
available on Unix-like systems. The terminal must be at least 80 columns wide
and 24 lines tall.

Within live mode, keyboard commands control the display:

:kbd:`q`
   Quit the profiler and return to the shell.

:kbd:`s` / :kbd:`S`
   Cycle through sort orders forward/backward (sample count, percentage,
   total time, cumulative percentage, cumulative time).

:kbd:`p`
   Pause or resume display updates. Sampling continues while paused.

:kbd:`r`
   Reset all statistics and start fresh.

:kbd:`/`
   Enter filter mode to search for functions by name. Type a pattern and
   press Enter to filter, or Escape to cancel.

:kbd:`c`
   Clear the current filter.

:kbd:`t`
   Toggle between viewing all threads combined or per-thread statistics.

:kbd:`←` :kbd:`→` or :kbd:`↑` :kbd:`↓`
   In per-thread view, navigate between threads.

:kbd:`+` / :kbd:`-`
   Increase or decrease the display refresh rate (range: 0.05s to 1.0s).

:kbd:`x`
   Toggle trend indicators that show whether functions are becoming hotter
   or cooler over time.

:kbd:`h` or :kbd:`?`
   Show the help screen with all available commands.

Live mode is incompatible with output format options (``--collapsed``,
``--flamegraph``, and so on) because it uses an interactive terminal
interface rather than producing file output.


Async-aware profiling
=====================

For programs using :mod:`asyncio`, the profiler offers async-aware mode
(``--async-aware``) that reconstructs call stacks based on the task structure
rather than the raw Python frames::

   python -m profiling.sampling run --async-aware async_script.py

Standard profiling of async code can be confusing because the physical call
stack often shows event loop internals rather than the logical flow of your
coroutines. Async-aware mode addresses this by tracking which task is running
and presenting stacks that reflect the ``await`` chain.

The ``--async-mode`` option controls which tasks appear in the profile::

   python -m profiling.sampling run --async-aware --async-mode=running async_script.py
   python -m profiling.sampling run --async-aware --async-mode=all async_script.py

With ``--async-mode=running`` (the default), only the currently executing task
is profiled. With ``--async-mode=all``, tasks that are suspended (awaiting
something) are also included, giving visibility into what your program is
waiting for.

Async-aware mode requires wall-clock mode and is incompatible with several
other options: ``--native``, ``--no-gc``, ``--all-threads``, and
``--mode=cpu`` or ``--mode=gil``. These restrictions exist because async-aware
profiling uses a different stack reconstruction mechanism that tracks task
relationships rather than raw Python frames.


Command-line interface
======================

.. program:: profiling.sampling

The complete command-line interface for reference.


Global options
--------------

.. option:: run

   Run and profile a Python script or module.

.. option:: attach

   Attach to and profile a running process by PID.


Sampling options
----------------

.. option:: -i <microseconds>, --interval <microseconds>

   Sampling interval in microseconds. Default: 100.

.. option:: -d <seconds>, --duration <seconds>

   Profiling duration in seconds. Default: 10.

.. option:: -a, --all-threads

   Sample all threads, not just the main thread.

.. option:: --realtime-stats

   Display sampling statistics during profiling.

.. option:: --native

   Include ``<native>`` frames for non-Python code.

.. option:: --no-gc

   Exclude ``<GC>`` frames for garbage collection.

.. option:: --async-aware

   Enable async-aware profiling for asyncio programs.


Mode options
------------

.. option:: --mode <mode>

   Sampling mode: ``wall`` (default), ``cpu``, or ``gil``.
   The ``cpu`` and ``gil`` modes are incompatible with ``--async-aware``.

.. option:: --async-mode <mode>

   Async profiling mode: ``running`` (default) or ``all``.
   Requires ``--async-aware``.


Output options
--------------

.. option:: --pstats

   Generate text statistics output. This is the default.

.. option:: --collapsed

   Generate collapsed stack format for external flame graph tools.

.. option:: --flamegraph

   Generate self-contained HTML flame graph.

.. option:: --gecko

   Generate Gecko JSON format for Firefox Profiler.

.. option:: --heatmap

   Generate HTML heatmap with line-level sample counts.

.. option:: -o <path>, --output <path>

   Output file or directory path. Default behavior varies by format:
   ``--pstats`` writes to stdout, ``--flamegraph`` and ``--gecko`` generate
   files like ``flamegraph.PID.html``, and ``--heatmap`` creates a directory
   named ``heatmap_PID``.


pstats display options
----------------------

These options apply only to pstats format output.

.. option:: --sort <key>

   Sort order: ``nsamples``, ``tottime``, ``cumtime``, ``sample-pct``,
   ``cumul-pct``, ``nsamples-cumul``, or ``name``. Default: ``nsamples``.

.. option:: -l <count>, --limit <count>

   Maximum number of entries to display. Default: 15.

.. option:: --no-summary

   Omit the Legend and Summary of Interesting Functions sections from output.


Run command options
-------------------

.. option:: -m, --module

   Treat the target as a module name rather than a script path.

.. option:: --live

   Start interactive terminal interface instead of batch profiling.


.. seealso::

   :mod:`profiling`
      Overview of Python profiling tools and guidance on choosing a profiler.

   :mod:`profiling.tracing`
      Deterministic tracing profiler for exact call counts and timing.

   :mod:`pstats`
      Statistics analysis for profile data.

   `Firefox Profiler <https://profiler.firefox.com>`__
      Web-based profiler that accepts Gecko format output. See the
      `documentation <https://profiler.firefox.com/docs/#/>`__ for usage details.

   `FlameGraph <https://github.com/brendangregg/FlameGraph>`__
      Tools for generating flame graphs from collapsed stack format.
