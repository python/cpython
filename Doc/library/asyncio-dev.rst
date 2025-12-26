.. currentmodule:: asyncio

.. _asyncio-dev:

=======================
Developing with asyncio
=======================

Asynchronous programming is different from classic "sequential"
programming.

This page lists common mistakes and traps and explains how
to avoid them.


.. _asyncio-debug-mode:

Debug Mode
==========

By default asyncio runs in production mode.  In order to ease
the development asyncio has a *debug mode*.

There are several ways to enable asyncio debug mode:

* Setting the :envvar:`PYTHONASYNCIODEBUG` environment variable to ``1``.

* Using the :ref:`Python Development Mode <devmode>`.

* Passing ``debug=True`` to :func:`asyncio.run`.

* Calling :meth:`loop.set_debug`.

In addition to enabling the debug mode, consider also:

* setting the log level of the :ref:`asyncio logger <asyncio-logger>` to
  :py:const:`logging.DEBUG`, for example the following snippet of code
  can be run at startup of the application::

    logging.basicConfig(level=logging.DEBUG)

* configuring the :mod:`warnings` module to display
  :exc:`ResourceWarning` warnings.  One way of doing that is by
  using the :option:`-W` ``default`` command line option.


When the debug mode is enabled:

* Many non-threadsafe asyncio APIs (such as :meth:`loop.call_soon` and
  :meth:`loop.call_at` methods) raise an exception if they are called
  from a wrong thread.

* The execution time of the I/O selector is logged if it takes too long to
  perform an I/O operation.

* Callbacks taking longer than 100 milliseconds are logged.  The
  :attr:`loop.slow_callback_duration` attribute can be used to set the
  minimum execution duration in seconds that is considered "slow".


.. _asyncio-multithreading:

Concurrency and Multithreading
==============================

An event loop runs in a thread (typically the main thread) and executes
all callbacks and Tasks in its thread.  While a Task is running in the
event loop, no other Tasks can run in the same thread.  When a Task
executes an ``await`` expression, the running Task gets suspended, and
the event loop executes the next Task.

To schedule a :term:`callback` from another OS thread, the
:meth:`loop.call_soon_threadsafe` method should be used. Example::

    loop.call_soon_threadsafe(callback, *args)

Almost all asyncio objects are not thread safe, which is typically
not a problem unless there is code that works with them from outside
of a Task or a callback.  If there's a need for such code to call a
low-level asyncio API, the :meth:`loop.call_soon_threadsafe` method
should be used, e.g.::

    loop.call_soon_threadsafe(fut.cancel)

To schedule a coroutine object from a different OS thread, the
:func:`run_coroutine_threadsafe` function should be used. It returns a
:class:`concurrent.futures.Future` to access the result::

     async def coro_func():
          return await asyncio.sleep(1, 42)

     # Later in another OS thread:

     future = asyncio.run_coroutine_threadsafe(coro_func(), loop)
     # Wait for the result:
     result = future.result()

To handle signals the event loop must be
run in the main thread.

The :meth:`loop.run_in_executor` method can be used with a
:class:`concurrent.futures.ThreadPoolExecutor` or
:class:`~concurrent.futures.InterpreterPoolExecutor` to execute
blocking code in a different OS thread without blocking the OS thread
that the event loop runs in.

There is currently no way to schedule coroutines or callbacks directly
from a different process (such as one started with
:mod:`multiprocessing`). The :ref:`asyncio-event-loop-methods`
section lists APIs that can read from pipes and watch file descriptors
without blocking the event loop. In addition, asyncio's
:ref:`Subprocess <asyncio-subprocess>` APIs provide a way to start a
process and communicate with it from the event loop. Lastly, the
aforementioned :meth:`loop.run_in_executor` method can also be used
with a :class:`concurrent.futures.ProcessPoolExecutor` to execute
code in a different process.

.. _asyncio-handle-blocking:

Running Blocking Code
=====================

Blocking (CPU-bound) code should not be called directly.  For example,
if a function performs a CPU-intensive calculation for 1 second,
all concurrent asyncio Tasks and IO operations would be delayed
by 1 second.

An executor can be used to run a task in a different thread,
including in a different interpreter, or even in
a different process to avoid blocking the OS thread with the
event loop.  See the :meth:`loop.run_in_executor` method for more
details.


.. _asyncio-logger:

Logging
=======

asyncio uses the :mod:`logging` module and all logging is performed
via the ``"asyncio"`` logger.

The default log level is :py:const:`logging.INFO`, which can be easily
adjusted::

   logging.getLogger("asyncio").setLevel(logging.WARNING)


Network logging can block the event loop. It is recommended to use
a separate thread for handling logs or use non-blocking IO. For example,
see :ref:`blocking-handlers`.


.. _asyncio-coroutine-not-scheduled:

Detect never-awaited coroutines
===============================

When a coroutine function is called, but not awaited
(e.g. ``coro()`` instead of ``await coro()``)
or the coroutine is not scheduled with :meth:`asyncio.create_task`, asyncio
will emit a :exc:`RuntimeWarning`::

    import asyncio

    async def test():
        print("never scheduled")

    async def main():
        test()

    asyncio.run(main())

Output::

  test.py:7: RuntimeWarning: coroutine 'test' was never awaited
    test()

Output in debug mode::

  test.py:7: RuntimeWarning: coroutine 'test' was never awaited
  Coroutine created at (most recent call last)
    File "../t.py", line 9, in <module>
      asyncio.run(main(), debug=True)

    < .. >

    File "../t.py", line 7, in main
      test()
    test()

The usual fix is to either await the coroutine or call the
:meth:`asyncio.create_task` function::

    async def main():
        await test()


Detect never-retrieved exceptions
=================================

If a :meth:`Future.set_exception` is called but the Future object is
never awaited on, the exception would never be propagated to the
user code.  In this case, asyncio would emit a log message when the
Future object is garbage collected.

Example of an unhandled exception::

    import asyncio

    async def bug():
        raise Exception("not consumed")

    async def main():
        asyncio.create_task(bug())

    asyncio.run(main())

Output::

    Task exception was never retrieved
    future: <Task finished coro=<bug() done, defined at test.py:3>
      exception=Exception('not consumed')>

    Traceback (most recent call last):
      File "test.py", line 4, in bug
        raise Exception("not consumed")
    Exception: not consumed

:ref:`Enable the debug mode <asyncio-debug-mode>` to get the
traceback where the task was created::

    asyncio.run(main(), debug=True)

Output in debug mode::

    Task exception was never retrieved
    future: <Task finished coro=<bug() done, defined at test.py:3>
        exception=Exception('not consumed') created at asyncio/tasks.py:321>

    source_traceback: Object created at (most recent call last):
      File "../t.py", line 9, in <module>
        asyncio.run(main(), debug=True)

    < .. >

    Traceback (most recent call last):
      File "../t.py", line 4, in bug
        raise Exception("not consumed")
    Exception: not consumed


Asynchronous generators best practices
======================================

By :term:`asynchronous generator` in this section we will mean
an :term:`asynchronous generator iterator` that is returned by an
:term:`asynchronous generator` function.


Manually close the generator
----------------------------

If an asynchronous generator happens to exit early by :keyword:`break`, the caller
task being cancelled, or other exceptions, the generator's async cleanup code
will run in an unexpected context -- perhaps after the lifetime of tasks it depends on, or
during the event loop shutdown when the async-generator garbage collection hook
is called.

To prevent this, it is recommended to explicitly close the async generator by
calling the :meth:`~agen.aclose` method, or using a :func:`contextlib.aclosing` context
manager::

  import asyncio
  import contextlib

  async def gen():
    yield 1
    yield 2

  async def func():
    async with contextlib.aclosing(gen()) as g:
      async for x in g:
        break  # Don't iterate until the end

  asyncio.run(func())


Only create a generator when a loop is already running
------------------------------------------------------

It is recommended to create asynchronous generators only after the event loop
has already been created.

To ensure that asynchronous generators close reliably, the event loop uses the
:func:`sys.set_asyncgen_hooks` function to register callback functions. These
callbacks update the list of running asynchronous generators to keep it in a
consistent state.

When the :meth:`loop.shutdown_asyncgens() <asyncio.loop.shutdown_asyncgens>`
function is called, the running generators are stopped gracefully, and the
list is cleared.

The asynchronous generator calls the corresponding system hook when on the
first iteration. At the same time, the generator remembers that the hook was
called and don't call it twice.

So, if the iteration begins before the event loop is created, the event loop
will not be able to add it to its list of active generators because the hooks
will be set after the generator tries to call it. Consequently, the event loop
will not be able to terminate the generator if necessary.


Avoid iterating and closing the same generator concurrently
-----------------------------------------------------------

Async generators may to be reentered while another
:meth:`~agen.anext`/:meth:`~agen.athrow`/:meth:`~agen.aclose` call is in
progress. This may lead to an inconsistent state of the async generator
and can cause errors.

Let's consider following example::

  import asyncio

  async def consumer():
      for idx in range(100):
          await asyncio.sleep(0)
          message = yield idx
          print('received', message)

  async def amain():
      agenerator = consumer()
      await agenerator.asend(None)

      fa = asyncio.create_task(agenerator.asend('A'))
      fb = asyncio.create_task(agenerator.asend('B'))
      await fa
      await fb

  asyncio.run(amain())

Output::

  received A
  Traceback (most recent call last):
    File "test.py", line 38, in <module>
      asyncio.run(amain())
      ~~~~~~~~~~~^^^^^^^^^
    File "Lib\asyncio\runners.py", line 204, in run
      return runner.run(main)
             ~~~~~~~~~~^^^^^^
    File "Lib\asyncio\runners.py", line 127, in run
      return self._loop.run_until_complete(task)
             ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~^^^^^^
    File "Lib\asyncio\base_events.py", line 719, in run_until_complete
      return future.result()
             ~~~~~~~~~~~~~^^
    File "test.py", line 36, in amain
      await fb
  RuntimeError: anext(): asynchronous generator is already running


Therefore, it is recommended to avoid using async generators in parallel
tasks or from multiple event loops.
