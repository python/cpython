"""Tests monitoring, sys.settrace, and sys.setprofile in a multi-threaded
environment to verify things are thread-safe in a free-threaded build"""

import sys
import threading
import time
import unittest
import weakref

from sys import monitoring
from test.support import threading_helper
from threading import Thread, _PyRLock, Barrier
from unittest import TestCase


class InstrumentationMultiThreadedMixin:
    thread_count = 10
    func_count = 50
    fib = 12

    def after_threads(self):
        """Runs once after all the threads have started"""
        pass

    def during_threads(self):
        """Runs repeatedly while the threads are still running"""
        pass

    def work(self, n, funcs):
        """Fibonacci function which also calls a bunch of random functions"""
        for func in funcs:
            func()
        if n < 2:
            return n
        return self.work(n - 1, funcs) + self.work(n - 2, funcs)

    def start_work(self, n, funcs, barrier):
        # With the GIL builds we need to make sure that the hooks have
        # a chance to run as it's possible to run w/o releasing the GIL.
        barrier.wait()
        self.work(n, funcs)

    def after_test(self):
        """Runs once after the test is done"""
        pass

    def test_instrumentation(self):
        # Setup a bunch of functions which will need instrumentation...
        funcs = []
        for i in range(self.func_count):
            x = {}
            exec("def f(): pass", x)
            funcs.append(x["f"])

        barrier = Barrier(self.thread_count + 1)
        threads = []
        for i in range(self.thread_count):
            # Each thread gets a copy of the func list to avoid contention
            t = Thread(target=self.start_work, args=(self.fib, list(funcs), barrier))
            t.start()
            threads.append(t)

        self.after_threads()
        barrier.wait()

        while True:
            any_alive = False
            for t in threads:
                if t.is_alive():
                    any_alive = True
                    break

            if not any_alive:
                break

            self.during_threads()
            # Sleep to avoid setting monitoring events too rapidly and
            # overflowing the global version counter
            time.sleep(0.0001)

        self.after_test()


class MonitoringTestMixin:
    def setUp(self):
        for i in range(6):
            if monitoring.get_tool(i) is None:
                self.tool_id = i
                monitoring.use_tool_id(i, self.__class__.__name__)
                break

    def tearDown(self):
        monitoring.free_tool_id(self.tool_id)


@threading_helper.requires_working_threading()
class SetPreTraceMultiThreaded(InstrumentationMultiThreadedMixin, TestCase):
    """Sets tracing one time after the threads have started"""

    def setUp(self):
        super().setUp()
        self.called = False

    def after_test(self):
        self.assertTrue(self.called)

    def trace_func(self, frame, event, arg):
        self.called = True
        return self.trace_func

    def after_threads(self):
        sys.settrace(self.trace_func)


@threading_helper.requires_working_threading()
class MonitoringMultiThreaded(
    MonitoringTestMixin, InstrumentationMultiThreadedMixin, TestCase
):
    """Uses sys.monitoring and repeatedly toggles instrumentation on and off"""

    def setUp(self):
        super().setUp()
        self.set = False
        monitoring.register_callback(
            self.tool_id, monitoring.events.LINE, self.callback
        )

    def tearDown(self):
        monitoring.set_events(self.tool_id, 0)
        super().tearDown()

    def callback(self, *args):
        pass

    def during_threads(self):
        if self.set:
            monitoring.set_events(
                self.tool_id, monitoring.events.CALL | monitoring.events.LINE
            )
        else:
            monitoring.set_events(self.tool_id, 0)
        self.set = not self.set


@threading_helper.requires_working_threading()
class SetTraceMultiThreaded(InstrumentationMultiThreadedMixin, TestCase):
    """Uses sys.settrace and repeatedly toggles instrumentation on and off"""

    def setUp(self):
        self.set = False

    def tearDown(self):
        sys.settrace(None)

    def trace_func(self, frame, event, arg):
        return self.trace_func

    def during_threads(self):
        if self.set:
            sys.settrace(self.trace_func)
        else:
            sys.settrace(None)
        self.set = not self.set


@threading_helper.requires_working_threading()
class SetProfileMultiThreaded(InstrumentationMultiThreadedMixin, TestCase):
    """Uses sys.setprofile and repeatedly toggles instrumentation on and off"""

    def setUp(self):
        self.set = False

    def tearDown(self):
        sys.setprofile(None)

    def trace_func(self, frame, event, arg):
        return self.trace_func

    def during_threads(self):
        if self.set:
            sys.setprofile(self.trace_func)
        else:
            sys.setprofile(None)
        self.set = not self.set


@threading_helper.requires_working_threading()
class SetProfileAllThreadsMultiThreaded(InstrumentationMultiThreadedMixin, TestCase):
    """Uses threading.setprofile_all_threads and repeatedly toggles instrumentation on and off"""

    def setUp(self):
        self.set = False

    def tearDown(self):
        threading.setprofile_all_threads(None)

    def trace_func(self, frame, event, arg):
        return self.trace_func

    def during_threads(self):
        if self.set:
            threading.setprofile_all_threads(self.trace_func)
        else:
            threading.setprofile_all_threads(None)
        self.set = not self.set


@threading_helper.requires_working_threading()
class SetProfileAllMultiThreaded(TestCase):
    def test_profile_all_threads(self):
        done = threading.Event()

        def func():
            pass

        def bg_thread():
            while not done.is_set():
                func()
                func()
                func()
                func()
                func()

        def my_profile(frame, event, arg):
            return None

        bg_threads = []
        for i in range(10):
            t = threading.Thread(target=bg_thread)
            t.start()
            bg_threads.append(t)

        for i in range(100):
            threading.setprofile_all_threads(my_profile)
            threading.setprofile_all_threads(None)

        done.set()
        for t in bg_threads:
            t.join()


@threading_helper.requires_working_threading()
class MonitoringMisc(MonitoringTestMixin, TestCase):
    def register_callback(self):
        def callback(*args):
            pass

        for i in range(200):
            monitoring.register_callback(self.tool_id, monitoring.events.LINE, callback)

        self.refs.append(weakref.ref(callback))

    def test_register_callback(self):
        self.refs = []
        threads = []
        for i in range(50):
            t = Thread(target=self.register_callback)
            t.start()
            threads.append(t)

        for thread in threads:
            thread.join()

        monitoring.register_callback(self.tool_id, monitoring.events.LINE, None)
        for ref in self.refs:
            self.assertEqual(ref(), None)

    def test_set_local_trace_opcodes(self):
        def trace(frame, event, arg):
            frame.f_trace_opcodes = True
            return trace

        loops = 1_000

        sys.settrace(trace)
        try:
            l = _PyRLock()

            def f():
                for i in range(loops):
                    with l:
                        pass

            t = Thread(target=f)
            t.start()
            for i in range(loops):
                with l:
                    pass
            t.join()
        finally:
            sys.settrace(None)


if __name__ == "__main__":
    unittest.main()
