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

    def test_concurrent_shared_aclose_does_not_resume(self):
        # gh-160347: concurrent calls on a shared aclose() awaitable must
        # never resume the async generator past its suspended yield.
        rounds = 500
        start_barrier = threading.Barrier(self.NUM_THREADS + 1)
        done_barrier = threading.Barrier(self.NUM_THREADS + 1)
        state = {"aw": None, "stop": False}
        errors = []

        def worker():
            try:
                while True:
                    start_barrier.wait(timeout=support.SHORT_TIMEOUT)
                    if state["stop"]:
                        return
                    try:
                        state["aw"].send(None)
                    except (RuntimeError, ValueError,
                            StopIteration, StopAsyncIteration):
                        pass
                    done_barrier.wait(timeout=support.SHORT_TIMEOUT)
            except threading.BrokenBarrierError:
                return
            except BaseException as exc:
                errors.append(exc)
                start_barrier.abort()
                done_barrier.abort()

        threads = [threading.Thread(target=worker)
                   for _ in range(self.NUM_THREADS)]
        started = []
        try:
            for t in threads:
                t.start()
                started.append(t)

            for _ in range(rounds):
                cleanups = []
                resumed = []

                async def agen():
                    try:
                        yield 1
                    finally:
                        cleanups.append(1)
                    resumed.append(1)
                    yield 2

                ag = agen()
                with self.assertRaises(StopIteration):
                    ag.asend(None).send(None)

                aw = ag.aclose()
                state["aw"] = aw
                start_barrier.wait(timeout=support.SHORT_TIMEOUT)
                done_barrier.wait(timeout=support.SHORT_TIMEOUT)

                self.assertEqual(cleanups, [1])
                self.assertEqual(resumed, [])
                self.assertFalse(ag.ag_running)
                self.assertRaises(RuntimeError, aw.send, None)
                self.assertRaises(
                    StopAsyncIteration, ag.asend(None).send, None)
        finally:
            state["stop"] = True
            try:
                if not errors and len(started) == len(threads):
                    start_barrier.wait(timeout=support.SHORT_TIMEOUT)
                else:
                    start_barrier.abort()
                    done_barrier.abort()
            except threading.BrokenBarrierError:
                start_barrier.abort()
                done_barrier.abort()
            for t in started:
                t.join(timeout=support.SHORT_TIMEOUT)
                self.assertFalse(t.is_alive())
            self.assertEqual(errors, [])

    def test_concurrent_shared_suspended_awaitable(self):
        # Concurrent calls on a shared awaitable that is suspended at
        # AWAITABLE_STATE_ITER must fail the racing caller without corrupting
        # ag_running or closing the awaitable while the active step finishes.
        for kind in ("asend", "athrow", "aclose"):
            driver_in_step = threading.Event()
            racer_done = threading.Event()
            racer_errors = []

            class SyncYieldTwice:
                def __await__(self):
                    yield "first"
                    driver_in_step.set()
                    self.wait_ok = racer_done.wait(
                        timeout=support.SHORT_TIMEOUT)
                    yield "second"

            sync = SyncYieldTwice()

            async def agen_asend():
                await sync
                yield 2

            async def agen_athrow():
                try:
                    yield 1
                except ValueError:
                    await sync
                    yield 2

            async def agen_aclose():
                try:
                    yield 1
                except GeneratorExit:
                    await sync
                    raise

            if kind == "asend":
                ag = agen_asend()
                aw = ag.asend(None)
            elif kind == "athrow":
                ag = agen_athrow()
                with self.assertRaises(StopIteration):
                    ag.asend(None).send(None)
                aw = ag.athrow(ValueError)
            else:
                ag = agen_aclose()
                with self.assertRaises(StopIteration):
                    ag.asend(None).send(None)
                aw = ag.aclose()

            # Advance to "first" so aw is suspended in AWAITABLE_STATE_ITER.
            self.assertEqual(aw.send(None), "first")
            self.assertTrue(ag.ag_running)

            driver_results = []

            def driver():
                driver_results.append(aw.send(None))

            def racer():
                if not driver_in_step.wait(timeout=support.SHORT_TIMEOUT):
                    racer_errors.append(
                        TimeoutError("driver did not enter step"))
                    return
                try:
                    aw.send(None)
                except ValueError as exc:
                    racer_errors.append(exc)
                finally:
                    racer_done.set()

            threading_helper.run_concurrently([driver, racer])
            self.assertTrue(sync.wait_ok)
            self.assertEqual(driver_results, ["second"])
            self.assertEqual(len(racer_errors), 1)
            self.assertIsInstance(racer_errors[0], ValueError)
            self.assertIn("already executing", str(racer_errors[0]))
            self.assertTrue(ag.ag_running)

            with self.assertRaises(StopIteration):
                aw.send(None)
            self.assertFalse(ag.ag_running)
            self.assertRaises(RuntimeError, aw.send, None)


if __name__ == "__main__":
    unittest.main()
