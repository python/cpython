"""Test suite for the sys.monitoring."""

import sys
import unittest
import enum
import operator
import functools
import types
import collections

PAIR = (0,1)

def f1():
    pass

def f2():
    len([])
    sys.getsizeof(0)

def floop():
    for item in PAIR:
        pass

def gen():
    yield
    yield

def g1():
    for _ in gen():
        pass

TEST_TOOL = 2
TEST_TOOL2 = 3
TEST_TOOL3 = 4

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
        sys.monitoring.set_events(TEST_TOOL, 15)
        self.assertEqual(sys.monitoring.get_events(TEST_TOOL), 15)
        sys.monitoring.set_events(TEST_TOOL, 0)
        with self.assertRaises(ValueError):
            sys.monitoring.set_events(TEST_TOOL, sys.monitoring.events.C_RETURN)
        with self.assertRaises(ValueError):
            sys.monitoring.set_events(TEST_TOOL, sys.monitoring.events.C_RAISE)


class MonitoringCountTest(unittest.TestCase):

    def check_event_count(self, func, event, expected):

        class Counter:
            def __init__(self):
                self.count = 0
            def __call__(self, *args):
                self.count += 1

        counter = Counter()
        sys.monitoring.register_callback(TEST_TOOL, event, counter)
        if event == E.C_RETURN or event == E.C_RAISE:
            sys.monitoring.set_events(TEST_TOOL, E.CALL)
        else:
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
    (E.JUMP, "jump"),
    (E.BRANCH, "branch"),
    (E.RAISE, "raise"),
    (E.PY_UNWIND, "unwind"),
    (E.EXCEPTION_HANDLED, "exception_handled"),
    (E.C_RAISE, "c_raise"),
    (E.C_RETURN, "c_return"),
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
    "branch",
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

class MonitoringTestBase:

    def setUp(self):
        # Check that a previous test hasn't left monitoring on.
        for tool in range(6):
            self.assertEqual(sys.monitoring.get_events(tool), 0)

    def tearDown(self):
        # Check that test hasn't left monitoring on.
        for tool in range(6):
            self.assertEqual(sys.monitoring.get_events(tool), 0)

class MonitoringEventsBase(MonitoringTestBase):

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
        self.assertEqual(stack, [sys._getframe()])
        self.assertEqual(len(seen), 9)


class CounterWithDisable:

    def __init__(self):
        self.disable = False
        self.count = 0

    def __call__(self, *args):
        self.count += 1
        if self.disable:
            return sys.monitoring.DISABLE


class RecorderWithDisable:

    def __init__(self, events):
        self.disable = False
        self.events = events

    def __call__(self, code, event):
        self.events.append(event)
        if self.disable:
            return sys.monitoring.DISABLE


class MontoringDisableAndRestartTest(unittest.TestCase, MonitoringTestBase):

    def test_disable(self):
        try:
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
            sys.monitoring.set_events(TEST_TOOL, 0)
        finally:
            sys.monitoring.restart_events()

    def test_restart(self):
        try:
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
            sys.monitoring.set_events(TEST_TOOL, 0)
        finally:
            sys.monitoring.restart_events()


class MultipleMonitorsTest(unittest.TestCase, MonitoringTestBase):

    def test_two_same(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2)})
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_three_same(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            counter3 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.register_callback(TEST_TOOL3, E.PY_START, counter3)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL3, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL3), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2) | (1 << TEST_TOOL3)})
            counter1.count = 0
            counter2.count = 0
            counter3.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            count3 = counter3.count
            self.assertEqual((count1, count2, count3), (1, 1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.set_events(TEST_TOOL3, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL3, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_two_different(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_RETURN, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_RETURN)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_RETURN)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': 1 << TEST_TOOL, 'PY_RETURN': 1 << TEST_TOOL2})
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_RETURN, None)
            self.assertEqual(sys.monitoring._all_events(), {})

    def test_two_with_disable(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            counter1 = CounterWithDisable()
            counter2 = CounterWithDisable()
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, counter1)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, counter2)
            sys.monitoring.set_events(TEST_TOOL, E.PY_START)
            sys.monitoring.set_events(TEST_TOOL2, E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL), E.PY_START)
            self.assertEqual(sys.monitoring.get_events(TEST_TOOL2), E.PY_START)
            self.assertEqual(sys.monitoring._all_events(), {'PY_START': (1 << TEST_TOOL) | (1 << TEST_TOOL2)})
            counter1.count = 0
            counter2.count = 0
            counter1.disable = True
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (1, 1))
            counter1.count = 0
            counter2.count = 0
            f1()
            count1 = counter1.count
            count2 = counter2.count
            self.assertEqual((count1, count2), (0, 1))
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.PY_START, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.PY_START, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

class LineMontoringTest(unittest.TestCase):

    def test_lines_single(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.set_events(TEST_TOOL, E.LINE)
            f1()
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            start = LineMontoringTest.test_lines_single.__code__.co_firstlineno
            self.assertEqual(events, [start+7, 14, start+8])
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

    def test_lines_loop(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.set_events(TEST_TOOL, E.LINE)
            floop()
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            start = LineMontoringTest.test_lines_loop.__code__.co_firstlineno
            self.assertEqual(events, [start+7, 21, 22, 22, 21, start+8])
        finally:
            sys.monitoring.set_events(TEST_TOOL, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()

    def test_lines_two(self):
        try:
            self.assertEqual(sys.monitoring._all_events(), {})
            events = []
            recorder = RecorderWithDisable(events)
            events2 = []
            recorder2 = RecorderWithDisable(events2)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, recorder)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, recorder2)
            sys.monitoring.set_events(TEST_TOOL, E.LINE); sys.monitoring.set_events(TEST_TOOL2, E.LINE)
            f1()
            sys.monitoring.set_events(TEST_TOOL, 0); sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, None)
            start = LineMontoringTest.test_lines_two.__code__.co_firstlineno
            expected = [start+10, 14, start+11]
            self.assertEqual(events, expected)
            self.assertEqual(events2, expected)
        finally:
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.set_events(TEST_TOOL2, 0)
            sys.monitoring.register_callback(TEST_TOOL, E.LINE, None)
            sys.monitoring.register_callback(TEST_TOOL2, E.LINE, None)
            self.assertEqual(sys.monitoring._all_events(), {})
            sys.monitoring.restart_events()




