:mod:`!interpreters` --- Multiple Interpreters in the Same Process
==================================================================

.. module:: interpreters
   :synopsis: Multiple Interpreters in the Same Process

.. moduleauthor:: Eric Snow <ericsnowcurrently@gmail.com>
.. sectionauthor:: Eric Snow <ericsnowcurrently@gmail.com>

.. versionadded:: 3.14

**Source code:** :source:`Lib/interpreters/__init__.py`

--------------


Introduction
------------

The :mod:`!interpreters` module constructs higher-level interfaces
on top of the lower level :mod:`!_interpreters` module.

.. XXX Add references to the upcoming HOWTO docs in the seealso block.

.. seealso::

   :ref:`isolating-extensions-howto`
       how to update an extension module to support multiple interpreters

   :pep:`554`

   :pep:`734`

   :pep:`684`

.. XXX Why do we disallow multiple interpreters on WASM?

.. include:: ../includes/wasm-notavail.rst


Key Details
-----------

Before we dive into examples, there are a small number of details
to keep in mind about using multiple interpreters:

* isolated, by default
* no implicit threads
* not all PyPI packages support use in multiple interpreters yet

.. XXX Are there other relevant details to list?


API Summary
-----------

+----------------------------------+----------------------------------------------+
| signature                        | description                                  |
+==================================+==============================================+
| ``list_all() -> [Interpreter]``  | Get all existing interpreters.               |
+----------------------------------+----------------------------------------------+
| ``get_current() -> Interpreter`` | Get the currently running interpreter.       |
+----------------------------------+----------------------------------------------+
| ``get_main() -> Interpreter``    | Get the main interpreter.                    |
+----------------------------------+----------------------------------------------+
| ``create() -> Interpreter``      | Initialize a new (idle) Python interpreter.  |
+----------------------------------+----------------------------------------------+

|

+---------------------------------------------------+---------------------------------------------------+
| signature                                         | description                                       |
+===================================================+===================================================+
| ``class Interpreter``                             | A single interpreter.                             |
+---------------------------------------------------+---------------------------------------------------+
| ``.id``                                           | The interpreter's ID (read-only).                 |
+---------------------------------------------------+---------------------------------------------------+
| ``.whence``                                       | Where the interpreter came from (read-only).      |
+---------------------------------------------------+---------------------------------------------------+
| ``.is_running() -> bool``                         | Is the interpreter currently executing code       |
|                                                   | in its :mod:`!__main__` module?                   |
+---------------------------------------------------+---------------------------------------------------+
| ``.close()``                                      | Finalize and destroy the interpreter.             |
+---------------------------------------------------+---------------------------------------------------+
| ``.prepare_main(ns=None, **kwargs)``              | Bind "shareable" objects in :mod:`!__main__`.     |
+---------------------------------------------------+---------------------------------------------------+
| ``.exec(src_str, /, dedent=True)``                | | Run the given source code in the interpreter    |
|                                                   | | (in the current thread).                        |
+---------------------------------------------------+---------------------------------------------------+
| ``.call(callable, /, *args, **kwargs)``           | | Run the given function in the interpreter       |
|                                                   | | (in the current thread).                        |
+---------------------------------------------------+---------------------------------------------------+
| ``.call_in_thread(callable, /, *args, **kwargs)`` | | Run the given function in the interpreter       |
|                                                   | | (in a new thread).                              |
+---------------------------------------------------+---------------------------------------------------+

Exceptions:

+--------------------------+------------------+---------------------------------------------------+
| class                    | base class       | description                                       |
+==========================+==================+===================================================+
| InterpreterError         | Exception        | An interpreter-related error happened.            |
+--------------------------+------------------+---------------------------------------------------+
| InterpreterNotFoundError | InterpreterError | The targeted interpreter no longer exists.        |
+--------------------------+------------------+---------------------------------------------------+
| ExecutionFailed          | InterpreterError | The running code raised an uncaught exception.    |
+--------------------------+------------------+---------------------------------------------------+
| NotShareableError        | TypeError        | The object cannot be sent to another interpreter. |
+--------------------------+------------------+---------------------------------------------------+

.. XXX Document the ExecutionFailed attrs.


.. XXX Add API summary for communicating between interpreters.


.. _interp-examples:

Basic Usage
-----------

Creating an interpreter and running code in it:

::

    import interpreters

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


.. XXX Describe module functions in more detail.


.. XXX Describe module types in more detail.


.. XXX Explain about object "sharing".
