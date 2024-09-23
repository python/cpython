.. _freethreading-python-howto:

*********************************
Python support for free threading
*********************************

Starting with the 3.13 release, CPython has experimental support for running
with the :term:`global interpreter lock` (GIL) disabled in a configuration
called :term:`free threading`.  This document describes the implications of
free threading for Python code.  See :ref:`freethreading-extensions-howto` for
information on how to write C extensions that support the free-threaded build.


Installation
============

Starting with Python 3.13, the official macOS and Windows installers
optionally support installing free-threaded Python binaries.  The installers
are available at https://www.python.org/downloads/.

.. seealso::

   `Installing a Free-Threaded Python
   <https://py-free-threading.github.io/installing_cpython/>`_:
   A community-maintained installation guide for installing free-threaded
   Python.


Identifying free-threaded Python
================================

The free-threaded build of CPython can optionally run with the global
interpreter lock enabled, such as when :envvar:`PYTHON_GIL` is set to ``1``,
or when importing an extension module that requires the GIL.

The :func:`sys._is_gil_enabled` function will return ``False`` if the global
interpreter lock is currently disabled.  This is the recommended mechanism for
decisions like whether to use multithreading or multiprocessing.

The ``sysconfig.get_config_var("Py_GIL_DISABLED")`` configuration variable can
be used to determine whether the build supports free threading.  If the variable
is set to ``1``, then the build supports free threading.  This is the recommended
mechanism for decisions related to the build configuration.


Thread safety
=============

The free-threaded build of CPython aims to provide similar thread-safety
behavior at the Python level to the GIL-enabled build.  Built-in
types like :class:`dict`, :class:`list`, and :class:`set` use internal locks
to protect against concurrent modifications in ways that behave similarly to
the GIL.  However, Python has not historically guaranteed specific behavior for
concurrent modifications to these built-in types, so this should be treated
as a description of the current implementation, not a guarantee of future
behavior.

.. note::

   It's recommended to use the :class:`threading.Lock` or other synchronization
   primitives instead of relying on the internal locks of built-in types, when
   possible.



Known limitations
=================

This section describes known limitations of the free-threaded CPython build.

Immortalization
---------------

The free-threaded build of the 3.13 release makes some objects :term:`immortal`
in order to avoid reference count contention that would prevent efficient
multi-threaded scaling.  This means that these objects are never deallocated.
This is expected to be addressed in Python 3.14 with
`deferred reference counting <https://peps.python.org/pep-0703/#deferred-reference-counting>`_.

The objects that are immortalized are:

* :ref:`function <user-defined-funcs>` objects declared at the module level
* :ref:`method <instance-methods>` descriptors
* :ref:`code <code-objects>` objects
* :term:`module` objects and their dictionaries
* :ref:`classes <classes>` (type objects)

The immortalization of these objects happens the first time a thread is started
after the main thread.

Additionally, numeric and string literals in the code as well as strings
returned by :func:`sys.intern` are also immortalized.  This behavior is
expected to remain in the 3.14 free-threaded build.


Frame objects
-------------

It is not safe to access :ref:`frame <frame-objects>` objects from other
threads.  This means that :func:`sys._current_frames` is generally not safe to
use in a free-threaded build.

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
less of an impact.  This overhead is expected to be reduced in the Python
3.14.
