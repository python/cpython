.. currentmodule:: asyncio

.. _asyncio-dev:

Develop with asyncio
====================

Asynchronous programming is different than classical "sequential" programming.
This page lists common traps and explains how to avoid them.


.. _asyncio-debug-mode:

Debug mode of asyncio
---------------------

To enable the debug mode globally, set the environment variable
:envvar:`PYTHONASYNCIODEBUG` to ``1``. To see debug traces, set the log level
of the :ref:`asyncio logger <asyncio-logger>` to :py:data:`logging.DEBUG`.  The
simplest configuration is::

   import logging
   # ...
   logging.basicConfig(level=logging.DEBUG)

Examples of effects of the debug mode:

* Log :ref:`coroutines defined but never "yielded from"
  <asyncio-coroutine-not-scheduled>`
* :meth:`~BaseEventLoop.call_soon` and :meth:`~BaseEventLoop.call_at` methods
  raise an exception if they are called from the wrong thread.
* Log the execution time of the selector
* Log callbacks taking more than 100 ms to be executed. The
  :attr:`BaseEventLoop.slow_callback_duration` attribute is the minimum
  duration in seconds of "slow" callbacks.

.. seealso::

   The :meth:`BaseEventLoop.set_debug` method and the :ref:`asyncio logger
   <asyncio-logger>`.


.. _asyncio-multithreading:

Concurrency and multithreading
------------------------------

An event loop runs in a thread and executes all callbacks and tasks in the same
thread. While a task is running in the event loop, no other task is running in
the same thread. But when the task uses ``yield from``, the task is suspended
and the event loop executes the next task.

To schedule a callback from a different thread, the
:meth:`BaseEventLoop.call_soon_threadsafe` method should be used. Example to
schedule a coroutine from a different thread::

    loop.call_soon_threadsafe(asyncio.async, coro_func())

Most asyncio objects are not thread safe. You should only worry if you access
objects outside the event loop. For example, to cancel a future, don't call
directly its :meth:`Future.cancel` method, but::

    loop.call_soon_threadsafe(fut.cancel)

To handle signals and to execute subprocesses, the event loop must be run in
the main thread.

The :meth:`BaseEventLoop.run_in_executor` method can be used with a thread pool
executor to execute a callback in different thread to not block the thread of
the event loop.

.. seealso::

   See the :ref:`Synchronization primitives <asyncio-sync>` section to
   synchronize tasks.


.. _asyncio-handle-blocking:

Handle blocking functions correctly
-----------------------------------

Blocking functions should not be called directly. For example, if a function
blocks for 1 second, other tasks are delayed by 1 second which can have an
important impact on reactivity.

For networking and subprocesses, the :mod:`asyncio` module provides high-level
APIs like :ref:`protocols <asyncio-protocol>`.

An executor can be used to run a task in a different thread or even in a
different process, to not block the thread of the event loop. See the
:meth:`BaseEventLoop.run_in_executor` method.

.. seealso::

   The :ref:`Delayed calls <asyncio-delayed-calls>` section details how the
   event loop handles time.


.. _asyncio-logger:

Logging
-------

The :mod:`asyncio` module logs information with the :mod:`logging` module in
the logger ``'asyncio'``.


.. _asyncio-coroutine-not-scheduled:

Detect coroutine objects never scheduled
----------------------------------------

When a coroutine function is called and its result is not passed to
:func:`async` or to the :meth:`BaseEventLoop.create_task` method: the execution
of the coroutine objet will never be scheduled and it is probably a bug.
:ref:`Enable the debug mode of asyncio <asyncio-debug-mode>` to :ref:`log a
warning <asyncio-logger>` to detect it.

Example with the bug::

    import asyncio

    @asyncio.coroutine
    def test():
        print("never scheduled")

    test()

Output in debug mode::

    Coroutine test() at test.py:3 was never yielded from
    Coroutine object created at (most recent call last):
      File "test.py", line 7, in <module>
        test()

The fix is to call the :func:`async` function or the
:meth:`BaseEventLoop.create_task` method with the coroutine object.

.. seealso::

   :ref:`Pending task destroyed <asyncio-pending-task-destroyed>`.


Detect exceptions never consumed
--------------------------------

Python usually calls :func:`sys.displayhook` on unhandled exceptions. If
:meth:`Future.set_exception` is called, but the exception is never consumed,
:func:`sys.displayhook` is not called. Instead, a :ref:`a log is emitted
<asyncio-logger>` when the future is deleted by the garbage collector, with the
traceback where the exception was raised.

Example of unhandled exception::

    import asyncio

    @asyncio.coroutine
    def bug():
        raise Exception("not consumed")

    loop = asyncio.get_event_loop()
    asyncio.async(bug())
    loop.run_forever()

Output::

    Task exception was never retrieved
    future: <Task finished bug() done at asyncio/coroutines.py:139 exception=Exception('not consumed',)>
    source_traceback: Object created at (most recent call last):
      File "test.py", line 10, in <module>
        asyncio.async(bug())
      File "asyncio/tasks.py", line 510, in async
        task = loop.create_task(coro_or_future)
    Traceback (most recent call last):
      File "asyncio/tasks.py", line 244, in _step
        result = next(coro)
      File "coroutines.py", line 78, in __next__
        return next(self.gen)
      File "asyncio/coroutines.py", line 141, in coro
        res = func(*args, **kw)
      File "test.py", line 7, in bug
        raise Exception("not consumed")
    Exception: not consumed

:ref:`Enable the debug mode of asyncio <asyncio-debug-mode>` to get the
traceback where the task was created.

There are different options to fix this issue. The first option is to chain to
coroutine in another coroutine and use classic try/except::

    @asyncio.coroutine
    def handle_exception():
        try:
            yield from bug()
        except Exception:
            print("exception consumed")

    loop = asyncio.get_event_loop()
    asyncio.async(handle_exception())
    loop.run_forever()

Another option is to use the :meth:`BaseEventLoop.run_until_complete`
function::

    task = asyncio.async(bug())
    try:
        loop.run_until_complete(task)
    except Exception:
        print("exception consumed")

See also the :meth:`Future.exception` method.


Chain correctly coroutines
--------------------------

When a coroutine function calls other coroutine functions and tasks, they
should be chained explicitly with ``yield from``. Otherwise, the execution is
not guaranteed to be sequential.

Example with different bugs using :func:`asyncio.sleep` to simulate slow
operations::

    import asyncio

    @asyncio.coroutine
    def create():
        yield from asyncio.sleep(3.0)
        print("(1) create file")

    @asyncio.coroutine
    def write():
        yield from asyncio.sleep(1.0)
        print("(2) write into file")

    @asyncio.coroutine
    def close():
        print("(3) close file")

    @asyncio.coroutine
    def test():
        asyncio.async(create())
        asyncio.async(write())
        asyncio.async(close())
        yield from asyncio.sleep(2.0)
        loop.stop()

    loop = asyncio.get_event_loop()
    asyncio.async(test())
    loop.run_forever()
    print("Pending tasks at exit: %s" % asyncio.Task.all_tasks(loop))
    loop.close()

Expected output::

    (1) create file
    (2) write into file
    (3) close file
    Pending tasks at exit: set()

Actual output::

    (3) close file
    (2) write into file
    Pending tasks at exit: {<Task pending create() at test.py:7 wait_for=<Future pending cb=[Task._wakeup()]>>}
    Task was destroyed but it is pending!
    task: <Task pending create() done at test.py:5 wait_for=<Future pending cb=[Task._wakeup()]>>

The loop stopped before the ``create()`` finished, ``close()`` has been called
before ``write()``, whereas coroutine functions were called in this order:
``create()``, ``write()``, ``close()``.

To fix the example, tasks must be marked with ``yield from``::

    @asyncio.coroutine
    def test():
        yield from asyncio.async(create())
        yield from asyncio.async(write())
        yield from asyncio.async(close())
        yield from asyncio.sleep(2.0)
        loop.stop()

Or without ``asyncio.async()``::

    @asyncio.coroutine
    def test():
        yield from create()
        yield from write()
        yield from close()
        yield from asyncio.sleep(2.0)
        loop.stop()


.. _asyncio-pending-task-destroyed:

Pending task destroyed
----------------------

If a pending task is destroyed, the execution of its wrapped :ref:`coroutine
<coroutine>` did not complete. It is probably a bug and so a warning is logged.

Example of log::

    Task was destroyed but it is pending!
    source_traceback: Object created at (most recent call last):
      File "test.py", line 17, in <module>
        task = asyncio.async(coro, loop=loop)
      File "asyncio/tasks.py", line 510, in async
        task = loop.create_task(coro_or_future)
    task: <Task pending kill_me() done at test.py:5 wait_for=<Future pending cb=[Task._wakeup()]>>

:ref:`Enable the debug mode of asyncio <asyncio-debug-mode>` to get the
traceback where the task was created.

.. seealso::

   :ref:`Detect coroutine objects never scheduled <asyncio-coroutine-not-scheduled>`.

