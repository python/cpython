""" Test the bdb module.

    A test defines a list of tuples that may be seen as paired tuples, each
    pair being defined by 'expect_tuple, set_tuple' as follows:

        ([event, [lineno[, co_name[, eargs]]]]), (set_type, [sargs])

    * 'expect_tuple' describes the expected current state of the Bdb instance.
      It may be the empty tuple and no check is done in that case.
    * 'set_tuple' defines the set_*() method to be invoked when the Bdb
      instance reaches this state.

    Example of an 'expect_tuple, set_tuple' pair:

        ('line', 2, 'tfunc_main'), ('step', )

    Definitions of the members of the 'expect_tuple':
        event:
            Name of the trace event. The set methods that do not give back
            control to the tracer [1] do not trigger a tracer event and in
            that case the next 'event' may be 'None' by convention, its value
            is not checked.
            [1] Methods that trigger a trace event are set_step(), set_next(),
            set_return(), set_until() and set_continue().
        lineno:
            Line number. Line numbers are relative to the start of the
            function when tracing a function in the test_bdb module (i.e. this
            module).
        co_name:
            Name of the function being currently traced.
        eargs:
            A tuple:
            * On an 'exception' event the tuple holds a class object, the
              current exception must be an instance of this class.
            * On a 'line' event, the tuple holds a dictionary and a list. The
              dictionary maps each breakpoint number that has been hit on this
              line to its hits count. The list holds the list of breakpoint
              number temporaries that are being deleted.

    Definitions of the members of the 'set_tuple':
        set_type:
            The type of the set method to be invoked. This may
            be the type of one of the Bdb set methods: 'step', 'next',
            'until', 'return', 'continue', 'break', 'quit', or the type of one
            of the set methods added by test_bdb.Bdb: 'ignore', 'enable',
            'disable', 'clear', 'up', 'down'.
        sargs:
            The arguments of the set method if any, packed in a tuple.
"""

import bdb as _bdb
import sys
import os
import unittest
import textwrap
import importlib
import linecache
from contextlib import contextmanager
from itertools import islice, repeat
import test.support
from test.support import import_helper
from test.support import os_helper


class BdbException(Exception): pass
class BdbError(BdbException): """Error raised by the Bdb instance."""
class BdbSyntaxError(BdbException): """Syntax error in the test case."""
class BdbNotExpectedError(BdbException): """Unexpected result."""

# When 'dry_run' is set to true, expect tuples are ignored and the actual
# state of the tracer is printed after running each set_*() method of the test
# case. The full list of breakpoints and their attributes is also printed
# after each 'line' event where a breakpoint has been hit.
dry_run = 0

def reset_Breakpoint():
    _bdb.Breakpoint.next = 1
    _bdb.Breakpoint.bplist = {}
    _bdb.Breakpoint.bpbynumber = [None]

def info_breakpoints():
    bp_list = [bp for  bp in _bdb.Breakpoint.bpbynumber if bp]
    if not bp_list:
        return ''

    header_added = False
    for bp in bp_list:
        if not header_added:
            info = 'BpNum Temp Enb Hits Ignore Where\n'
            header_added = True

        disp = 'yes ' if bp.temporary else 'no  '
        enab = 'yes' if bp.enabled else 'no '
        info += ('%-5d %s %s %-4d %-6d at %s:%d' %
                    (bp.number, disp, enab, bp.hits, bp.ignore,
                     os.path.basename(bp.file), bp.line))
        if bp.cond:
            info += '\n\tstop only if %s' % (bp.cond,)
        info += '\n'
    return info

class Bdb(_bdb.Bdb):
    """Extend Bdb to enhance test coverage."""

    def trace_dispatch(self, frame, event, arg):
        self.currentbp = None
        return super().trace_dispatch(frame, event, arg)

    def set_break(self, filename, lineno, temporary=False, cond=None,
                  funcname=None):
        if isinstance(funcname, str):
            if filename == __file__:
                globals_ = globals()
            else:
                module = importlib.import_module(filename[:-3])
                globals_ = module.__dict__
            func = eval(funcname, globals_)
            code = func.__code__
            filename = code.co_filename
            lineno = code.co_firstlineno
            funcname = code.co_name

        res = super().set_break(filename, lineno, temporary=temporary,
                                 cond=cond, funcname=funcname)
        if isinstance(res, str):
            raise BdbError(res)
        return res

    def get_stack(self, f, t):
        self.stack, self.index = super().get_stack(f, t)
        self.frame = self.stack[self.index][0]
        return self.stack, self.index

    def set_ignore(self, bpnum):
        """Increment the ignore count of Breakpoint number 'bpnum'."""
        bp = self.get_bpbynumber(bpnum)
        bp.ignore += 1

    def set_enable(self, bpnum):
        bp = self.get_bpbynumber(bpnum)
        bp.enabled = True

    def set_disable(self, bpnum):
        bp = self.get_bpbynumber(bpnum)
        bp.enabled = False

    def set_clear(self, fname, lineno):
        err = self.clear_break(fname, lineno)
        if err:
            raise BdbError(err)

    def set_up(self):
        """Move up in the frame stack."""
        if not self.index:
            raise BdbError('Oldest frame')
        self.index -= 1
        self.frame = self.stack[self.index][0]

    def set_down(self):
        """Move down in the frame stack."""
        if self.index + 1 == len(self.stack):
            raise BdbError('Newest frame')
        self.index += 1
        self.frame = self.stack[self.index][0]

class Tracer(Bdb):
    """A tracer for testing the bdb module."""

    def __init__(self, expect_set, skip=None, dry_run=False, test_case=None):
        super().__init__(skip=skip)
        self.expect_set = expect_set
        self.dry_run = dry_run
        self.header = ('Dry-run results for %s:' % test_case if
                       test_case is not None else None)
        self.init_test()

    def init_test(self):
        self.cur_except = None
        self.expect_set_no = 0
        self.breakpoint_hits = None
        self.expected_list = list(islice(self.expect_set, 0, None, 2))
        self.set_list = list(islice(self.expect_set, 1, None, 2))

    def trace_dispatch(self, frame, event, arg):
        # On an 'exception' event, call_exc_trace() in Python/ceval.c discards
        # a BdbException raised by the Tracer instance, so we raise it on the
        # next trace_dispatch() call that occurs unless the set_quit() or
        # set_continue() method has been invoked on the 'exception' event.
        if self.cur_except is not None:
            raise self.cur_except

        if event == 'exception':
            try:
                res = super().trace_dispatch(frame, event, arg)
                return res
            except BdbException as e:
                self.cur_except = e
                return self.trace_dispatch
        else:
            return super().trace_dispatch(frame, event, arg)

    def user_call(self, frame, argument_list):
        # Adopt the same behavior as pdb and, as a side effect, skip also the
        # first 'call' event when the Tracer is started with Tracer.runcall()
        # which may be possibly considered as a bug.
        if not self.stop_here(frame):
            return
        self.process_event('call', frame, argument_list)
        self.next_set_method()

    def user_line(self, frame):
        self.process_event('line', frame)

        if self.dry_run and self.breakpoint_hits:
            info = info_breakpoints().strip('\n')
            # Indent each line.
            for line in info.split('\n'):
                print('  ' + line)
        self.delete_temporaries()
        self.breakpoint_hits = None

        self.next_set_method()

    def user_return(self, frame, return_value):
        self.process_event('return', frame, return_value)
        self.next_set_method()

    def user_exception(self, frame, exc_info):
        self.exc_info = exc_info
        self.process_event('exception', frame)
        self.next_set_method()

    def do_clear(self, arg):
        # The temporary breakpoints are deleted in user_line().
        bp_list = [self.currentbp]
        self.breakpoint_hits = (bp_list, bp_list)

    def delete_temporaries(self):
        if self.breakpoint_hits:
            for n in self.breakpoint_hits[1]:
                self.clear_bpbynumber(n)

    def pop_next(self):
        self.expect_set_no += 1
        try:
            self.expect = self.expected_list.pop(0)
        except IndexError:
            raise BdbNotExpectedError(
                'expect_set list exhausted, cannot pop item %d' %
                self.expect_set_no)
        self.set_tuple = self.set_list.pop(0)

    def process_event(self, event, frame, *args):
        # Call get_stack() to enable walking the stack with set_up() and
        # set_down().
        tb = None
        if event == 'exception':
            tb = self.exc_info[2]
        self.get_stack(frame, tb)

        # A breakpoint has been hit and it is not a temporary.
        if self.currentbp is not None and not self.breakpoint_hits:
            bp_list = [self.currentbp]
            self.breakpoint_hits = (bp_list, [])

        # Pop next event.
        self.event= event
        self.pop_next()
        if self.dry_run:
            self.print_state(self.header)
            return

        # Validate the expected results.
        if self.expect:
            self.check_equal(self.expect[0], event, 'Wrong event type')
            self.check_lno_name()

        if event in ('call', 'return'):
            self.check_expect_max_size(3)
        elif len(self.expect) > 3:
            if event == 'line':
                bps, temporaries = self.expect[3]
                bpnums = sorted(bps.keys())
                if not self.breakpoint_hits:
                    self.raise_not_expected(
                        'No breakpoints hit at expect_set item %d' %
                        self.expect_set_no)
                self.check_equal(bpnums, self.breakpoint_hits[0],
                    'Breakpoint numbers do not match')
                self.check_equal([bps[n] for n in bpnums],
                    [self.get_bpbynumber(n).hits for
                        n in self.breakpoint_hits[0]],
                    'Wrong breakpoint hit count')
                self.check_equal(sorted(temporaries), self.breakpoint_hits[1],
                    'Wrong temporary breakpoints')

            elif event == 'exception':
                if not isinstance(self.exc_info[1], self.expect[3]):
                    self.raise_not_expected(
                        "Wrong exception at expect_set item %d, got '%s'" %
                        (self.expect_set_no, self.exc_info))

    def check_equal(self, expected, result, msg):
        if expected == result:
            return
        self.raise_not_expected("%s at expect_set item %d, got '%s'" %
                                (msg, self.expect_set_no, result))

    def check_lno_name(self):
        """Check the line number and function co_name."""
        s = len(self.expect)
        if s > 1:
            lineno = self.lno_abs2rel()
            self.check_equal(self.expect[1], lineno, 'Wrong line number')
        if s > 2:
            self.check_equal(self.expect[2], self.frame.f_code.co_name,
                                                'Wrong function name')

    def check_expect_max_size(self, size):
        if len(self.expect) > size:
            raise BdbSyntaxError('Invalid size of the %s expect tuple: %s' %
                                 (self.event, self.expect))

    def lno_abs2rel(self):
        fname = self.canonic(self.frame.f_code.co_filename)
        lineno = self.frame.f_lineno
        return ((lineno - self.frame.f_code.co_firstlineno + 1)
            if fname == self.canonic(__file__) else lineno)

    def lno_rel2abs(self, fname, lineno):
        return (self.frame.f_code.co_firstlineno + lineno - 1
            if (lineno and self.canonic(fname) == self.canonic(__file__))
            else lineno)

    def get_state(self):
        lineno = self.lno_abs2rel()
        co_name = self.frame.f_code.co_name
        state = "('%s', %d, '%s'" % (self.event, lineno, co_name)
        if self.breakpoint_hits:
            bps = '{'
            for n in self.breakpoint_hits[0]:
                if bps != '{':
                    bps += ', '
                bps += '%s: %s' % (n, self.get_bpbynumber(n).hits)
            bps += '}'
            bps = '(' + bps + ', ' + str(self.breakpoint_hits[1]) + ')'
            state += ', ' + bps
        elif self.event == 'exception':
            state += ', ' + self.exc_info[0].__name__
        state += '), '
        return state.ljust(32) + str(self.set_tuple) + ','

    def print_state(self, header=None):
        if header is not None and self.expect_set_no == 1:
            print()
            print(header)
        print('%d: %s' % (self.expect_set_no, self.get_state()))

    def raise_not_expected(self, msg):
        msg += '\n'
        msg += '  Expected: %s\n' % str(self.expect)
        msg += '  Got:      ' + self.get_state()
        raise BdbNotExpectedError(msg)

    def next_set_method(self):
        set_type = self.set_tuple[0]
        args = self.set_tuple[1] if len(self.set_tuple) == 2 else None
        set_method = getattr(self, 'set_' + set_type)

        # The following set methods give back control to the tracer.
        if set_type in ('step', 'continue', 'quit'):
            set_method()
            return
        elif set_type in ('next', 'return'):
            set_method(self.frame)
            return
        elif set_type == 'until':
            lineno = None
            if args:
                lineno = self.lno_rel2abs(self.frame.f_code.co_filename,
                                          args[0])
            set_method(self.frame, lineno)
            return

        # The following set methods do not give back control to the tracer and
        # next_set_method() is called recursively.
        if (args and set_type in ('break', 'clear', 'ignore', 'enable',
                                    'disable')) or set_type in ('up', 'down'):
            if set_type in ('break', 'clear'):
                fname, lineno, *remain = args
                lineno = self.lno_rel2abs(fname, lineno)
                args = [fname, lineno]
                args.extend(remain)
                set_method(*args)
            elif set_type in ('ignore', 'enable', 'disable'):
                set_method(*args)
            elif set_type in ('up', 'down'):
                set_method()

            # Process the next expect_set item.
            # It is not expected that a test may reach the recursion limit.
            self.event= None
            self.pop_next()
            if self.dry_run:
                self.print_state()
            else:
                if self.expect:
                    self.check_lno_name()
                self.check_expect_max_size(3)
            self.next_set_method()
        else:
            raise BdbSyntaxError('"%s" is an invalid set_tuple' %
                                 self.set_tuple)

class TracerRun():
    """Provide a context for running a Tracer instance with a test case."""

    def __init__(self, test_case, skip=None):
        self.test_case = test_case
        self.dry_run = test_case.dry_run
        self.tracer = Tracer(test_case.expect_set, skip=skip,
                             dry_run=self.dry_run, test_case=test_case.id())
        self._original_tracer = None

    def __enter__(self):
        # test_pdb does not reset Breakpoint class attributes on exit :-(
        reset_Breakpoint()
        self._original_tracer = sys.gettrace()
        return self.tracer

    def __exit__(self, type_=None, value=None, traceback=None):
        reset_Breakpoint()
        sys.settrace(self._original_tracer)

        not_empty = ''
        if self.tracer.set_list:
            not_empty += 'All paired tuples have not been processed, '
            not_empty += ('the last one was number %d' %
                          self.tracer.expect_set_no)

        # Make a BdbNotExpectedError a unittest failure.
        if type_ is not None and issubclass(BdbNotExpectedError, type_):
            if isinstance(value, BaseException) and value.args:
                err_msg = value.args[0]
                if not_empty:
                    err_msg += '\n' + not_empty
                if self.dry_run:
                    print(err_msg)
                    return True
                else:
                    self.test_case.fail(err_msg)
            else:
                assert False, 'BdbNotExpectedError with empty args'

        if not_empty:
            if self.dry_run:
                print(not_empty)
            else:
                self.test_case.fail(not_empty)

def run_test(modules, set_list, skip=None):
    """Run a test and print the dry-run results.

    'modules':  A dictionary mapping module names to their source code as a
                string. The dictionary MUST include one module named
                'test_module' with a main() function.
    'set_list': A list of set_type tuples to be run on the module.

    For example, running the following script outputs the following results:

    *****************************   SCRIPT   ********************************

    from test.test_bdb import run_test, break_in_func

    code = '''
        def func():
            lno = 3

        def main():
            func()
            lno = 7
    '''

    set_list = [
                break_in_func('func', 'test_module.py'),
                ('continue', ),
                ('step', ),
                ('step', ),
                ('step', ),
                ('quit', ),
            ]

    modules = { 'test_module': code }
    run_test(modules, set_list)

    ****************************   results   ********************************

    1: ('line', 2, 'tfunc_import'),    ('next',),
    2: ('line', 3, 'tfunc_import'),    ('step',),
    3: ('call', 5, 'main'),            ('break', ('test_module.py', None, False, None, 'func')),
    4: ('None', 5, 'main'),            ('continue',),
    5: ('line', 3, 'func', ({1: 1}, [])), ('step',),
      BpNum Temp Enb Hits Ignore Where
      1     no   yes 1    0      at test_module.py:2
    6: ('return', 3, 'func'),          ('step',),
    7: ('line', 7, 'main'),            ('step',),
    8: ('return', 7, 'main'),          ('quit',),

    *************************************************************************

    """
    def gen(a, b):
        try:
            while 1:
                x = next(a)
                y = next(b)
                yield x
                yield y
        except StopIteration:
            return

    # Step over the import statement in tfunc_import using 'next' and step
    # into main() in test_module.
    sl = [('next', ), ('step', )]
    sl.extend(set_list)

    test = BaseTestCase()
    test.dry_run = True
    test.id = lambda : None
    test.expect_set = list(gen(repeat(()), iter(sl)))
    with create_modules(modules):
        with TracerRun(test, skip=skip) as tracer:
            tracer.runcall(tfunc_import)

@contextmanager
def create_modules(modules):
    with os_helper.temp_cwd():
        sys.path.append(os.getcwd())
        try:
            for m in modules:
                fname = m + '.py'
                with open(fname, 'w', encoding="utf-8") as f:
                    f.write(textwrap.dedent(modules[m]))
                linecache.checkcache(fname)
            importlib.invalidate_caches()
            yield
        finally:
            for m in modules:
                import_helper.forget(m)
            sys.path.pop()

def break_in_func(funcname, fname=__file__, temporary=False, cond=None):
    return 'break', (fname, None, temporary, cond, funcname)

TEST_MODULE = 'test_module_for_bdb'
TEST_MODULE_FNAME = TEST_MODULE + '.py'
def tfunc_import():
    import test_module_for_bdb
    test_module_for_bdb.main()

def tfunc_main():
    lno = 2
    tfunc_first()
    tfunc_second()
    lno = 5
    lno = 6
    lno = 7

def tfunc_first():
    lno = 2
    lno = 3
    lno = 4

def tfunc_second():
    lno = 2

class BaseTestCase(unittest.TestCase):
    """Base class for all tests."""

    dry_run = dry_run

    def fail(self, msg=None):
        # Override fail() to use 'raise from None' to avoid repetition of the
        # error message and traceback.
        raise self.failureException(msg) from None

class StateTestCase(BaseTestCase):
    """Test the step, next, return, until and quit 'set_' methods."""

    def test_step(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('step', ),
            ('line', 2, 'tfunc_first'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_step_next_on_last_statement(self):
        for set_type in ('step', 'next'):
            with self.subTest(set_type=set_type):
                self.expect_set = [
                    ('line', 2, 'tfunc_main'),               ('step', ),
                    ('line', 3, 'tfunc_main'),               ('step', ),
                    ('call', 1, 'tfunc_first'),              ('break', (__file__, 3)),
                    ('None', 1, 'tfunc_first'),              ('continue', ),
                    ('line', 3, 'tfunc_first', ({1:1}, [])), (set_type, ),
                    ('line', 4, 'tfunc_first'),              ('quit', ),
                ]
                with TracerRun(self) as tracer:
                    tracer.runcall(tfunc_main)

    def test_next(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),   ('step', ),
            ('line', 3, 'tfunc_main'),   ('next', ),
            ('line', 4, 'tfunc_main'),   ('step', ),
            ('call', 1, 'tfunc_second'), ('step', ),
            ('line', 2, 'tfunc_second'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_next_over_import(self):
        code = """
            def main():
                lno = 3
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'), ('next', ),
                ('line', 3, 'tfunc_import'), ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_next_on_plain_statement(self):
        # Check that set_next() is equivalent to set_step() on a plain
        # statement.
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('next', ),
            ('line', 2, 'tfunc_first'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_next_in_caller_frame(self):
        # Check that set_next() in the caller frame causes the tracer
        # to stop next in the caller frame.
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('up', ),
            ('None', 3, 'tfunc_main'),  ('next', ),
            ('line', 4, 'tfunc_main'),  ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_return(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),    ('step', ),
            ('line', 3, 'tfunc_main'),    ('step', ),
            ('call', 1, 'tfunc_first'),   ('step', ),
            ('line', 2, 'tfunc_first'),   ('return', ),
            ('return', 4, 'tfunc_first'), ('step', ),
            ('line', 4, 'tfunc_main'),    ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_return_in_caller_frame(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),   ('step', ),
            ('line', 3, 'tfunc_main'),   ('step', ),
            ('call', 1, 'tfunc_first'),  ('up', ),
            ('None', 3, 'tfunc_main'),   ('return', ),
            ('return', 7, 'tfunc_main'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_until(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('step', ),
            ('line', 2, 'tfunc_first'), ('until', (4, )),
            ('line', 4, 'tfunc_first'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_until_with_too_large_count(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),               break_in_func('tfunc_first'),
            ('None', 2, 'tfunc_main'),               ('continue', ),
            ('line', 2, 'tfunc_first', ({1:1}, [])), ('until', (9999, )),
            ('return', 4, 'tfunc_first'),            ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_until_in_caller_frame(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('up', ),
            ('None', 3, 'tfunc_main'),  ('until', (6, )),
            ('line', 6, 'tfunc_main'),  ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

    def test_skip(self):
        # Check that tracing is skipped over the import statement in
        # 'tfunc_import()'.
        code = """
            def main():
                lno = 3
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'), ('step', ),
                ('line', 3, 'tfunc_import'), ('quit', ),
            ]
            skip = ('importlib*', 'zipimport', 'encodings.*', TEST_MODULE)
            with TracerRun(self, skip=skip) as tracer:
                tracer.runcall(tfunc_import)

    def test_skip_with_no_name_module(self):
        # some frames have `globals` with no `__name__`
        # for instance the second frame in this traceback
        # exec(compile('raise ValueError()', '', 'exec'), {})
        bdb = Bdb(skip=['anything*'])
        self.assertIs(bdb.is_skipped_module(None), False)

    def test_down(self):
        # Check that set_down() raises BdbError at the newest frame.
        self.expect_set = [
            ('line', 2, 'tfunc_main'), ('down', ),
        ]
        with TracerRun(self) as tracer:
            self.assertRaises(BdbError, tracer.runcall, tfunc_main)

    def test_up(self):
        self.expect_set = [
            ('line', 2, 'tfunc_main'),  ('step', ),
            ('line', 3, 'tfunc_main'),  ('step', ),
            ('call', 1, 'tfunc_first'), ('up', ),
            ('None', 3, 'tfunc_main'),  ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.runcall(tfunc_main)

class BreakpointTestCase(BaseTestCase):
    """Test the breakpoint set method."""

    def test_bp_on_non_existent_module(self):
        self.expect_set = [
            ('line', 2, 'tfunc_import'), ('break', ('/non/existent/module.py', 1))
        ]
        with TracerRun(self) as tracer:
            self.assertRaises(BdbError, tracer.runcall, tfunc_import)

    def test_bp_after_last_statement(self):
        code = """
            def main():
                lno = 3
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'), ('break', (TEST_MODULE_FNAME, 4))
            ]
            with TracerRun(self) as tracer:
                self.assertRaises(BdbError, tracer.runcall, tfunc_import)

    def test_temporary_bp(self):
        code = """
            def func():
                lno = 3

            def main():
                for i in range(2):
                    func()
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME, True),
                ('None', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME, True),
                ('None', 2, 'tfunc_import'),       ('continue', ),
                ('line', 3, 'func', ({1:1}, [1])), ('continue', ),
                ('line', 3, 'func', ({2:1}, [2])), ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_disabled_temporary_bp(self):
        code = """
            def func():
                lno = 3

            def main():
                for i in range(3):
                    func()
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME, True),
                ('None', 2, 'tfunc_import'),       ('disable', (2, )),
                ('None', 2, 'tfunc_import'),       ('continue', ),
                ('line', 3, 'func', ({1:1}, [])),  ('enable', (2, )),
                ('None', 3, 'func'),               ('disable', (1, )),
                ('None', 3, 'func'),               ('continue', ),
                ('line', 3, 'func', ({2:1}, [2])), ('enable', (1, )),
                ('None', 3, 'func'),               ('continue', ),
                ('line', 3, 'func', ({1:2}, [])),  ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_bp_condition(self):
        code = """
            def func(a):
                lno = 3

            def main():
                for i in range(3):
                    func(i)
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME, False, 'a == 2'),
                ('None', 2, 'tfunc_import'),       ('continue', ),
                ('line', 3, 'func', ({1:3}, [])),  ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_bp_exception_on_condition_evaluation(self):
        code = """
            def func(a):
                lno = 3

            def main():
                func(0)
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME, False, '1 / 0'),
                ('None', 2, 'tfunc_import'),       ('continue', ),
                ('line', 3, 'func', ({1:1}, [])),  ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_bp_ignore_count(self):
        code = """
            def func():
                lno = 3

            def main():
                for i in range(2):
                    func()
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),      ('ignore', (1, )),
                ('None', 2, 'tfunc_import'),      ('continue', ),
                ('line', 3, 'func', ({1:2}, [])), ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_ignore_count_on_disabled_bp(self):
        code = """
            def func():
                lno = 3

            def main():
                for i in range(3):
                    func()
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),
                    break_in_func('func', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),      ('ignore', (1, )),
                ('None', 2, 'tfunc_import'),      ('disable', (1, )),
                ('None', 2, 'tfunc_import'),      ('continue', ),
                ('line', 3, 'func', ({2:1}, [])), ('enable', (1, )),
                ('None', 3, 'func'),              ('continue', ),
                ('line', 3, 'func', ({2:2}, [])), ('continue', ),
                ('line', 3, 'func', ({1:2}, [])), ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_clear_two_bp_on_same_line(self):
        code = """
            def func():
                lno = 3
                lno = 4

            def main():
                for i in range(3):
                    func()
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),      ('break', (TEST_MODULE_FNAME, 3)),
                ('None', 2, 'tfunc_import'),      ('break', (TEST_MODULE_FNAME, 3)),
                ('None', 2, 'tfunc_import'),      ('break', (TEST_MODULE_FNAME, 4)),
                ('None', 2, 'tfunc_import'),      ('continue', ),
                ('line', 3, 'func', ({1:1}, [])), ('continue', ),
                ('line', 4, 'func', ({3:1}, [])), ('clear', (TEST_MODULE_FNAME, 3)),
                ('None', 4, 'func'),              ('continue', ),
                ('line', 4, 'func', ({3:2}, [])), ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_clear_at_no_bp(self):
        self.expect_set = [
            ('line', 2, 'tfunc_import'), ('clear', (__file__, 1))
        ]
        with TracerRun(self) as tracer:
            self.assertRaises(BdbError, tracer.runcall, tfunc_import)

class RunTestCase(BaseTestCase):
    """Test run, runeval and set_trace."""

    def test_run_step(self):
        # Check that the bdb 'run' method stops at the first line event.
        code = """
            lno = 2
        """
        self.expect_set = [
            ('line', 2, '<module>'),   ('step', ),
            ('return', 2, '<module>'), ('quit', ),
        ]
        with TracerRun(self) as tracer:
            tracer.run(compile(textwrap.dedent(code), '<string>', 'exec'))

    def test_runeval_step(self):
        # Test bdb 'runeval'.
        code = """
            def main():
                lno = 3
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 1, '<module>'),   ('step', ),
                ('call', 2, 'main'),       ('step', ),
                ('line', 3, 'main'),       ('step', ),
                ('return', 3, 'main'),     ('step', ),
                ('return', 1, '<module>'), ('quit', ),
            ]
            import test_module_for_bdb
            with TracerRun(self) as tracer:
                tracer.runeval('test_module_for_bdb.main()', globals(), locals())

class IssuesTestCase(BaseTestCase):
    """Test fixed bdb issues."""

    def test_step_at_return_with_no_trace_in_caller(self):
        # Issue #13183.
        # Check that the tracer does step into the caller frame when the
        # trace function is not set in that frame.
        code_1 = """
            from test_module_for_bdb_2 import func
            def main():
                func()
                lno = 5
        """
        code_2 = """
            def func():
                lno = 3
        """
        modules = {
            TEST_MODULE: code_1,
            'test_module_for_bdb_2': code_2,
        }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('func', 'test_module_for_bdb_2.py'),
                ('None', 2, 'tfunc_import'),      ('continue', ),
                ('line', 3, 'func', ({1:1}, [])), ('step', ),
                ('return', 3, 'func'),            ('step', ),
                ('line', 5, 'main'),              ('quit', ),
            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_next_until_return_in_generator(self):
        # Issue #16596.
        # Check that set_next(), set_until() and set_return() do not treat the
        # `yield` and `yield from` statements as if they were returns and stop
        # instead in the current frame.
        code = """
            def test_gen():
                yield 0
                lno = 4
                return 123

            def main():
                it = test_gen()
                next(it)
                next(it)
                lno = 11
        """
        modules = { TEST_MODULE: code }
        for set_type in ('next', 'until', 'return'):
            with self.subTest(set_type=set_type):
                with create_modules(modules):
                    self.expect_set = [
                        ('line', 2, 'tfunc_import'),
                            break_in_func('test_gen', TEST_MODULE_FNAME),
                        ('None', 2, 'tfunc_import'),          ('continue', ),
                        ('line', 3, 'test_gen', ({1:1}, [])), (set_type, ),
                    ]

                    if set_type == 'return':
                        self.expect_set.extend(
                            [('exception', 10, 'main', StopIteration), ('step',),
                             ('return', 10, 'main'),                   ('quit', ),
                            ]
                        )
                    else:
                        self.expect_set.extend(
                            [('line', 4, 'test_gen'), ('quit', ),]
                        )
                    with TracerRun(self) as tracer:
                        tracer.runcall(tfunc_import)

    def test_next_command_in_generator_for_loop(self):
        # Issue #16596.
        code = """
            def test_gen():
                yield 0
                lno = 4
                yield 1
                return 123

            def main():
                for i in test_gen():
                    lno = 10
                lno = 11
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('test_gen', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),             ('continue', ),
                ('line', 3, 'test_gen', ({1:1}, [])),    ('next', ),
                ('line', 4, 'test_gen'),                 ('next', ),
                ('line', 5, 'test_gen'),                 ('next', ),
                ('line', 6, 'test_gen'),                 ('next', ),
                ('exception', 9, 'main', StopIteration), ('step', ),
                ('line', 11, 'main'),                    ('quit', ),

            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_next_command_in_generator_with_subiterator(self):
        # Issue #16596.
        code = """
            def test_subgen():
                yield 0
                return 123

            def test_gen():
                x = yield from test_subgen()
                return 456

            def main():
                for i in test_gen():
                    lno = 12
                lno = 13
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('test_gen', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),              ('continue', ),
                ('line', 7, 'test_gen', ({1:1}, [])),     ('next', ),
                ('line', 8, 'test_gen'),                  ('next', ),
                ('exception', 11, 'main', StopIteration), ('step', ),
                ('line', 13, 'main'),                     ('quit', ),

            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

    def test_return_command_in_generator_with_subiterator(self):
        # Issue #16596.
        code = """
            def test_subgen():
                yield 0
                return 123

            def test_gen():
                x = yield from test_subgen()
                return 456

            def main():
                for i in test_gen():
                    lno = 12
                lno = 13
        """
        modules = { TEST_MODULE: code }
        with create_modules(modules):
            self.expect_set = [
                ('line', 2, 'tfunc_import'),
                    break_in_func('test_subgen', TEST_MODULE_FNAME),
                ('None', 2, 'tfunc_import'),                  ('continue', ),
                ('line', 3, 'test_subgen', ({1:1}, [])),      ('return', ),
                ('exception', 7, 'test_gen', StopIteration),  ('return', ),
                ('exception', 11, 'main', StopIteration),     ('step', ),
                ('line', 13, 'main'),                         ('quit', ),

            ]
            with TracerRun(self) as tracer:
                tracer.runcall(tfunc_import)

def test_main():
    test.support.run_unittest(
        StateTestCase,
        RunTestCase,
        BreakpointTestCase,
        IssuesTestCase,
    )

if __name__ == "__main__":
    test_main()
