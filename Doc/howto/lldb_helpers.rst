.. _lldb:

Debug CPython with LLDB
=======================

.. versionadded:: 3.16

The ``python_lldb.py`` extension adds Python object summaries and Python stack
commands to LLDB. It uses the same object layouts and command implementation
as the :ref:`GDB helpers <gdb>`. It is based on the original
`cpython-lldb <https://github.com/malor/cpython-lldb>`__ extension.

Setup
-----

You need LLDB with Python support and a CPython executable with debug symbols.
A build with :option:`--with-pydebug` provides these symbols. Compiler
optimizations can remove variables and prevent access to some frames.

The CPython build creates ``python_lldb.py`` beside the executable.
Keep this file with ``python-gdb.py`` (``python.exe-gdb.py`` on macOS builds
that use ``python.exe``).
LLDB does not load the helpers automatically.

Start LLDB and import the helpers::

   lldb ./python
   (lldb) command script import /path/to/build/python_lldb.py

You can also import ``Tools/lldb/libpython.py`` directly from the source tree.
Use the helpers from the same CPython version as the target executable.

Object summaries
----------------

LLDB shows Python values for ``PyObject *`` pointers, concrete object structs,
and ``_PyStackRef`` values. For example::

   (lldb) breakpoint set --name builtin_id
   (lldb) process launch -- -c 'id([1, "hello", None])'
   (lldb) frame variable v
   (PyObject *) v = 0x0000000123456780 [1, 'hello', None]

Python commands
---------------

The helpers add the commands of the :ref:`GDB helpers <gdb>`, with the same
arguments and Python output: ``py-bt``, ``py-bt-full``, ``py-up``,
``py-down``, ``py-list``, ``py-print``, and ``py-locals``.

``py-up`` and ``py-down`` behave as described in :ref:`gdb`. Native frame
numbers can differ between GDB and LLDB.

Use ``thread select`` to select a thread before a Python command::

   (lldb) thread select 2
   (lldb) py-bt

The helpers read process memory without calls to Python functions in the
target process. They also work with core files that contain the required
memory and debug symbols. ``py-list`` needs the same version of the source
files on the debugger's host.
