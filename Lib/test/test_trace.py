# Testing the line trace facility.

from test import test_support
import unittest
import sys
import difflib
import gc

# A very basic example.  If this fails, we're in deep trouble.
def basic():
    return 1

basic.events = [(0, 'call'),
                (1, 'line'),
                (1, 'return')]

# Many of the tests below are tricky because they involve pass statements.
# If there is implicit control flow around a pass statement (in an except
# clause or else caluse) under what conditions do you set a line number
# following that clause?


# The entire "while 0:" statement is optimized away.  No code
# exists for it, so the line numbers skip directly from "del x"
# to "x = 1".
def arigo_example():
    x = 1
    del x
    while 0:
        pass
    x = 1

arigo_example.events = [(0, 'call'),
                        (1, 'line'),
                        (2, 'line'),
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
                      (2, 'return')]

def no_pop_blocks():
    y = 1
    while not y:
        bla
    x = 1

no_pop_blocks.events = [(0, 'call'),
                        (1, 'line'),
                        (2, 'line'),
                        (4, 'line'),
                        (4, 'return')]

def called(): # line -3
    x = 1

def call():   # line 0
    called()

call.events = [(0, 'call'),
               (1, 'line'),
               (-3, 'call'),
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
                     (1, 'line'),
                     (2, 'line'),
                     (-3, 'call'),
                     (-2, 'line'),
                     (-2, 'exception'),
                     (-2, 'return'),
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

# implicit return example
# This test is interesting because of the else: pass
# part of the code.  The code generate for the true
# part of the if contains a jump past the else branch.
# The compiler then generates an implicit "return None"
# Internally, the compiler visits the pass statement
# and stores its line number for use on the next instruction.
# The next instruction is the implicit return None.
def ireturn_example():
    a = 5
    b = 5
    if a == b:
        b = a+1
    else:
        pass

ireturn_example.events = [(0, 'call'),
                          (1, 'line'),
                          (2, 'line'),
                          (3, 'line'),
                          (4, 'line'),
                          (6, 'line'),
                          (6, 'return')]

# Tight loop with while(1) example (SF #765624)
def tightloop_example():
    items = range(0, 3)
    try:
        i = 0
        while 1:
            b = items[i]; i+=1
    except IndexError:
        pass

tightloop_example.events = [(0, 'call'),
                            (1, 'line'),
                            (2, 'line'),
                            (3, 'line'),
                            (4, 'line'),
                            (5, 'line'),
                            (5, 'line'),
                            (5, 'line'),
                            (5, 'line'),
                            (5, 'exception'),
                            (6, 'line'),
                            (7, 'line'),
                            (7, 'return')]

def tighterloop_example():
    items = range(1, 4)
    try:
        i = 0
        while 1: i = items[i]
    except IndexError:
        pass

tighterloop_example.events = [(0, 'call'),
                            (1, 'line'),
                            (2, 'line'),
                            (3, 'line'),
                            (4, 'line'),
                            (4, 'line'),
                            (4, 'line'),
                            (4, 'line'),
                            (4, 'exception'),
                            (5, 'line'),
                            (6, 'line'),
                            (6, 'return')]

def generator_function():
    try:
        yield True
        "continued"
    finally:
        "finally"
def generator_example():
    # any() will leave the generator before its end
    x = any(generator_function())

    # the following lines were not traced
    for x in range(10):
        y = x

generator_example.events = ([(0, 'call'),
                             (2, 'line'),
                             (-6, 'call'),
                             (-5, 'line'),
                             (-4, 'line'),
                             (-4, 'return'),
                             (-4, 'call'),
                             (-4, 'exception'),
                             (-1, 'line'),
                             (-1, 'return')] +
                            [(5, 'line'), (6, 'line')] * 10 +
                            [(5, 'line'), (5, 'return')])


class Tracer:
    def __init__(self):
        self.events = []
    def trace(self, frame, event, arg):
        self.events.append((frame.f_lineno, event))
        return self.trace
    def traceWithGenexp(self, frame, event, arg):
        (o for o in [1])
        self.events.append((frame.f_lineno, event))
        return self.trace

class TraceTestCase(unittest.TestCase):

    # Disable gc collection when tracing, otherwise the
    # deallocators may be traced as well.
    def setUp(self):
        self.using_gc = gc.isenabled()
        gc.disable()

    def tearDown(self):
        if self.using_gc:
            gc.enable()

    def compare_events(self, line_offset, events, expected_events):
        events = [(l - line_offset, e) for (l, e) in events]
        if events != expected_events:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff([str(x) for x in expected_events],
                                        [str(x) for x in events])))

    def run_and_compare(self, func, events):
        tracer = Tracer()
        sys.settrace(tracer.trace)
        func()
        sys.settrace(None)
        self.compare_events(func.func_code.co_firstlineno,
                            tracer.events, events)

    def run_test(self, func):
        self.run_and_compare(func, func.events)

    def run_test2(self, func):
        tracer = Tracer()
        func(tracer.trace)
        sys.settrace(None)
        self.compare_events(func.func_code.co_firstlineno,
                            tracer.events, func.events)

    def set_and_retrieve_none(self):
        sys.settrace(None)
        assert sys.gettrace() is None

    def set_and_retrieve_func(self):
        def fn(*args):
            pass

        sys.settrace(fn)
        try:
            assert sys.gettrace() is fn
        finally:
            sys.settrace(None)

    def test_01_basic(self):
        self.run_test(basic)
    def test_02_arigo(self):
        self.run_test(arigo_example)
    def test_03_one_instr(self):
        self.run_test(one_instr_line)
    def test_04_no_pop_blocks(self):
        self.run_test(no_pop_blocks)
    def test_05_no_pop_tops(self):
        self.run_test(no_pop_tops)
    def test_06_call(self):
        self.run_test(call)
    def test_07_raise(self):
        self.run_test(test_raise)

    def test_08_settrace_and_return(self):
        self.run_test2(settrace_and_return)
    def test_09_settrace_and_raise(self):
        self.run_test2(settrace_and_raise)
    def test_10_ireturn(self):
        self.run_test(ireturn_example)
    def test_11_tightloop(self):
        self.run_test(tightloop_example)
    def test_12_tighterloop(self):
        self.run_test(tighterloop_example)

    def test_13_genexp(self):
        self.run_test(generator_example)
        # issue1265: if the trace function contains a generator,
        # and if the traced function contains another generator
        # that is not completely exhausted, the trace stopped.
        # Worse: the 'finally' clause was not invoked.
        tracer = Tracer()
        sys.settrace(tracer.traceWithGenexp)
        generator_example()
        sys.settrace(None)
        self.compare_events(generator_example.__code__.co_firstlineno,
                            tracer.events, generator_example.events)

    def test_14_onliner_if(self):
        def onliners():
            if True: False
            else: True
            return 0
        self.run_and_compare(
            onliners,
            [(0, 'call'),
             (1, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_15_loops(self):
        # issue1750076: "while" expression is skipped by debugger
        def for_example():
            for x in range(2):
                pass
        self.run_and_compare(
            for_example,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (1, 'line'),
             (2, 'line'),
             (1, 'line'),
             (1, 'return')])

        def while_example():
            # While expression should be traced on every loop
            x = 2
            while x > 0:
                x -= 1
        self.run_and_compare(
            while_example,
            [(0, 'call'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (3, 'line'),
             (4, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_16_blank_lines(self):
        exec("def f():\n" + "\n" * 256 + "    pass")
        self.run_and_compare(
            f,
            [(0, 'call'),
             (257, 'line'),
             (257, 'return')])


class RaisingTraceFuncTestCase(unittest.TestCase):
    def trace(self, frame, event, arg):
        """A trace function that raises an exception in response to a
        specific trace event."""
        if event == self.raiseOnEvent:
            raise ValueError # just something that isn't RuntimeError
        else:
            return self.trace

    def f(self):
        """The function to trace; raises an exception if that's the case
        we're testing, so that the 'exception' trace event fires."""
        if self.raiseOnEvent == 'exception':
            x = 0
            y = 1/x
        else:
            return 1

    def run_test_for_event(self, event):
        """Tests that an exception raised in response to the given event is
        handled OK."""
        self.raiseOnEvent = event
        try:
            for i in xrange(sys.getrecursionlimit() + 1):
                sys.settrace(self.trace)
                try:
                    self.f()
                except ValueError:
                    pass
                else:
                    self.fail("exception not thrown!")
        except RuntimeError:
            self.fail("recursion counter not reset")

    # Test the handling of exceptions raised by each kind of trace event.
    def test_call(self):
        self.run_test_for_event('call')
    def test_line(self):
        self.run_test_for_event('line')
    def test_return(self):
        self.run_test_for_event('return')
    def test_exception(self):
        self.run_test_for_event('exception')

    def test_trash_stack(self):
        def f():
            for i in range(5):
                print i  # line tracing will raise an exception at this line

        def g(frame, why, extra):
            if (why == 'line' and
                frame.f_lineno == f.func_code.co_firstlineno + 2):
                raise RuntimeError, "i am crashing"
            return g

        sys.settrace(g)
        try:
            f()
        except RuntimeError:
            # the test is really that this doesn't segfault:
            import gc
            gc.collect()
        else:
            self.fail("exception not propagated")


# 'Jump' tests: assigning to frame.f_lineno within a trace function
# moves the execution position - it's how debuggers implement a Jump
# command (aka. "Set next statement").

class JumpTracer:
    """Defines a trace function that jumps from one place to another,
    with the source and destination lines of the jump being defined by
    the 'jump' property of the function under test."""

    def __init__(self, function):
        self.function = function
        self.jumpFrom = function.jump[0]
        self.jumpTo = function.jump[1]
        self.done = False

    def trace(self, frame, event, arg):
        if not self.done and frame.f_code == self.function.func_code:
            firstLine = frame.f_code.co_firstlineno
            if frame.f_lineno == firstLine + self.jumpFrom:
                # Cope with non-integer self.jumpTo (because of
                # no_jump_to_non_integers below).
                try:
                    frame.f_lineno = firstLine + self.jumpTo
                except TypeError:
                    frame.f_lineno = self.jumpTo
                self.done = True
        return self.trace

# The first set of 'jump' tests are for things that are allowed:

def jump_simple_forwards(output):
    output.append(1)
    output.append(2)
    output.append(3)

jump_simple_forwards.jump = (1, 3)
jump_simple_forwards.output = [3]

def jump_simple_backwards(output):
    output.append(1)
    output.append(2)

jump_simple_backwards.jump = (2, 1)
jump_simple_backwards.output = [1, 1, 2]

def jump_out_of_block_forwards(output):
    for i in 1, 2:
        output.append(2)
        for j in [3]:  # Also tests jumping over a block
            output.append(4)
    output.append(5)

jump_out_of_block_forwards.jump = (3, 5)
jump_out_of_block_forwards.output = [2, 5]

def jump_out_of_block_backwards(output):
    output.append(1)
    for i in [1]:
        output.append(3)
        for j in [2]:  # Also tests jumping over a block
            output.append(5)
        output.append(6)
    output.append(7)

jump_out_of_block_backwards.jump = (6, 1)
jump_out_of_block_backwards.output = [1, 3, 5, 1, 3, 5, 6, 7]

def jump_to_codeless_line(output):
    output.append(1)
    # Jumping to this line should skip to the next one.
    output.append(3)

jump_to_codeless_line.jump = (1, 2)
jump_to_codeless_line.output = [3]

def jump_to_same_line(output):
    output.append(1)
    output.append(2)
    output.append(3)

jump_to_same_line.jump = (2, 2)
jump_to_same_line.output = [1, 2, 3]

# Tests jumping within a finally block, and over one.
def jump_in_nested_finally(output):
    try:
        output.append(2)
    finally:
        output.append(4)
        try:
            output.append(6)
        finally:
            output.append(8)
        output.append(9)

jump_in_nested_finally.jump = (4, 9)
jump_in_nested_finally.output = [2, 9]

# The second set of 'jump' tests are for things that are not allowed:

def no_jump_too_far_forwards(output):
    try:
        output.append(2)
        output.append(3)
    except ValueError, e:
        output.append('after' in str(e))

no_jump_too_far_forwards.jump = (3, 6)
no_jump_too_far_forwards.output = [2, True]

def no_jump_too_far_backwards(output):
    try:
        output.append(2)
        output.append(3)
    except ValueError, e:
        output.append('before' in str(e))

no_jump_too_far_backwards.jump = (3, -1)
no_jump_too_far_backwards.output = [2, True]

# Test each kind of 'except' line.
def no_jump_to_except_1(output):
    try:
        output.append(2)
    except:
        e = sys.exc_info()[1]
        output.append('except' in str(e))

no_jump_to_except_1.jump = (2, 3)
no_jump_to_except_1.output = [True]

def no_jump_to_except_2(output):
    try:
        output.append(2)
    except ValueError:
        e = sys.exc_info()[1]
        output.append('except' in str(e))

no_jump_to_except_2.jump = (2, 3)
no_jump_to_except_2.output = [True]

def no_jump_to_except_3(output):
    try:
        output.append(2)
    except ValueError, e:
        output.append('except' in str(e))

no_jump_to_except_3.jump = (2, 3)
no_jump_to_except_3.output = [True]

def no_jump_to_except_4(output):
    try:
        output.append(2)
    except (ValueError, RuntimeError), e:
        output.append('except' in str(e))

no_jump_to_except_4.jump = (2, 3)
no_jump_to_except_4.output = [True]

def no_jump_forwards_into_block(output):
    try:
        output.append(2)
        for i in 1, 2:
            output.append(4)
    except ValueError, e:
        output.append('into' in str(e))

no_jump_forwards_into_block.jump = (2, 4)
no_jump_forwards_into_block.output = [True]

def no_jump_backwards_into_block(output):
    try:
        for i in 1, 2:
            output.append(3)
        output.append(4)
    except ValueError, e:
        output.append('into' in str(e))

no_jump_backwards_into_block.jump = (4, 3)
no_jump_backwards_into_block.output = [3, 3, True]

def no_jump_into_finally_block(output):
    try:
        try:
            output.append(3)
            x = 1
        finally:
            output.append(6)
    except ValueError, e:
        output.append('finally' in str(e))

no_jump_into_finally_block.jump = (4, 6)
no_jump_into_finally_block.output = [3, 6, True]  # The 'finally' still runs

def no_jump_out_of_finally_block(output):
    try:
        try:
            output.append(3)
        finally:
            output.append(5)
            output.append(6)
    except ValueError, e:
        output.append('finally' in str(e))

no_jump_out_of_finally_block.jump = (5, 1)
no_jump_out_of_finally_block.output = [3, True]

# This verifies the line-numbers-must-be-integers rule.
def no_jump_to_non_integers(output):
    try:
        output.append(2)
    except ValueError, e:
        output.append('integer' in str(e))

no_jump_to_non_integers.jump = (2, "Spam")
no_jump_to_non_integers.output = [True]

# This verifies that you can't set f_lineno via _getframe or similar
# trickery.
def no_jump_without_trace_function():
    try:
        previous_frame = sys._getframe().f_back
        previous_frame.f_lineno = previous_frame.f_lineno
    except ValueError, e:
        # This is the exception we wanted; make sure the error message
        # talks about trace functions.
        if 'trace' not in str(e):
            raise
    else:
        # Something's wrong - the expected exception wasn't raised.
        raise RuntimeError, "Trace-function-less jump failed to fail"


class JumpTestCase(unittest.TestCase):
    def compare_jump_output(self, expected, received):
        if received != expected:
            self.fail( "Outputs don't match:\n" +
                       "Expected: " + repr(expected) + "\n" +
                       "Received: " + repr(received))

    def run_test(self, func):
        tracer = JumpTracer(func)
        sys.settrace(tracer.trace)
        output = []
        func(output)
        sys.settrace(None)
        self.compare_jump_output(func.output, output)

    def test_01_jump_simple_forwards(self):
        self.run_test(jump_simple_forwards)
    def test_02_jump_simple_backwards(self):
        self.run_test(jump_simple_backwards)
    def test_03_jump_out_of_block_forwards(self):
        self.run_test(jump_out_of_block_forwards)
    def test_04_jump_out_of_block_backwards(self):
        self.run_test(jump_out_of_block_backwards)
    def test_05_jump_to_codeless_line(self):
        self.run_test(jump_to_codeless_line)
    def test_06_jump_to_same_line(self):
        self.run_test(jump_to_same_line)
    def test_07_jump_in_nested_finally(self):
        self.run_test(jump_in_nested_finally)
    def test_08_no_jump_too_far_forwards(self):
        self.run_test(no_jump_too_far_forwards)
    def test_09_no_jump_too_far_backwards(self):
        self.run_test(no_jump_too_far_backwards)
    def test_10_no_jump_to_except_1(self):
        self.run_test(no_jump_to_except_1)
    def test_11_no_jump_to_except_2(self):
        self.run_test(no_jump_to_except_2)
    def test_12_no_jump_to_except_3(self):
        self.run_test(no_jump_to_except_3)
    def test_13_no_jump_to_except_4(self):
        self.run_test(no_jump_to_except_4)
    def test_14_no_jump_forwards_into_block(self):
        self.run_test(no_jump_forwards_into_block)
    def test_15_no_jump_backwards_into_block(self):
        self.run_test(no_jump_backwards_into_block)
    def test_16_no_jump_into_finally_block(self):
        self.run_test(no_jump_into_finally_block)
    def test_17_no_jump_out_of_finally_block(self):
        self.run_test(no_jump_out_of_finally_block)
    def test_18_no_jump_to_non_integers(self):
        self.run_test(no_jump_to_non_integers)
    def test_19_no_jump_without_trace_function(self):
        no_jump_without_trace_function()

def test_main():
    test_support.run_unittest(
        TraceTestCase,
        RaisingTraceFuncTestCase,
        JumpTestCase
    )

if __name__ == "__main__":
    test_main()
