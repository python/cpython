import contextlib
import queue
import signal
import sys
import time
import unittest
import unittest.mock
from pickle import PicklingError
from concurrent import futures
from concurrent.futures.process import BrokenProcessPool, _ThreadWakeup

from test import support
from test.support import warnings_helper

from .util import (
    create_executor_tests, setup_module,
    ProcessPoolForkMixin, ProcessPoolForkserverMixin, ProcessPoolSpawnMixin)


def _crash(delay=None):
    """Induces a segfault."""
    if delay:
        time.sleep(delay)
    import faulthandler
    faulthandler.disable()
    faulthandler._sigsegv()


def _crash_with_data(data):
    """Induces a segfault with dummy data in input."""
    _crash()


def _exit():
    """Induces a sys exit with exitcode 1."""
    sys.exit(1)


def _raise_error(Err):
    """Function that raises an Exception in process."""
    raise Err()


def _raise_error_ignore_stderr(Err):
    """Function that raises an Exception in process and ignores stderr."""
    import io
    sys.stderr = io.StringIO()
    raise Err()


def _return_instance(cls):
    """Function that returns a instance of cls."""
    return cls()


class CrashAtPickle(object):
    """Bad object that triggers a segfault at pickling time."""
    def __reduce__(self):
        _crash()


class CrashAtUnpickle(object):
    """Bad object that triggers a segfault at unpickling time."""
    def __reduce__(self):
        return _crash, ()


class ExitAtPickle(object):
    """Bad object that triggers a process exit at pickling time."""
    def __reduce__(self):
        _exit()


class ExitAtUnpickle(object):
    """Bad object that triggers a process exit at unpickling time."""
    def __reduce__(self):
        return _exit, ()


class ErrorAtPickle(object):
    """Bad object that triggers an error at pickling time."""
    def __reduce__(self):
        from pickle import PicklingError
        raise PicklingError("Error in pickle")


class ErrorAtUnpickle(object):
    """Bad object that triggers an error at unpickling time."""
    def __reduce__(self):
        from pickle import UnpicklingError
        return _raise_error_ignore_stderr, (UnpicklingError, )


class ExecutorDeadlockTest:
    TIMEOUT = support.LONG_TIMEOUT

    def _fail_on_deadlock(self, executor):
        # If we did not recover before TIMEOUT seconds, consider that the
        # executor is in a deadlock state and forcefully clean all its
        # composants.
        import faulthandler
        from tempfile import TemporaryFile
        with TemporaryFile(mode="w+") as f:
            faulthandler.dump_traceback(file=f)
            f.seek(0)
            tb = f.read()
        for p in executor._processes.values():
            p.terminate()
        # This should be safe to call executor.shutdown here as all possible
        # deadlocks should have been broken.
        executor.shutdown(wait=True)
        print(f"\nTraceback:\n {tb}", file=sys.__stderr__)
        self.fail(f"Executor deadlock:\n\n{tb}")

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def _check_error(self, error, func, *args, ignore_stderr=False):
        # test for deadlock caused by crashes or exiting in a pool
        self.executor.shutdown(wait=True)

        executor = self.executor_type(
            max_workers=2, mp_context=self.get_context())
        res = executor.submit(func, *args)

        if ignore_stderr:
            cm = support.captured_stderr()
        else:
            cm = contextlib.nullcontext()

        try:
            with self.assertRaises(error):
                with cm:
                    res.result(timeout=self.TIMEOUT)
        except futures.TimeoutError:
            # If we did not recover before TIMEOUT seconds,
            # consider that the executor is in a deadlock state
            self._fail_on_deadlock(executor)
        executor.shutdown(wait=True)

    def test_error_at_task_pickle(self):
        # Check problem occurring while pickling a task in
        # the task_handler thread
        self._check_error(PicklingError, id, ErrorAtPickle())

    def test_exit_at_task_unpickle(self):
        # Check problem occurring while unpickling a task on workers
        self._check_error(BrokenProcessPool, id, ExitAtUnpickle())

    def test_error_at_task_unpickle(self):
        # gh-109832: Restore stderr overridden by _raise_error_ignore_stderr()
        self.addCleanup(setattr, sys, 'stderr', sys.stderr)

        # Check problem occurring while unpickling a task on workers
        self._check_error(BrokenProcessPool, id, ErrorAtUnpickle())

    @support.skip_if_sanitizer("UBSan: explicit SIGSEV not allowed", ub=True)
    def test_crash_at_task_unpickle(self):
        # Check problem occurring while unpickling a task on workers
        self._check_error(BrokenProcessPool, id, CrashAtUnpickle())

    @support.skip_if_sanitizer("UBSan: explicit SIGSEV not allowed", ub=True)
    def test_crash_during_func_exec_on_worker(self):
        # Check problem occurring during func execution on workers
        self._check_error(BrokenProcessPool, _crash)

    def test_exit_during_func_exec_on_worker(self):
        # Check problem occurring during func execution on workers
        self._check_error(SystemExit, _exit)

    def test_error_during_func_exec_on_worker(self):
        # Check problem occurring during func execution on workers
        self._check_error(RuntimeError, _raise_error, RuntimeError)

    @support.skip_if_sanitizer("UBSan: explicit SIGSEV not allowed", ub=True)
    def test_crash_during_result_pickle_on_worker(self):
        # Check problem occurring while pickling a task result
        # on workers
        self._check_error(BrokenProcessPool, _return_instance, CrashAtPickle)

    def test_exit_during_result_pickle_on_worker(self):
        # Check problem occurring while pickling a task result
        # on workers
        self._check_error(SystemExit, _return_instance, ExitAtPickle)

    def test_error_during_result_pickle_on_worker(self):
        # Check problem occurring while pickling a task result
        # on workers
        self._check_error(PicklingError, _return_instance, ErrorAtPickle)

    def test_error_during_result_unpickle_in_result_handler(self):
        # gh-109832: Restore stderr overridden by _raise_error_ignore_stderr()
        self.addCleanup(setattr, sys, 'stderr', sys.stderr)

        # Check problem occurring while unpickling a task in
        # the result_handler thread
        self._check_error(BrokenProcessPool,
                          _return_instance, ErrorAtUnpickle,
                          ignore_stderr=True)

    def test_exit_during_result_unpickle_in_result_handler(self):
        # Check problem occurring while unpickling a task in
        # the result_handler thread
        self._check_error(BrokenProcessPool, _return_instance, ExitAtUnpickle)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @support.skip_if_sanitizer("UBSan: explicit SIGSEV not allowed", ub=True)
    def test_shutdown_deadlock(self):
        # Test that the pool calling shutdown do not cause deadlock
        # if a worker fails after the shutdown call.
        self.executor.shutdown(wait=True)
        with self.executor_type(max_workers=2,
                                mp_context=self.get_context()) as executor:
            self.executor = executor  # Allow clean up in fail_on_deadlock
            f = executor.submit(_crash, delay=.1)
            executor.shutdown(wait=True)
            with self.assertRaises(BrokenProcessPool):
                f.result()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_shutdown_deadlock_pickle(self):
        # Test that the pool calling shutdown with wait=False does not cause
        # a deadlock if a task fails at pickle after the shutdown call.
        # Reported in bpo-39104.
        self.executor.shutdown(wait=True)
        with self.executor_type(max_workers=2,
                                mp_context=self.get_context()) as executor:
            self.executor = executor  # Allow clean up in fail_on_deadlock

            # Start the executor and get the executor_manager_thread to collect
            # the threads and avoid dangling thread that should be cleaned up
            # asynchronously.
            executor.submit(id, 42).result()
            executor_manager = executor._executor_manager_thread

            # Submit a task that fails at pickle and shutdown the executor
            # without waiting
            f = executor.submit(id, ErrorAtPickle())
            executor.shutdown(wait=False)
            with self.assertRaises(PicklingError):
                f.result()

        # Make sure the executor is eventually shutdown and do not leave
        # dangling threads
        executor_manager.join()

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @support.skip_if_sanitizer("UBSan: explicit SIGSEV not allowed", ub=True)
    def test_crash_big_data(self):
        # Test that there is a clean exception instead of a deadlock when a
        # child process crashes while some data is being written into the
        # queue.
        # https://github.com/python/cpython/issues/94777
        self.executor.shutdown(wait=True)
        data = "a" * support.PIPE_MAX_SIZE
        with self.executor_type(max_workers=2,
                                mp_context=self.get_context()) as executor:
            self.executor = executor  # Allow clean up in fail_on_deadlock
            with self.assertRaises(BrokenProcessPool):
                list(executor.map(_crash_with_data, [data] * 10))

        executor.shutdown(wait=True)

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    def test_gh105829_should_not_deadlock_if_wakeup_pipe_full(self):
        # Issue #105829: The _ExecutorManagerThread wakeup pipe could
        # fill up and block. See: https://github.com/python/cpython/issues/105829

        # Lots of cargo culting while writing this test, apologies if
        # something is really stupid...

        self.executor.shutdown(wait=True)

        if not hasattr(signal, 'alarm'):
            raise unittest.SkipTest(
                "Tested platform does not support the alarm signal")

        def timeout(_signum, _frame):
            import faulthandler
            faulthandler.dump_traceback()

            raise RuntimeError("timed out while submitting jobs?")

        thread_run = futures.process._ExecutorManagerThread.run
        def mock_run(self):
            # Delay thread startup so the wakeup pipe can fill up and block
            time.sleep(3)
            thread_run(self)

        class MockWakeup(_ThreadWakeup):
            """Mock wakeup object to force the wakeup to block"""
            def __init__(self):
                super().__init__()
                self._dummy_queue = queue.Queue(maxsize=1)

            def wakeup(self):
                self._dummy_queue.put(None, block=True)
                super().wakeup()

            def clear(self):
                super().clear()
                try:
                    while True:
                        self._dummy_queue.get_nowait()
                except queue.Empty:
                    pass

        with (unittest.mock.patch.object(futures.process._ExecutorManagerThread,
                                         'run', mock_run),
              unittest.mock.patch('concurrent.futures.process._ThreadWakeup',
                                  MockWakeup)):
            with self.executor_type(max_workers=2,
                                    mp_context=self.get_context()) as executor:
                self.executor = executor  # Allow clean up in fail_on_deadlock

                job_num = 100
                job_data = range(job_num)

                # Need to use sigalarm for timeout detection because
                # Executor.submit is not guarded by any timeout (both
                # self._work_ids.put(self._queue_count) and
                # self._executor_manager_thread_wakeup.wakeup() might
                # timeout, maybe more?). In this specific case it was
                # the wakeup call that deadlocked on a blocking pipe.
                old_handler = signal.signal(signal.SIGALRM, timeout)
                try:
                    signal.alarm(int(self.TIMEOUT))
                    self.assertEqual(job_num, len(list(executor.map(int, job_data))))
                finally:
                    signal.alarm(0)
                    signal.signal(signal.SIGALRM, old_handler)


create_executor_tests(globals(), ExecutorDeadlockTest,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))

def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
