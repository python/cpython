# Testing the line trace facility.

from test import test_support
import unittest
import sys
import difflib

# A very basic example.  If this fails, we're in deep trouble.
def basic():
    return 1

basic.events = [(0, 'call'),
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
                        (1, 'line'),
                        (2, 'line'),
                        (3, 'line'),
                        (5, 'line'),
                        (5, 'return')]

# check that lines consisting of just one instruction get traced:
def one_instr_line():
    x = 1
    del x
    x = 1

one_instr_line.events = [(0, 'call'),
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
                      (1, 'line'),
                      (2, 'line'),
                      (3, 'line'),
                      (6, 'line'),
                      (2, 'line'),
                      (3, 'line'),
                      (4, 'line'),
                      (2, 'line'),
                      (6, 'return')]

def no_pop_blocks():
    while 0:
        bla
    x = 1

no_pop_blocks.events = [(0, 'call'),
                        (1, 'line'),
                        (3, 'line'),
                        (3, 'return')]

class Tracer:
    def __init__(self):
        self.events = []
    def trace(self, frame, event, arg):
        self.events.append((frame.f_lineno, event))
        return self.trace

class TraceTestCase(unittest.TestCase):
    def run_test(self, func):
        tracer = Tracer()
        sys.settrace(tracer.trace)
        func()
        sys.settrace(None)
        fl = func.func_code.co_firstlineno
        events = [(l - fl, e) for (l, e) in tracer.events]
        if events != func.events:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff(map(str, func.events),
                                        map(str, events))))
    
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
                                      
                                  

def test_main():
    test_support.run_unittest(TraceTestCase)

if __name__ == "__main__":
    test_main()
