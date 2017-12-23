import test.support

# Skip tests if _multiprocessing wasn't built.
test.support.import_module('_multiprocessing')
# Skip tests if sem_open implementation is broken.
test.support.import_module('multiprocessing.synchronize')

from test.support.script_helper import assert_python_ok

import contextlib
import itertools
import logging
from logging.handlers import QueueHandler
import os
import queue
import sys
import threading
import time
import unittest
import weakref

from concurrent import futures
from concurrent.futures._base import (
    PENDING, RUNNING, CANCELLED, CANCELLED_AND_NOTIFIED, FINISHED, Future,
    BrokenExecutor)
from concurrent.futures.process import BrokenProcessPool
from multiprocessing import get_context


def create_future(state=PENDING, exception=None, result=None):
    f = Future()
    f._state = state
    f._exception = exception
    f._result = result
    return f


PENDING_FUTURE = create_future(state=PENDING)
RUNNING_FUTURE = create_future(state=RUNNING)
CANCELLED_FUTURE = create_future(state=CANCELLED)
CANCELLED_AND_NOTIFIED_FUTURE = create_future(state=CANCELLED_AND_NOTIFIED)
EXCEPTION_FUTURE = create_future(state=FINISHED, exception=OSError())
SUCCESSFUL_FUTURE = create_future(state=FINISHED, result=42)

INITIALIZER_STATUS = 'uninitialized'


def mul(x, y):
    return x * y

def sleep_and_raise(t):
    time.sleep(t)
    raise Exception('this is an exception')

def sleep_and_print(t, msg):
    time.sleep(t)
    print(msg)
    sys.stdout.flush()

def init(x):
    global INITIALIZER_STATUS
    INITIALIZER_STATUS = x

def get_init_status():
    return INITIALIZER_STATUS

def init_fail(log_queue=None):
    if log_queue is not None:
        logger = logging.getLogger('concurrent.futures')
        logger.addHandler(QueueHandler(log_queue))
        logger.setLevel('CRITICAL')
        logger.propagate = False
    time.sleep(0.1)  # let some futures be scheduled
    raise ValueError('error in initializer')


class MyObject(object):
    def my_method(self):
        pass


class EventfulGCObj():
    def __init__(self, ctx):
        mgr = get_context(ctx).Manager()
        self.event = mgr.Event()

    def __del__(self):
        self.event.set()


def make_dummy_object(_):
    return MyObject()


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._thread_key = test.support.threading_setup()

    def tearDown(self):
        test.support.reap_children()
        test.support.threading_cleanup(*self._thread_key)


class ExecutorMixin:
    worker_count = 5
    executor_kwargs = {}

    def setUp(self):
        super().setUp()

        self.t1 = time.time()
        if hasattr(self, "ctx"):
            self.executor = self.executor_type(
                max_workers=self.worker_count,
                mp_context=self.get_context(),
                **self.executor_kwargs)
        else:
            self.executor = self.executor_type(
                max_workers=self.worker_count,
                **self.executor_kwargs)
        self._prime_executor()

    def tearDown(self):
        self.executor.shutdown(wait=True)
        self.executor = None

        dt = time.time() - self.t1
        if test.support.verbose:
            print("%.2fs" % dt, end=' ')
        self.assertLess(dt, 60, "synchronization issue: test lasted too long")

        super().tearDown()

    def get_context(self):
        return get_context(self.ctx)

    def _prime_executor(self):
        # Make sure that the executor is ready to do work before running the
        # tests. This should reduce the probability of timeouts in the tests.
        futures = [self.executor.submit(time.sleep, 0.1)
                   for _ in range(self.worker_count)]
        for f in futures:
            f.result()


class ThreadPoolMixin(ExecutorMixin):
    executor_type = futures.ThreadPoolExecutor


class ProcessPoolForkMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "fork"

    def get_context(self):
        if sys.platform == "win32":
            self.skipTest("require unix system")
        return super().get_context()


class ProcessPoolSpawnMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "spawn"


class ProcessPoolForkserverMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "forkserver"

    def get_context(self):
        if sys.platform == "win32":
            self.skipTest("require unix system")
        return super().get_context()


def create_executor_tests(mixin, bases=(BaseTestCase,),
                          executor_mixins=(ThreadPoolMixin,
                                           ProcessPoolForkMixin,
                                           ProcessPoolForkserverMixin,
                                           ProcessPoolSpawnMixin)):
    def strip_mixin(name):
        if name.endswith(('Mixin', 'Tests')):
            return name[:-5]
        elif name.endswith('Test'):
            return name[:-4]
        else:
            return name

    for exe in executor_mixins:
        name = ("%s%sTest"
                % (strip_mixin(exe.__name__), strip_mixin(mixin.__name__)))
        cls = type(name, (mixin,) + (exe,) + bases, {})
        globals()[name] = cls


class InitializerMixin(ExecutorMixin):
    worker_count = 2

    def setUp(self):
        global INITIALIZER_STATUS
        INITIALIZER_STATUS = 'uninitialized'
        self.executor_kwargs = dict(initializer=init,
                                    initargs=('initialized',))
        super().setUp()

    def test_initializer(self):
        futures = [self.executor.submit(get_init_status)
                   for _ in range(self.worker_count)]

        for f in futures:
            self.assertEqual(f.result(), 'initialized')


class FailingInitializerMixin(ExecutorMixin):
    worker_count = 2

    def setUp(self):
        if hasattr(self, "ctx"):
            # Pass a queue to redirect the child's logging output
            self.mp_context = self.get_context()
            self.log_queue = self.mp_context.Queue()
            self.executor_kwargs = dict(initializer=init_fail,
                                        initargs=(self.log_queue,))
        else:
            # In a thread pool, the child shares our logging setup
            # (see _assert_logged())
            self.mp_context = None
            self.log_queue = None
            self.executor_kwargs = dict(initializer=init_fail)
        super().setUp()

    def test_initializer(self):
        with self._assert_logged('ValueError: error in initializer'):
            try:
                future = self.executor.submit(get_init_status)
            except BrokenExecutor:
                # Perhaps the executor is already broken
                pass
            else:
                with self.assertRaises(BrokenExecutor):
                    future.result()
            # At some point, the executor should break
            t1 = time.time()
            while not self.executor._broken:
                if time.time() - t1 > 5:
                    self.fail("executor not broken after 5 s.")
                time.sleep(0.01)
            # ... and from this point submit() is guaranteed to fail
            with self.assertRaises(BrokenExecutor):
                self.executor.submit(get_init_status)

    def _prime_executor(self):
        pass

    @contextlib.contextmanager
    def _assert_logged(self, msg):
        if self.log_queue is not None:
            yield
            output = []
            try:
                while True:
                    output.append(self.log_queue.get_nowait().getMessage())
            except queue.Empty:
                pass
        else:
            with self.assertLogs('concurrent.futures', 'CRITICAL') as cm:
                yield
            output = cm.output
        self.assertTrue(any(msg in line for line in output),
                        output)


create_executor_tests(InitializerMixin)
create_executor_tests(FailingInitializerMixin)


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
            from test.test_concurrent_futures import sleep_and_print
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

    def test_hang_issue12364(self):
        fs = [self.executor.submit(time.sleep, 0.1) for _ in range(50)]
        self.executor.shutdown()
        for f in fs:
            f.result()


class ThreadPoolShutdownTest(ThreadPoolMixin, ExecutorShutdownTest, BaseTestCase):
    def _prime_executor(self):
        pass

    def test_threads_terminate(self):
        self.executor.submit(mul, 21, 2)
        self.executor.submit(mul, 6, 7)
        self.executor.submit(mul, 3, 14)
        self.assertEqual(len(self.executor._threads), 3)
        self.executor.shutdown()
        for t in self.executor._threads:
            t.join()

    def test_context_manager_shutdown(self):
        with futures.ThreadPoolExecutor(max_workers=5) as e:
            executor = e
            self.assertEqual(list(e.map(abs, range(-5, 5))),
                             [5, 4, 3, 2, 1, 0, 1, 2, 3, 4])

        for t in executor._threads:
            t.join()

    def test_del_shutdown(self):
        executor = futures.ThreadPoolExecutor(max_workers=5)
        executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor

        for t in threads:
            t.join()

    def test_thread_names_assigned(self):
        executor = futures.ThreadPoolExecutor(
            max_workers=5, thread_name_prefix='SpecialPool')
        executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor

        for t in threads:
            self.assertRegex(t.name, r'^SpecialPool_[0-4]$')
            t.join()

    def test_thread_names_default(self):
        executor = futures.ThreadPoolExecutor(max_workers=5)
        executor.map(abs, range(-5, 5))
        threads = executor._threads
        del executor

        for t in threads:
            # Ensure that our default name is reasonably sane and unique when
            # no thread_name_prefix was supplied.
            self.assertRegex(t.name, r'ThreadPoolExecutor-\d+_[0-4]$')
            t.join()


class ProcessPoolShutdownTest(ExecutorShutdownTest):
    def _prime_executor(self):
        pass

    def test_processes_terminate(self):
        self.executor.submit(mul, 21, 2)
        self.executor.submit(mul, 6, 7)
        self.executor.submit(mul, 3, 14)
        self.assertEqual(len(self.executor._processes), 5)
        processes = self.executor._processes
        self.executor.shutdown()

        for p in processes.values():
            p.join()

    def test_context_manager_shutdown(self):
        with futures.ProcessPoolExecutor(max_workers=5) as e:
            processes = e._processes
            self.assertEqual(list(e.map(abs, range(-5, 5))),
                             [5, 4, 3, 2, 1, 0, 1, 2, 3, 4])

        for p in processes.values():
            p.join()

    def test_del_shutdown(self):
        executor = futures.ProcessPoolExecutor(max_workers=5)
        list(executor.map(abs, range(-5, 5)))
        queue_management_thread = executor._queue_management_thread
        processes = executor._processes
        call_queue = executor._call_queue
        del executor

        queue_management_thread.join()
        for p in processes.values():
            p.join()
        call_queue.close()
        call_queue.join_thread()



create_executor_tests(ProcessPoolShutdownTest,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


class WaitTests:

    def test_first_completed(self):
        future1 = self.executor.submit(mul, 21, 2)
        future2 = self.executor.submit(time.sleep, 1.5)

        done, not_done = futures.wait(
                [CANCELLED_FUTURE, future1, future2],
                 return_when=futures.FIRST_COMPLETED)

        self.assertEqual(set([future1]), done)
        self.assertEqual(set([CANCELLED_FUTURE, future2]), not_done)

    def test_first_completed_some_already_completed(self):
        future1 = self.executor.submit(time.sleep, 1.5)

        finished, pending = futures.wait(
                 [CANCELLED_AND_NOTIFIED_FUTURE, SUCCESSFUL_FUTURE, future1],
                 return_when=futures.FIRST_COMPLETED)

        self.assertEqual(
                set([CANCELLED_AND_NOTIFIED_FUTURE, SUCCESSFUL_FUTURE]),
                finished)
        self.assertEqual(set([future1]), pending)

    def test_first_exception(self):
        future1 = self.executor.submit(mul, 2, 21)
        future2 = self.executor.submit(sleep_and_raise, 1.5)
        future3 = self.executor.submit(time.sleep, 3)

        finished, pending = futures.wait(
                [future1, future2, future3],
                return_when=futures.FIRST_EXCEPTION)

        self.assertEqual(set([future1, future2]), finished)
        self.assertEqual(set([future3]), pending)

    def test_first_exception_some_already_complete(self):
        future1 = self.executor.submit(divmod, 21, 0)
        future2 = self.executor.submit(time.sleep, 1.5)

        finished, pending = futures.wait(
                [SUCCESSFUL_FUTURE,
                 CANCELLED_FUTURE,
                 CANCELLED_AND_NOTIFIED_FUTURE,
                 future1, future2],
                return_when=futures.FIRST_EXCEPTION)

        self.assertEqual(set([SUCCESSFUL_FUTURE,
                              CANCELLED_AND_NOTIFIED_FUTURE,
                              future1]), finished)
        self.assertEqual(set([CANCELLED_FUTURE, future2]), pending)

    def test_first_exception_one_already_failed(self):
        future1 = self.executor.submit(time.sleep, 2)

        finished, pending = futures.wait(
                 [EXCEPTION_FUTURE, future1],
                 return_when=futures.FIRST_EXCEPTION)

        self.assertEqual(set([EXCEPTION_FUTURE]), finished)
        self.assertEqual(set([future1]), pending)

    def test_all_completed(self):
        future1 = self.executor.submit(divmod, 2, 0)
        future2 = self.executor.submit(mul, 2, 21)

        finished, pending = futures.wait(
                [SUCCESSFUL_FUTURE,
                 CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 future1,
                 future2],
                return_when=futures.ALL_COMPLETED)

        self.assertEqual(set([SUCCESSFUL_FUTURE,
                              CANCELLED_AND_NOTIFIED_FUTURE,
                              EXCEPTION_FUTURE,
                              future1,
                              future2]), finished)
        self.assertEqual(set(), pending)

    def test_timeout(self):
        future1 = self.executor.submit(mul, 6, 7)
        future2 = self.executor.submit(time.sleep, 6)

        finished, pending = futures.wait(
                [CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 SUCCESSFUL_FUTURE,
                 future1, future2],
                timeout=5,
                return_when=futures.ALL_COMPLETED)

        self.assertEqual(set([CANCELLED_AND_NOTIFIED_FUTURE,
                              EXCEPTION_FUTURE,
                              SUCCESSFUL_FUTURE,
                              future1]), finished)
        self.assertEqual(set([future2]), pending)


class ThreadPoolWaitTests(ThreadPoolMixin, WaitTests, BaseTestCase):

    def test_pending_calls_race(self):
        # Issue #14406: multi-threaded race condition when waiting on all
        # futures.
        event = threading.Event()
        def future_func():
            event.wait()
        oldswitchinterval = sys.getswitchinterval()
        sys.setswitchinterval(1e-6)
        try:
            fs = {self.executor.submit(future_func) for i in range(100)}
            event.set()
            futures.wait(fs, return_when=futures.ALL_COMPLETED)
        finally:
            sys.setswitchinterval(oldswitchinterval)


create_executor_tests(WaitTests,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


class AsCompletedTests:
    # TODO(brian@sweetapp.com): Should have a test with a non-zero timeout.
    def test_no_timeout(self):
        future1 = self.executor.submit(mul, 2, 21)
        future2 = self.executor.submit(mul, 7, 6)

        completed = set(futures.as_completed(
                [CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 SUCCESSFUL_FUTURE,
                 future1, future2]))
        self.assertEqual(set(
                [CANCELLED_AND_NOTIFIED_FUTURE,
                 EXCEPTION_FUTURE,
                 SUCCESSFUL_FUTURE,
                 future1, future2]),
                completed)

    def test_zero_timeout(self):
        future1 = self.executor.submit(time.sleep, 2)
        completed_futures = set()
        try:
            for future in futures.as_completed(
                    [CANCELLED_AND_NOTIFIED_FUTURE,
                     EXCEPTION_FUTURE,
                     SUCCESSFUL_FUTURE,
                     future1],
                    timeout=0):
                completed_futures.add(future)
        except futures.TimeoutError:
            pass

        self.assertEqual(set([CANCELLED_AND_NOTIFIED_FUTURE,
                              EXCEPTION_FUTURE,
                              SUCCESSFUL_FUTURE]),
                         completed_futures)

    def test_duplicate_futures(self):
        # Issue 20367. Duplicate futures should not raise exceptions or give
        # duplicate responses.
        # Issue #31641: accept arbitrary iterables.
        future1 = self.executor.submit(time.sleep, 2)
        completed = [
            f for f in futures.as_completed(itertools.repeat(future1, 3))
        ]
        self.assertEqual(len(completed), 1)

    def test_free_reference_yielded_future(self):
        # Issue #14406: Generator should not keep references
        # to finished futures.
        futures_list = [Future() for _ in range(8)]
        futures_list.append(create_future(state=CANCELLED_AND_NOTIFIED))
        futures_list.append(create_future(state=FINISHED, result=42))

        with self.assertRaises(futures.TimeoutError):
            for future in futures.as_completed(futures_list, timeout=0):
                futures_list.remove(future)
                wr = weakref.ref(future)
                del future
                self.assertIsNone(wr())

        futures_list[0].set_result("test")
        for future in futures.as_completed(futures_list):
            futures_list.remove(future)
            wr = weakref.ref(future)
            del future
            self.assertIsNone(wr())
            if futures_list:
                futures_list[0].set_result("test")

    def test_correct_timeout_exception_msg(self):
        futures_list = [CANCELLED_AND_NOTIFIED_FUTURE, PENDING_FUTURE,
                        RUNNING_FUTURE, SUCCESSFUL_FUTURE]

        with self.assertRaises(futures.TimeoutError) as cm:
            list(futures.as_completed(futures_list, timeout=0))

        self.assertEqual(str(cm.exception), '2 (of 4) futures unfinished')


create_executor_tests(AsCompletedTests)


class ExecutorTest:
    # Executor.shutdown() and context manager usage is tested by
    # ExecutorShutdownTest.
    def test_submit(self):
        future = self.executor.submit(pow, 2, 8)
        self.assertEqual(256, future.result())

    def test_submit_keyword(self):
        future = self.executor.submit(mul, 2, y=8)
        self.assertEqual(16, future.result())

    def test_map(self):
        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10))),
                list(map(pow, range(10), range(10))))

        self.assertEqual(
                list(self.executor.map(pow, range(10), range(10), chunksize=3)),
                list(map(pow, range(10), range(10))))

    def test_map_exception(self):
        i = self.executor.map(divmod, [1, 1, 1, 1], [2, 3, 0, 5])
        self.assertEqual(i.__next__(), (0, 1))
        self.assertEqual(i.__next__(), (0, 1))
        self.assertRaises(ZeroDivisionError, i.__next__)

    def test_map_timeout(self):
        results = []
        try:
            for i in self.executor.map(time.sleep,
                                       [0, 0, 6],
                                       timeout=5):
                results.append(i)
        except futures.TimeoutError:
            pass
        else:
            self.fail('expected TimeoutError')

        self.assertEqual([None, None], results)

    def test_shutdown_race_issue12456(self):
        # Issue #12456: race condition at shutdown where trying to post a
        # sentinel in the call queue blocks (the queue is full while processes
        # have exited).
        self.executor.map(str, [2] * (self.worker_count + 1))
        self.executor.shutdown()

    @test.support.cpython_only
    def test_no_stale_references(self):
        # Issue #16284: check that the executors don't unnecessarily hang onto
        # references.
        my_object = MyObject()
        my_object_collected = threading.Event()
        my_object_callback = weakref.ref(
            my_object, lambda obj: my_object_collected.set())
        # Deliberately discarding the future.
        self.executor.submit(my_object.my_method)
        del my_object

        collected = my_object_collected.wait(timeout=5.0)
        self.assertTrue(collected,
                        "Stale reference not collected within timeout.")

    def test_max_workers_negative(self):
        for number in (0, -1):
            with self.assertRaisesRegex(ValueError,
                                        "max_workers must be greater "
                                        "than 0"):
                self.executor_type(max_workers=number)

    def test_free_reference(self):
        # Issue #14406: Result iterator should not keep an internal
        # reference to result objects.
        for obj in self.executor.map(make_dummy_object, range(10)):
            wr = weakref.ref(obj)
            del obj
            self.assertIsNone(wr())


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
        self.assertEqual(executor._max_workers,
                         (os.cpu_count() or 1) * 5)


class ProcessPoolExecutorTest(ExecutorTest):
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

        with test.support.captured_stderr() as f1:
            try:
                raise exc
            except RuntimeError:
                sys.excepthook(*sys.exc_info())
        self.assertIn('raise RuntimeError(123) # some comment',
                      f1.getvalue())

    def test_ressources_gced_in_workers(self):
        # Ensure that argument for a job are correctly gc-ed after the job
        # is finished
        obj = EventfulGCObj(self.ctx)
        future = self.executor.submit(id, obj)
        future.result()

        self.assertTrue(obj.event.wait(timeout=1))


create_executor_tests(ProcessPoolExecutorTest,
                      executor_mixins=(ProcessPoolForkMixin,
                                       ProcessPoolForkserverMixin,
                                       ProcessPoolSpawnMixin))


class FutureTests(BaseTestCase):
    def test_done_callback_with_result(self):
        callback_result = None
        def fn(callback_future):
            nonlocal callback_result
            callback_result = callback_future.result()

        f = Future()
        f.add_done_callback(fn)
        f.set_result(5)
        self.assertEqual(5, callback_result)

    def test_done_callback_with_exception(self):
        callback_exception = None
        def fn(callback_future):
            nonlocal callback_exception
            callback_exception = callback_future.exception()

        f = Future()
        f.add_done_callback(fn)
        f.set_exception(Exception('test'))
        self.assertEqual(('test',), callback_exception.args)

    def test_done_callback_with_cancel(self):
        was_cancelled = None
        def fn(callback_future):
            nonlocal was_cancelled
            was_cancelled = callback_future.cancelled()

        f = Future()
        f.add_done_callback(fn)
        self.assertTrue(f.cancel())
        self.assertTrue(was_cancelled)

    def test_done_callback_raises(self):
        with test.support.captured_stderr() as stderr:
            raising_was_called = False
            fn_was_called = False

            def raising_fn(callback_future):
                nonlocal raising_was_called
                raising_was_called = True
                raise Exception('doh!')

            def fn(callback_future):
                nonlocal fn_was_called
                fn_was_called = True

            f = Future()
            f.add_done_callback(raising_fn)
            f.add_done_callback(fn)
            f.set_result(5)
            self.assertTrue(raising_was_called)
            self.assertTrue(fn_was_called)
            self.assertIn('Exception: doh!', stderr.getvalue())

    def test_done_callback_already_successful(self):
        callback_result = None
        def fn(callback_future):
            nonlocal callback_result
            callback_result = callback_future.result()

        f = Future()
        f.set_result(5)
        f.add_done_callback(fn)
        self.assertEqual(5, callback_result)

    def test_done_callback_already_failed(self):
        callback_exception = None
        def fn(callback_future):
            nonlocal callback_exception
            callback_exception = callback_future.exception()

        f = Future()
        f.set_exception(Exception('test'))
        f.add_done_callback(fn)
        self.assertEqual(('test',), callback_exception.args)

    def test_done_callback_already_cancelled(self):
        was_cancelled = None
        def fn(callback_future):
            nonlocal was_cancelled
            was_cancelled = callback_future.cancelled()

        f = Future()
        self.assertTrue(f.cancel())
        f.add_done_callback(fn)
        self.assertTrue(was_cancelled)

    def test_repr(self):
        self.assertRegex(repr(PENDING_FUTURE),
                         '<Future at 0x[0-9a-f]+ state=pending>')
        self.assertRegex(repr(RUNNING_FUTURE),
                         '<Future at 0x[0-9a-f]+ state=running>')
        self.assertRegex(repr(CANCELLED_FUTURE),
                         '<Future at 0x[0-9a-f]+ state=cancelled>')
        self.assertRegex(repr(CANCELLED_AND_NOTIFIED_FUTURE),
                         '<Future at 0x[0-9a-f]+ state=cancelled>')
        self.assertRegex(
                repr(EXCEPTION_FUTURE),
                '<Future at 0x[0-9a-f]+ state=finished raised OSError>')
        self.assertRegex(
                repr(SUCCESSFUL_FUTURE),
                '<Future at 0x[0-9a-f]+ state=finished returned int>')


    def test_cancel(self):
        f1 = create_future(state=PENDING)
        f2 = create_future(state=RUNNING)
        f3 = create_future(state=CANCELLED)
        f4 = create_future(state=CANCELLED_AND_NOTIFIED)
        f5 = create_future(state=FINISHED, exception=OSError())
        f6 = create_future(state=FINISHED, result=5)

        self.assertTrue(f1.cancel())
        self.assertEqual(f1._state, CANCELLED)

        self.assertFalse(f2.cancel())
        self.assertEqual(f2._state, RUNNING)

        self.assertTrue(f3.cancel())
        self.assertEqual(f3._state, CANCELLED)

        self.assertTrue(f4.cancel())
        self.assertEqual(f4._state, CANCELLED_AND_NOTIFIED)

        self.assertFalse(f5.cancel())
        self.assertEqual(f5._state, FINISHED)

        self.assertFalse(f6.cancel())
        self.assertEqual(f6._state, FINISHED)

    def test_cancelled(self):
        self.assertFalse(PENDING_FUTURE.cancelled())
        self.assertFalse(RUNNING_FUTURE.cancelled())
        self.assertTrue(CANCELLED_FUTURE.cancelled())
        self.assertTrue(CANCELLED_AND_NOTIFIED_FUTURE.cancelled())
        self.assertFalse(EXCEPTION_FUTURE.cancelled())
        self.assertFalse(SUCCESSFUL_FUTURE.cancelled())

    def test_done(self):
        self.assertFalse(PENDING_FUTURE.done())
        self.assertFalse(RUNNING_FUTURE.done())
        self.assertTrue(CANCELLED_FUTURE.done())
        self.assertTrue(CANCELLED_AND_NOTIFIED_FUTURE.done())
        self.assertTrue(EXCEPTION_FUTURE.done())
        self.assertTrue(SUCCESSFUL_FUTURE.done())

    def test_running(self):
        self.assertFalse(PENDING_FUTURE.running())
        self.assertTrue(RUNNING_FUTURE.running())
        self.assertFalse(CANCELLED_FUTURE.running())
        self.assertFalse(CANCELLED_AND_NOTIFIED_FUTURE.running())
        self.assertFalse(EXCEPTION_FUTURE.running())
        self.assertFalse(SUCCESSFUL_FUTURE.running())

    def test_result_with_timeout(self):
        self.assertRaises(futures.TimeoutError,
                          PENDING_FUTURE.result, timeout=0)
        self.assertRaises(futures.TimeoutError,
                          RUNNING_FUTURE.result, timeout=0)
        self.assertRaises(futures.CancelledError,
                          CANCELLED_FUTURE.result, timeout=0)
        self.assertRaises(futures.CancelledError,
                          CANCELLED_AND_NOTIFIED_FUTURE.result, timeout=0)
        self.assertRaises(OSError, EXCEPTION_FUTURE.result, timeout=0)
        self.assertEqual(SUCCESSFUL_FUTURE.result(timeout=0), 42)

    def test_result_with_success(self):
        # TODO(brian@sweetapp.com): This test is timing dependent.
        def notification():
            # Wait until the main thread is waiting for the result.
            time.sleep(1)
            f1.set_result(42)

        f1 = create_future(state=PENDING)
        t = threading.Thread(target=notification)
        t.start()

        self.assertEqual(f1.result(timeout=5), 42)
        t.join()

    def test_result_with_cancel(self):
        # TODO(brian@sweetapp.com): This test is timing dependent.
        def notification():
            # Wait until the main thread is waiting for the result.
            time.sleep(1)
            f1.cancel()

        f1 = create_future(state=PENDING)
        t = threading.Thread(target=notification)
        t.start()

        self.assertRaises(futures.CancelledError, f1.result, timeout=5)
        t.join()

    def test_exception_with_timeout(self):
        self.assertRaises(futures.TimeoutError,
                          PENDING_FUTURE.exception, timeout=0)
        self.assertRaises(futures.TimeoutError,
                          RUNNING_FUTURE.exception, timeout=0)
        self.assertRaises(futures.CancelledError,
                          CANCELLED_FUTURE.exception, timeout=0)
        self.assertRaises(futures.CancelledError,
                          CANCELLED_AND_NOTIFIED_FUTURE.exception, timeout=0)
        self.assertTrue(isinstance(EXCEPTION_FUTURE.exception(timeout=0),
                                   OSError))
        self.assertEqual(SUCCESSFUL_FUTURE.exception(timeout=0), None)

    def test_exception_with_success(self):
        def notification():
            # Wait until the main thread is waiting for the exception.
            time.sleep(1)
            with f1._condition:
                f1._state = FINISHED
                f1._exception = OSError()
                f1._condition.notify_all()

        f1 = create_future(state=PENDING)
        t = threading.Thread(target=notification)
        t.start()

        self.assertTrue(isinstance(f1.exception(timeout=5), OSError))
        t.join()


@test.support.reap_threads
def test_main():
    try:
        test.support.run_unittest(__name__)
    finally:
        test.support.reap_children()

if __name__ == "__main__":
    test_main()
