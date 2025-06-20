:mod:`!concurrent.interpreters` --- Multiple interpreters in the same process
=============================================================================

.. module:: concurrent.interpreters
   :synopsis: Multiple interpreters in the same process

.. moduleauthor:: Eric Snow <ericsnowcurrently@gmail.com>
.. sectionauthor:: Eric Snow <ericsnowcurrently@gmail.com>

.. versionadded:: 3.14

**Source code:** :source:`Lib/concurrent/interpreters.py`

--------------


Introduction
------------

The :mod:`!concurrent.interpreters` module constructs higher-level
interfaces on top of the lower level :mod:`!_interpreters` module.

.. XXX Add references to the upcoming HOWTO docs in the seealso block.

.. seealso::

   :ref:`isolating-extensions-howto`
       how to update an extension module to support multiple interpreters

   :pep:`554`

   :pep:`734`

   :pep:`684`

.. XXX Why do we disallow multiple interpreters on WASM?

.. include:: ../includes/wasm-notavail.rst


Key details
-----------

Before we dive into examples, there are a small number of details
to keep in mind about using multiple interpreters:

* isolated, by default
* no implicit threads
* not all PyPI packages support use in multiple interpreters yet

.. XXX Are there other relevant details to list?

In the context of multiple interpreters, "isolated" means that
different interpreters do not share any state.  In practice, there is some
process-global data they all share, but that is managed by the runtime.


Reference
---------

This module defines the following functions:

.. function:: list_all()

   Return a :class:`list` of :class:`Interpreter` objects,
   one for each existing interpreter.

.. function:: get_current()

   Return an :class:`Interpreter` object for the currently running
   interpreter.

.. function:: get_main()

   Return an :class:`Interpreter` object for the main interpreter.

.. function:: create()

   Initialize a new (idle) Python interpreter
   and return a :class:`Interpreter` object for it.


Interpreter objects
^^^^^^^^^^^^^^^^^^^

.. class:: Interpreter(id)

   A single interpreter in the current process.

   Generally, :class:`Interpreter` shouldn't be called directly.
   Instead, use :func:`create` or one of the other module functions.

   .. attribute:: id

      (read-only)

      The interpreter's ID.

   .. attribute:: whence

      (read-only)

      A string describing where the interpreter came from.

   .. method:: is_running()

      Return ``True`` if the interpreter is currently executing code
      in its :mod:`!__main__` module and ``False`` otherwise.

   .. method:: close()

      Finalize and destroy the interpreter.

   .. method:: prepare_main(ns=None, **kwargs)

      Bind "shareable" objects in the interpreter's
      :mod:`!__main__` module.

   .. method:: exec(code, /, dedent=True)

      Run the given source code in the interpreter (in the current thread).

   .. method:: call(callable, /, *args, **kwargs)

      Return the result of calling running the given function in the
      interpreter (in the current thread).

   .. method:: call_in_thread(callable, /, *args, **kwargs)

      Run the given function in the interpreter (in a new thread).

Exceptions
^^^^^^^^^^

.. exception:: InterpreterError

   This exception, a subclass of :exc:`Exception`, is raised when
   an interpreter-related error happens.

.. exception:: InterpreterNotFoundError

   This exception, a subclass of :exc:`InterpreterError`, is raised when
   the targeted interpreter no longer exists.

.. exception:: ExecutionFailed

   This exception, a subclass of :exc:`InterpreterError`, is raised when
   the running code raised an uncaught exception.

   .. attribute:: excinfo

      A basic snapshot of the exception raised in the other interpreter.

.. XXX Document the excinfoattrs?

.. exception:: NotShareableError

   This exception, a subclass of :exc:`TypeError`, is raised when
   an object cannot be sent to another interpreter.


.. XXX Add functions for communicating between interpreters.


Basic usage
-----------

Creating an interpreter and running code in it::

    from concurrent import interpreters

    interp = interpreters.create()

    # Run in the current OS thread.

    interp.exec('print("spam!")')

    interp.exec("""if True:
        print('spam!')
        """)

    from textwrap import dedent
    interp.exec(dedent("""
        print('spam!')
        """))

    def run():
        print('spam!')

    interp.call(run)

    # Run in new OS thread.

    t = interp.call_in_thread(run)
    t.join()


.. XXX Explain about object "sharing".
