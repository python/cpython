.. _freethreading-python-howto:

*********************************
Python support for free threading
*********************************

Starting with the 3.13 release, CPython has support for a build of
Python called :term:`free threading` where the :term:`global interpreter lock`
(GIL) is disabled.  Free-threaded execution allows for full utilization of the
available processing power by running threads in parallel on available CPU cores.
While not all software will benefit from this automatically, programs
designed with threading in mind will run faster on multi-core hardware.

Some third-party packages, in particular ones
with an :term:`extension module`, may not be ready for use in a
free-threaded build, and will re-enable the :term:`GIL`.

This document describes the implications of free threading
for Python code.  See :ref:`freethreading-extensions-howto` for information on
how to write C extensions that support the free-threaded build.

.. seealso::

   :pep:`703` – Making the Global Interpreter Lock Optional in CPython for an
   overall description of free-threaded Python.


Installation
============

Starting with Python 3.13, the official macOS and Windows installers
optionally support installing free-threaded Python binaries.  The installers
are available at https://www.python.org/downloads/.

For information on other platforms, see the `Installing a Free-Threaded Python
<https://py-free-threading.github.io/installing-cpython/>`_, a
community-maintained installation guide for installing free-threaded Python.

When building CPython from source, the :option:`--disable-gil` configure option
should be used to build a free-threaded Python interpreter.


Identifying free-threaded Python
================================

To check if the current interpreter supports free-threading, :option:`python -VV <-V>`
and :data:`sys.version` contain "free-threading build".
The new :func:`sys._is_gil_enabled` function can be used to check whether
the GIL is actually disabled in the running process.

The ``sysconfig.get_config_var("Py_GIL_DISABLED")`` configuration variable can
be used to determine whether the build supports free threading.  If the variable
is set to ``1``, then the build supports free threading.  This is the recommended
mechanism for decisions related to the build configuration.


The global interpreter lock in free-threaded Python
===================================================

Free-threaded builds of CPython support optionally running with the GIL enabled
at runtime using the environment variable :envvar:`PYTHON_GIL` or
the command-line option :option:`-X gil`.

The GIL may also automatically be enabled when importing a C-API extension
module that is not explicitly marked as supporting free threading.  A warning
will be printed in this case.

In addition to individual package documentation, the following websites track
the status of popular packages support for free threading:

* https://py-free-threading.github.io/tracking/
* https://hugovk.github.io/free-threaded-wheels/


Thread safety
=============

The free-threaded build of CPython aims to provide similar thread-safety
behavior at the Python level to the default GIL-enabled build.  Built-in
types like :class:`dict`, :class:`list`, and :class:`set` use internal locks
to protect against concurrent modifications in ways that behave similarly to
the GIL.  However, Python has not historically guaranteed specific behavior for
concurrent modifications to these built-in types, so this should be treated
as a description of the current implementation, not a guarantee of current or
future behavior.

.. note::

   It's recommended to use the :class:`threading.Lock` or other synchronization
   primitives instead of relying on the internal locks of built-in types, when
   possible.


Known limitations
=================

This section describes known limitations of the free-threaded CPython build.

Immortalization
---------------

In the free-threaded build, some objects are :term:`immortal`.
Immortal objects are not deallocated and have reference counts that are
never modified.  This is done to avoid reference count contention that would
prevent efficient multi-threaded scaling.

As of the 3.14 release, immortalization is limited to:

* Code constants: numeric literals, string literals, and tuple literals
  composed of other constants.
* Strings interned by :func:`sys.intern`.


Frame objects
-------------

It is not safe to access :attr:`frame.f_locals` from a :ref:`frame <frame-objects>`
object if that frame is currently executing in another thread, and doing so may
crash the interpreter.


Iterators
---------

It is generally not thread-safe to access the same iterator object from
multiple threads concurrently, and threads may see duplicate or missing
elements.


Single-threaded performance
---------------------------

The free-threaded build has additional overhead when executing Python code
compared to the default GIL-enabled build.  The amount of overhead depends
on the workload and hardware.  On the pyperformance benchmark suite, the
average overhead ranges from about 1% on macOS aarch64 to 8% on x86-64 Linux
systems.


Behavioral changes
==================

This section describes CPython behavioural changes with the free-threaded
build.


Context variables
-----------------

In the free-threaded build, the flag :data:`~sys.flags.thread_inherit_context`
is set to true by default which causes threads created with
:class:`threading.Thread` to start with a copy of the
:class:`~contextvars.Context()` of the caller of
:meth:`~threading.Thread.start`.  In the default GIL-enabled build, the flag
defaults to false so threads start with an
empty :class:`~contextvars.Context()`.


Warning filters
---------------

In the free-threaded build, the flag :data:`~sys.flags.context_aware_warnings`
is set to true by default.  In the default GIL-enabled build, the flag defaults
to false.  If the flag is true then the :class:`warnings.catch_warnings`
context manager uses a context variable for warning filters.  If the flag is
false then :class:`~warnings.catch_warnings` modifies the global filters list,
which is not thread-safe.  See the :mod:`warnings` module for more details.
