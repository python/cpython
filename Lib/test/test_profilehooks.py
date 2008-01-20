import pprint
import sys
import unittest

from test import test_support

class TestGetProfile(unittest.TestCase):
    def setUp(self):
        sys.setprofile(None)

    def tearDown(self):
        sys.setprofile(None)

    def test_empty(self):
        assert sys.getprofile() == None

    def test_setget(self):
        def fn(*args):
            pass

        sys.setprofile(fn)
        assert sys.getprofile() == fn

class HookWatcher:
    def __init__(self):
        self.frames = []
        self.events = []

    def callback(self, frame, event, arg):
        if (event == "call"
            or event == "return"
            or event == "exception"):
            self.add_event(event, frame)

    def add_event(self, event, frame=None):
        """Add an event to the log."""
        if frame is None:
            frame = sys._getframe(1)

        try:
            frameno = self.frames.index(frame)
        except ValueError:
            frameno = len(self.frames)
            self.frames.append(frame)

        self.events.append((frameno, event, ident(frame)))

    def get_events(self):
        """Remove calls to add_event()."""
        disallowed = [ident(self.add_event.__func__), ident(ident)]
        self.frames = None

        return [item for item in self.events if item[2] not in disallowed]


class ProfileSimulator(HookWatcher):
    def __init__(self, testcase):
        self.testcase = testcase
        self.stack = []
        HookWatcher.__init__(self)

    def callback(self, frame, event, arg):
        # Callback registered with sys.setprofile()/sys.settrace()
        self.dispatch[event](self, frame)

    def trace_call(self, frame):
        self.add_event('call', frame)
        self.stack.append(frame)

    def trace_return(self, frame):
        self.add_event('return', frame)
        self.stack.pop()

    def trace_exception(self, frame):
        self.testcase.fail(
            "the profiler should never receive exception events")

    def trace_pass(self, frame):
        pass

    dispatch = {
        'call': trace_call,
        'exception': trace_exception,
        'return': trace_return,
        'c_call': trace_pass,
        'c_return': trace_pass,
        'c_exception': trace_pass,
        }


class TestCaseBase(unittest.TestCase):
    def check_events(self, callable, expected):
        events = capture_events(callable, self.new_watcher())
        if events != expected:
            self.fail("Expected events:\n%s\nReceived events:\n%s"
                      % (pprint.pformat(expected), pprint.pformat(events)))


class ProfileHookTestCase(TestCaseBase):
    def new_watcher(self):
        return HookWatcher()

    def test_simple(self):
        def f(p):
            pass
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_exception(self):
        def f(p):
            1/0
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_caught_exception(self):
        def f(p):
            try: 1/0
            except: pass
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_caught_nested_exception(self):
        def f(p):
            try: 1/0
            except: pass
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_nested_exception(self):
        def f(p):
            1/0
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              # This isn't what I expected:
                              # (0, 'exception', protect_ident),
                              # I expected this again:
                              (1, 'return', f_ident),
                              ])

    def test_exception_in_except_clause(self):
        def f(p):
            1/0
        def g(p):
            try:
                f(p)
            except:
                try: f(p)
                except: pass
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(1, 'call', g_ident),
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (3, 'call', f_ident),
                              (3, 'return', f_ident),
                              (1, 'return', g_ident),
                              ])

    def test_exception_propogation(self):
        def f(p):
            1/0
        def g(p):
            try: f(p)
            finally: p.add_event("falling through")
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(1, 'call', g_ident),
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (1, 'falling through', g_ident),
                              (1, 'return', g_ident),
                              ])

    def test_raise_twice(self):
        def f(p):
            try: 1/0
            except: 1/0
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_raise_reraise(self):
        def f(p):
            try: 1/0
            except: raise
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_raise(self):
        def f(p):
            raise Exception()
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_distant_exception(self):
        def f():
            1/0
        def g():
            f()
        def h():
            g()
        def i():
            h()
        def j(p):
            i()
        f_ident = ident(f)
        g_ident = ident(g)
        h_ident = ident(h)
        i_ident = ident(i)
        j_ident = ident(j)
        self.check_events(j, [(1, 'call', j_ident),
                              (2, 'call', i_ident),
                              (3, 'call', h_ident),
                              (4, 'call', g_ident),
                              (5, 'call', f_ident),
                              (5, 'return', f_ident),
                              (4, 'return', g_ident),
                              (3, 'return', h_ident),
                              (2, 'return', i_ident),
                              (1, 'return', j_ident),
                              ])

    def test_generator(self):
        def f():
            for i in range(2):
                yield i
        def g(p):
            for i in f():
                pass
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(1, 'call', g_ident),
                              # call the iterator twice to generate values
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              # once more; returns end-of-iteration with
                              # actually raising an exception
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (1, 'return', g_ident),
                              ])

    def test_stop_iteration(self):
        def f():
            for i in range(2):
                yield i
            raise StopIteration
        def g(p):
            for i in f():
                pass
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(1, 'call', g_ident),
                              # call the iterator twice to generate values
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              # once more to hit the raise:
                              (2, 'call', f_ident),
                              (2, 'return', f_ident),
                              (1, 'return', g_ident),
                              ])


class ProfileSimulatorTestCase(TestCaseBase):
    def new_watcher(self):
        return ProfileSimulator(self)

    def test_simple(self):
        def f(p):
            pass
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_basic_exception(self):
        def f(p):
            1/0
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_caught_exception(self):
        def f(p):
            try: 1/0
            except: pass
        f_ident = ident(f)
        self.check_events(f, [(1, 'call', f_ident),
                              (1, 'return', f_ident),
                              ])

    def test_distant_exception(self):
        def f():
            1/0
        def g():
            f()
        def h():
            g()
        def i():
            h()
        def j(p):
            i()
        f_ident = ident(f)
        g_ident = ident(g)
        h_ident = ident(h)
        i_ident = ident(i)
        j_ident = ident(j)
        self.check_events(j, [(1, 'call', j_ident),
                              (2, 'call', i_ident),
                              (3, 'call', h_ident),
                              (4, 'call', g_ident),
                              (5, 'call', f_ident),
                              (5, 'return', f_ident),
                              (4, 'return', g_ident),
                              (3, 'return', h_ident),
                              (2, 'return', i_ident),
                              (1, 'return', j_ident),
                              ])


def ident(function):
    if hasattr(function, "f_code"):
        code = function.f_code
    else:
        code = function.__code__
    return code.co_firstlineno, code.co_name


def protect(f, p):
    try: f(p)
    except: pass

protect_ident = ident(protect)


def capture_events(callable, p=None):
    try:
        sys.setprofile()
    except TypeError:
        pass
    else:
        raise test_support.TestFailed(
            'sys.setprofile() did not raise TypeError')

    if p is None:
        p = HookWatcher()
    sys.setprofile(p.callback)
    protect(callable, p)
    sys.setprofile(None)
    return p.get_events()[1:-1]


def show_events(callable):
    import pprint
    pprint.pprint(capture_events(callable))


def test_main():
    test_support.run_unittest(
        TestGetProfile,
        ProfileHookTestCase,
        ProfileSimulatorTestCase
    )


if __name__ == "__main__":
    test_main()
