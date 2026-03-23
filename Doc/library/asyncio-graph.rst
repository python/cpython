.. currentmodule:: asyncio


.. _asyncio-graph:

========================
Call Graph Introspection
========================

**Source code:** :source:`Lib/asyncio/graph.py`

-------------------------------------

asyncio has powerful runtime call graph introspection utilities
to trace the entire call graph of a running *coroutine* or *task*, or
a suspended *future*.  These utilities and the underlying machinery
can be used from within a Python program or by external profilers
and debuggers.

.. versionadded:: 3.14


.. function:: print_call_graph(future=None, /, *, file=None, depth=1, limit=None)

   Print the async call graph for the current task or the provided
   :class:`Task` or :class:`Future`.

   This function prints entries starting from the top frame and going
   down towards the invocation point.

   The function receives an optional *future* argument.
   If not passed, the current running task will be used.

   If the function is called on *the current task*, the optional
   keyword-only *depth* argument can be used to skip the specified
   number of frames from top of the stack.

   If the optional keyword-only *limit* argument is provided, each call stack
   in the resulting graph is truncated to include at most ``abs(limit)``
   entries. If *limit* is positive, the entries left are the closest to
   the invocation point. If *limit* is negative, the topmost entries are
   left. If *limit* is omitted or ``None``, all entries are present.
   If *limit* is ``0``, the call stack is not printed at all, only
   "awaited by" information is printed.

   If *file* is omitted or ``None``, the function will print
   to :data:`sys.stdout`.

   **Example:**

   The following Python code:

   .. code-block:: python

      import asyncio

      async def test():
          asyncio.print_call_graph()

      async def main():
          async with asyncio.TaskGroup() as g:
              g.create_task(test(), name='test')

      asyncio.run(main())

   will print::

      * Task(name='test', id=0x1039f0fe0)
      + Call stack:
      |   File 't2.py', line 4, in async test()
      + Awaited by:
         * Task(name='Task-1', id=0x103a5e060)
            + Call stack:
            |   File 'taskgroups.py', line 107, in async TaskGroup.__aexit__()
            |   File 't2.py', line 7, in async main()

.. function:: format_call_graph(future=None, /, *, depth=1, limit=None)

   Like :func:`print_call_graph`, but returns a string.
   If *future* is ``None`` and there's no current task,
   the function returns an empty string.


.. function:: capture_call_graph(future=None, /, *, depth=1, limit=None)

   Capture the async call graph for the current task or the provided
   :class:`Task` or :class:`Future`.

   The function receives an optional *future* argument.
   If not passed, the current running task will be used. If there's no
   current task, the function returns ``None``.

   If the function is called on *the current task*, the optional
   keyword-only *depth* argument can be used to skip the specified
   number of frames from top of the stack.

   Returns a ``FutureCallGraph`` data class object:

   * ``FutureCallGraph(future, call_stack, awaited_by)``

      Where *future* is a reference to a :class:`Future` or
      a :class:`Task` (or their subclasses.)

      ``call_stack`` is a tuple of ``FrameCallGraphEntry`` objects.

      ``awaited_by`` is a tuple of ``FutureCallGraph`` objects.

   * ``FrameCallGraphEntry(frame)``

      Where *frame* is a frame object of a regular Python function
      in the call stack.


Low level utility functions
===========================

To introspect an async call graph asyncio requires cooperation from
control flow structures, such as :func:`shield` or :class:`TaskGroup`.
Any time an intermediate :class:`Future` object with low-level APIs like
:meth:`Future.add_done_callback() <asyncio.Future.add_done_callback>` is
involved, the following two functions should be used to inform asyncio
about how exactly such intermediate future objects are connected with
the tasks they wrap or control.


.. function:: future_add_to_awaited_by(future, waiter, /)

   Record that *future* is awaited on by *waiter*.

   Both *future* and *waiter* must be instances of
   :class:`Future` or :class:`Task` or their subclasses,
   otherwise the call would have no effect.

   A call to ``future_add_to_awaited_by()`` must be followed by an
   eventual call to the :func:`future_discard_from_awaited_by` function
   with the same arguments.


.. function:: future_discard_from_awaited_by(future, waiter, /)

   Record that *future* is no longer awaited on by *waiter*.

   Both *future* and *waiter* must be instances of
   :class:`Future` or :class:`Task` or their subclasses, otherwise
   the call would have no effect.
