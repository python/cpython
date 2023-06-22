.. highlight:: c

.. _perfmaps:

Support for Perf Maps
----------------------

On supported platforms (as of this writing, only Linux), the runtime can take
advantage of *perf map files* to make Python functions visible to an external
profiling tool (such as `perf <https://perf.wiki.kernel.org/index.php/Main_Page>`_).
A running process may create a file in the ``/tmp`` directory, which contains entries
that can map a section of executable code to a name. This interface is described in the
`documentation of the Linux Perf tool <https://git.kernel.org/pub/scm/linux/
kernel/git/torvalds/linux.git/tree/tools/perf/Documentation/jit-interface.txt>`_.

In Python, these helper APIs can be used by libraries and features that rely
on generating machine code on the fly.

Note that holding the Global Interpreter Lock (GIL) is not required for these APIs.

.. c:function:: int PyUnstable_PerfMapState_Init(void)

   Open the ``/tmp/perf-$pid.map`` file, unless it's already opened, and create
   a lock to ensure thread-safe writes to the file (provided the writes are
   done through :c:func:`PyUnstable_WritePerfMapEntry`). Normally, there's no need
   to call this explicitly; just use :c:func:`PyUnstable_WritePerfMapEntry`
   and it will initialize the state on first call.

   Returns ``0`` on success, ``-1`` on failure to create/open the perf map file,
   or ``-2`` on failure to create a lock. Check ``errno`` for more information
   about the cause of a failure.

.. c:function:: int PyUnstable_WritePerfMapEntry(const void *code_addr, unsigned int code_size, const char *entry_name)

   Write one single entry to the ``/tmp/perf-$pid.map`` file. This function is
   thread safe. Here is what an example entry looks like::

      # address      size  name
      7f3529fcf759 b     py::bar:/run/t.py

   Will call :c:func:`PyUnstable_PerfMapState_Init` before writing the entry, if
   the perf map file is not already opened. Returns ``0`` on success, or the
   same error codes as :c:func:`PyUnstable_PerfMapState_Init` on failure.

.. c:function:: void PyUnstable_PerfMapState_Fini(void)

   Close the perf map file opened by :c:func:`PyUnstable_PerfMapState_Init`.
   This is called by the runtime itself during interpreter shut-down. In
   general, there shouldn't be a reason to explicitly call this, except to
   handle specific scenarios such as forking.
