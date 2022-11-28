"""Test suite for the sys.monitoring."""

import sys
import unittest
import enum
import operator
import functools
import types
import collections

def f1():
    pass

def f2():
    len([])
    sys.getsizeof(0)

def gen():
    yield
    yield

def g1():
    for _ in gen():
        pass

TEST_TOOL = 3
TEST_TOOL2 = 4

class MonitoringBaseTest(unittest.TestCase):

    def test_has_objects(self):
        m = sys.monitoring
        m.events
        m.use_tool
        m.free_tool
        m.get_tool
        m.get_events
        m.set_events
        # m.get_local_events
        # m.set_local_events
        m.register_callback
        # m.insert_marker
        # m.remove_marker
        m.restart_events
        m.DISABLE

    def test_tool(self):
        sys.monitoring.use_tool(TEST_TOOL, "MonitoringTest.Tool")
        self.assertEqual(sys.monitoring.get_tool(TEST_TOOL), "MonitoringTest.Tool")
        sys.monitoring.free_tool(TEST_TOOL)
        self.assertEqual(sys.monitoring.get_tool(TEST_TOOL), None)
        sys.monitoring.set_events(TEST_TOOL, 19)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 19)
        sys.monitoring.set_events(TEST_TOOL, 0)


class MonitoringCountTest(unittest.TestCase):

    def check_event_count(self, func, event, expected):

        class Counter:
            def __init__(self):
                self.count = 0
            def __call__(self, *args):
                self.count += 1

        counter = Counter()
        sys.monitoring.register_callback(TEST_TOOL, event, counter)
        sys.monitoring.set_events(TEST_TOOL, event)
        self.assertEqual(counter.count, 0)
        counter.count = 0
        func()
        self.assertEqual(counter.count, expected)
        prev = sys.monitoring.register_callback(TEST_TOOL, event, None)
        counter.count = 0
        func()
        self.assertEqual(counter.count, 0)
        self.assertEqual(prev, counter)
        sys.monitoring.set_events(TEST_TOOL, 0)

    def test_start_count(self):
        self.check_event_count(f1, E.PY_START, 1)

    def test_resume_count(self):
        self.check_event_count(g1, E.PY_RESUME, 2)

    def test_return_count(self):
        self.check_event_count(f1, E.PY_RETURN, 1)

    def test_call_count(self):
        self.check_event_count(f2, E.CALL, 3)

    def test_c_return_count(self):
        self.check_event_count(f2, E.C_RETURN, 2)


E = sys.monitoring.events

SIMPLE_EVENTS = [
    (E.PY_START, "start"),
    (E.PY_RESUME, "resume"),
    (E.PY_RETURN, "return"),
    (E.PY_YIELD, "yield"),
    (E.C_RAISE, "c_raise"),
    (E.C_RETURN, "c_return"),
    (E.JUMP, "jump"),
    (E.BRANCH, "branch"),
    (E.RAISE, "raise"),
    (E.PY_UNWIND, "unwind"),
    (E.EXCEPTION_HANDLED, "exception_handled"),
]

SIMPLE_EVENT_SET = functools.reduce(operator.or_, [ev for (ev, _) in SIMPLE_EVENTS], 0) | E.CALL

def just_pass():
    pass

just_pass.events = [
    "py_call",
    "start",
    "return",
]

def just_raise():
    raise Exception

just_raise.events = [
    'py_call',
    "start",
    "raise",
    "unwind",
]

def just_call():
    len([])

just_call.events = [
    'py_call',
    "start",
    "c_call",
    "c_return",
    "return",
]

def caught():
    try:
        1/0
    except Exception:
        pass

caught.events = [
    'py_call',
    "start",
    "raise",
    "exception_handled",
    "return",
]

def nested_call():
    just_pass()

nested_call.events = [
    "py_call",
    "start",
    "py_call",
    "start",
    "return",
    "return",
]

PY_CALLABLES = (types.FunctionType, types.MethodType)

class MonitoringEventsBase:

    def gather_events(self, func):
        events = []
        for event, event_name in SIMPLE_EVENTS:
            def record(*args, event_name=event_name):
                events.append(event_name)
            sys.monitoring.register_callback(TEST_TOOL, event, record)
        def record_call(code, offset, obj):
            if isinstance(obj, PY_CALLABLES):
                events.append("py_call")
            else:
                events.append("c_call")
        sys.monitoring.register_callback(TEST_TOOL, E.CALL, record_call)
        sys.monitoring.set_events(TEST_TOOL, SIMPLE_EVENT_SET)
        events = []
        try:
            func()
        except:
            pass
        sys.monitoring.set_events(TEST_TOOL, 0)
        #Remove the final event, the call to `sys.monitoring.set_events`
        events = events[:-1]
        return events

    def check_events(self, func, expected=None):
        events = self.gather_events(func)
        if expected is None:
            expected = func.events
        self.assertEqual(events, expected)


class MonitoringEventsTest(unittest.TestCase, MonitoringEventsBase):

    def test_just_pass(self):
        self.check_events(just_pass)

    def test_just_raise(self):
        try:
            self.check_events(just_raise)
        except Exception:
            pass
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 0)

    def test_just_call(self):
        self.check_events(just_call)

    def test_caught(self):
        self.check_events(caught)

    def test_nested_call(self):
        self.check_events(nested_call)

UP_EVENTS = (E.C_RETURN, E.C_RAISE, E.PY_RETURN, E.PY_UNWIND, E.PY_YIELD)
DOWN_EVENTS = (E.PY_START, E.PY_RESUME)

from test.profilee import testfunc

class SimulateProfileTest(unittest.TestCase, MonitoringEventsBase):

    def test_balanced(self):
        events = self.gather_events(testfunc)
        c = collections.Counter(events)
        self.assertEqual(c["c_call"], c["c_return"])
        self.assertEqual(c["start"], c["return"] + c["unwind"])
        self.assertEqual(c["raise"], c["exception_handled"] + c["unwind"])

    def test_frame_stack(self):
        self.maxDiff = None
        stack = []
        errors = []
        seen = set()
        def up(*args):
            frame = sys._getframe(1)
            if not stack:
                errors.append("empty")
            else:
                expected = stack.pop()
                if frame != expected:
                    errors.append(f" Popping {frame} expected {expected}")
        def down(*args):
            frame = sys._getframe(1)
            stack.append(frame)
            seen.add(frame.f_code)
        def call(code, offset, callable):
            if not isinstance(callable, PY_CALLABLES):
                stack.append(sys._getframe(1))
        for event in UP_EVENTS:
            sys.monitoring.register_callback(TEST_TOOL, event, up)
        for event in DOWN_EVENTS:
            sys.monitoring.register_callback(TEST_TOOL, event, down)
        sys.monitoring.register_callback(TEST_TOOL, E.CALL, call)
        sys.monitoring.set_events(TEST_TOOL, SIMPLE_EVENT_SET)
        testfunc()
        sys.monitoring.set_events(TEST_TOOL, 0)
        self.assertEqual(errors, [])
        self.assertEqual(len(seen), 9)
        self.assertEqual(stack, [sys._getframe()])


class CounterWithDisable:
    def __init__(self):
        self.disable = False
        self.count = 0
    def __call__(self, *args):
        self.count += 1
        if self.disable:
            return sys.monitoring.DISABLE


class MontoringDisableAndRestartTest(unittest.TestCase):

    def test_disable(self):
        counter = CounterWithDisable()
        sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter)
        sys.monitoring.set_events(TEST_TOOL, E.PY_START)
        self.assertEqual(counter.count, 0)
        counter.count = 0
        f1()
        self.assertEqual(counter.count, 1)
        counter.disable = True
        counter.count = 0
        f1()
        self.assertEqual(counter.count, 1)
        counter.count = 0
        f1()
        self.assertEqual(counter.count, 0)

    def test_restart(self):
        counter = CounterWithDisable()
        sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter)
        sys.monitoring.set_events(TEST_TOOL, E.PY_START)
        counter.disable = True
        f1()
        counter.count = 0
        f1()
        self.assertEqual(counter.count, 0)
        sys.monitoring.restart_events()
        counter.count = 0
        f1()
        self.assertEqual(counter.count, 1)



