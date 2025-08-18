import contextlib
import logging
import queue
import time
import unittest
import sys
import io
from concurrent.futures._base import BrokenExecutor
from concurrent.futures.process import _check_system_limits

from logging.handlers import QueueHandler

from test import support

from .util import ExecutorMixin, create_executor_tests, setup_module


INITIALIZER_STATUS = 'uninitialized'

def init(x):
    global INITIALIZER_STATUS
    INITIALIZER_STATUS = x
    # InterpreterPoolInitializerTest.test_initializer fails
    # if we don't have a LOAD_GLOBAL.  (It could be any global.)
    # We will address this separately.
    INITIALIZER_STATUS

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
            for _ in support.sleeping_retry(support.SHORT_TIMEOUT,
                                            "executor not broken"):
                if self.executor._broken:
                    break

            # ... and from this point submit() is guaranteed to fail
            with self.assertRaises(BrokenExecutor):
                self.executor.submit(get_init_status)

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


create_executor_tests(globals(), InitializerMixin)
create_executor_tests(globals(), FailingInitializerMixin)


@unittest.skipIf(sys.platform == "win32", "Resource Tracker doesn't run on Windows")
class FailingInitializerResourcesTest(unittest.TestCase):
    """
    Source: https://github.com/python/cpython/issues/104090
    """

    def _test(self, test_class):
        try:
            _check_system_limits()
        except NotImplementedError:
            self.skipTest("ProcessPoolExecutor unavailable on this system")

        runner = unittest.TextTestRunner(stream=io.StringIO())
        runner.run(test_class('test_initializer'))

        # GH-104090:
        # Stop resource tracker manually now, so we can verify there are not leaked resources by checking
        # the process exit code
        from multiprocessing.resource_tracker import _resource_tracker
        _resource_tracker._stop()

        self.assertEqual(_resource_tracker._exitcode, 0)

    def test_spawn(self):
        self._test(ProcessPoolSpawnFailingInitializerTest)

    @support.skip_if_sanitizer("TSAN doesn't support threads after fork", thread=True)
    def test_forkserver(self):
        self._test(ProcessPoolForkserverFailingInitializerTest)


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
