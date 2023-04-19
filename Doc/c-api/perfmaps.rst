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

.. c:function:: int _PyOS_PerfMapState_Init(void)

   Open the ``/tmp/perf-$pid.map`` file, unless it's already opened, and create
   a lock to ensure thread-safe writes to the file (provided the writes are
   done through :c:func:`PyOS_WritePerfMapEntry`). Normally, there's no need
   to call this explicitly, and it is safe to directly use :c:func:`PyOS_WritePerfMapEntry`
   in your code. If the state isn't already initialized, it will be created on
   the first call.

.. c:function:: int PyOS_WritePerfMapEntry(const void *code_addr, unsigned int code_size, const char *entry_name)

   Write one single entry to the ``/tmp/perf-$pid.map`` file. This function is
   thread safe. Here is what an example entry looks like::

      # address      size  name
      0x7f3529fcf759 b     py::bar:/run/t.py

   Extensions are encouraged to directly call this API when needed, instead of
   separately initializing the state by calling :c:func:`_PyOS_PerfMapState_Init`.

.. c:function:: int _PyOS_PerfMapState_Fini(void)

   Close the perf map file, which was opened in ``_PyOS_PerfMapState_Init``. This
   API is called by the runtime itself, during interpreter shut-down. In general,
   there shouldn't be a reason to explicitly call this, except to handle specific
   scenarios such as forking.
