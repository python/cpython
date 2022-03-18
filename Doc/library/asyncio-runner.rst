.. currentmodule:: asyncio


=======
Runners
=======

**Source code:** :source:`Lib/asyncio/runners.py`


This section outlines high-level asyncio primitives to run asyncio code.

They are built on top of :ref:`event loop <asyncio-event-loop>` with the aim to simplify
async code usage for common wide-spread scenarion.

.. contents::
   :depth: 1
   :local:



Running an asyncio Program
==========================

.. function:: run(coro, *, debug=None)

   Execute the :term:`coroutine` *coro* and return the result.

   This function runs the passed coroutine, taking care of
   managing the asyncio event loop, *finalizing asynchronous
   generators*, and closing the threadpool.

   This function cannot be called when another asyncio event loop is
   running in the same thread.

   If *debug* is ``True``, the event loop will be run in debug mode. ``False`` disables
   debug mode explicitly. ``None`` is used to respect the global
   :ref:`asyncio-debug-mode` settings.

   This function always creates a new event loop and closes it at
   the end.  It should be used as a main entry point for asyncio
   programs, and should ideally only be called once.

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


Runner context manager
======================

.. class:: Runner(*, debug=None, factory=None)

   A context manager that simplifies *multiple* async function calls in the same
   context.

   Sometimes several top-level async functions should be called in the same :ref:`event
   loop <asyncio-event-loop>` and :class:`contextvars.Context`.

   If *debug* is ``True``, the event loop will be run in debug mode. ``False`` disables
   debug mode explicitly. ``None`` is used to respect the global
   :ref:`asyncio-debug-mode` settings.

   *factory* could be used for overriding the loop creation.
   :func:`asyncio.new_event_loop` is used if ``None``.

   Basically, :func:`asyncio.run()` example can be revealed with the runner usage::

        async def main():
            await asyncio.sleep(1)
            print('hello')

        with asyncio.Runner() as runner:
            runner.run(main())

   .. versionadded:: 3.11

   .. method:: run(coro, *, context=None)

      Run a :term:`coroutine <coroutine>` *coro* in the embedded loop.

      Return the coroutine's result or raise its exception.

      An optional keyword-only *context* argument allows specifying a
      custom :class:`contextvars.Context` for the *coro* to run in.
      The runner's context is used if ``None``.

   .. method:: get_loop()

      Return the event loop associated with the runner instance.

   .. method:: get_context()

      Return the :class:`contextvars.Context` associated with the runner object.
