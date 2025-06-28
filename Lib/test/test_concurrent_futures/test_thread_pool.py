import contextlib
import multiprocessing as mp
import multiprocessing.process
import multiprocessing.util
import os
import threading
import unittest
from concurrent import futures
from test import support

from .executor import ExecutorTest, mul
from .util import BaseTestCase, ThreadPoolMixin, setup_module


class ThreadPoolExecutorTest(ThreadPoolMixin, ExecutorTest, BaseTestCase):
    def test_map_submits_without_iteration(self):
        """Tests verifying issue 11777."""
        finished = []
        def record_finished(n):
            finished.append(n)

        self.executor.map(record_finished, range(10))
        self.executor.shutdown(wait=True)
        self.assertCountEqual(finished, range(10))

    def test_default_workers(self):
        executor = self.executor_type()
        expected = min(32, (os.process_cpu_count() or 1) + 4)
        self.assertEqual(executor._max_workers, expected)

    def test_saturation(self):
        executor = self.executor_type(4)
        def acquire_lock(lock):
            lock.acquire()

        sem = threading.Semaphore(0)
        for i in range(15 * executor._max_workers):
            executor.submit(acquire_lock, sem)
        self.assertEqual(len(executor._threads), executor._max_workers)
        for i in range(15 * executor._max_workers):
            sem.release()
        executor.shutdown(wait=True)

    @support.requires_gil_enabled("gh-117344: test is flaky without the GIL")
    def test_idle_thread_reuse(self):
        executor = self.executor_type()
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._threads), 1)
        executor.shutdown(wait=True)

    @support.requires_fork()
    @unittest.skipUnless(hasattr(os, 'register_at_fork'), 'need os.register_at_fork')
    @support.requires_resource('cpu')
    def test_hang_global_shutdown_lock(self):
        # bpo-45021: _global_shutdown_lock should be reinitialized in the child
        # process, otherwise it will never exit
        def submit(pool):
            pool.submit(submit, pool)

        with futures.ThreadPoolExecutor(1) as pool:
            pool.submit(submit, pool)

            for _ in range(50):
                with futures.ProcessPoolExecutor(1, mp_context=mp.get_context('fork')) as workers:
                    workers.submit(tuple)

    @support.requires_fork()
    @unittest.skipUnless(hasattr(os, 'register_at_fork'), 'need os.register_at_fork')
    def test_process_fork_from_a_threadpool(self):
        # bpo-43944: clear concurrent.futures.thread._threads_queues after fork,
        # otherwise child process will try to join parent thread
        def fork_process_and_return_exitcode():
            # Ignore the warning about fork with threads.
            with self.assertWarnsRegex(DeprecationWarning,
                                       r"use of fork\(\) may lead to deadlocks in the child"):
                p = mp.get_context('fork').Process(target=lambda: 1)
                p.start()
            p.join()
            return p.exitcode

        with futures.ThreadPoolExecutor(1) as pool:
            process_exitcode = pool.submit(fork_process_and_return_exitcode).result()

        self.assertEqual(process_exitcode, 0)

    def test_executor_map_current_future_cancel(self):
        stop_event = threading.Event()
        log = []

        def log_n_wait(ident):
            log.append(f"{ident=} started")
            try:
                stop_event.wait()
            finally:
                log.append(f"{ident=} stopped")

        with self.executor_type(max_workers=1) as pool:
            # submit work to saturate the pool
            fut = pool.submit(log_n_wait, ident="first")
            try:
                with contextlib.closing(
                    pool.map(log_n_wait, ["second", "third"], timeout=0)
                ) as gen:
                    with self.assertRaises(TimeoutError):
                        next(gen)
            finally:
                stop_event.set()
            fut.result()
        # ident='second' is cancelled as a result of raising a TimeoutError
        # ident='third' is cancelled because it remained in the collection of futures
        self.assertListEqual(log, ["ident='first' started", "ident='first' stopped"])


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
