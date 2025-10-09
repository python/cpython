.. currentmodule:: asyncio


=======
Runners
=======

**Source code:** :source:`Lib/asyncio/runners.py`


This section outlines high-level asyncio primitives to run asyncio code.

They are built on top of an :ref:`event loop <asyncio-event-loop>` with the aim
to simplify async code usage for common wide-spread scenarios.

.. contents::
   :depth: 1
   :local:



Running an asyncio Program
==========================

.. function:: run(coro, *, debug=None, loop_factory=None)

   Execute *coro* in an asyncio event loop and return the result.

   The argument can be any awaitable object.

   This function runs the awaitable, taking care of managing the
   asyncio event loop, *finalizing asynchronous generators*, and
   closing the executor.

   This function cannot be called when another asyncio event loop is
   running in the same thread.

   If *debug* is ``True``, the event loop will be run in debug mode. ``False`` disables
   debug mode explicitly. ``None`` is used to respect the global
   :ref:`asyncio-debug-mode` settings.

   If *loop_factory* is not ``None``, it is used to create a new event loop;
   otherwise :func:`asyncio.new_event_loop` is used. The loop is closed at the end.
   This function should be used as a main entry point for asyncio programs,
   and should ideally only be called once. It is recommended to use
   *loop_factory* to configure the event loop instead of policies.
   Passing :class:`asyncio.EventLoop` allows running asyncio without the
   policy system.

   The executor is given a timeout duration of 5 minutes to shutdown.
   If the executor hasn't finished within that duration, a warning is
   emitted and the executor is closed.

   Example::

       async def main():
           await asyncio.sleep(1)
           print('hello')

       asyncio.run(main())

   .. versionadded:: 3.7

   .. versionchanged:: 3.9
      Updated to use :meth:`loop.shutdown_default_executor`.

   .. versionchanged:: 3.10

      *debug* is ``None`` by default to respect the global debug mode settings.

   .. versionchanged:: 3.12

      Added *loop_factory* parameter.

   .. versionchanged:: 3.14

      *coro* can be any awaitable object.

   .. note::

      The :mod:`!asyncio` policy system is deprecated and will be removed
      in Python 3.16; from there on, an explicit *loop_factory* is needed
      to configure the event loop.


Runner context manager
======================

.. class:: Runner(*, debug=None, loop_factory=None)

   A context manager that simplifies *multiple* async function calls in the same
   context.

   Sometimes several top-level async functions should be called in the same :ref:`event
   loop <asyncio-event-loop>` and :class:`contextvars.Context`.

   If *debug* is ``True``, the event loop will be run in debug mode. ``False`` disables
   debug mode explicitly. ``None`` is used to respect the global
   :ref:`asyncio-debug-mode` settings.

   *loop_factory* could be used for overriding the loop creation.
   It is the responsibility of the *loop_factory* to set the created loop as the
   current one. By default :func:`asyncio.new_event_loop` is used and set as
   current event loop with :func:`asyncio.set_event_loop` if *loop_factory* is ``None``.

   Basically, :func:`asyncio.run` example can be rewritten with the runner usage::

        async def main():
            await asyncio.sleep(1)
            print('hello')

        with asyncio.Runner() as runner:
            runner.run(main())

   .. versionadded:: 3.11

   .. method:: run(coro, *, context=None)

      Execute *coro* in the embedded event loop.

      The argument can be any awaitable object.

      If the argument is a coroutine, it is wrapped in a Task.

      An optional keyword-only *context* argument allows specifying a
      custom :class:`contextvars.Context` for the code to run in.
      The runner's default context is used if context is ``None``.

      Returns the awaitable's result or raises an exception.

      This function cannot be called when another asyncio event loop is
      running in the same thread.

      .. versionchanged:: 3.14

         *coro* can be any awaitable object.

   .. method:: close()

      Close the runner.

      Finalize asynchronous generators, shutdown default executor, close the event loop
      and release embedded :class:`contextvars.Context`.

   .. method:: get_loop()

      Return the event loop associated with the runner instance.

   .. note::

      :class:`Runner` uses the lazy initialization strategy, its constructor doesn't
      initialize underlying low-level structures.

      Embedded *loop* and *context* are created at the :keyword:`with` body entering
      or the first call of :meth:`run` or :meth:`get_loop`.


Handling Keyboard Interruption
==============================

.. versionadded:: 3.11

When :const:`signal.SIGINT` is raised by :kbd:`Ctrl-C`, :exc:`KeyboardInterrupt`
exception is raised in the main thread by default. However this doesn't work with
:mod:`asyncio` because it can interrupt asyncio internals and can hang the program from
exiting.

To mitigate this issue, :mod:`asyncio` handles :const:`signal.SIGINT` as follows:

1. :meth:`asyncio.Runner.run` installs a custom :const:`signal.SIGINT` handler before
   any user code is executed and removes it when exiting from the function.
2. The :class:`~asyncio.Runner` creates the main task for the passed coroutine for its
   execution.
3. When :const:`signal.SIGINT` is raised by :kbd:`Ctrl-C`, the custom signal handler
   cancels the main task by calling :meth:`asyncio.Task.cancel` which raises
   :exc:`asyncio.CancelledError` inside the main task.  This causes the Python stack
   to unwind, ``try/except`` and ``try/finally`` blocks can be used for resource
   cleanup.  After the main task is cancelled, :meth:`asyncio.Runner.run` raises
   :exc:`KeyboardInterrupt`.
4. A user could write a tight loop which cannot be interrupted by
   :meth:`asyncio.Task.cancel`, in which case the second following :kbd:`Ctrl-C`
   immediately raises the :exc:`KeyboardInterrupt` without cancelling the main task.
