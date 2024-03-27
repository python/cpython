import contextlib
from test.support import interpreters
import os
import unittest

from concurrent.futures import InterpreterPoolExecutor
from .executor import ExecutorTest, mul
from .util import BaseTestCase, InterpreterPoolMixin, setup_module


def wait_for_token(queue):
    while True:
        try:
            queue.get(timeout=0.1)
        except interpreters.QueueEmpty:
            continue
        break


def log_n_wait(args):
    logqueue, waitqueue, ident = args
    logqueue.put_nowait(f"{ident=} started")
    try:
        wait_for_token(waitqueue)
    finally:
        logqueue.put_nowait(f"{ident=} stopped")


class InterpreterPoolExecutorTest(InterpreterPoolMixin, ExecutorTest, BaseTestCase):

    def _assertRaises(self, exctype, *args, **kwargs):
        return self.assertRaises(interpreters.ExecutionFailed, *args, **kwargs)

    def test_default_workers(self):
        executor = InterpreterPoolExecutor()
        expected = min(32, (os.process_cpu_count() or 1) + 4)
        self.assertEqual(executor._max_workers, expected)

    def test_saturation(self):
        executor = InterpreterPoolExecutor(max_workers=4)
        waitqueue = interpreters.create_queue(syncobj=True)

        for i in range(15 * executor._max_workers):
            executor.submit(wait_for_token, waitqueue)
        self.assertEqual(len(executor._threads), executor._max_workers)
        for i in range(15 * executor._max_workers):
            waitqueue.put(None)
        executor.shutdown(wait=True)

    def test_idle_thread_reuse(self):
        executor = InterpreterPoolExecutor()
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._threads), 1)
        executor.shutdown(wait=True)

    def test_executor_map_current_future_cancel(self):
        logqueue = interpreters.create_queue(syncobj=True)
        waitqueue = interpreters.create_queue(syncobj=True)
        idents = ['first', 'second', 'third']
        _idents = iter(idents)

        with InterpreterPoolExecutor(max_workers=1) as pool:
            # submit work to saturate the pool
            fut = pool.submit(log_n_wait, (logqueue, waitqueue, next(_idents)))
            try:
                with contextlib.closing(
                    pool.map(log_n_wait,
                             [(logqueue, waitqueue, ident) for ident in _idents],
                             timeout=0)
                ) as gen:
                    with self.assertRaises(TimeoutError):
                        next(gen)
            finally:
                for i, ident in enumerate(idents, 1):
                    waitqueue.put_nowait(None)
                #for ident in idents:
                #    waitqueue.put_nowait(None)
            fut.result()

            # When the pool shuts down (due to the context manager),
            # each worker's interpreter will be finalized.  When that
            # happens, each item in the queue owned by each finalizing
            # interpreter will be removed.  Thus, we must copy the
            # items *before* leaving the context manager.
            # XXX This can be surprising.  Perhaps give users
            # the option to keep objects beyond the original interpreter?
            assert not logqueue.empty(), logqueue.qsize()
            log = []
            while not logqueue.empty():
                log.append(logqueue.get_nowait())

        # ident='second' is cancelled as a result of raising a TimeoutError
        # ident='third' is cancelled because it remained in the collection of futures
        self.assertListEqual(log, ["ident='first' started", "ident='first' stopped"])


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
