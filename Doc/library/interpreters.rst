:mod:`interpreters` --- high-level interpreters API
===================================================

.. module:: interpreters
   :synopsis: High-level interpreters API.

**Source code:** :source:`Lib/interpreters.py`

.. versionadded:: 3,7

--------------

This module provides high-level interfaces for interacting with
the Python interpreters.  It is built on top of the lower- level
:mod:`_interpreters` module.

.. XXX Summarize :ref:`_sub-interpreter-support` here.

This module defines the following functions:

.. function:: enumerate()

   Return a list of all existing interpreters.


.. function:: get_current()

   Return the currently running interpreter.


.. function:: get_main()

   Return the main interpreter.


.. function:: create()

   Initialize a new Python interpreter and return it.  The
   interpreter will be created in the current thread and will remain
   idle until something is run in it.


This module also defines the following class:

.. class:: Interpreter(id)

   ``id`` is the interpreter's ID.

   .. property:: id

      The interpreter's ID.

   .. method:: is_running()

      Return whether or not the interpreter is currently running.

   .. method:: destroy()

      Finalize and destroy the interpreter.

   .. method:: run(code)

      Run the provided Python code in the interpreter, in the current
      OS thread.  Supported code: source text.
