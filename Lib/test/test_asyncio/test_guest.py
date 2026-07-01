"""Tests for asyncio.start_guest_run()."""

import asyncio
import queue
import threading
import time
import unittest


class MockHost:
    """A minimal host event loop that uses a thread-safe queue.

    Simulates a GUI toolkit main loop without any actual GUI dependency.
    Callbacks are collected in a queue and drained by :meth:`run`.
    """

    def __init__(self):
        self._queue = queue.Queue()
        self._done = threading.Event()
        self._task = None

    def run_sync_soon_threadsafe(self, fn):
        self._queue.put(fn)

    def done_callback(self, task):
        self._task = task
        self._done.set()

    def run(self, timeout=10.0):
        """Drain callbacks until *done_callback* fires or *timeout* expires."""
        deadline = time.monotonic() + timeout
        while not self._done.is_set():
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("MockHost.run() timed out")
            try:
                fn = self._queue.get(timeout=min(remaining, 0.05))
                fn()
            except queue.Empty:
                pass
        # Drain any trailing callbacks.
        while True:
            try:
                fn = self._queue.get_nowait()
                fn()
            except queue.Empty:
                break
        return self._task


class TestGuestRun(unittest.TestCase):
    """Test asyncio.start_guest_run with a mock host loop."""

    def _run_guest(self, async_fn, *args, timeout=10.0):
        """Helper: run *async_fn* in guest mode and return the completed task."""
        host = MockHost()
        asyncio.start_guest_run(
            async_fn, *args,
            run_sync_soon_threadsafe=host.run_sync_soon_threadsafe,
            done_callback=host.done_callback,
        )
        return host.run(timeout=timeout)

    # -- basic lifecycle -----------------------------------------------

    def test_simple_return(self):
        async def coro():
            return 42

        task = self._run_guest(coro)
        self.assertTrue(task.done())
        self.assertEqual(task.result(), 42)

    def test_return_none(self):
        async def coro():
            pass

        task = self._run_guest(coro)
        self.assertIsNone(task.result())

    def test_with_args(self):
        async def add(a, b):
            return a + b

        task = self._run_guest(add, 3, 7)
        self.assertEqual(task.result(), 10)

    # -- exception propagation -----------------------------------------

    def test_exception(self):
        async def coro():
            raise ValueError("boom")

        task = self._run_guest(coro)
        self.assertTrue(task.done())
        with self.assertRaises(ValueError) as cm:
            task.result()
        self.assertEqual(str(cm.exception), "boom")

    # -- cancellation --------------------------------------------------

    def test_cancel_from_host(self):
        started = threading.Event()

        async def coro():
            started.set()
            await asyncio.sleep(3600)

        host = MockHost()
        task = asyncio.start_guest_run(
            coro,
            run_sync_soon_threadsafe=host.run_sync_soon_threadsafe,
            done_callback=host.done_callback,
        )
        # Wait for the coroutine to start, then cancel.
        # Use call_soon_threadsafe to wake the I/O thread's selector.
        started.wait(timeout=5)
        loop = task.get_loop()
        loop.call_soon_threadsafe(task.cancel)
        host.run(timeout=5)
        self.assertTrue(task.cancelled())

    # -- asyncio primitives work inside guest --------------------------

    def test_sleep(self):
        async def coro():
            t0 = asyncio.get_event_loop().time()
            await asyncio.sleep(0.1)
            elapsed = asyncio.get_event_loop().time() - t0
            return elapsed

        task = self._run_guest(coro)
        elapsed = task.result()
        self.assertGreaterEqual(elapsed, 0.05)

    def test_create_task(self):
        async def helper():
            await asyncio.sleep(0.01)
            return "helper"

        async def coro():
            t = asyncio.ensure_future(helper())
            result = await t
            return result

        task = self._run_guest(coro)
        self.assertEqual(task.result(), "helper")

    def test_gather(self):
        async def sleeper(n):
            await asyncio.sleep(0.01 * n)
            return n

        async def coro():
            results = await asyncio.gather(
                sleeper(1), sleeper(2), sleeper(3)
            )
            return results

        task = self._run_guest(coro)
        self.assertEqual(task.result(), [1, 2, 3])

    def test_call_later(self):
        async def coro():
            loop = asyncio.get_event_loop()
            fut = loop.create_future()
            loop.call_later(0.05, fut.set_result, "later")
            return await fut

        task = self._run_guest(coro)
        self.assertEqual(task.result(), "later")

    def test_call_soon_threadsafe(self):
        async def coro():
            loop = asyncio.get_event_loop()
            fut = loop.create_future()

            def setter():
                loop.call_soon_threadsafe(fut.set_result, "safe")
            threading.Timer(0.05, setter).start()
            return await fut

        task = self._run_guest(coro)
        self.assertEqual(task.result(), "safe")


class TestBaseEventLoopDecomposition(unittest.TestCase):
    """Verify that poll_events / process_events / process_ready exist
    and compose correctly (i.e. _run_once still works)."""

    def test_methods_exist(self):
        loop = asyncio.new_event_loop()
        try:
            self.assertTrue(hasattr(loop, 'poll_events'))
            self.assertTrue(hasattr(loop, 'process_events'))
            self.assertTrue(hasattr(loop, 'process_ready'))
        finally:
            loop.close()

    def test_run_once_still_works(self):
        """asyncio.run() exercises _run_once(); ensure it still functions
        after the refactor."""
        async def coro():
            await asyncio.sleep(0)
            return "ok"

        result = asyncio.run(coro())
        self.assertEqual(result, "ok")


if __name__ == '__main__':
    unittest.main()
