.. currentmodule:: asyncio

Develop with asyncio
====================

Asynchronous programming is different than classical "sequential" programming.
This page lists common traps and explains how to avoid them.


.. _asyncio-multithreading:

Concurrency and multithreading
------------------------------

An event loop runs in a thread and executes all callbacks and tasks in the same
thread.  If a callback should be scheduled from a different thread, the
:meth:`BaseEventLoop.call_soon_threadsafe` method should be used.

While a task in running in the event loop, no other task is running in the same
thread. But when the task uses ``yield from``, the task is suspended and the
event loop executes the next task.

To handle signals and to execute subprocesses, the event loop must be run in
the main thread.

The :meth:`BaseEventLoop.run_in_executor` method can be used with a thread pool
executor to execute a callback in different thread to not block the thread of
the event loop.

.. seealso::

   See the :ref:`Synchronization primitives <asyncio-sync>` section to
   synchronize tasks.


.. _asyncio-handle-blocking:

Handle correctly blocking functions
-----------------------------------

Blocking functions should not be called directly. For example, if a function
blocks for 1 second, other tasks are delayed by 1 second which can have an
important impact on reactivity.

For networking and subprocesses, the :mod:`asyncio` module provides high-level
APIs like :ref:`protocols <protocol>`.

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

When a coroutine function is called but not passed to :func:`async` or to the
:class:`Task` constructor, it is not scheduled and it is probably a bug.

To detect such bug, set :data:`asyncio.tasks._DEBUG` to ``True``. When the
coroutine object is destroyed by the garbage collector, a log will be emitted
with the traceback where the coroutine function was called. See the
:ref:`asyncio logger <asyncio-logger>`.

The debug flag changes the behaviour of the :func:`coroutine` decorator. The
debug flag value is only used when then coroutine function is defined, not when
it is called.  Coroutine functions defined before the debug flag is set to
``True`` will not be tracked. For example, it is not possible to debug
coroutines defined in the :mod:`asyncio` module, because the module must be
imported before the flag value can be changed.

Example with the bug::

    import asyncio
    asyncio.tasks._DEBUG = True

    @asyncio.coroutine
    def test():
        print("never scheduled")

    test()

Output in debug mode::

    Coroutine 'test' defined at test.py:4 was never yielded from

The fix is to call the :func:`async` function or create a :class:`Task` object
with this coroutine object.


Detect exceptions not consumed
------------------------------

Python usually calls :func:`sys.displayhook` on unhandled exceptions. If
:meth:`Future.set_exception` is called, but the exception is not consumed,
:func:`sys.displayhook` is not called. Instead, a log is emitted when the
future is deleted by the garbage collector, with the traceback where the
exception was raised. See the :ref:`asyncio logger <asyncio-logger>`.

Example of unhandled exception::

    import asyncio

    @asyncio.coroutine
    def bug():
        raise Exception("not consumed")

    loop = asyncio.get_event_loop()
    asyncio.async(bug())
    loop.run_forever()

Output::

    Future/Task exception was never retrieved:
    Traceback (most recent call last):
      File "/usr/lib/python3.4/asyncio/tasks.py", line 279, in _step
        result = next(coro)
      File "/usr/lib/python3.4/asyncio/tasks.py", line 80, in coro
        res = func(*args, **kw)
      File "test.py", line 5, in bug
        raise Exception("not consumed")
    Exception: not consumed

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


Chain coroutines correctly
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
    Pending tasks at exit: {Task(<create>)<PENDING>}

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

