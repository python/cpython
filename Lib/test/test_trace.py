# Testing the line trace facility.

import test_support
import unittest
import sys
import difflib

if not __debug__:
    raise test_support.TestSkipped, "tracing not supported under -O"

# A very basic example.  If this fails, we're in deep trouble.
def basic():
    return 1

basic.events = [(0, 'call'),
                (0, 'line'),
                (1, 'line'),
                (1, 'return')]

# Armin Rigo's failing example:
def arigo_example():
    x = 1
    del x
    while 0:
        pass
    x = 1

arigo_example.events = [(0, 'call'),
                        (0, 'line'),
                        (1, 'line'),
                        (2, 'line'),
                        (3, 'line'),
                        (3, 'line'),
                        (5, 'line'),
                        (5, 'return')]

# check that lines consisting of just one instruction get traced:
def one_instr_line():
    x = 1
    del x
    x = 1

one_instr_line.events = [(0, 'call'),
                         (0, 'line'),
                         (1, 'line'),
                         (2, 'line'),
                         (3, 'line'),
                         (3, 'return')]

def no_pop_tops():      # 0
    x = 1               # 1
    for a in range(2):  # 2
        if a:           # 3
            x = 1       # 4
        else:           # 5
            x = 1       # 6

no_pop_tops.events = [(0, 'call'),
                      (0, 'line'),
                      (1, 'line'),
                      (2, 'line'),
                      (2, 'line'),
                      (3, 'line'),
                      (6, 'line'),
                      (2, 'line'),
                      (3, 'line'),
                      (4, 'line'),
                      (2, 'line'),
                      (2, 'return')]

def no_pop_blocks():
    while 0:
        bla
    x = 1

no_pop_blocks.events = [(0, 'call'),
                        (0, 'line'),
                        (1, 'line'),
                        (1, 'line'),
                        (3, 'line'),
                        (3, 'return')]

def called(): # line -3
    x = 1

def call():   # line 0
    called()

call.events = [(0, 'call'),
               (0, 'line'),
               (1, 'line'),
               (-3, 'call'),
               (-3, 'line'),
               (-2, 'line'),
               (-2, 'return'),
               (1, 'return')]

def raises():
    raise Exception

def test_raise():
    try:
        raises()
    except Exception, exc:
        x = 1

test_raise.events = [(0, 'call'),
                     (0, 'line'),
                     (1, 'line'),
                     (2, 'line'),
                     (-3, 'call'),
                     (-3, 'line'),
                     (-2, 'line'),
                     (-2, 'exception'),
                     (2, 'exception'),
                     (3, 'line'),
                     (4, 'line'),
                     (4, 'return')]

def _settrace_and_return(tracefunc):
    sys.settrace(tracefunc)
    sys._getframe().f_back.f_trace = tracefunc
def settrace_and_return(tracefunc):
    _settrace_and_return(tracefunc)

settrace_and_return.events = [(1, 'return')]

def _settrace_and_raise(tracefunc):
    sys.settrace(tracefunc)
    sys._getframe().f_back.f_trace = tracefunc
    raise RuntimeError
def settrace_and_raise(tracefunc):
    try:
        _settrace_and_raise(tracefunc)
    except RuntimeError, exc:
        pass

settrace_and_raise.events = [(2, 'exception'),
                             (3, 'line'),
                             (4, 'line'),
                             (4, 'return')]

class Tracer:
    def __init__(self):
        self.events = []
    def trace(self, frame, event, arg):
        self.events.append((frame.f_lineno, event))
        return self.trace

class TraceTestCase(unittest.TestCase):
    def compare_events(self, line_offset, events, expected_events):
        events = [(l - line_offset, e) for (l, e) in events]
        if events != expected_events:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff(map(str, expected_events),
                                        map(str, events))))


    def run_test(self, func):
        tracer = Tracer()
        sys.settrace(tracer.trace)
        func()
        sys.settrace(None)
        self.compare_events(func.func_code.co_firstlineno,
                            tracer.events, func.events)

    def run_test2(self, func):
        tracer = Tracer()
        func(tracer.trace)
        sys.settrace(None)
        self.compare_events(func.func_code.co_firstlineno,
                            tracer.events, func.events)

    def test_1_basic(self):
        self.run_test(basic)
    def test_2_arigo(self):
        self.run_test(arigo_example)
    def test_3_one_instr(self):
        self.run_test(one_instr_line)
    def test_4_no_pop_blocks(self):
        self.run_test(no_pop_blocks)
    def test_5_no_pop_tops(self):
        self.run_test(no_pop_tops)
    def test_6_call(self):
        self.run_test(call)
    def test_7_raise(self):
        self.run_test(test_raise)

    def test_8_settrace_and_return(self):
        self.run_test2(settrace_and_return)
    def test_9_settrace_and_raise(self):
        self.run_test2(settrace_and_raise)

class RaisingTraceFuncTestCase(unittest.TestCase):
    def test_it(self):
        def tr(frame, event, arg):
            raise ValueError # just something that isn't RuntimeError
        def f():
            return 1
        try:
            for i in xrange(sys.getrecursionlimit() + 1):
                sys.settrace(tr)
                try:
                    f()
                except ValueError:
                    pass
                else:
                    self.fail("exception not thrown!")
        except RuntimeError:
            self.fail("recursion counter not reset")


def test_main():
    test_support.run_unittest(TraceTestCase)
    test_support.run_unittest(RaisingTraceFuncTestCase)

if __name__ == "__main__":
    test_main()
