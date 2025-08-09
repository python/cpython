"""Tests monitoring, sys.settrace, and sys.setprofile in a multi-threaded
environment to verify things are thread-safe in a free-threaded build"""

import sys
import threading
import time
import unittest
import weakref

from contextlib import contextmanager
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

    def start_work(self, n, funcs):
        # With the GIL builds we need to make sure that the hooks have
        # a chance to run as it's possible to run w/o releasing the GIL.
        time.sleep(0.1)
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

        threads = []
        for i in range(self.thread_count):
            # Each thread gets a copy of the func list to avoid contention
            t = Thread(target=self.start_work, args=(self.fib, list(funcs)))
            t.start()
            threads.append(t)

        self.after_threads()

        while True:
            any_alive = False
            for t in threads:
                if t.is_alive():
                    any_alive = True
                    break

            if not any_alive:
                break

            self.during_threads()

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
        self.called = False
        monitoring.register_callback(
            self.tool_id, monitoring.events.LINE, self.callback
        )

    def tearDown(self):
        monitoring.set_events(self.tool_id, 0)
        super().tearDown()

    def callback(self, *args):
        self.called = True

    def after_test(self):
        self.assertTrue(self.called)

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
        self.called = False

    def after_test(self):
        self.assertTrue(self.called)

    def tearDown(self):
        sys.settrace(None)

    def trace_func(self, frame, event, arg):
        self.called = True
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
        self.called = False

    def after_test(self):
        self.assertTrue(self.called)

    def tearDown(self):
        sys.setprofile(None)

    def trace_func(self, frame, event, arg):
        self.called = True
        return self.trace_func

    def during_threads(self):
        if self.set:
            sys.setprofile(self.trace_func)
        else:
            sys.setprofile(None)
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


class TraceBuf:
    def __init__(self):
        self.traces = []
        self.traces_lock = threading.Lock()

    def append(self, trace):
        with self.traces_lock:
            self.traces.append(trace)


@threading_helper.requires_working_threading()
class MonitoringMisc(MonitoringTestMixin, TestCase):
    def register_callback(self, barrier):
        barrier.wait()

        def callback(*args):
            pass

        for i in range(200):
            monitoring.register_callback(self.tool_id, monitoring.events.LINE, callback)

        self.refs.append(weakref.ref(callback))

    def test_register_callback(self):
        self.refs = []
        threads = []
        barrier = Barrier(5)
        for i in range(5):
            t = Thread(target=self.register_callback, args=(barrier,))
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

    def test_toggle_setprofile_no_new_events(self):
        # gh-136396: Make sure that profile functions are called for newly
        # created threads when profiling is toggled but the set of monitoring
        # events doesn't change
        traces = []

        def profiler(frame, event, arg):
            traces.append((frame.f_code.co_name, event, arg))

        def a(x, y):
            return b(x, y)

        def b(x, y):
            return max(x, y)

        sys.setprofile(profiler)
        try:
            a(1, 2)
        finally:
            sys.setprofile(None)
        traces.clear()

        def thread_main(x, y):
            sys.setprofile(profiler)
            try:
                a(x, y)
            finally:
                sys.setprofile(None)
        t = Thread(target=thread_main, args=(100, 200))
        t.start()
        t.join()

        expected = [
            ("a", "call", None),
            ("b", "call", None),
            ("b", "c_call", max),
            ("b", "c_return", max),
            ("b", "return", 200),
            ("a", "return", 200),
            ("thread_main", "c_call", sys.setprofile),
        ]
        self.assertEqual(traces, expected)

    def observe_threads(self, observer, buf):
        def in_child(ident):
            return ident

        def child(ident):
            with observer():
                in_child(ident)

        def in_parent(ident):
            return ident

        def parent(barrier, ident):
            barrier.wait()
            with observer():
                t = Thread(target=child, args=(ident,))
                t.start()
                t.join()
                in_parent(ident)

        num_threads = 5
        barrier = Barrier(num_threads)
        threads = []
        for i in range(num_threads):
            t = Thread(target=parent, args=(barrier, i))
            t.start()
            threads.append(t)
        for t in threads:
            t.join()

        for i in range(num_threads):
            self.assertIn(("in_parent", "return", i), buf.traces)
            self.assertIn(("in_child", "return", i), buf.traces)

    def test_profile_threads(self):
        buf = TraceBuf()

        def profiler(frame, event, arg):
            buf.append((frame.f_code.co_name, event, arg))

        @contextmanager
        def profile():
            sys.setprofile(profiler)
            try:
                yield
            finally:
                sys.setprofile(None)

        self.observe_threads(profile, buf)

    def test_trace_threads(self):
        buf = TraceBuf()

        def tracer(frame, event, arg):
            buf.append((frame.f_code.co_name, event, arg))
            return tracer

        @contextmanager
        def trace():
            sys.settrace(tracer)
            try:
                yield
            finally:
                sys.settrace(None)

        self.observe_threads(trace, buf)

    def test_monitor_threads(self):
        buf = TraceBuf()

        def monitor_py_return(code, off, retval):
            buf.append((code.co_name, "return", retval))

        monitoring.register_callback(
            self.tool_id, monitoring.events.PY_RETURN, monitor_py_return
        )

        monitoring.set_events(
            self.tool_id, monitoring.events.PY_RETURN
        )

        @contextmanager
        def noop():
            yield

        self.observe_threads(noop, buf)


if __name__ == "__main__":
    unittest.main()
