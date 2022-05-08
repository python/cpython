# Testing the line trace facility.

from test import support
import unittest
import sys
import difflib
import gc
from functools import wraps
import asyncio

support.requires_working_socket(module=True)

class tracecontext:
    """Context manager that traces its enter and exit."""
    def __init__(self, output, value):
        self.output = output
        self.value = value

    def __enter__(self):
        self.output.append(self.value)

    def __exit__(self, *exc_info):
        self.output.append(-self.value)

class asynctracecontext:
    """Asynchronous context manager that traces its aenter and aexit."""
    def __init__(self, output, value):
        self.output = output
        self.value = value

    async def __aenter__(self):
        self.output.append(self.value)

    async def __aexit__(self, *exc_info):
        self.output.append(-self.value)

async def asynciter(iterable):
    """Convert an iterable to an asynchronous iterator."""
    for x in iterable:
        yield x


# A very basic example.  If this fails, we're in deep trouble.
def basic():
    return 1

basic.events = [(0, 'call'),
                (1, 'line'),
                (1, 'return')]

# Many of the tests below are tricky because they involve pass statements.
# If there is implicit control flow around a pass statement (in an except
# clause or else clause) under what conditions do you set a line number
# following that clause?


# Some constructs like "while 0:", "if 0:" or "if 1:...else:..." could be optimized
# away.  Make sure that those lines aren't skipped.
def arigo_example0():
    x = 1
    del x
    while 0:
        pass
    x = 1

arigo_example0.events = [(0, 'call'),
                        (1, 'line'),
                        (2, 'line'),
                        (3, 'line'),
                        (5, 'line'),
                        (5, 'return')]

def arigo_example1():
    x = 1
    del x
    if 0:
        pass
    x = 1

arigo_example1.events = [(0, 'call'),
                        (1, 'line'),
                        (2, 'line'),
                        (3, 'line'),
                        (5, 'line'),
                        (5, 'return')]

def arigo_example2():
    x = 1
    del x
    if 1:
        x = 1
    else:
        pass
    return None

arigo_example2.events = [(0, 'call'),
                        (1, 'line'),
                        (2, 'line'),
                        (3, 'line'),
                        (4, 'line'),
                        (7, 'line'),
                        (7, 'return')]


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
    except Exception:
        pass

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
    except RuntimeError:
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
                          (4, 'return')]

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
                            (4, 'line'),
                            (5, 'line'),
                            (4, 'line'),
                            (5, 'line'),
                            (4, 'line'),
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
    def __init__(self, trace_line_events=None, trace_opcode_events=None):
        self.trace_line_events = trace_line_events
        self.trace_opcode_events = trace_opcode_events
        self.events = []

    def _reconfigure_frame(self, frame):
        if self.trace_line_events is not None:
            frame.f_trace_lines = self.trace_line_events
        if self.trace_opcode_events is not None:
            frame.f_trace_opcodes = self.trace_opcode_events

    def trace(self, frame, event, arg):
        self._reconfigure_frame(frame)
        self.events.append((frame.f_lineno, event))
        return self.trace

    def traceWithGenexp(self, frame, event, arg):
        self._reconfigure_frame(frame)
        (o for o in [1])
        self.events.append((frame.f_lineno, event))
        return self.trace


class TraceTestCase(unittest.TestCase):

    # Disable gc collection when tracing, otherwise the
    # deallocators may be traced as well.
    def setUp(self):
        self.using_gc = gc.isenabled()
        gc.disable()
        self.addCleanup(sys.settrace, sys.gettrace())

    def tearDown(self):
        if self.using_gc:
            gc.enable()

    @staticmethod
    def make_tracer():
        """Helper to allow test subclasses to configure tracers differently"""
        return Tracer()

    def compare_events(self, line_offset, events, expected_events):
        events = [(l - line_offset, e) for (l, e) in events]
        if events != expected_events:
            self.fail(
                "events did not match expectation:\n" +
                "\n".join(difflib.ndiff([str(x) for x in expected_events],
                                        [str(x) for x in events])))

    def run_and_compare(self, func, events):
        tracer = self.make_tracer()
        sys.settrace(tracer.trace)
        func()
        sys.settrace(None)
        self.compare_events(func.__code__.co_firstlineno,
                            tracer.events, events)

    def run_test(self, func):
        self.run_and_compare(func, func.events)

    def run_test2(self, func):
        tracer = self.make_tracer()
        func(tracer.trace)
        sys.settrace(None)
        self.compare_events(func.__code__.co_firstlineno,
                            tracer.events, func.events)

    def test_set_and_retrieve_none(self):
        sys.settrace(None)
        assert sys.gettrace() is None

    def test_set_and_retrieve_func(self):
        def fn(*args):
            pass

        sys.settrace(fn)
        try:
            assert sys.gettrace() is fn
        finally:
            sys.settrace(None)

    def test_01_basic(self):
        self.run_test(basic)
    def test_02_arigo0(self):
        self.run_test(arigo_example0)
    def test_02_arigo1(self):
        self.run_test(arigo_example1)
    def test_02_arigo2(self):
        self.run_test(arigo_example2)
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
        tracer = self.make_tracer()
        sys.settrace(tracer.traceWithGenexp)
        generator_example()
        sys.settrace(None)
        self.compare_events(generator_example.__code__.co_firstlineno,
                            tracer.events, generator_example.events)

    def test_14_onliner_if(self):
        def onliners():
            if True: x=False
            else: x=True
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
        namespace = {}
        exec("def f():\n" + "\n" * 256 + "    pass", namespace)
        self.run_and_compare(
            namespace["f"],
            [(0, 'call'),
             (257, 'line'),
             (257, 'return')])

    def test_17_none_f_trace(self):
        # Issue 20041: fix TypeError when f_trace is set to None.
        def func():
            sys._getframe().f_trace = None
            lineno = 2
        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line')])

    def test_18_except_with_name(self):
        def func():
            try:
                try:
                    raise Exception
                except Exception as e:
                    raise
                    x = "Something"
                    y = "Something"
            except Exception:
                pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (5, 'line'),
             (8, 'line'),
             (9, 'line'),
             (9, 'return')])

    def test_19_except_with_finally(self):
        def func():
            try:
                try:
                    raise Exception
                finally:
                    y = "Something"
            except Exception:
                b = 23

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (5, 'line'),
             (6, 'line'),
             (7, 'line'),
             (7, 'return')])

    def test_20_async_for_loop(self):
        class AsyncIteratorWrapper:
            def __init__(self, obj):
                self._it = iter(obj)

            def __aiter__(self):
                return self

            async def __anext__(self):
                try:
                    return next(self._it)
                except StopIteration:
                    raise StopAsyncIteration

        async def doit_async():
            async for letter in AsyncIteratorWrapper("abc"):
                x = letter
            y = 42

        def run(tracer):
            x = doit_async()
            try:
                sys.settrace(tracer)
                x.send(None)
            finally:
                sys.settrace(None)

        tracer = self.make_tracer()
        events = [
                (0, 'call'),
                (1, 'line'),
                (-12, 'call'),
                (-11, 'line'),
                (-11, 'return'),
                (-9, 'call'),
                (-8, 'line'),
                (-8, 'return'),
                (-6, 'call'),
                (-5, 'line'),
                (-4, 'line'),
                (-4, 'return'),
                (1, 'exception'),
                (2, 'line'),
                (1, 'line'),
                (-6, 'call'),
                (-5, 'line'),
                (-4, 'line'),
                (-4, 'return'),
                (1, 'exception'),
                (2, 'line'),
                (1, 'line'),
                (-6, 'call'),
                (-5, 'line'),
                (-4, 'line'),
                (-4, 'return'),
                (1, 'exception'),
                (2, 'line'),
                (1, 'line'),
                (-6, 'call'),
                (-5, 'line'),
                (-4, 'line'),
                (-4, 'exception'),
                (-3, 'line'),
                (-2, 'line'),
                (-2, 'exception'),
                (-2, 'return'),
                (1, 'exception'),
                (3, 'line'),
                (3, 'return')]
        try:
            run(tracer.trace)
        except Exception:
            pass
        self.compare_events(doit_async.__code__.co_firstlineno,
                            tracer.events, events)

    def test_21_repeated_pass(self):
        def func():
            pass
            pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'return')])

    def test_loop_in_try_except(self):
        # https://bugs.python.org/issue41670

        def func():
            try:
                for i in []: pass
                return 1
            except:
                return 2

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_try_except_no_exception(self):

        def func():
            try:
                2
            except:
                4
            else:
                6
                if False:
                    8
                else:
                    10
                if func.__name__ == 'Fred':
                    12
            finally:
                14

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (6, 'line'),
             (7, 'line'),
             (10, 'line'),
             (11, 'line'),
             (14, 'line'),
             (14, 'return')])

    def test_try_exception_in_else(self):

        def func():
            try:
                try:
                    3
                except:
                    5
                else:
                    7
                    raise Exception
                finally:
                    10
            except:
                12
            finally:
                14

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (7, 'line'),
             (8, 'line'),
             (8, 'exception'),
             (10, 'line'),
             (11, 'line'),
             (12, 'line'),
             (14, 'line'),
             (14, 'return')])

    def test_nested_loops(self):

        def func():
            for i in range(2):
                for j in range(2):
                    a = i + j
            return a == 1

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (2, 'line'),
             (3, 'line'),
             (2, 'line'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (2, 'line'),
             (3, 'line'),
             (2, 'line'),
             (1, 'line'),
             (4, 'line'),
             (4, 'return')])

    def test_if_break(self):

        def func():
            seq = [1, 0]
            while seq:
                n = seq.pop()
                if n:
                    break   # line 5
            else:
                n = 99
            return n        # line 8

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (8, 'line'),
             (8, 'return')])

    def test_break_through_finally(self):

        def func():
            a, c, d, i = 1, 1, 1, 99
            try:
                for i in range(3):
                    try:
                        a = 5
                        if i > 0:
                            break                   # line 7
                        a = 8
                    finally:
                        c = 10
            except:
                d = 12                              # line 12
            assert a == 5 and c == 10 and d == 1    # line 13

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (8, 'line'),
             (10, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (7, 'line'),
             (10, 'line'),
             (13, 'line'),
             (13, 'return')])

    def test_continue_through_finally(self):

        def func():
            a, b, c, d, i = 1, 1, 1, 1, 99
            try:
                for i in range(2):
                    try:
                        a = 5
                        if i > 0:
                            continue                # line 7
                        b = 8
                    finally:
                        c = 10
            except:
                d = 12                              # line 12
            assert (a, b, c, d) == (5, 8, 10, 1)    # line 13

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (8, 'line'),
             (10, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (7, 'line'),
             (10, 'line'),
             (3, 'line'),
             (13, 'line'),
             (13, 'return')])

    def test_return_through_finally(self):

        def func():
            try:
                return 2
            finally:
                4

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (4, 'line'),
             (4, 'return')])

    def test_try_except_with_wrong_type(self):

        def func():
            try:
                2/0
            except IndexError:
                4
            finally:
                return 6

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'exception'),
             (3, 'line'),
             (6, 'line'),
             (6, 'return')])

    def test_break_to_continue1(self):

        def func():
            TRUE = 1
            x = [1]
            while x:
                x.pop()
                while TRUE:
                    break
                continue

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (7, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_break_to_continue2(self):

        def func():
            TRUE = 1
            x = [1]
            while x:
                x.pop()
                while TRUE:
                    break
                else:
                    continue

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (6, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_break_to_break(self):

        def func():
            TRUE = 1
            while TRUE:
                while TRUE:
                    break
                break

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (5, 'return')])

    def test_nested_ifs(self):

        def func():
            a = b = 1
            if a == 1:
                if b == 1:
                    x = 4
                else:
                    y = 6
            else:
                z = 8

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (4, 'return')])

    def test_nested_ifs_with_and(self):

        def func():
            if A:
                if B:
                    if C:
                        if D:
                            return False
                else:
                    return False
            elif E and F:
                return True

        A = B = True
        C = False

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_nested_try_if(self):

        def func():
            x = "hello"
            try:
                3/0
            except ZeroDivisionError:
                if x == 'raise':
                    raise ValueError()   # line 6
            f = 7

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (5, 'line'),
             (7, 'line'),
             (7, 'return')])

    def test_if_false_in_with(self):

        class C:
            def __enter__(self):
                return self
            def __exit__(*args):
                pass

        def func():
            with C():
                if False:
                    pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (-5, 'call'),
             (-4, 'line'),
             (-4, 'return'),
             (2, 'line'),
             (1, 'line'),
             (-3, 'call'),
             (-2, 'line'),
             (-2, 'return'),
             (1, 'return')])

    def test_if_false_in_try_except(self):

        def func():
            try:
                if False:
                    pass
            except Exception:
                X

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'return')])

    def test_implicit_return_in_class(self):

        def func():
            class A:
                if 3 < 9:
                    a = 1
                else:
                    a = 2

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (1, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return'),
             (1, 'return')])

    def test_try_in_try(self):
        def func():
            try:
                try:
                    pass
                except Exception as ex:
                    pass
            except Exception:
                pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_try_in_try_with_exception(self):

        def func():
            try:
                try:
                    raise TypeError
                except ValueError as ex:
                    5
            except TypeError:
                7

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (6, 'line'),
             (7, 'line'),
             (7, 'return')])

        def func():
            try:
                try:
                    raise ValueError
                except ValueError as ex:
                    5
            except TypeError:
                7

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (5, 'line'),
             (5, 'return')])

    def test_if_in_if_in_if(self):
        def func(a=0, p=1, z=1):
            if p:
                if a:
                    if z:
                        pass
                    else:
                        pass
            else:
                pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'return')])

    def test_early_exit_with(self):

        class C:
            def __enter__(self):
                return self
            def __exit__(*args):
                pass

        def func_break():
            for i in (1,2):
                with C():
                    break
            pass

        def func_return():
            with C():
                return

        self.run_and_compare(func_break,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (-5, 'call'),
             (-4, 'line'),
             (-4, 'return'),
             (3, 'line'),
             (2, 'line'),
             (-3, 'call'),
             (-2, 'line'),
             (-2, 'return'),
             (4, 'line'),
             (4, 'return')])

        self.run_and_compare(func_return,
            [(0, 'call'),
             (1, 'line'),
             (-11, 'call'),
             (-10, 'line'),
             (-10, 'return'),
             (2, 'line'),
             (1, 'line'),
             (-9, 'call'),
             (-8, 'line'),
             (-8, 'return'),
             (1, 'return')])

    def test_flow_converges_on_same_line(self):

        def foo(x):
            if x:
                try:
                    1/(x - 1)
                except ZeroDivisionError:
                    pass
            return x

        def func():
            for i in range(2):
                foo(i)

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (-8, 'call'),
             (-7, 'line'),
             (-2, 'line'),
             (-2, 'return'),
             (1, 'line'),
             (2, 'line'),
             (-8, 'call'),
             (-7, 'line'),
             (-6, 'line'),
             (-5, 'line'),
             (-5, 'exception'),
             (-4, 'line'),
             (-3, 'line'),
             (-2, 'line'),
             (-2, 'return'),
             (1, 'line'),
             (1, 'return')])

    def test_no_tracing_of_named_except_cleanup(self):

        def func():
            x = 0
            try:
                1/x
            except ZeroDivisionError as error:
                if x:
                    raise
            return "done"

        self.run_and_compare(func,
        [(0, 'call'),
            (1, 'line'),
            (2, 'line'),
            (3, 'line'),
            (3, 'exception'),
            (4, 'line'),
            (5, 'line'),
            (7, 'line'),
            (7, 'return')])

    def test_tracing_exception_raised_in_with(self):

        class NullCtx:
            def __enter__(self):
                return self
            def __exit__(self, *excinfo):
                pass

        def func():
            try:
                with NullCtx():
                    1/0
            except ZeroDivisionError:
                pass

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (-5, 'call'),
             (-4, 'line'),
             (-4, 'return'),
             (3, 'line'),
             (3, 'exception'),
             (2, 'line'),
             (-3, 'call'),
             (-2, 'line'),
             (-2, 'return'),
             (4, 'line'),
             (5, 'line'),
             (5, 'return')])

    def test_try_except_star_no_exception(self):

        def func():
            try:
                2
            except* Exception:
                4
            else:
                6
                if False:
                    8
                else:
                    10
                if func.__name__ == 'Fred':
                    12
            finally:
                14

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (6, 'line'),
             (7, 'line'),
             (10, 'line'),
             (11, 'line'),
             (14, 'line'),
             (14, 'return')])

    def test_try_except_star_named_no_exception(self):

        def func():
            try:
                2
            except* Exception as e:
                4
            else:
                6
            finally:
                8

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (6, 'line'),
             (8, 'line'),
             (8, 'return')])

    def test_try_except_star_exception_caught(self):

        def func():
            try:
                raise ValueError(2)
            except* ValueError:
                4
            else:
                6
            finally:
                8

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'exception'),
             (3, 'line'),
             (4, 'line'),
             (8, 'line'),
             (8, 'return')])

    def test_try_except_star_named_exception_caught(self):

        def func():
            try:
                raise ValueError(2)
            except* ValueError as e:
                4
            else:
                6
            finally:
                8

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (2, 'exception'),
             (3, 'line'),
             (4, 'line'),
             (8, 'line'),
             (8, 'return')])

    def test_try_except_star_exception_not_caught(self):

        def func():
            try:
                try:
                    raise ValueError(3)
                except* TypeError:
                    5
            except ValueError:
                7

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (6, 'line'),
             (7, 'line'),
             (7, 'return')])

    def test_try_except_star_named_exception_not_caught(self):

        def func():
            try:
                try:
                    raise ValueError(3)
                except* TypeError as e:
                    5
            except ValueError:
                7

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (4, 'line'),
             (6, 'line'),
             (7, 'line'),
             (7, 'return')])

    def test_try_except_star_nested(self):

        def func():
            try:
                try:
                    raise ExceptionGroup(
                        'eg',
                        [ValueError(5), TypeError('bad type')])
                except* TypeError as e:
                    7
                except* OSError:
                    9
                except* ValueError:
                    raise
            except* ValueError:
                try:
                    raise TypeError(14)
                except* OSError:
                    16
                except* TypeError as e:
                    18
            return 0

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (4, 'line'),
             (5, 'line'),
             (3, 'line'),
             (3, 'exception'),
             (6, 'line'),
             (7, 'line'),
             (8, 'line'),
             (10, 'line'),
             (11, 'line'),
             (12, 'line'),
             (13, 'line'),
             (14, 'line'),
             (14, 'exception'),
             (15, 'line'),
             (17, 'line'),
             (18, 'line'),
             (19, 'line'),
             (19, 'return')])

    def test_notrace_lambda(self):
        #Regression test for issue 46314

        def func():
            1
            lambda x: 2
            3

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return')])

    def test_class_creation_with_docstrings(self):

        def func():
            class Class_1:
                ''' the docstring. 2'''
                def __init__(self):
                    ''' Another docstring. 4'''
                    self.a = 5

        self.run_and_compare(func,
            [(0, 'call'),
             (1, 'line'),
             (1, 'call'),
             (1, 'line'),
             (2, 'line'),
             (3, 'line'),
             (3, 'return'),
             (1, 'return')])


class SkipLineEventsTraceTestCase(TraceTestCase):
    """Repeat the trace tests, but with per-line events skipped"""

    def compare_events(self, line_offset, events, expected_events):
        skip_line_events = [e for e in expected_events if e[1] != 'line']
        super().compare_events(line_offset, events, skip_line_events)

    @staticmethod
    def make_tracer():
        return Tracer(trace_line_events=False)


@support.cpython_only
class TraceOpcodesTestCase(TraceTestCase):
    """Repeat the trace tests, but with per-opcodes events enabled"""

    def compare_events(self, line_offset, events, expected_events):
        skip_opcode_events = [e for e in events if e[1] != 'opcode']
        if len(events) > 1:
            self.assertLess(len(skip_opcode_events), len(events),
                            msg="No 'opcode' events received by the tracer")
        super().compare_events(line_offset, skip_opcode_events, expected_events)

    @staticmethod
    def make_tracer():
        return Tracer(trace_opcode_events=True)


class RaisingTraceFuncTestCase(unittest.TestCase):
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())

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
            for i in range(sys.getrecursionlimit() + 1):
                sys.settrace(self.trace)
                try:
                    self.f()
                except ValueError:
                    pass
                else:
                    self.fail("exception not raised!")
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
                print(i)  # line tracing will raise an exception at this line

        def g(frame, why, extra):
            if (why == 'line' and
                frame.f_lineno == f.__code__.co_firstlineno + 2):
                raise RuntimeError("i am crashing")
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


    def test_exception_arguments(self):
        def f():
            x = 0
            # this should raise an error
            x.no_such_attr
        def g(frame, event, arg):
            if (event == 'exception'):
                type, exception, trace = arg
                self.assertIsInstance(exception, Exception)
            return g

        existing = sys.gettrace()
        try:
            sys.settrace(g)
            try:
                f()
            except AttributeError:
                # this is expected
                pass
        finally:
            sys.settrace(existing)


# 'Jump' tests: assigning to frame.f_lineno within a trace function
# moves the execution position - it's how debuggers implement a Jump
# command (aka. "Set next statement").

class JumpTracer:
    """Defines a trace function that jumps from one place to another."""

    def __init__(self, function, jumpFrom, jumpTo, event='line',
                 decorated=False):
        self.code = function.__code__
        self.jumpFrom = jumpFrom
        self.jumpTo = jumpTo
        self.event = event
        self.firstLine = None if decorated else self.code.co_firstlineno
        self.done = False

    def trace(self, frame, event, arg):
        if self.done:
            return
        # frame.f_code.co_firstlineno is the first line of the decorator when
        # 'function' is decorated and the decorator may be written using
        # multiple physical lines when it is too long. Use the first line
        # trace event in 'function' to find the first line of 'function'.
        if (self.firstLine is None and frame.f_code == self.code and
                event == 'line'):
            self.firstLine = frame.f_lineno - 1
        if (event == self.event and self.firstLine is not None and
                frame.f_lineno == self.firstLine + self.jumpFrom):
            f = frame
            while f is not None and f.f_code != self.code:
                f = f.f_back
            if f is not None:
                # Cope with non-integer self.jumpTo (because of
                # no_jump_to_non_integers below).
                try:
                    frame.f_lineno = self.firstLine + self.jumpTo
                except TypeError:
                    frame.f_lineno = self.jumpTo
                self.done = True
        return self.trace

# This verifies the line-numbers-must-be-integers rule.
def no_jump_to_non_integers(output):
    try:
        output.append(2)
    except ValueError as e:
        output.append('integer' in str(e))

# This verifies that you can't set f_lineno via _getframe or similar
# trickery.
def no_jump_without_trace_function():
    try:
        previous_frame = sys._getframe().f_back
        previous_frame.f_lineno = previous_frame.f_lineno
    except ValueError as e:
        # This is the exception we wanted; make sure the error message
        # talks about trace functions.
        if 'trace' not in str(e):
            raise
    else:
        # Something's wrong - the expected exception wasn't raised.
        raise AssertionError("Trace-function-less jump failed to fail")


class JumpTestCase(unittest.TestCase):
    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        sys.settrace(None)

    def compare_jump_output(self, expected, received):
        if received != expected:
            self.fail( "Outputs don't match:\n" +
                       "Expected: " + repr(expected) + "\n" +
                       "Received: " + repr(received))

    def run_test(self, func, jumpFrom, jumpTo, expected, error=None,
                 event='line', decorated=False):
        tracer = JumpTracer(func, jumpFrom, jumpTo, event, decorated)
        sys.settrace(tracer.trace)
        output = []
        if error is None:
            func(output)
        else:
            with self.assertRaisesRegex(*error):
                func(output)
        sys.settrace(None)
        self.compare_jump_output(expected, output)

    def run_async_test(self, func, jumpFrom, jumpTo, expected, error=None,
                 event='line', decorated=False):
        tracer = JumpTracer(func, jumpFrom, jumpTo, event, decorated)
        sys.settrace(tracer.trace)
        output = []
        if error is None:
            asyncio.run(func(output))
        else:
            with self.assertRaisesRegex(*error):
                asyncio.run(func(output))
        sys.settrace(None)
        asyncio.set_event_loop_policy(None)
        self.compare_jump_output(expected, output)

    def jump_test(jumpFrom, jumpTo, expected, error=None, event='line'):
        """Decorator that creates a test that makes a jump
        from one place to another in the following code.
        """
        def decorator(func):
            @wraps(func)
            def test(self):
                self.run_test(func, jumpFrom, jumpTo, expected,
                              error=error, event=event, decorated=True)
            return test
        return decorator

    def async_jump_test(jumpFrom, jumpTo, expected, error=None, event='line'):
        """Decorator that creates a test that makes a jump
        from one place to another in the following asynchronous code.
        """
        def decorator(func):
            @wraps(func)
            def test(self):
                self.run_async_test(func, jumpFrom, jumpTo, expected,
                              error=error, event=event, decorated=True)
            return test
        return decorator

    ## The first set of 'jump' tests are for things that are allowed:

    @jump_test(1, 3, [3])
    def test_jump_simple_forwards(output):
        output.append(1)
        output.append(2)
        output.append(3)

    @jump_test(2, 1, [1, 1, 2])
    def test_jump_simple_backwards(output):
        output.append(1)
        output.append(2)

    @jump_test(3, 5, [2, 5])
    def test_jump_out_of_block_forwards(output):
        for i in 1, 2:
            output.append(2)
            for j in [3]:  # Also tests jumping over a block
                output.append(4)
        output.append(5)

    @jump_test(6, 1, [1, 3, 5, 1, 3, 5, 6, 7])
    def test_jump_out_of_block_backwards(output):
        output.append(1)
        for i in [1]:
            output.append(3)
            for j in [2]:  # Also tests jumping over a block
                output.append(5)
            output.append(6)
        output.append(7)

    @async_jump_test(4, 5, [3], (ValueError, 'into'))
    async def test_jump_out_of_async_for_block_forwards(output):
        for i in [1]:
            async for i in asynciter([1, 2]):
                output.append(3)
                output.append(4)
            output.append(5)

    @async_jump_test(5, 2, [2, 4, 2, 4, 5, 6])
    async def test_jump_out_of_async_for_block_backwards(output):
        for i in [1]:
            output.append(2)
            async for i in asynciter([1]):
                output.append(4)
                output.append(5)
            output.append(6)

    @jump_test(1, 2, [3])
    def test_jump_to_codeless_line(output):
        output.append(1)
        # Jumping to this line should skip to the next one.
        output.append(3)

    @jump_test(2, 2, [1, 2, 3])
    def test_jump_to_same_line(output):
        output.append(1)
        output.append(2)
        output.append(3)

    # Tests jumping within a finally block, and over one.
    @jump_test(4, 9, [2, 9])
    def test_jump_in_nested_finally(output):
        try:
            output.append(2)
        finally:
            output.append(4)
            try:
                output.append(6)
            finally:
                output.append(8)
            output.append(9)

    @jump_test(6, 7, [2], (ValueError, 'within'))
    def test_jump_in_nested_finally_2(output):
        try:
            output.append(2)
            1/0
            return
        finally:
            output.append(6)
            output.append(7)
        output.append(8)

    @jump_test(6, 11, [2], (ValueError, 'within'))
    def test_jump_in_nested_finally_3(output):
        try:
            output.append(2)
            1/0
            return
        finally:
            output.append(6)
            try:
                output.append(8)
            finally:
                output.append(10)
            output.append(11)
        output.append(12)

    @jump_test(5, 11, [2, 4], (ValueError, 'exception'))
    def test_no_jump_over_return_try_finally_in_finally_block(output):
        try:
            output.append(2)
        finally:
            output.append(4)
            output.append(5)
            return
            try:
                output.append(8)
            finally:
                output.append(10)
            pass
        output.append(12)

    @jump_test(3, 4, [1], (ValueError, 'after'))
    def test_no_jump_infinite_while_loop(output):
        output.append(1)
        while True:
            output.append(3)
        output.append(4)

    @jump_test(2, 4, [4, 4])
    def test_jump_forwards_into_while_block(output):
        i = 1
        output.append(2)
        while i <= 2:
            output.append(4)
            i += 1

    @jump_test(5, 3, [3, 3, 3, 5])
    def test_jump_backwards_into_while_block(output):
        i = 1
        while i <= 2:
            output.append(3)
            i += 1
        output.append(5)

    @jump_test(2, 3, [1, 3])
    def test_jump_forwards_out_of_with_block(output):
        with tracecontext(output, 1):
            output.append(2)
        output.append(3)

    @async_jump_test(2, 3, [1, 3])
    async def test_jump_forwards_out_of_async_with_block(output):
        async with asynctracecontext(output, 1):
            output.append(2)
        output.append(3)

    @jump_test(3, 1, [1, 2, 1, 2, 3, -2])
    def test_jump_backwards_out_of_with_block(output):
        output.append(1)
        with tracecontext(output, 2):
            output.append(3)

    @async_jump_test(3, 1, [1, 2, 1, 2, 3, -2])
    async def test_jump_backwards_out_of_async_with_block(output):
        output.append(1)
        async with asynctracecontext(output, 2):
            output.append(3)

    @jump_test(2, 5, [5])
    def test_jump_forwards_out_of_try_finally_block(output):
        try:
            output.append(2)
        finally:
            output.append(4)
        output.append(5)

    @jump_test(3, 1, [1, 1, 3, 5])
    def test_jump_backwards_out_of_try_finally_block(output):
        output.append(1)
        try:
            output.append(3)
        finally:
            output.append(5)

    @jump_test(2, 6, [6])
    def test_jump_forwards_out_of_try_except_block(output):
        try:
            output.append(2)
        except:
            output.append(4)
            raise
        output.append(6)

    @jump_test(3, 1, [1, 1, 3])
    def test_jump_backwards_out_of_try_except_block(output):
        output.append(1)
        try:
            output.append(3)
        except:
            output.append(5)
            raise

    @jump_test(5, 7, [4], (ValueError, 'within'))
    def test_no_jump_between_except_blocks(output):
        try:
            1/0
        except ZeroDivisionError:
            output.append(4)
            output.append(5)
        except FloatingPointError:
            output.append(7)
        output.append(8)

    @jump_test(5, 6, [4], (ValueError, 'within'))
    def test_no_jump_within_except_block(output):
        try:
            1/0
        except:
            output.append(4)
            output.append(5)
            output.append(6)
        output.append(7)

    @jump_test(2, 4, [1, 4, 5, -4])
    def test_jump_across_with(output):
        output.append(1)
        with tracecontext(output, 2):
            output.append(3)
        with tracecontext(output, 4):
            output.append(5)

    @async_jump_test(2, 4, [1, 4, 5, -4])
    async def test_jump_across_async_with(output):
        output.append(1)
        async with asynctracecontext(output, 2):
            output.append(3)
        async with asynctracecontext(output, 4):
            output.append(5)

    @jump_test(4, 5, [1, 3, 5, 6])
    def test_jump_out_of_with_block_within_for_block(output):
        output.append(1)
        for i in [1]:
            with tracecontext(output, 3):
                output.append(4)
            output.append(5)
        output.append(6)

    @async_jump_test(4, 5, [1, 3, 5, 6])
    async def test_jump_out_of_async_with_block_within_for_block(output):
        output.append(1)
        for i in [1]:
            async with asynctracecontext(output, 3):
                output.append(4)
            output.append(5)
        output.append(6)

    @jump_test(4, 5, [1, 2, 3, 5, -2, 6])
    def test_jump_out_of_with_block_within_with_block(output):
        output.append(1)
        with tracecontext(output, 2):
            with tracecontext(output, 3):
                output.append(4)
            output.append(5)
        output.append(6)

    @async_jump_test(4, 5, [1, 2, 3, 5, -2, 6])
    async def test_jump_out_of_async_with_block_within_with_block(output):
        output.append(1)
        with tracecontext(output, 2):
            async with asynctracecontext(output, 3):
                output.append(4)
            output.append(5)
        output.append(6)

    @jump_test(5, 6, [2, 4, 6, 7])
    def test_jump_out_of_with_block_within_finally_block(output):
        try:
            output.append(2)
        finally:
            with tracecontext(output, 4):
                output.append(5)
            output.append(6)
        output.append(7)

    @async_jump_test(5, 6, [2, 4, 6, 7])
    async def test_jump_out_of_async_with_block_within_finally_block(output):
        try:
            output.append(2)
        finally:
            async with asynctracecontext(output, 4):
                output.append(5)
            output.append(6)
        output.append(7)

    @jump_test(8, 11, [1, 3, 5, 11, 12])
    def test_jump_out_of_complex_nested_blocks(output):
        output.append(1)
        for i in [1]:
            output.append(3)
            for j in [1, 2]:
                output.append(5)
                try:
                    for k in [1, 2]:
                        output.append(8)
                finally:
                    output.append(10)
            output.append(11)
        output.append(12)

    @jump_test(3, 5, [1, 2, 5])
    def test_jump_out_of_with_assignment(output):
        output.append(1)
        with tracecontext(output, 2) \
                as x:
            output.append(4)
        output.append(5)

    @async_jump_test(3, 5, [1, 2, 5])
    async def test_jump_out_of_async_with_assignment(output):
        output.append(1)
        async with asynctracecontext(output, 2) \
                as x:
            output.append(4)
        output.append(5)

    @jump_test(3, 6, [1, 6, 8, 9])
    def test_jump_over_return_in_try_finally_block(output):
        output.append(1)
        try:
            output.append(3)
            if not output: # always false
                return
            output.append(6)
        finally:
            output.append(8)
        output.append(9)

    @jump_test(5, 8, [1, 3, 8, 10, 11, 13])
    def test_jump_over_break_in_try_finally_block(output):
        output.append(1)
        while True:
            output.append(3)
            try:
                output.append(5)
                if not output: # always false
                    break
                output.append(8)
            finally:
                output.append(10)
            output.append(11)
            break
        output.append(13)

    @jump_test(1, 7, [7, 8])
    def test_jump_over_for_block_before_else(output):
        output.append(1)
        if not output:  # always false
            for i in [3]:
                output.append(4)
        else:
            output.append(6)
            output.append(7)
        output.append(8)

    @async_jump_test(1, 7, [7, 8])
    async def test_jump_over_async_for_block_before_else(output):
        output.append(1)
        if not output:  # always false
            async for i in asynciter([3]):
                output.append(4)
        else:
            output.append(6)
            output.append(7)
        output.append(8)

    # The second set of 'jump' tests are for things that are not allowed:

    @jump_test(2, 3, [1], (ValueError, 'after'))
    def test_no_jump_too_far_forwards(output):
        output.append(1)
        output.append(2)

    @jump_test(2, -2, [1], (ValueError, 'before'))
    def test_no_jump_too_far_backwards(output):
        output.append(1)
        output.append(2)

    # Test each kind of 'except' line.
    @jump_test(2, 3, [4], (ValueError, 'except'))
    def test_no_jump_to_except_1(output):
        try:
            output.append(2)
        except:
            output.append(4)
            raise

    @jump_test(2, 3, [4], (ValueError, 'except'))
    def test_no_jump_to_except_2(output):
        try:
            output.append(2)
        except ValueError:
            output.append(4)
            raise

    @jump_test(2, 3, [4], (ValueError, 'except'))
    def test_no_jump_to_except_3(output):
        try:
            output.append(2)
        except ValueError as e:
            output.append(4)
            raise e

    @jump_test(2, 3, [4], (ValueError, 'except'))
    def test_no_jump_to_except_4(output):
        try:
            output.append(2)
        except (ValueError, RuntimeError) as e:
            output.append(4)
            raise e

    @jump_test(1, 3, [], (ValueError, 'into'))
    def test_no_jump_forwards_into_for_block(output):
        output.append(1)
        for i in 1, 2:
            output.append(3)

    @async_jump_test(1, 3, [], (ValueError, 'into'))
    async def test_no_jump_forwards_into_async_for_block(output):
        output.append(1)
        async for i in asynciter([1, 2]):
            output.append(3)
        pass

    @jump_test(3, 2, [2, 2], (ValueError, 'into'))
    def test_no_jump_backwards_into_for_block(output):
        for i in 1, 2:
            output.append(2)
        output.append(3)

    @async_jump_test(3, 2, [2, 2], (ValueError, 'within'))
    async def test_no_jump_backwards_into_async_for_block(output):
        async for i in asynciter([1, 2]):
            output.append(2)
        output.append(3)

    @jump_test(1, 3, [], (ValueError, 'depth'))
    def test_no_jump_forwards_into_with_block(output):
        output.append(1)
        with tracecontext(output, 2):
            output.append(3)

    @async_jump_test(1, 3, [], (ValueError, 'depth'))
    async def test_no_jump_forwards_into_async_with_block(output):
        output.append(1)
        async with asynctracecontext(output, 2):
            output.append(3)

    @jump_test(3, 2, [1, 2, -1], (ValueError, 'depth'))
    def test_no_jump_backwards_into_with_block(output):
        with tracecontext(output, 1):
            output.append(2)
        output.append(3)

    @async_jump_test(3, 2, [1, 2, -1], (ValueError, 'depth'))
    async def test_no_jump_backwards_into_async_with_block(output):
        async with asynctracecontext(output, 1):
            output.append(2)
        output.append(3)

    @jump_test(1, 3, [3, 5])
    def test_jump_forwards_into_try_finally_block(output):
        output.append(1)
        try:
            output.append(3)
        finally:
            output.append(5)

    @jump_test(5, 2, [2, 4, 2, 4, 5])
    def test_jump_backwards_into_try_finally_block(output):
        try:
            output.append(2)
        finally:
            output.append(4)
        output.append(5)

    @jump_test(1, 3, [3])
    def test_jump_forwards_into_try_except_block(output):
        output.append(1)
        try:
            output.append(3)
        except:
            output.append(5)
            raise

    @jump_test(6, 2, [2, 2, 6])
    def test_jump_backwards_into_try_except_block(output):
        try:
            output.append(2)
        except:
            output.append(4)
            raise
        output.append(6)

    # 'except' with a variable creates an implicit finally block
    @jump_test(5, 7, [4], (ValueError, 'within'))
    def test_no_jump_between_except_blocks_2(output):
        try:
            1/0
        except ZeroDivisionError:
            output.append(4)
            output.append(5)
        except FloatingPointError as e:
            output.append(7)
        output.append(8)

    @jump_test(1, 5, [5])
    def test_jump_into_finally_block(output):
        output.append(1)
        try:
            output.append(3)
        finally:
            output.append(5)

    @jump_test(3, 6, [2, 6, 7])
    def test_jump_into_finally_block_from_try_block(output):
        try:
            output.append(2)
            output.append(3)
        finally:  # still executed if the jump is failed
            output.append(5)
            output.append(6)
        output.append(7)

    @jump_test(5, 1, [1, 3, 1, 3, 5])
    def test_jump_out_of_finally_block(output):
        output.append(1)
        try:
            output.append(3)
        finally:
            output.append(5)

    @jump_test(1, 5, [], (ValueError, "into an exception"))
    def test_no_jump_into_bare_except_block(output):
        output.append(1)
        try:
            output.append(3)
        except:
            output.append(5)

    @jump_test(1, 5, [], (ValueError, "into an exception"))
    def test_no_jump_into_qualified_except_block(output):
        output.append(1)
        try:
            output.append(3)
        except Exception:
            output.append(5)

    @jump_test(3, 6, [2, 5, 6], (ValueError, "into an exception"))
    def test_no_jump_into_bare_except_block_from_try_block(output):
        try:
            output.append(2)
            output.append(3)
        except:  # executed if the jump is failed
            output.append(5)
            output.append(6)
            raise
        output.append(8)

    @jump_test(3, 6, [2], (ValueError, "into an exception"))
    def test_no_jump_into_qualified_except_block_from_try_block(output):
        try:
            output.append(2)
            output.append(3)
        except ZeroDivisionError:
            output.append(5)
            output.append(6)
            raise
        output.append(8)

    @jump_test(7, 1, [1, 3, 6], (ValueError, "within"))
    def test_no_jump_out_of_bare_except_block(output):
        output.append(1)
        try:
            output.append(3)
            1/0
        except:
            output.append(6)
            output.append(7)

    @jump_test(7, 1, [1, 3, 6], (ValueError, "within"))
    def test_no_jump_out_of_qualified_except_block(output):
        output.append(1)
        try:
            output.append(3)
            1/0
        except Exception:
            output.append(6)
            output.append(7)

    @jump_test(3, 5, [1, 2, 5, -2])
    def test_jump_between_with_blocks(output):
        output.append(1)
        with tracecontext(output, 2):
            output.append(3)
        with tracecontext(output, 4):
            output.append(5)

    @async_jump_test(3, 5, [1, 2, 5, -2])
    async def test_jump_between_async_with_blocks(output):
        output.append(1)
        async with asynctracecontext(output, 2):
            output.append(3)
        async with asynctracecontext(output, 4):
            output.append(5)

    @jump_test(5, 7, [2, 4], (ValueError, "after"))
    def test_no_jump_over_return_out_of_finally_block(output):
        try:
            output.append(2)
        finally:
            output.append(4)
            output.append(5)
            return
        output.append(7)

    @jump_test(7, 4, [1, 6], (ValueError, 'into'))
    def test_no_jump_into_for_block_before_else(output):
        output.append(1)
        if not output:  # always false
            for i in [3]:
                output.append(4)
        else:
            output.append(6)
            output.append(7)
        output.append(8)

    @async_jump_test(7, 4, [1, 6], (ValueError, 'into'))
    async def test_no_jump_into_async_for_block_before_else(output):
        output.append(1)
        if not output:  # always false
            async for i in asynciter([3]):
                output.append(4)
        else:
            output.append(6)
            output.append(7)
        output.append(8)

    def test_no_jump_to_non_integers(self):
        self.run_test(no_jump_to_non_integers, 2, "Spam", [True])

    def test_no_jump_without_trace_function(self):
        # Must set sys.settrace(None) in setUp(), else condition is not
        # triggered.
        no_jump_without_trace_function()

    def test_large_function(self):
        d = {}
        exec("""def f(output):        # line 0
            x = 0                     # line 1
            y = 1                     # line 2
            '''                       # line 3
            %s                        # lines 4-1004
            '''                       # line 1005
            x += 1                    # line 1006
            output.append(x)          # line 1007
            return""" % ('\n' * 1000,), d)
        f = d['f']
        self.run_test(f, 2, 1007, [0])

    def test_jump_to_firstlineno(self):
        # This tests that PDB can jump back to the first line in a
        # file.  See issue #1689458.  It can only be triggered in a
        # function call if the function is defined on a single line.
        code = compile("""
# Comments don't count.
output.append(2)  # firstlineno is here.
output.append(3)
output.append(4)
""", "<fake module>", "exec")
        class fake_function:
            __code__ = code
        tracer = JumpTracer(fake_function, 4, 1)
        sys.settrace(tracer.trace)
        namespace = {"output": []}
        exec(code, namespace)
        sys.settrace(None)
        self.compare_jump_output([2, 3, 2, 3, 4], namespace["output"])

    @jump_test(2, 3, [1], event='call', error=(ValueError, "can't jump from"
               " the 'call' trace event of a new frame"))
    def test_no_jump_from_call(output):
        output.append(1)
        def nested():
            output.append(3)
        nested()
        output.append(5)

    @jump_test(2, 1, [1], event='return', error=(ValueError,
               "can only jump from a 'line' trace event"))
    def test_no_jump_from_return_event(output):
        output.append(1)
        return

    @jump_test(2, 1, [1], event='exception', error=(ValueError,
               "can only jump from a 'line' trace event"))
    def test_no_jump_from_exception_event(output):
        output.append(1)
        1 / 0

    @jump_test(3, 2, [2, 5], event='return')
    def test_jump_from_yield(output):
        def gen():
            output.append(2)
            yield 3
        next(gen())
        output.append(5)


class TestExtendedArgs(unittest.TestCase):

    def setUp(self):
        self.addCleanup(sys.settrace, sys.gettrace())
        sys.settrace(None)

    def count_traces(self, func):
        # warmup
        for _ in range(20):
            func()

        counts = {"call": 0, "line": 0, "return": 0}
        def trace(frame, event, arg):
            counts[event] += 1
            return trace

        sys.settrace(trace)
        func()
        sys.settrace(None)

        return counts

    def test_trace_unpack_long_sequence(self):
        ns = {}
        code = "def f():\n  (" + "y,\n   "*300 + ") = range(300)"
        exec(code, ns)
        counts = self.count_traces(ns["f"])
        self.assertEqual(counts, {'call': 1, 'line': 301, 'return': 1})

    def test_trace_lots_of_globals(self):
        code = """if 1:
            def f():
                return (
                    {}
                )
        """.format("\n+\n".join(f"var{i}\n" for i in range(1000)))
        ns = {f"var{i}": i for i in range(1000)}
        exec(code, ns)
        counts = self.count_traces(ns["f"])
        self.assertEqual(counts, {'call': 1, 'line': 2000, 'return': 1})


if __name__ == "__main__":
    unittest.main()
