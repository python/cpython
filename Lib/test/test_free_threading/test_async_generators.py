import sys
import threading
import unittest

from test import support
from test.support import threading_helper

threading_helper.requires_working_threading(module=True)


class TestFTAsyncGenerators(unittest.TestCase):
    NUM_THREADS = 4

    def test_concurrent_anext(self):
        # Each yielded value must be delivered to exactly one thread.
        async def agen():
            for i in range(100):
                yield i

        ag = agen()
        values = []

        def drive():
            while True:
                try:
                    ag.asend(None).send(None)
                except StopIteration as e:
                    values.append(e.value)
                except StopAsyncIteration:
                    break
                except RuntimeError:
                    # Another thread is currently driving the generator.
                    continue

        threading_helper.run_concurrently(drive, self.NUM_THREADS)
        self.assertEqual(sorted(values), list(range(100)))

    def test_concurrent_athrow(self):
        # Each thrown exception must be delivered to the generator
        # exactly once.
        received = []

        async def agen():
            while True:
                try:
                    yield 1
                except ValueError:
                    received.append(1)

        ag = agen()
        with self.assertRaises(StopIteration):
            ag.asend(None).send(None)  # advance to the first yield

        delivered = []

        def worker():
            for _ in range(50):
                try:
                    ag.athrow(ValueError).send(None)
                except StopIteration as e:
                    # The generator received the exception and yielded again.
                    delivered.append(e.value)
                except RuntimeError:
                    # Another thread is currently driving the generator.
                    pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertEqual(len(received), len(delivered))

    def test_concurrent_aclose(self):
        # The generator must be cleaned up exactly once.
        cleanups = []

        async def agen():
            try:
                while True:
                    yield 1
            finally:
                cleanups.append(1)

        ag = agen()
        with self.assertRaises(StopIteration):
            ag.asend(None).send(None)  # advance to the first yield

        def worker():
            try:
                ag.aclose().send(None)
            except StopIteration:
                # aclose() completed.
                pass
            except StopAsyncIteration:
                # The generator was already closed.
                pass
            except RuntimeError:
                # Another thread is currently driving the generator.
                pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertEqual(cleanups, [1])
        self.assertRaises(StopAsyncIteration, ag.asend(None).send, None)

    def test_concurrent_shared_asend(self):
        # Multiple threads racing on a single asend awaitable: the value
        # must be delivered exactly once.
        async def agen():
            yield 1

        ag = agen()
        aw = ag.asend(None)
        results = []

        def worker():
            try:
                aw.send(None)
            except StopIteration as e:
                results.append(e.value)
            except (RuntimeError, ValueError, StopAsyncIteration):
                pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertEqual(results, [1])
        # The awaitable is closed after the operation completed.
        self.assertRaises(RuntimeError, aw.send, None)

    def test_concurrent_shared_athrow(self):
        # Multiple threads racing on a single athrow awaitable.
        async def agen():
            while True:
                yield 1

        ag = agen()
        with self.assertRaises(StopIteration):
            ag.asend(None).send(None)  # advance to the first yield
        aw = ag.athrow(ValueError)

        def worker():
            try:
                aw.send(None)
            except (RuntimeError, ValueError,
                    StopIteration, StopAsyncIteration):
                pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertRaises(StopAsyncIteration, ag.asend(None).send, None)
        # The awaitable is closed after the operation completed.
        self.assertRaises(RuntimeError, aw.send, None)

    def test_concurrent_shared_aclose(self):
        # Multiple threads racing on a single aclose awaitable: the
        # generator must be cleaned up exactly once.
        cleanups = []

        async def agen():
            try:
                while True:
                    yield 1
            finally:
                cleanups.append(1)

        ag = agen()
        with self.assertRaises(StopIteration):
            ag.asend(None).send(None)  # advance to the first yield
        aw = ag.aclose()

        def worker():
            try:
                aw.send(None)
            except (RuntimeError, ValueError,
                    StopIteration, StopAsyncIteration):
                pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertEqual(cleanups, [1])
        self.assertRaises(StopAsyncIteration, ag.asend(None).send, None)
        # The awaitable is closed after the operation completed.
        self.assertRaises(RuntimeError, aw.send, None)

    def test_concurrent_shared_awaitable_resume(self):
        class MyError(Exception):
            pass

        class YieldOnce:
            def __await__(self):
                yield 'suspended'

        cases = (
            ('asend', (None,), StopAsyncIteration),
            ('athrow', (MyError,), MyError),
            ('aclose', (), StopIteration),
        )
        for kind, args, completion in cases:
            for method in ('send', 'throw', 'close'):
                with self.subTest(kind=kind, method=method):
                    entered = threading.Event()
                    release = threading.Event()
                    cleanups = []

                    async def agen():
                        try:
                            yield 1
                        finally:
                            entered.set()
                            self.assertTrue(
                                release.wait(support.SHORT_TIMEOUT))
                            await YieldOnce()
                            cleanups.append(1)

                    ag = agen()
                    self.addCleanup(ag.aclose().close)
                    with self.assertRaises(StopIteration):
                        ag.__anext__().send(None)
                    aw = getattr(ag, kind)(*args)

                    def owner():
                        self.assertEqual(aw.send(None), 'suspended')

                    def contender():
                        try:
                            self.assertTrue(
                                entered.wait(support.SHORT_TIMEOUT))
                            with self.assertRaisesRegex(
                                ValueError, 'async generator already executing'
                            ):
                                if method == 'send':
                                    aw.send(None)
                                elif method == 'throw':
                                    aw.throw(MyError)
                                else:
                                    aw.close()
                            self.assertTrue(ag.ag_running)
                        finally:
                            release.set()

                    threading_helper.run_concurrently([owner, contender])
                    # Rejecting the contender must leave the owner's pending
                    # await intact, including its claim on the generator.
                    self.assertTrue(ag.ag_running)
                    with self.assertRaisesRegex(
                        RuntimeError,
                        'asynchronous generator is already running',
                    ):
                        ag.__anext__().send(None)
                    with self.assertRaises(completion):
                        aw.send(None)
                    self.assertEqual(cleanups, [1])
                    self.assertIsNone(ag.ag_frame)
                    with self.assertRaisesRegex(
                        RuntimeError, 'cannot reuse already awaited'
                    ):
                        aw.send(None)

    def test_concurrent_anext_athrow(self):
        async def agen():
            while True:
                try:
                    yield 1
                except ValueError:
                    pass

        ag = agen()

        def worker():
            for i in range(100):
                try:
                    if i % 2:
                        ag.asend(None).send(None)
                    else:
                        ag.athrow(ValueError).send(None)
                except (RuntimeError, ValueError,
                        StopIteration, StopAsyncIteration):
                    pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)

    def test_concurrent_anext_aclose(self):
        async def agen():
            for i in range(100):
                yield i

        ag = agen()

        def anext_worker():
            for _ in range(100):
                try:
                    ag.asend(None).send(None)
                except (RuntimeError, ValueError,
                        StopIteration, StopAsyncIteration):
                    pass

        def aclose_worker():
            for _ in range(100):
                try:
                    ag.aclose().send(None)
                except (RuntimeError, ValueError,
                        StopIteration, StopAsyncIteration):
                    pass

        threading_helper.run_concurrently(
            [anext_worker, aclose_worker, anext_worker, aclose_worker])

    def test_firstiter_hook_called_once(self):
        # Racing the first iteration must invoke the firstiter hook
        # exactly once.
        async def agen():
            yield 1

        ag = agen()
        calls = []

        def worker():
            # Async generator hooks are per-thread state.
            sys.set_asyncgen_hooks(firstiter=calls.append)
            try:
                ag.asend(None).send(None)
            except (RuntimeError, ValueError,
                    StopIteration, StopAsyncIteration):
                pass

        threading_helper.run_concurrently(worker, self.NUM_THREADS)
        self.assertEqual(calls, [ag])


if __name__ == "__main__":
    unittest.main()
