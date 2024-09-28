.. currentmodule:: asyncio


.. _asyncio-stack:

===================
Stack Introspection
===================

**Source code:** :source:`Lib/asyncio/stack.py`

-------------------------------------

asyncio has powerful runtime call stack introspection utilities
to trace the entire call graph of a running coroutine or task, or
a suspended *future*.

.. versionadded:: 3.14


.. function:: print_call_stack(*, future=None, file=None)

   Print the async call stack for the current task or the provided
   :class:`Task` or :class:`Future`.

   The function recieves an optional keyword-only *future* argument.
   If not passed, the current running task will be used. If there's no
   current task, the function returns ``None``.

   If *file* is not specified the function will print to :data:`sys.stdout`.

   **Example:**

   The following Python code:

   .. code-block:: python

      import asyncio

      async def test():
         asyncio.print_call_stack()

      async def main():
         async with asyncio.TaskGroup() as g:
            g.create_task(test())

      asyncio.run(main())

   will print::

      * Task(name='Task-2', id=0x105038fe0)
        + Call stack:
        | * print_call_stack()
        |   asyncio/stack.py:231
        | * async test()
        |   test.py:4
        + Awaited by:
           * Task(name='Task-1', id=0x1050a6060)
              + Call stack:
              | * async TaskGroup.__aexit__()
              |   asyncio/taskgroups.py:107
              | * async main()
              |   test.py:7

   For rendering the call stack to a string the following pattern
   should be used:

   .. code-block:: python

      import io

      ...

      buf = io.StringIO()
      asyncio.print_call_stack(file=buf)
      output = buf.getvalue()


.. function:: capture_call_stack(*, future=None)

   Capture the async call stack for the current task or the provided
   :class:`Task` or :class:`Future`.

   The function recieves an optional keyword-only *future* argument.
   If not passed, the current running task will be used. If there's no
   current task, the function returns ``None``.

   Returns a ``FutureCallStack`` named tuple:

   * ``FutureCallStack(future, call_stack, awaited_by)``

      Where 'future' is a reference to a *Future* or a *Task*
      (or their subclasses.)

      ``call_stack`` is a list of ``FrameCallStackEntry`` and
      ``CoroutineCallStackEntry`` objects (more on them below.)

      ``awaited_by`` is a list of ``FutureCallStack`` tuples.

   * ``FrameCallStackEntry(frame)``

      Where ``frame`` is a frame object of a regular Python function
      in the call stack.

   * ``CoroutineCallStackEntry(coroutine)``

      Where ``coroutine`` is a coroutine object of an awaiting coroutine
      or asyncronous generator.


Low level utility functions
===========================

To introspect an async call stack asyncio requires cooperation from
control flow structures, such as :func:`shield` or :class:`TaskGroup`.
Any time an intermediate ``Future`` object with low-level APIs like
:meth:`Future.add_done_callback() <asyncio.Future.add_done_callback>` is
involved, the following two functions should be used to inform *asyncio*
about how exactly such intermediate future objects are connected with
the tasks they wrap or control.


.. function:: future_add_to_awaited_by(future, waiter, /)

   Record that *future* is awaited on by *waiter*.

   Both *future* and *waiter* must be instances of
   :class:`asyncio.Future <Future>` or :class:`asyncio.Task <Task>` or
   their subclasses, otherwise the call would have no effect.

   A call to ``future_add_to_awaited_by()`` must be followed by an
   eventual call to the ``future_discard_from_awaited_by()`` function
   with the same arguments.


.. function:: future_discard_from_awaited_by(future, waiter, /)

   Record that *future* is no longer awaited on by *waiter*.

   Both *future* and *waiter* must be instances of
   :class:`asyncio.Future <Future>` or :class:`asyncio.Task <Task>` or
   their subclasses, otherwise the call would have no effect.
