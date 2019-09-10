:mod:`interpreters` --- High-level Sub-interpreters Module
==========================================================

.. module:: interpreters
   :synopsis: High-level Sub-Interpreters Module.

**Source code:** :source:`Lib/interpreters.py`

--------------

This module constructs higher-level interpreters interfaces on top of the lower
level :mod:`_interpreters` module.

.. versionchanged:: 3.9


This module defines the following functions:


.. function:: create()

   Create a new interpreter and return a unique generated ID.

.. function:: list_all()

   Return a list containing the ID of every existing interpreter.

.. function:: get_current()

   Return the ID of the currently running interpreter.

.. function:: destroy(id)

   Destroy the interpreter whose ID is *id*. Attempting to destroy the current
   interpreter results in a `RuntimeError`. So does an unrecognized ID.

.. function:: get_main()

   Return the ID of the main interpreter.

.. function:: run_string()

    Execute the provided string in the identified interpreter.
    See `PyRun_SimpleStrings`.
