:mod:`_interpreters` --- Low-level interpreters API
===================================================

.. module:: _interpreters
   :synopsis: Low-level interpreters API.

.. versionadded:: 3,7

  :ref:`_sub-interpreter-support`

threading

--------------

This module provides low-level primitives for working with multiple
Python interpreters in the same process.

.. XXX The :mod:`interpreters` module provides an easier to use and
   higher-level API built on top of this module.

This module is optional.  It is provided by Python implementations which
support multiple interpreters.

.. XXX For systems lacking the :mod:`_interpreters` module, the
   :mod:`_dummy_interpreters` module is available.  It duplicates this
   module's interface and can be used as a drop-in replacement.

It defines the following functions:

.. function:: create()

   Initialize a new Python interpreter and return its identifier.  The
   interpreter will be created in the current thread and will remain
   idle until something is run in it.


.. function:: destroy(id)

   Finalize and destroy the identified interpreter.


.. function:: run_string(id, command)

   A wrapper around :c:func:`PyRun_SimpleString` which runs the provided
   Python program using the identified interpreter.  Providing an
   invalid or unknown ID results in a RuntimeError, likewise if the main
   interpreter or any other running interpreter is used.

   Any value returned from the code is thrown away, similar to what
   threads do.  If the code results in an exception then that exception
   is raised in the thread in which run_string() was called, similar to
   how :func:`exec` works.  This aligns with how interpreters are not
   inherently threaded.

.. XXX sys.exit() (and SystemExit) is swallowed?


.. function:: run_string_unrestricted(id, command, ns=None)

   Like :c:func:`run_string` but returns the dict in which the code
   was executed.  It also supports providing a namespace that gets
   merged into the execution namespace before execution.  Note that
   this allows objects to leak between interpreters, which may not
   be desirable.


**Caveats:**

* ...

