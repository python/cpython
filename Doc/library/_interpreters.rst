:mod:`_interpreters` --- Low-level interpreters API
===================================================

.. module:: _interpreters
   :synopsis: Low-level interpreters API.

.. versionadded:: 3,7

--------------

This module provides low-level primitives for working with multiple
Python interpreters in the same runtime in the current process.

More information about (sub)interpreters is found at
:ref:`sub-interpreter-support`, including what data is shared between
interpreters and what is unique.  Note particularly that interpreters
aren't inherently threaded, even though they track and manage Python
threads.  To run code in an interpreter in a different OS thread, call
:func:`run_string` in a function that you run in a new Python thread.
For example::

   id = _interpreters.create()
   def f():
       _interpreters.run_string(id, 'print("in a thread")')

   t = threading.Thread(target=f)
   t.start()

This module is optional.  It is provided by Python implementations which
support multiple interpreters.

It defines the following functions:

.. function:: enumerate()

   Return a list of the IDs of every existing interpreter.


.. function:: get_current()

   Return the ID of the currently running interpreter.


.. function:: get_main()

   Return the ID of the main interpreter.


.. function:: is_running(id)

   Return whether or not the identified interpreter is currently
   running any code.


.. function:: create()

   Initialize a new Python interpreter and return its identifier.  The
   interpreter will be created in the current thread and will remain
   idle until something is run in it.


.. function:: destroy(id)

   Finalize and destroy the identified interpreter.


.. function:: run_string(id, command)

   A wrapper around :c:func:`PyRun_SimpleString` which runs the provided
   Python program in the main thread of the identified interpreter.
   Providing an invalid or unknown ID results in a RuntimeError,
   likewise if the main interpreter or any other running interpreter
   is used.

   Any value returned from the code is thrown away, similar to what
   threads do.  If the code results in an exception then that exception
   is raised in the thread in which run_string() was called, similar to
   how :func:`exec` works.  This aligns with how interpreters are not
   inherently threaded.  Note that SystemExit (as raised by sys.exit())
   is not treated any differently and will result in the process ending
   if not caught explicitly.


.. function:: run_string_unrestricted(id, command, ns=None)

   Like :c:func:`run_string` but returns the dict in which the code
   was executed.  It also supports providing a namespace that gets
   merged into the execution namespace before execution.  Note that
   this allows objects to leak between interpreters, which may not
   be desirable.
