import signal
import sys
import threading
import time
import unittest
from concurrent import futures

from test import support
from test.support import warnings_helper
from test.support.script_helper import assert_python_ok

from .util import (
    BaseTestCase, ThreadPoolMixin, ProcessPoolForkMixin,
    ProcessPoolForkserverMixin, ProcessPoolSpawnMixin,
    create_executor_tests, setup_module)


def sleep_and_print(t, msg):
    time.sleep(t)
    print(msg)
    sys.stdout.flush()


class ExecutorShutdownTest:
    def test_run_after_shutdown(self):
        self.executor.shutdown()
        self.assertRaises(RuntimeError,
                          self.executor.submit,
                          pow, 2, 5)

    def test_interpreter_shutdown(self):
        # Test the atexit hook for shutdown of worker threads and processes
        rc, out, err = assert_python_ok('-c', """if 1:
            from concurrent.futures import {executor_type}
            from time import sleep
            from test.test_concurrent_futures.test_shutdown import sleep_and_print
            if __name__ == "__main__":
                context = '{context}'
                if context == "":
                    t = {executor_type}(5)
                else:
                    from multiprocessing import get_context
                    context = get_context(context)
                    t = {executor_type}(5, mp_context=context)
                t.submit(sleep_and_print, 1.0, "apple")
            """.format(executor_type=self.executor_type.__name__,
                       context=getattr(self, "ctx", "")))
        # Errors in atexit hooks don't change the process exit code, check
        # stderr manually.
        self.assertFalse(err)
        self.assertEqual(out.strip(), b"apple")

    @support.force_not_colorized
    def test_submit_after_interpreter_shutdown(self):
        # Test the atexit hook for shutdown of worker threads and processes
        rc, out, err = assert_python_ok('-c', """if 1:
            import atexit
            @atexit.register
            def run_last():
                try:
                    t.submit(id, None)
                except RuntimeError:
                    print("runtime-error")
                    raise
            from concurrent.futures import {executor_type}
            if __name__ == "__main__":
                context = '{context}'
                if not context:
                    t = {executor_type}(5)
                else:
                    from multiprocessing import get_context
                    context = get_context(context)
                    t = {executor_type}(5, mp_context=context)
                    t.submit(id, 42).result()
            """.format(executor_type=self.executor_type.__name__,
                       context=getattr(self, "ctx", "")))
        # Errors in atexit hooks don't change the process exit code, check
        # stderr manually.
        self.assertIn("RuntimeError: cannot schedule new futures", err.decode())
        self.assertEqual(out.strip(), b"runtime-error")

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_hang_issue12364(self):
        fs = [self.executor.submit(time.sleep, 0.1) for _ in range(50)]
        self.executor.shutdown()
        for f in fs:
            f.result()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_cancel_futures(self):
        assert self.worker_count <= 5, "test needs few workers"
        fs = [self.executor.submit(time.sleep, .1) for _ in range(50)]
        self.executor.shutdown(cancel_futures=True)
        # We can't guarantee the exact number of cancellations, but we can
        # guarantee that *some* were cancelled. With few workers, many of
        # the submitted futures should have been cancelled.
        cancelled = [fut for fut in fs if fut.cancelled()]
        self.assertGreater(len(cancelled), 20)

        # Ensure the other futures were able to finish.
        # Use "not fut.cancelled()" instead of "fut.done()" to include futures
        # that may have been left in a pending state.
        others = [fut for fut in fs if not fut.cancelled()]
        for fut in others:
            self.assertTrue(fut.done(), msg=f"{fut._state=}")
            self.assertIsNone(fut.exception())

        # Similar to the number of cancelled futures, we can't guarantee the
        # exact number that completed. But, we can guarantee that at least
        # one finished.
        self.assertGreater(len(others), 0)

    def test_hang_gh83386(self):
        """shutdown(wait=False) doesn't hang at exit with running futures.

        See https://github.com/python/cpython/issues/83386.
        """
        if self.executor_type == futures.ProcessPoolExecutor:
            raise unittest.SkipTest(
                "Hangs, see https://github.com/python/cpython/issues/83386")

        rc, out, err = assert_python_ok('-c', """if True:
            from concurrent.futures import {executor_type}
            from test.test_concurrent_futures.test_shutdown import sleep_and_print
            if __name__ == "__main__":
                if {context!r}: multiprocessing.set_start_method({context!r})
                t = {executor_type}(max_workers=3)
                t.submit(sleep_and_print, 1.0, "apple")
                t.shutdown(wait=False)
            """.format(executor_type=self.executor_type.__name__,
                       context=getattr(self, 'ctx', None)))
        self.assertFalse(err)
        self.assertEqual(out.strip(), b"apple")

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_hang_gh94440(self):
        """shutdown(wait=True) doesn't hang when a future was submitted and
        quickly canceled right before shutdown.

        See https://github.com/python/cpython/issues/94440.
        """
        if not hasattr(signal, 'alarm'):
            raise unittest.SkipTest(
                "Tested platform does not support the alarm signal")

        def timeout(_signum, _frame):
            raise RuntimeError("timed out waiting for shutdown")

        kwargs = {}
        if getattr(self, 'ctx', None):
            kwargs['mp_context'] = self.get_context()
        executor = self.executor_type(max_workers=1, **kwargs)
        executor.submit(int).result()
        old_handler = signal.signal(signal.SIGALRM, timeout)
        try:
            signal.alarm(5)
            executor.submit(int).cancel()
            executor.shutdown(wait=True)
        finally:
            signal.alarm(0)
            signal.signal(signal.SIGALRM, old_handler)


class ThreadPoolShutdownTest(ThreadPoolMixin, ExecutorShutdownTest, BaseTestCase):
    def test_threads_terminate(self):
        def acquire_lock(lock):
            lock.acquire()

        sem = threading.Semaphore(0)
        for i in range(3):
            self.executor.submit(acquire_lock, sem)
        self.assertEqual(len(self.executor._threads), 3)
        for i in range(3):
            sem.release()
        self.executor.shutdown()
        for t in self.executor._threads:
            t.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_context_manager_shutdown(self):
        with futures.ThreadPoolExecutor(max_workers=5) as e:
            executor = e
            self.assertEqual(list(e.map(abs, range(-5, 5))),
                             [5, 4, 3, 2, 1, 0, 1, 2, 3, 4])

        for t in executor._threads:
            t.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_del_shutdown(self):
        executor = futures.ThreadPoolExecutor(max_workers=5)
        res = executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor

        for t in threads:
            t.join()

        # Make sure the results were all computed before the
        # executor got shutdown.
        assert all([r == abs(v) for r, v in zip(res, range(-5, 5))])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_shutdown_no_wait(self):
        # Ensure that the executor cleans up the threads when calling
        # shutdown with wait=False
        executor = futures.ThreadPoolExecutor(max_workers=5)
        res = executor.map(abs, range(-5, 5))
        threads = executor._threads
        executor.shutdown(wait=False)
        for t in threads:
            t.join()

        # Make sure the results were all computed before the
        # executor got shutdown.
        assert all([r == abs(v) for r, v in zip(res, range(-5, 5))])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_thread_names_assigned(self):
        executor = futures.ThreadPoolExecutor(
            max_workers=5, thread_name_prefix='SpecialPool')
        executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor
        support.gc_collect()  # For PyPy or other GCs.

        for t in threads:
            self.assertRegex(t.name, r'^SpecialPool_[0-4]$')
            t.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_thread_names_default(self):
        executor = futures.ThreadPoolExecutor(max_workers=5)
        executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor
        support.gc_collect()  # For PyPy or other GCs.

        for t in threads:
            # Ensure that our default name is reasonably sane and unique when
            # no thread_name_prefix was supplied.
            self.assertRegex(t.name, r'ThreadPoolExecutor-\d+_[0-4]$')
            t.join()

    def test_cancel_futures_wait_false(self):
        # Can only be reliably tested for TPE, since PPE often hangs with
        # `wait=False` (even without *cancel_futures*).
        rc, out, err = assert_python_ok('-c', """if True:
            from concurrent.futures import ThreadPoolExecutor
            from test.test_concurrent_futures.test_shutdown import sleep_and_print
            if __name__ == "__main__":
                t = ThreadPoolExecutor()
                t.submit(sleep_and_print, .1, "apple")
                t.shutdown(wait=False, cancel_futures=True)
            """)
        # Errors in atexit hooks don't change the process exit code, check
        # stderr manually.
        self.assertFalse(err)
        # gh-116682: stdout may be empty if shutdown happens before task
        # starts executing.
        self.assertIn(out.strip(), [b"apple", b""])


class ProcessPoolShutdownTest(ExecutorShutdownTest):
    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_processes_terminate(self):
        def acquire_lock(lock):
            lock.acquire()

        mp_context = self.get_context()
        if mp_context.get_start_method(allow_none=False) == "fork":
            # fork pre-spawns, not on demand.
            expected_num_processes = self.worker_count
        else:
            expected_num_processes = 3

        sem = mp_context.Semaphore(0)
        for _ in range(3):
            self.executor.submit(acquire_lock, sem)
        self.assertEqual(len(self.executor._processes), expected_num_processes)
        for _ in range(3):
            sem.release()
        processes = self.executor._processes
        self.executor.shutdown()

        for p in processes.values():
            p.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_context_manager_shutdown(self):
        with futures.ProcessPoolExecutor(
                max_workers=5, mp_context=self.get_context()) as e:
            processes = e._processes
            self.assertEqual(list(e.map(abs, range(-5, 5))),
                             [5, 4, 3, 2, 1, 0, 1, 2, 3, 4])

        for p in processes.values():
            p.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_del_shutdown(self):
        executor = futures.ProcessPoolExecutor(
                max_workers=5, mp_context=self.get_context())
        res = executor.map(abs, range(-5, 5))
        executor_manager_thread = executor._executor_manager_thread
        processes = executor._processes
        call_queue = executor._call_queue
        executor_manager_thread = executor._executor_manager_thread
        del executor
        support.gc_collect()  # For PyPy or other GCs.

        # Make sure that all the executor resources were properly cleaned by
        # the shutdown process
        executor_manager_thread.join()
        for p in processes.values():
            p.join()
        call_queue.join_thread()

        # Make sure the results were all computed before the
        # executor got shutdown.
        assert all([r == abs(v) for r, v in zip(res, range(-5, 5))])

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_shutdown_no_wait(self):
        # Ensure that the executor cleans up the processes when calling
        # shutdown with wait=False
        executor = futures.ProcessPoolExecutor(
                max_workers=5, mp_context=self.get_context())
        res = executor.map(abs, range(-5, 5))
        processes = executor._processes
        call_queue = executor._call_queue
        executor_manager_thread = executor._executor_manager_thread
        executor.shutdown(wait=False)

        # Make sure that all the executor resources were properly cleaned by
        # the shutdown process
        executor_manager_thread.join()
        for p in processes.values():
            p.join()
        call_queue.join_thread()

        # Make sure the results were all computed before the executor got
        # shutdown.
        assert all([r == abs(v) for r, v in zip(res, range(-5, 5))])

    @classmethod
    def _failing_task_gh_132969(cls, n):
        raise ValueError("failing task")

    @classmethod
    def _good_task_gh_132969(cls, n):
        time.sleep(0.1 * n)
        return n

    def _run_test_issue_gh_132969(self, max_workers):
        # max_workers=2 will repro exception
        # max_workers=4 will repro exception and then hang

        # Repro conditions
        #   max_tasks_per_child=1
        #   a task ends abnormally
        #   shutdown(wait=False) is called
        start_method = self.get_context().get_start_method()
        if (start_method == "fork" or
           (start_method == "forkserver" and sys.platform.startswith("win"))):
                self.skipTest(f"Skipping test for {start_method = }")
        executor = futures.ProcessPoolExecutor(
                max_workers=max_workers,
                max_tasks_per_child=1,
                mp_context=self.get_context())
        f1 = executor.submit(ProcessPoolShutdownTest._good_task_gh_132969, 1)
        f2 = executor.submit(ProcessPoolShutdownTest._failing_task_gh_132969, 2)
        f3 = executor.submit(ProcessPoolShutdownTest._good_task_gh_132969, 3)
        result = 0
        try:
            result += f1.result()
            result += f2.result()
            result += f3.result()
        except ValueError:
            # stop processing results upon first exception
            pass

        # Ensure that the executor cleans up after called
        # shutdown with wait=False
        executor_manager_thread = executor._executor_manager_thread
        executor.shutdown(wait=False)
        time.sleep(0.2)
        executor_manager_thread.join()
        return result

    def test_shutdown_gh_132969_case_1(self):
        # gh-132969: test that exception "object of type 'NoneType' has no len()"
        # is not raised when shutdown(wait=False) is called.
        result = self._run_test_issue_gh_132969(2)
        self.assertEqual(result, 1)

    def test_shutdown_gh_132969_case_2(self):
        # gh-132969: test that process does not hang and
        # exception "object of type 'NoneType' has no len()" is not raised
        # when shutdown(wait=False) is called.
        result = self._run_test_issue_gh_132969(4)
        self.assertEqual(result, 1)


create_executor_tests(globals(), ProcessPoolShutdownTest,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
