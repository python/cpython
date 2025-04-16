.. _freethreading-python-howto:

**********************************************
Python experimental support for free threading
**********************************************

Starting with the 3.13 release, CPython has experimental support for a build of
Python called :term:`free threading` where the :term:`global interpreter lock`
(GIL) is disabled.  Free-threaded execution allows for full utilization of the
available processing power by running threads in parallel on available CPU cores.
While not all software will benefit from this automatically, programs
designed with threading in mind will run faster on multi-core hardware.

**The free-threaded mode is experimental** and work is ongoing to improve it:
expect some bugs and a substantial single-threaded performance hit.

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
<https://py-free-threading.github.io/installing_cpython/>`_, a
community-maintained installation guide for installing free-threaded Python.

When building CPython from source, the :option:`--disable-gil` configure option
should be used to build a free-threaded Python interpreter.


Identifying free-threaded Python
================================

To check if the current interpreter supports free-threading, :option:`python -VV <-V>`
and :data:`sys.version` contain "experimental free-threading build".
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

The free-threaded build of the 3.13 release makes some objects :term:`immortal`.
Immortal objects are not deallocated and have reference counts that are
never modified.  This is done to avoid reference count contention that would
prevent efficient multi-threaded scaling.

An object will be made immortal when a new thread is started for the first time
after the main thread is running.  The following objects are immortalized:

* :ref:`function <user-defined-funcs>` objects declared at the module level
* :ref:`method <instance-methods>` descriptors
* :ref:`code <code-objects>` objects
* :term:`module` objects and their dictionaries
* :ref:`classes <classes>` (type objects)

Because immortal objects are never deallocated, applications that create many
objects of these types may see increased memory usage.  This is expected to be
addressed in the 3.14 release.

Additionally, numeric and string literals in the code as well as strings
returned by :func:`sys.intern` are also immortalized.  This behavior is
expected to remain in the 3.14 free-threaded build.


Frame objects
-------------

It is not safe to access :ref:`frame <frame-objects>` objects from other
threads and doing so may cause your program to crash .  This means that
:func:`sys._current_frames` is generally not safe to use in a free-threaded
build.  Functions like :func:`inspect.currentframe` and :func:`sys._getframe`
are generally safe as long as the resulting frame object is not passed to
another thread.

Iterators
---------

Sharing the same iterator object between multiple threads is generally not
safe and threads may see duplicate or missing elements when iterating or crash
the interpreter.


Single-threaded performance
---------------------------

The free-threaded build has additional overhead when executing Python code
compared to the default GIL-enabled build.  In 3.13, this overhead is about
40% on the `pyperformance <https://pyperformance.readthedocs.io/>`_ suite.
Programs that spend most of their time in C extensions or I/O will see
less of an impact.  The largest impact is because the specializing adaptive
interpreter (:pep:`659`) is disabled in the free-threaded build.  We expect
to re-enable it in a thread-safe way in the 3.14 release.  This overhead is
expected to be reduced in upcoming Python release.   We are aiming for an
overhead of 10% or less on the pyperformance suite compared to the default
GIL-enabled build.
