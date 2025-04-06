import multiprocessing
import sys
import time
import unittest
from concurrent import futures
from concurrent.futures._base import (
    PENDING, RUNNING, CANCELLED, CANCELLED_AND_NOTIFIED, FINISHED, Future,
    )
from concurrent.futures.process import _check_system_limits

from test import support
from test.support import threading_helper


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


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self._thread_key = threading_helper.threading_setup()

    def tearDown(self):
        support.reap_children()
        threading_helper.threading_cleanup(*self._thread_key)


class ExecutorMixin:
    worker_count = 5
    executor_kwargs = {}

    def setUp(self):
        super().setUp()

        self.t1 = time.monotonic()
        if hasattr(self, "ctx"):
            self.executor = self.executor_type(
                max_workers=self.worker_count,
                mp_context=self.get_context(),
                **self.executor_kwargs)
        else:
            self.executor = self.executor_type(
                max_workers=self.worker_count,
                **self.executor_kwargs)

    def tearDown(self):
        self.executor.shutdown(wait=True)
        self.executor = None

        dt = time.monotonic() - self.t1
        if support.verbose:
            print("%.2fs" % dt, end=' ')
        self.assertLess(dt, 300, "synchronization issue: test lasted too long")

        super().tearDown()

    def get_context(self):
        return multiprocessing.get_context(self.ctx)


class ThreadPoolMixin(ExecutorMixin):
    executor_type = futures.ThreadPoolExecutor


class ProcessPoolForkMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "fork"

    def get_context(self):
        try:
            _check_system_limits()
        except NotImplementedError:
            self.skipTest("ProcessPoolExecutor unavailable on this system")
        if sys.platform == "win32":
            self.skipTest("require unix system")
        if support.check_sanitizer(thread=True):
            self.skipTest("TSAN doesn't support threads after fork")
        return super().get_context()


class ProcessPoolSpawnMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "spawn"

    def get_context(self):
        try:
            _check_system_limits()
        except NotImplementedError:
            self.skipTest("ProcessPoolExecutor unavailable on this system")
        return super().get_context()


class ProcessPoolForkserverMixin(ExecutorMixin):
    executor_type = futures.ProcessPoolExecutor
    ctx = "forkserver"

    def get_context(self):
        try:
            _check_system_limits()
        except NotImplementedError:
            self.skipTest("ProcessPoolExecutor unavailable on this system")
        if sys.platform == "win32":
            self.skipTest("require unix system")
        if support.check_sanitizer(thread=True):
            self.skipTest("TSAN doesn't support threads after fork")
        return super().get_context()


def create_executor_tests(remote_globals, mixin, bases=(BaseTestCase,),
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

    module = remote_globals['__name__']
    for exe in executor_mixins:
        name = ("%s%sTest"
                % (strip_mixin(exe.__name__), strip_mixin(mixin.__name__)))
        cls = type(name, (mixin,) + (exe,) + bases, {'__module__': module})
        remote_globals[name] = cls


def setup_module():
    unittest.addModuleCleanup(multiprocessing.util._cleanup_tests)
    thread_info = threading_helper.threading_setup()
    unittest.addModuleCleanup(threading_helper.threading_cleanup, *thread_info)
