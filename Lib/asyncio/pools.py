"""Support for high-level asynchronous pools in asyncio."""

__all__ = 'ThreadPool',


import concurrent.futures
import functools
import threading
import os

from abc import ABC, abstractmethod

from . import events
from . import exceptions
from . import futures


class AbstractPool(ABC):
    """Abstract base class for asynchronous pools."""

    @abstractmethod
    async def astart(self):
        raise NotImplementedError

    @abstractmethod
    async def __aenter__(self):
        await self.astart()
        return self

    @abstractmethod
    async def aclose(self):
        raise NotImplementedError

    @abstractmethod
    async def __aexit__(self, exc_type, exc_value, exc_traceback):
        await self.aclose()

    @abstractmethod
    async def run(self, /, func, *args, **kwargs):
        """Asynchronously run function *func* using the pool.

        Return a future, representing the eventual result of *func*.
        """
        raise NotImplementedError


class ThreadPool(AbstractPool):
    """Asynchronous threadpool for running IO-bound functions.

    Directly calling an IO-bound function within the main thread will block
    other operations from occurring until it is completed. By using a
    threadpool, several IO-bound functions can be ran concurrently within
    their own threads, without blocking other operations.

    The optional argument *concurrency* sets the number of threads within the
    threadpool. If *concurrency* is `None`, the maximum number of threads will
    be used; based on the number of CPU cores.

    This threadpool is intended to be used as an asynchronous context manager,
    using the `async with` syntax, which provides automatic initialization and
    finalization of resources. For example:

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
                pool.run(other_blocking_io))

    asyncio.run(main())
    """

    def __init__(self, concurrency=None):
        if concurrency is None:
            concurrency = min(32, (os.cpu_count() or 1) + 4)

        self._concurrency = concurrency
        self._running = False
        self._closed = False
        self._loop = None
        self._pool = None

    async def astart(self):
        self._loop = events.get_running_loop()
        await self._spawn_threadpool()

    async def __aenter__(self):
        await self.astart()
        return self

    async def aclose(self):
        await self._shutdown_threadpool()

    async def __aexit__(self, exc_type, exc_value, exc_traceback):
        await self.aclose()

    async def run(self, /, func, *args, **kwargs):
        if not self._running:
            raise RuntimeError(f"unable to run {func!r}, "
                               "threadpool is not running")

        func_call = functools.partial(func, *args, **kwargs)
        executor = self._pool
        return await futures.wrap_future(
            executor.submit(func_call), loop=self._loop)

    async def _spawn_threadpool(self):
        """Schedule the spawning of the threadpool.

        Asynchronously spawns a threadpool with *concurrency* threads.
        """
        if self._running:
            raise RuntimeError("threadpool is already running")

        if self._closed:
            raise RuntimeError("threadpool is closed")

        future = self._loop.create_future()
        thread = threading.Thread(target=self._do_spawn, args=(future,))
        thread.start()
        try:
            await future
        finally:
            thread.join()

    def _do_spawn(self, future):
        try:
            self._pool = concurrent.futures.ThreadPoolExecutor(
                                                max_workers=self._concurrency)
            self._running = True
            self._loop.call_soon_threadsafe(future.set_result, None)
        except Exception as ex:
            self._loop.call_soon_threadsafe(future.exception, ex)

    async def _shutdown_threadpool(self):
        """Schedule the shutdown of the threadpool.

        Asynchronously joins all of the threads in the threadpool.
        """
        if self._closed:
            raise RuntimeError("threadpool is already closed")

        # Set _running to False as early as possible
        self._running = False

        future = self._loop.create_future()
        thread = threading.Thread(target=self._do_shutdown, args=(future,))
        thread.start()
        try:
            await future
        finally:
            thread.join()

    def _do_shutdown(self, future):
        try:
            self._pool.shutdown()
            self._closed = True
            self._loop.call_soon_threadsafe(future.set_result, None)
        except Exception as ex:
            self._loop.call_soon_threadsafe(future.exception, ex)
