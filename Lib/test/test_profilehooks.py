import pprint
import sys
import unittest

import test_support


class HookWatcher:
    def __init__(self):
        self.frames = []
        self.events = []

    def callback(self, frame, event, arg):
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
        disallowed = [ident(self.add_event.im_func), ident(ident)]
        self.frames = None

        return [item for item in self.events if item[2] not in disallowed]


class ProfileHookTestCase(unittest.TestCase):

    def check_events(self, callable, expected):
        events = capture_events(callable)
        if events != expected:
            self.fail("Expected events:\n%s\nReceived events:\n%s"
                      % (pprint.pformat(expected), pprint.pformat(events)))

    def test_simple(self):
        def f(p):
            pass
        f_ident = ident(f)
        self.check_events(f, [(0, 'call', f_ident),
                              (0, 'return', f_ident),
                              ])

    def test_exception(self):
        def f(p):
            try: 1/0
            except: pass
        f_ident = ident(f)
        self.check_events(f, [(0, 'call', f_ident),
                              (0, 'exception', f_ident),
                              (0, 'return', f_ident),
                              ])

    def test_caught_nested_exception(self):
        def f(p):
            try: 1/0
            except: pass
        def g(p):
            f(p)
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(0, 'call', g_ident),
                              (1, 'call', f_ident),
                              (1, 'exception', f_ident),
                              (1, 'return', f_ident),
                              (0, 'return', g_ident),
                              ])

    def test_nested_exception(self):
        def f(p):
            1/0
        def g(p):
            try: f(p)
            except: pass
        f_ident = ident(f)
        g_ident = ident(g)
        self.check_events(g, [(0, 'call', g_ident),
                              (1, 'call', f_ident),
                              (1, 'exception', f_ident),
                              # This isn't what I expected:
                              (0, 'exception', g_ident),
                              # I expected this again:
                              # (1, 'exception', f_ident),
                              (0, 'return', g_ident),
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
        self.check_events(g, [(0, 'call', g_ident),
                              (1, 'call', f_ident),
                              (1, 'exception', f_ident),
                              (0, 'exception', g_ident),
                              (2, 'call', f_ident),
                              (2, 'exception', f_ident),
                              (0, 'exception', g_ident),
                              (0, 'return', g_ident),
                              ])

    def test_exception_propogation(self):
        def f(p):
            1/0
        def g(p):
            try: f(p)
            finally: p.add_event("falling through")
        def h(p):
            try: g(p)
            except: pass
        f_ident = ident(f)
        g_ident = ident(g)
        h_ident = ident(h)
        self.check_events(h, [(0, 'call', h_ident),
                              (1, 'call', g_ident),
                              (2, 'call', f_ident),
                              (2, 'exception', f_ident),
                              (1, 'exception', g_ident),
                              (1, 'falling through', g_ident),
                              (0, 'exception', h_ident),
                              (0, 'return', h_ident),
                              ])

def ident(function):
    if hasattr(function, "f_code"):
        code = function.f_code
    else:
        code = function.func_code
    return code.co_firstlineno, code.co_name


def capture_events(callable):
    p = HookWatcher()
    sys.setprofile(p.callback)
    callable(p)
    sys.setprofile(None)
    return p.get_events()


def show_events(callable):
    import pprint
    pprint.pprint(capture_events(callable))


def test_main():
    test_support.run_unittest(ProfileHookTestCase)


if __name__ == "__main__":
    test_main()
