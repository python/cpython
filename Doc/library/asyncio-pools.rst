.. currentmodule:: asyncio

.. versionadded:: 3.9

.. _asyncio-pools:

=====
Pools
=====

**Source code:** :source:`Lib/asyncio/pools.py`

-----------------------------------------------

.. note::
    This section of the documentation and all of its members have been
    added *provisionally* to asyncio's API. For more details, see
    :term:`provisional api`.

Asyncio pools are high-level, asynchronous context managers that can be used
to concurrently run blocking functions and methods.

There are many potential use cases, but a particularly useful one is for
combining libraries without asyncio support with existing asyncio programs.
Normally, calling a non-async function within the event loop would will result
in blocking the event loop until the function returns. However, by using a
pool to run the function, it can be executed in a separate worker (such as a
thread or process) without blocking the event loop.

.. class:: ThreadPool(concurrency=None)

   An asynchronous threadpool that provides methods to concurrently
   run IO-bound functions, without blocking the event loop.

   *concurrency* is an optional argument that limits the number of
   threads to utilize in the threadpool. With the default value of
   ``None``, the amount of threads used will scale based on the
   number of processors.

   .. coroutinemethod:: run(/, func, *args, **kwargs)

      Asynchronously run *func* with its arguments and keyword-arguments
      within the threadpool, and return a :class:`asyncio.Future` object
      that represents the eventual result of its execution. ::

          async with asyncio.ThreadPool() as pool:
              await pool.run(time.sleep, 1)

      Raises a :exc:`RuntimeError` if the threadpool is *not* running.

   .. coroutinemethod:: astart()

      Schedule the start of the threadpool and spawn its threads. Note that
      this function is called automatically when using ``asyncio.ThreadPool``
      as an asynchronous context manager, and does not need to be called
      directly.

      Raises a :exc:`RuntimeError` if the threadpool is already running or
      if it's been closed.

   .. coroutinemethod:: aclose()

      Schedule the closing of the threadpool. Note that this function is
      called automatically when using ``asyncio.ThreadPool`` as an
      asynchronous context manager, and does not need to be called directly.

      Raises a :exc:`RuntimeError` if the threadpool has already been closed.

Examples
========

Here's an example of concurrently running two IO-bound functions using
:class:`asyncio.ThreadPool`::

    import asyncio

    def blocking_io():
        print("start blocking_io")
        with open('/dev/urandom', 'rb') as f:
            f.read(100_000)
        print("blocking_io complete")

    def other_blocking_io():
        print("start other_blocking_io")
        with open('/dev/zero', 'rb') as f:
            f.read(10)
        print("other_blocking_io complete")

    async def main():
        async with asyncio.ThreadPool() as pool:
            await asyncio.gather(
                pool.run(blocking_io),
                pool.run(other_io))

    asyncio.run(main())
