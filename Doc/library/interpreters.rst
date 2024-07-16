:mod:`!interpreters` --- Multiple Interpreters in the Same Process
==================================================================

XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

.. module:: interpreters
   :synopsis: Multiple Interpreters in the Same Process

.. moduleauthor:: Eric Snow <ericsnowcurrently@gmail.com>
.. sectionauthor:: Eric Snow <ericsnowcurrently@gmail.com>

.. versionadded:: 3.14

**Source code:** :source:`Lib/interpreters/__init__.py`

--------------


Introduction
------------

This module constructs higher-level interfaces on top of the lower
level ``_interpreters`` module.

.. seealso::

   :ref:`execcomponents`
       ...

   :mod:`interpreters.queues`
       cross-interpreter queues

   :mod:`interpreters.channels`
       cross-interpreter channels

   :ref:`multiple-interpreters-howto`
       how to use multiple interpreters

   :ref:`isolating-extensions-howto`
       how to update an extension module to support multiple interpreters

   :pep:`554`

   :pep:`734`

   :pep:`684`

.. include:: ../includes/wasm-notavail.rst


Key Details
-----------

Before we dive into examples, there are a small number of details
to keep in mind about using multiple interpreters:

* isolated, by default
* no implicit threads
* limited callables
* limited shareable objects
* not all PyPI packages support use in multiple interpreters yet
* ...


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

+------------------------------------+---------------------------------------------------+
| signature                          | description                                       |
+====================================+===================================================+
| ``class Interpreter``              | A single interpreter.                             |
+------------------------------------+---------------------------------------------------+
| ``.id``                            | The interpreter's ID (read-only).                 |
+------------------------------------+---------------------------------------------------+
| ``.whence``                        | Where the interpreter came from (read-only).      |
+------------------------------------+---------------------------------------------------+
| ``.is_running() -> bool``          | Is the interpreter currently executing code?      |
+------------------------------------+---------------------------------------------------+
| ``.close()``                       | Finalize and destroy the interpreter.             |
+------------------------------------+---------------------------------------------------+
| ``.prepare_main(**kwargs)``        | Bind "shareable" objects in ``__main__``.         |
+------------------------------------+---------------------------------------------------+
| ``.exec(src_str, /, dedent=True)`` | | Run the given source code in the interpreter    |
|                                    | | (in the current thread).                        |
+------------------------------------+---------------------------------------------------+
| ``.call(callable, /)``             | | Run the given function in the interpreter       |
|                                    | | (in the current thread).                        |
+------------------------------------+---------------------------------------------------+
| ``.call_in_thread(callable, /)``   | | Run the given function in the interpreter       |
|                                    | | (in a new thread).                              |
+------------------------------------+---------------------------------------------------+

Exceptions:

+--------------------------+------------------+---------------------------------------------------+
| class                    | base class       | description                                       |
+==========================+==================+===================================================+
| InterpreterError         | Exception        | An interpreter-relaed error happened.             |
+--------------------------+------------------+---------------------------------------------------+
| InterpreterNotFoundError | InterpreterError | The targeted interpreter no longer exists.        |
+--------------------------+------------------+---------------------------------------------------+
| ExecutionFailed          | InterpreterError | The running code raised an uncaught exception.    |
+--------------------------+------------------+---------------------------------------------------+
| NotShareableError        | Exception        | The object cannot be sent to another interpreter. |
+--------------------------+------------------+---------------------------------------------------+

ExecutionFailed
  excinfo
    type
      (builtin)
      __name__
      __qualname__
      __module__
    msg
    formatted
    errdisplay


For communicating between interpreters:

+-------------------------------------------------------------------+--------------------------------------------+
| signature                                                         | description                                |
+===================================================================+============================================+
| ``is_shareable(obj) -> Bool``                                     | | Can the object's data be passed          |
|                                                                   | | between interpreters?                    |
+-------------------------------------------------------------------+--------------------------------------------+
| ``create_queue(*, syncobj=False, unbounditems=UNBOUND) -> Queue`` | | Create a new queue for passing           |
|                                                                   | | data between interpreters.               |
+-------------------------------------------------------------------+--------------------------------------------+
| ``create_channel(*, unbounditems=UNBOUND) -> (RecvChannel, SendChannel)`` | | Create a new channel for passing           |
|                                                                   | | data between interpreters.               |
+-------------------------------------------------------------------+--------------------------------------------+

Also see :mod:`interpreters.queues` and :mod:`interpreters.channels`.


.. _interp-examples:

Basic Usage
-----------

Creating an interpreter and running code in it:

::

    import interpreters

    interp = interpreters.create()

    # Run in the current OS thread.

    interp.exec('print("spam!")')

    interp.exec("""
        print('spam!')
        """)

    def run():
        print('spam!')

    interp.call(run)

    # Run in new OS thread.

    t = interp.call_in_thread(run)
    t.join()

For additional examples, see :ref:`interp-queue-examples`,
:ref:`interp-channel-examples`, and :ref:`multiple-interpreters-howto`.


Functions
---------

This module defines the following functions:

...


.. _interp-shareable-objects

Shareable Objects
-----------------

A "shareable" object is one with a class that specifically supports
passing between interpreters.

The primary characteristic of shareable objects is that their values
are guaranteed to always be in sync when "shared" between interpreters.
Effectively, they will be strictly equivalent and functionally
identical.

That means one of the following is true:

* the object is actually shared by the interpreters
  (e.g. :const:`None`)
* the underlying data of the object is actually shared
  (e.g. :class:`memoryview`)
* for immutable objects, the underlying data was copied
  (e.g. :class:`str`)
* the underlying data of the corresponding object in each intepreter
is effectively always identical.


Passing a non-shareable object where one is expected results in a
:class:`NotShareableError` exception.  You can use
:func:`interpreters.is_shareable` to know ahead of time if an
object can be passed between interpreters.

By default, the following types are shareable:

* :const:`None`
* :class:`bool` (:const:`True` and :const:`False`)
* :class:`bytes`
* :class:`str`
* :class:`int`
* :class:`float`
* :class:`tuple` (of shareable objects)
* :class:`memoryview`
* :class:`interpreters.queues.Queue`
* :class:`interpreters.channels.Channel`
