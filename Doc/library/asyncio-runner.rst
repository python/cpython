.. currentmodule:: asyncio


======
Runner
======

**Source code:** :source:`Lib/asyncio/runners.py`

------------------------------------


:func:`asyncio.run` provides a convinient very high-level API for running asyncio code.

It is the preferred approach that satisfies almost all use cases.

Sometimes several top-level async calls are needed in the same loop and contextvars
context instead of the single ``main()`` call provided by :func:`asyncio.run`.

The *Runner* context manager can be used for such things:

.. code:: python

   with asyncio.Runner() as runner:
       runner.run(func1())
       runner.run(func2())

On the :class:`~asyncio.Runner` instantiation the new event loop is created.

All :meth:`~asyncio.Runner.run` calls share the same :class:`~contextvars.Context` and
internal :class:`~asyncio.loop`.

On the exit of :keyword:`with` block all background tasks are cancelled, the embedded
loop is closing.


.. class:: Runner(*, debug=None, factory=None)






enter
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
