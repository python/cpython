.. highlight:: shell-session

.. _instrumentation:

===============================================
Instrumenting CPython with DTrace and SystemTap
===============================================

:author: David Malcolm
:author: Łukasz Langa

DTrace and SystemTap are monitoring tools, each providing a way to inspect
what the processes on a computer system are doing.  They both use
domain-specific languages allowing a user to write scripts which:

- filter which processes are to be observed
- gather data from the processes of interest
- generate reports on the data

As of Python 3.6, CPython can be built with embedded "markers", also
known as "probes", that can be observed by a DTrace or SystemTap script,
making it easier to monitor what the CPython processes on a system are
doing.

.. note::

   As of Python 3.12, the ``function__entry``, ``function__return``, and ``line``
   probes have been removed due to the implementation of :pep:`669` (Low Impact
   Monitoring for CPython). The remaining available probes focus on garbage
   collection, module imports, and audit events.

.. impl-detail::

   DTrace markers are implementation details of the CPython interpreter.
   No guarantees are made about probe compatibility between versions of
   CPython. DTrace scripts can stop working or work incorrectly without
   warning when changing CPython versions.


Enabling the static markers
---------------------------

macOS comes with built-in support for DTrace.  On Linux, in order to
build CPython with the embedded markers for SystemTap, the SystemTap
development tools must be installed.

On a Linux machine, this can be done via::

   $ yum install systemtap-sdt-devel

or::

   $ sudo apt-get install systemtap-sdt-dev


CPython must then be :option:`configured with the --with-dtrace option
<--with-dtrace>`:

.. code-block:: none

   checking for --with-dtrace... yes

On macOS, you can list available DTrace probes by running a Python
process in the background and listing all probes made available by the
Python provider::

   $ python3.12 -q &
   $ sudo dtrace -l -P python$!  # or: dtrace -l -m python3.12

      ID   PROVIDER            MODULE                          FUNCTION NAME
   29568 python18035        python3.12                          collect gc-done
   29569 python18035        python3.12                          collect gc-start
   29570 python18035        python3.12                          import import-find-load-start
   29571 python18035        python3.12                          import import-find-load-done
   29572 python18035        python3.12                          audit audit

On Linux, you can verify if the SystemTap static markers are present in
the built binary by seeing if it contains a ".note.stapsdt" section.

::

   $ readelf -S ./python | grep .note.stapsdt
   [30] .note.stapsdt        NOTE         0000000000000000 00308d78

If you've built Python as a shared library
(with the :option:`--enable-shared` configure option), you
need to look instead within the shared library.  For example::

   $ readelf -S libpython3.3dm.so.1.0 | grep .note.stapsdt
   [29] .note.stapsdt        NOTE         0000000000000000 00365b68

Sufficiently modern readelf can print the metadata::

    $ readelf -n ./python

    Displaying notes found at file offset 0x00000254 with length 0x00000020:
        Owner                 Data size          Description
        GNU                  0x00000010          NT_GNU_ABI_TAG (ABI version tag)
            OS: Linux, ABI: 2.6.32

    Displaying notes found at file offset 0x00000274 with length 0x00000024:
        Owner                 Data size          Description
        GNU                  0x00000014          NT_GNU_BUILD_ID (unique build ID bitstring)
            Build ID: df924a2b08a7e89f6e11251d4602022977af2670

    Displaying notes found at file offset 0x002d6c30 with length 0x00000144:
        Owner                 Data size          Description
        stapsdt              0x00000031          NT_STAPSDT (SystemTap probe descriptors)
            Provider: python
            Name: gc__start
            Location: 0x00000000004371c3, Base: 0x0000000000630ce2, Semaphore: 0x00000000008d6bf6
            Arguments: -4@%ebx
        stapsdt              0x00000030          NT_STAPSDT (SystemTap probe descriptors)
            Provider: python
            Name: gc__done
            Location: 0x00000000004374e1, Base: 0x0000000000630ce2, Semaphore: 0x00000000008d6bf8
            Arguments: -8@%rax
        stapsdt              0x00000045          NT_STAPSDT (SystemTap probe descriptors)
            Provider: python
            Name: import__find__load__start
            Location: 0x000000000053db6c, Base: 0x0000000000630ce2, Semaphore: 0x00000000008d6be8
            Arguments: 8@%rbp
        stapsdt              0x00000046          NT_STAPSDT (SystemTap probe descriptors)
            Provider: python
            Name: import__find__load__done
            Location: 0x000000000053dba8, Base: 0x0000000000630ce2, Semaphore: 0x00000000008d6bea
            Arguments: 8@%rbp 8@%r12
        stapsdt              0x00000047          NT_STAPSDT (SystemTap probe descriptors)
            Provider: python
            Name: audit
            Location: 0x000000000053dba8, Base: 0x0000000000630ce2, Semaphore: 0x00000000008d6bec
            Arguments: 8@%rbp 8@%r12

The above metadata contains information for SystemTap describing how it
can patch strategically placed machine code instructions to enable the
tracing hooks used by a SystemTap script.


Static DTrace probes
--------------------

The following example DTrace script can be used to monitor garbage collection
activity in a Python script:

.. code-block:: none

    python$target:::gc-start
    {
            printf("%d: GC started (generation %d)\n", timestamp, arg0);
    }

    python$target:::gc-done
    {
            printf("%d: GC completed (%d objects collected)\n", timestamp, arg0);
    }

    python$target:::import-find-load-start
    {
            printf("%d: Import started: %s\n", timestamp, copyinstr(arg0));
    }

    python$target:::import-find-load-done
    {
            printf("%d: Import %s: %s\n", timestamp,
                   arg1 ? "successful" : "failed", copyinstr(arg0));
    }

    python$target:::audit
    {
            printf("%d: Audit event: %s\n", timestamp, copyinstr(arg0));
    }

It can be invoked like this::

  $ sudo dtrace -q -s monitoring.d -c "python3.12 script.py"

The output looks like this:

.. code-block:: none

    156641360502280: Import started: sys
    156641360518804: Import successful: sys
    156641360532797: Import started: os
    156641360546807: Import successful: os
    156641360563367: GC started (generation 0)
    156641360578365: GC completed (15 objects collected)
    156641360591757: Audit event: open
    156641360605556: Import started: json
    156641360617482: Import successful: json


Static SystemTap markers
------------------------

The low-level way to use the SystemTap integration is to use the static
markers directly.  This requires you to explicitly state the binary file
containing them.

For example, this SystemTap script can be used to monitor module imports
and garbage collection in a Python script:

.. code-block:: none

   probe process("python").mark("import__find__load__start") {
        modulename = user_string($arg1);
        printf("%s Import started: %s\\n",
               ctime(gettimeofday_s()), modulename);
   }

   probe process("python").mark("import__find__load__done") {
       modulename = user_string($arg1);
       found = $arg2;
       printf("%s Import %s: %s\\n",
              ctime(gettimeofday_s()),
              found ? "successful" : "failed", modulename);
   }

   probe process("python").mark("gc__start") {
       generation = $arg1;
       printf("%s GC started (generation %d)\\n",
              ctime(gettimeofday_s()), generation);
   }

   probe process("python").mark("gc__done") {
       collected = $arg1;
       printf("%s GC completed (%d objects collected)\\n",
              ctime(gettimeofday_s()), collected);
   }

It can be invoked like this::

   $ stap \
     monitor-python.stp \
     -c "./python test.py"

The output shows import and garbage collection activity:

.. code-block:: none

   Mon Sep 26 10:15:23 2025 Import started: sys
   Mon Sep 26 10:15:23 2025 Import successful: sys
   Mon Sep 26 10:15:23 2025 Import started: os
   Mon Sep 26 10:15:23 2025 Import successful: os
   Mon Sep 26 10:15:24 2025 GC started (generation 0)
   Mon Sep 26 10:15:24 2025 GC completed (15 objects collected)

For a :option:`--enable-shared` build of CPython, the markers are contained within the
libpython shared library, and the probe's dotted path needs to reflect this. For
example, this line from the above example:

.. code-block:: none

   probe process("python").mark("function__entry") {

should instead read:

.. code-block:: none

   probe process("python").library("libpython3.12dm.so.1.0").mark("gc__start") {

(assuming a :ref:`debug build <debug-build>` of CPython 3.12)


.. _static-markers:

Available static markers
------------------------

.. note::

   The ``function__entry``, ``function__return``, and ``line`` markers were
   removed in Python 3.12 due to the implementation of :pep:`669` (Low Impact
   Monitoring for CPython). For function-level monitoring, consider using
   Python's built-in profiling tools or the new monitoring APIs instead.

.. object:: gc__start(int generation)

   Fires when the Python interpreter starts a garbage collection cycle.
   ``arg0`` is the generation to scan, like :func:`gc.collect`.

.. object:: gc__done(long collected)

   Fires when the Python interpreter finishes a garbage collection
   cycle. ``arg0`` is the number of collected objects.

.. object:: import__find__load__start(str modulename)

   Fires before :mod:`importlib` attempts to find and load the module.
   ``arg0`` is the module name.

   .. versionadded:: 3.7

.. object:: import__find__load__done(str modulename, int found)

   Fires after :mod:`importlib`'s find_and_load function is called.
   ``arg0`` is the module name, ``arg1`` indicates if module was
   successfully loaded.

   .. versionadded:: 3.7


.. object:: audit(str event, void *tuple)

   Fires when :func:`sys.audit` or :c:func:`PySys_Audit` is called.
   ``arg0`` is the event name as C string, ``arg1`` is a :c:type:`PyObject`
   pointer to a tuple object.

   .. versionadded:: 3.8


SystemTap Tapsets
-----------------

The higher-level way to use the SystemTap integration is to use a "tapset":
SystemTap's equivalent of a library, which hides some of the lower-level
details of the static markers.

Here is a tapset file for the available markers, based on a non-shared build of CPython:

.. code-block:: none

    /*
       Provide higher-level wrapping around the remaining Python markers
     */
    probe python.gc.start = process("python").mark("gc__start")
    {
        generation = $arg1;
    }
    probe python.gc.done = process("python").mark("gc__done")
    {
        collected = $arg1;
    }
    probe python.import.start = process("python").mark("import__find__load__start")
    {
        modulename = user_string($arg1);
    }
    probe python.import.done = process("python").mark("import__find__load__done")
    {
        modulename = user_string($arg1);
        found = $arg2;
    }
    probe python.audit = process("python").mark("audit")
    {
        event = user_string($arg1);
    }

If this file is installed in SystemTap's tapset directory (e.g.
``/usr/share/systemtap/tapset``), then these additional probepoints become
available:

.. object:: python.gc.start(int generation)

   This probe point indicates that garbage collection has started for the
   specified generation.

.. object:: python.gc.done(int collected)

   This probe point indicates that garbage collection has completed,
   with the number of objects collected.

.. object:: python.import.start(str modulename)

   This probe point indicates that module import has started.

.. object:: python.import.done(str modulename, int found)

   This probe point indicates that module import has completed,
   with a flag indicating success or failure.

.. object:: python.audit(str event)

   This probe point fires when an audit event occurs.


Examples
--------
This SystemTap script uses the tapset above to monitor Python import and
garbage collection activity:

.. code-block:: none

    probe python.import.start
    {
      printf("%s Import starting: %s\n",
             ctime(gettimeofday_s()), modulename);
    }

    probe python.import.done
    {
      printf("%s Import %s: %s\n",
             ctime(gettimeofday_s()),
             found ? "successful" : "failed", modulename);
    }

    probe python.gc.start
    {
      printf("%s GC starting (generation %d)\n",
             ctime(gettimeofday_s()), generation);
    }

    probe python.gc.done
    {
      printf("%s GC completed (%d objects collected)\n",
             ctime(gettimeofday_s()), collected);
    }


The following script provides a summary of Python activity, showing import
and garbage collection statistics:

.. code-block:: none

    global import_count, gc_collections, gc_total_collected;

    probe python.import.done
    {
        if (found) import_count++;
    }

    probe python.gc.done
    {
        gc_collections++;
        gc_total_collected += collected;
    }

    probe timer.ms(5000) {
        printf("\033[2J\033[1;1H") /* clear screen */
        printf("Python Activity Summary:\n");
        printf("Successful imports: %d\n", import_count);
        printf("GC collections: %d\n", gc_collections);
        printf("Total objects collected: %d\n", gc_total_collected);
        printf("Average objects per collection: %d\n",
               gc_collections ? gc_total_collected / gc_collections : 0);
    }

