import os
import queue
import sys
import threading
import time
import unittest
import unittest.mock
from concurrent import futures
from concurrent.futures.process import BrokenProcessPool

from test import support
from test.support import hashlib_helper
from test.test_importlib.metadata.fixtures import parameterize

from .executor import ExecutorTest, mul
from .util import (
    ProcessPoolForkMixin, ProcessPoolForkserverMixin, ProcessPoolSpawnMixin,
    create_executor_tests, setup_module)


class EventfulGCObj():
    def __init__(self, mgr):
        self.event = mgr.Event()

    def __del__(self):
        self.event.set()

TERMINATE_WORKERS = futures.ProcessPoolExecutor.terminate_workers.__name__
KILL_WORKERS = futures.ProcessPoolExecutor.kill_workers.__name__
FORCE_SHUTDOWN_PARAMS = [
    dict(function_name=TERMINATE_WORKERS),
    dict(function_name=KILL_WORKERS),
]

def _put_wait_put(queue, event):
    """ Used as part of test_terminate_workers """
    queue.put('started')
    event.wait()

    # We should never get here since the event will not get set
    queue.put('finished')


class ProcessPoolExecutorTest(ExecutorTest):

    @unittest.skipUnless(sys.platform=='win32', 'Windows-only process limit')
    def test_max_workers_too_large(self):
        with self.assertRaisesRegex(ValueError,
                                    "max_workers must be <= 61"):
            futures.ProcessPoolExecutor(max_workers=62)

    def test_killed_child(self):
        # When a child process is abruptly terminated, the whole pool gets
        # "broken".
        futures = [self.executor.submit(time.sleep, 3)]
        # Get one of the processes, and terminate (kill) it
        p = next(iter(self.executor._processes.values()))
        p.terminate()
        for fut in futures:
            self.assertRaises(BrokenProcessPool, fut.result)
        # Submitting other jobs fails as well.
        self.assertRaises(BrokenProcessPool, self.executor.submit, pow, 2, 8)

    def test_map_chunksize(self):
        def bad_map():
            list(self.executor.map(pow, range(40), range(40), chunksize=-1))

        ref = list(map(pow, range(40), range(40)))
        self.assertEqual(
            list(self.executor.map(pow, range(40), range(40), chunksize=6)),
            ref)
        self.assertEqual(
            list(self.executor.map(pow, range(40), range(40), chunksize=50)),
            ref)
        self.assertEqual(
            list(self.executor.map(pow, range(40), range(40), chunksize=40)),
            ref)
        self.assertRaises(ValueError, bad_map)

    @classmethod
    def _test_traceback(cls):
        raise RuntimeError(123) # some comment

    def test_traceback(self):
        # We want ensure that the traceback from the child process is
        # contained in the traceback raised in the main process.
        future = self.executor.submit(self._test_traceback)
        with self.assertRaises(Exception) as cm:
            future.result()

        exc = cm.exception
        self.assertIs(type(exc), RuntimeError)
        self.assertEqual(exc.args, (123,))
        cause = exc.__cause__
        self.assertIs(type(cause), futures.process._RemoteTraceback)
        self.assertIn('raise RuntimeError(123) # some comment', cause.tb)

        with support.captured_stderr() as f1:
            try:
                raise exc
            except RuntimeError:
                sys.excepthook(*sys.exc_info())
        self.assertIn('raise RuntimeError(123) # some comment',
                      f1.getvalue())

    @hashlib_helper.requires_hashdigest('md5')
    def test_ressources_gced_in_workers(self):
        # Ensure that argument for a job are correctly gc-ed after the job
        # is finished
        mgr = self.get_context().Manager()
        obj = EventfulGCObj(mgr)
        future = self.executor.submit(id, obj)
        future.result()

        self.assertTrue(obj.event.wait(timeout=1))

        # explicitly destroy the object to ensure that EventfulGCObj.__del__()
        # is called while manager is still running.
        support.gc_collect()
        obj = None
        support.gc_collect()

        mgr.shutdown()
        mgr.join()

    def test_saturation(self):
        executor = self.executor
        mp_context = self.get_context()
        sem = mp_context.Semaphore(0)
        job_count = 15 * executor._max_workers
        for _ in range(job_count):
            executor.submit(sem.acquire)
        self.assertEqual(len(executor._processes), executor._max_workers)
        for _ in range(job_count):
            sem.release()

    @support.requires_gil_enabled("gh-117344: test is flaky without the GIL")
    def test_idle_process_reuse_one(self):
        executor = self.executor
        assert executor._max_workers >= 4
        if self.get_context().get_start_method(allow_none=False) == "fork":
            raise unittest.SkipTest("Incompatible with the fork start method.")
        executor.submit(mul, 21, 2).result()
        executor.submit(mul, 6, 7).result()
        executor.submit(mul, 3, 14).result()
        self.assertEqual(len(executor._processes), 1)

    def test_idle_process_reuse_multiple(self):
        executor = self.executor
        assert executor._max_workers <= 5
        if self.get_context().get_start_method(allow_none=False) == "fork":
            raise unittest.SkipTest("Incompatible with the fork start method.")
        executor.submit(mul, 12, 7).result()
        executor.submit(mul, 33, 25)
        executor.submit(mul, 25, 26).result()
        executor.submit(mul, 18, 29)
        executor.submit(mul, 1, 2).result()
        executor.submit(mul, 0, 9)
        self.assertLessEqual(len(executor._processes), 3)
        executor.shutdown()

    def test_max_tasks_per_child(self):
        context = self.get_context()
        if context.get_start_method(allow_none=False) == "fork":
            with self.assertRaises(ValueError):
                self.executor_type(1, mp_context=context, max_tasks_per_child=3)
            return
        # not using self.executor as we need to control construction.
        # arguably this could go in another class w/o that mixin.
        executor = self.executor_type(
                1, mp_context=context, max_tasks_per_child=3)
        f1 = executor.submit(os.getpid)
        original_pid = f1.result()
        # The worker pid remains the same as the worker could be reused
        f2 = executor.submit(os.getpid)
        self.assertEqual(f2.result(), original_pid)
        self.assertEqual(len(executor._processes), 1)
        f3 = executor.submit(os.getpid)
        self.assertEqual(f3.result(), original_pid)

        # A new worker is spawned, with a statistically different pid,
        # while the previous was reaped.
        f4 = executor.submit(os.getpid)
        new_pid = f4.result()
        self.assertNotEqual(original_pid, new_pid)
        self.assertEqual(len(executor._processes), 1)

        executor.shutdown()

    def test_max_tasks_per_child_defaults_to_spawn_context(self):
        # not using self.executor as we need to control construction.
        # arguably this could go in another class w/o that mixin.
        executor = self.executor_type(1, max_tasks_per_child=3)
        self.assertEqual(executor._mp_context.get_start_method(), "spawn")

    def test_max_tasks_early_shutdown(self):
        context = self.get_context()
        if context.get_start_method(allow_none=False) == "fork":
            raise unittest.SkipTest("Incompatible with the fork start method.")
        # not using self.executor as we need to control construction.
        # arguably this could go in another class w/o that mixin.
        executor = self.executor_type(
                3, mp_context=context, max_tasks_per_child=1)
        futures = []
        for i in range(6):
            futures.append(executor.submit(mul, i, i))
        executor.shutdown()
        for i, future in enumerate(futures):
            self.assertEqual(future.result(), mul(i, i))

    def test_python_finalization_error(self):
        # gh-109047: Catch RuntimeError on thread creation
        # during Python finalization.

        context = self.get_context()

        # gh-109047: Mock the threading.start_joinable_thread() function to inject
        # RuntimeError: simulate the error raised during Python finalization.
        # Block the second creation: create _ExecutorManagerThread, but block
        # QueueFeederThread.
        orig_start_new_thread = threading._start_joinable_thread
        nthread = 0
        def mock_start_new_thread(func, *args, **kwargs):
            nonlocal nthread
            if nthread >= 1:
                raise RuntimeError("can't create new thread at "
                                   "interpreter shutdown")
            nthread += 1
            return orig_start_new_thread(func, *args, **kwargs)

        with support.swap_attr(threading, '_start_joinable_thread',
                               mock_start_new_thread):
            executor = self.executor_type(max_workers=2, mp_context=context)
            with executor:
                with self.assertRaises(BrokenProcessPool):
                    list(executor.map(mul, [(2, 3)] * 10))
            executor.shutdown()

    def test_terminate_workers(self):
        mock_fn = unittest.mock.Mock()
        with self.executor_type(max_workers=1) as executor:
            executor._force_shutdown = mock_fn
            executor.terminate_workers()

        mock_fn.assert_called_once_with(operation=futures.process._TERMINATE)

    def test_kill_workers(self):
        mock_fn = unittest.mock.Mock()
        with self.executor_type(max_workers=1) as executor:
            executor._force_shutdown = mock_fn
            executor.kill_workers()

        mock_fn.assert_called_once_with(operation=futures.process._KILL)

    def test_force_shutdown_workers_invalid_op(self):
        with self.executor_type(max_workers=1) as executor:
            self.assertRaises(ValueError,
                              executor._force_shutdown,
                              operation='invalid operation'),

    @parameterize(*FORCE_SHUTDOWN_PARAMS)
    def test_force_shutdown_workers(self, function_name):
        manager = self.get_context().Manager()
        q = manager.Queue()
        e = manager.Event()

        with self.executor_type(max_workers=1) as executor:
            executor.submit(_put_wait_put, q, e)

            # We should get started, but not finished since we'll terminate the
            # workers just after and never set the event.
            self.assertEqual(q.get(timeout=support.SHORT_TIMEOUT), 'started')

            worker_process = list(executor._processes.values())[0]

            Mock = unittest.mock.Mock
            worker_process.terminate = Mock(wraps=worker_process.terminate)
            worker_process.kill = Mock(wraps=worker_process.kill)

            getattr(executor, function_name)()
            worker_process.join()

            if function_name == TERMINATE_WORKERS:
                worker_process.terminate.assert_called()
            elif function_name == KILL_WORKERS:
                worker_process.kill.assert_called()
            else:
                self.fail(f"Unknown operation: {function_name}")

            self.assertRaises(queue.Empty, q.get, timeout=0.01)

    @parameterize(*FORCE_SHUTDOWN_PARAMS)
    def test_force_shutdown_workers_dead_workers(self, function_name):
        with self.executor_type(max_workers=1) as executor:
            future = executor.submit(os._exit, 1)
            self.assertRaises(BrokenProcessPool, future.result)

            # even though the pool is broken, this shouldn't raise
            getattr(executor, function_name)()

    @parameterize(*FORCE_SHUTDOWN_PARAMS)
    def test_force_shutdown_workers_not_started_yet(self, function_name):
        ctx = self.get_context()
        with unittest.mock.patch.object(ctx, 'Process') as mock_process:
            with self.executor_type(max_workers=1, mp_context=ctx) as executor:
                # The worker has not been started yet, terminate/kill_workers
                # should basically no-op
                getattr(executor, function_name)()

            mock_process.return_value.kill.assert_not_called()
            mock_process.return_value.terminate.assert_not_called()

    @parameterize(*FORCE_SHUTDOWN_PARAMS)
    def test_force_shutdown_workers_stops_pool(self, function_name):
        with self.executor_type(max_workers=1) as executor:
            task = executor.submit(time.sleep, 0)
            self.assertIsNone(task.result())

            worker_process = list(executor._processes.values())[0]
            getattr(executor, function_name)()

            self.assertRaises(RuntimeError, executor.submit, time.sleep, 0)

            # A signal sent, is not a signal reacted to.
            # So wait a moment here for the process to die.
            # If we don't, every once in a while we may get an ENV CHANGE
            # error since the process would be alive immediately after the
            # test run.. and die a moment later.
            worker_process.join(support.SHORT_TIMEOUT)

            # Oddly enough, even though join completes, sometimes it takes a
            # moment for the process to actually be marked as dead.
            # ...  that seems a bit buggy.
            # We need it dead before ending the test to ensure it doesn't
            # get marked as an ENV CHANGE due to living child process.
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT):
                if not worker_process.is_alive():
                    break


create_executor_tests(globals(), ProcessPoolExecutorTest,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
