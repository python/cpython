.. currentmodule:: asyncio


======
Runner
======

**Source code:** :source:`Lib/asyncio/runners.py`

------------------------------------

*Runner* context manager is used for two purposes:

1. Providing a primitive for event loop initialization and finalization with correct
   resources cleanup (cancelling background tasks, shutdowning the default thread-pool
   executor and pending async generators, etc.)

2. Processing *context variables* between async function calls.

:func:`asyncio.run` is used for running asyncio code usually, but sometimes several
top-level async calls are needed in the same loop and context instead of the
single ``main()`` call provided by :func:`asyncio.run`.

For example, there is a synchronous unittest library or console framework that should
work with async code.

A code that The following examples are equal:

.. code:: python

   async def main():
       ...

   asyncio.run(main())


Usually,

.. rubric:: Preface

The event loop is the core of every asyncio application.
Event loops run asynchronous tasks and callbacks, perform network
IO operations, and run subprocesses.

Application developers should typically use the high-level asyncio functions,
such as :func:`asyncio.run`, and should rarely need to reference the loop
object or call its methods.  This section is intended mostly for authors
of lower-level code, libraries, and frameworks, who need finer control over
the event loop behavior.
