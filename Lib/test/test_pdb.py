# A test suite for pdb; not very comprehensive at the moment.

import _colorize
import doctest
import gc
import io
import os
import pdb
import sys
import types
import codecs
import unittest
import subprocess
import textwrap
import linecache
import zipapp
import zipfile

from asyncio.events import _set_event_loop_policy
from contextlib import ExitStack, redirect_stdout
from io import StringIO
from test import support
from test.support import has_socket_support, os_helper
from test.support.import_helper import import_module
from test.support.pty_helper import run_pty, FakeInput
from test.support.script_helper import kill_python
from unittest.mock import patch

SKIP_CORO_TESTS = False


class PdbTestInput(object):
    """Context manager that makes testing Pdb in doctests easier."""

    def __init__(self, input):
        self.input = input

    def __enter__(self):
        self.real_stdin = sys.stdin
        sys.stdin = FakeInput(self.input)
        self.orig_trace = sys.gettrace() if hasattr(sys, 'gettrace') else None

    def __exit__(self, *exc):
        sys.stdin = self.real_stdin
        if self.orig_trace:
            sys.settrace(self.orig_trace)


def test_pdb_displayhook():
    """This tests the custom displayhook for pdb.

    >>> def test_function(foo, bar):
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([
    ...     'foo',
    ...     'bar',
    ...     'for i in range(5): print(i)',
    ...     'continue',
    ... ]):
    ...     test_function(1, None)
    > <doctest test.test_pdb.test_pdb_displayhook[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) foo
    1
    (Pdb) bar
    (Pdb) for i in range(5): print(i)
    0
    1
    2
    3
    4
    (Pdb) continue
    """


def test_pdb_basic_commands():
    """Test the basic commands of pdb.

    >>> def test_function_2(foo, bar='default'):
    ...     print(foo)
    ...     for i in range(5):
    ...         print(i)
    ...     print(bar)
    ...     for i in range(10):
    ...         never_executed
    ...     print('after for')
    ...     print('...')
    ...     return foo.upper()

    >>> def test_function3(arg=None, *, kwonly=None):
    ...     pass

    >>> def test_function4(a, b, c, /):
    ...     pass

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     ret = test_function_2('baz')
    ...     test_function3(kwonly=True)
    ...     test_function4(1, 2, 3)
    ...     print(ret)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',       # go to line ret = test_function_2('baz')
    ...     'step',       # entering the function call
    ...     'args',       # display function args
    ...     'list',       # list function source
    ...     'bt',         # display backtrace
    ...     'up',         # step up to test_function()
    ...     'down',       # step down to test_function_2() again
    ...     'next',       # stepping to print(foo)
    ...     'next',       # stepping to the for loop
    ...     'step',       # stepping into the for loop
    ...     'until',      # continuing until out of the for loop
    ...     'next',       # executing the print(bar)
    ...     'jump 8',     # jump over second for loop
    ...     'return',     # return out of function
    ...     'retval',     # display return value
    ...     'next',       # step to test_function3()
    ...     'step',       # stepping into test_function3()
    ...     'args',       # display function args
    ...     'return',     # return out of function
    ...     'next',       # step to test_function4()
    ...     'step',       # stepping to test_function4()
    ...     'args',       # display function args
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_basic_commands[3]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_basic_commands[3]>(3)test_function()
    -> ret = test_function_2('baz')
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(1)test_function_2()
    -> def test_function_2(foo, bar='default'):
    (Pdb) args
    foo = 'baz'
    bar = 'default'
    (Pdb) list
      1  ->     def test_function_2(foo, bar='default'):
      2             print(foo)
      3             for i in range(5):
      4                 print(i)
      5             print(bar)
      6             for i in range(10):
      7                 never_executed
      8             print('after for')
      9             print('...')
     10             return foo.upper()
    [EOF]
    (Pdb) bt
    ...
      <doctest test.test_pdb.test_pdb_basic_commands[4]>(26)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_basic_commands[3]>(3)test_function()
    -> ret = test_function_2('baz')
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(1)test_function_2()
    -> def test_function_2(foo, bar='default'):
    (Pdb) up
    > <doctest test.test_pdb.test_pdb_basic_commands[3]>(3)test_function()
    -> ret = test_function_2('baz')
    (Pdb) down
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(1)test_function_2()
    -> def test_function_2(foo, bar='default'):
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(2)test_function_2()
    -> print(foo)
    (Pdb) next
    baz
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(3)test_function_2()
    -> for i in range(5):
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(4)test_function_2()
    -> print(i)
    (Pdb) until
    0
    1
    2
    3
    4
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(5)test_function_2()
    -> print(bar)
    (Pdb) next
    default
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(6)test_function_2()
    -> for i in range(10):
    (Pdb) jump 8
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(8)test_function_2()
    -> print('after for')
    (Pdb) return
    after for
    ...
    --Return--
    > <doctest test.test_pdb.test_pdb_basic_commands[0]>(10)test_function_2()->'BAZ'
    -> return foo.upper()
    (Pdb) retval
    'BAZ'
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_basic_commands[3]>(4)test_function()
    -> test_function3(kwonly=True)
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_basic_commands[1]>(1)test_function3()
    -> def test_function3(arg=None, *, kwonly=None):
    (Pdb) args
    arg = None
    kwonly = True
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_pdb_basic_commands[1]>(2)test_function3()->None
    -> pass
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_basic_commands[3]>(5)test_function()
    -> test_function4(1, 2, 3)
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_basic_commands[2]>(1)test_function4()
    -> def test_function4(a, b, c, /):
    (Pdb) args
    a = 1
    b = 2
    c = 3
    (Pdb) continue
    BAZ
    """

def test_pdb_breakpoint_commands():
    """Test basic commands related to breakpoints.

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     print(1)
    ...     print(2)
    ...     print(3)
    ...     print(4)

    Now test the breakpoint commands.  NORMALIZE_WHITESPACE is needed because
    the breakpoint list outputs a tab for the "stop only" and "ignore next"
    lines, which we don't want to put in here.

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 3',
    ...     'break 4, +',
    ...     'disable 1',
    ...     'ignore 1 10',
    ...     'condition 1 1 < 2',
    ...     'condition 1 1 <',
    ...     'break 4',
    ...     'break 4',
    ...     'break',
    ...     'clear 3',
    ...     'break',
    ...     'condition 1',
    ...     'commands 1',
    ...     'EOF',       # Simulate Ctrl-D/Ctrl-Z from user, should end input
    ...     'enable 1',
    ...     'clear 1',
    ...     'commands 2',
    ...     'p "42"',
    ...     'print("42", 7*6)',     # Issue 18764 (not about breakpoints)
    ...     'end',
    ...     'continue',  # will stop at breakpoint 2 (line 4)
    ...     'clear',     # clear all!
    ...     'y',
    ...     'tbreak 5',
    ...     'continue',  # will stop at temporary breakpoint
    ...     'break',     # make sure breakpoint is gone
    ...     'commands 10',  # out of range
    ...     'commands a',   # display help
    ...     'commands 4',   # already deleted
    ...     'break 6, undefined', # condition causing `NameError` during evaluation
    ...     'continue', # will stop, ignoring runtime error
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break 3
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) break 4, +
    *** Invalid condition +: SyntaxError: invalid syntax
    (Pdb) disable 1
    Disabled breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) ignore 1 10
    Will ignore next 10 crossings of breakpoint 1.
    (Pdb) condition 1 1 < 2
    New condition set for breakpoint 1.
    (Pdb) condition 1 1 <
    *** Invalid condition 1 <: SyntaxError: invalid syntax
    (Pdb) break 4
    Breakpoint 2 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) break 4
    Breakpoint 3 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) break
    Num Type         Disp Enb   Where
    1   breakpoint   keep no    at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
            stop only if 1 < 2
            ignore next 10 hits
    2   breakpoint   keep yes   at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    3   breakpoint   keep yes   at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) clear 3
    Deleted breakpoint 3 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) break
    Num Type         Disp Enb   Where
    1   breakpoint   keep no    at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
            stop only if 1 < 2
            ignore next 10 hits
    2   breakpoint   keep yes   at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) condition 1
    Breakpoint 1 is now unconditional.
    (Pdb) commands 1
    (com) EOF
    <BLANKLINE>
    (Pdb) enable 1
    Enabled breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) clear 1
    Deleted breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) commands 2
    (com) p "42"
    (com) print("42", 7*6)
    (com) end
    (Pdb) continue
    1
    '42'
    42 42
    > <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>(4)test_function()
    -> print(2)
    (Pdb) clear
    Clear all breaks? y
    Deleted breakpoint 2 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:4
    (Pdb) tbreak 5
    Breakpoint 4 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:5
    (Pdb) continue
    2
    Deleted breakpoint 4 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:5
    > <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>(5)test_function()
    -> print(3)
    (Pdb) break
    (Pdb) commands 10
    *** cannot set commands: Breakpoint number 10 out of range
    (Pdb) commands a
    *** Invalid argument: a
          Usage: (Pdb) commands [bpnumber]
                 (com) ...
                 (com) end
                 (Pdb)
    (Pdb) commands 4
    *** cannot set commands: Breakpoint 4 already deleted
    (Pdb) break 6, undefined
    Breakpoint 5 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:6
    (Pdb) continue
    3
    > <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>(6)test_function()
    -> print(4)
    (Pdb) continue
    4
    """

def test_pdb_breakpoint_ignore_and_condition():
    """
    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     for i in range(5):
    ...         print(i)

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 4',
    ...     'ignore 1 2',  # ignore once
    ...     'continue',
    ...     'condition 1 i == 4',
    ...     'continue',
    ...     'clear 1',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_ignore_and_condition[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break 4
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_ignore_and_condition[0]>:4
    (Pdb) ignore 1 2
    Will ignore next 2 crossings of breakpoint 1.
    (Pdb) continue
    0
    1
    > <doctest test.test_pdb.test_pdb_breakpoint_ignore_and_condition[0]>(4)test_function()
    -> print(i)
    (Pdb) condition 1 i == 4
    New condition set for breakpoint 1.
    (Pdb) continue
    2
    3
    > <doctest test.test_pdb.test_pdb_breakpoint_ignore_and_condition[0]>(4)test_function()
    -> print(i)
    (Pdb) clear 1
    Deleted breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_ignore_and_condition[0]>:4
    (Pdb) continue
    4
    """

def test_pdb_breakpoint_on_annotated_function_def():
    """Test breakpoints on function definitions with annotation.

    >>> def foo[T]():
    ...     return 0

    >>> def bar() -> int:
    ...     return 0

    >>> def foobar[T]() -> int:
    ...     return 0

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     pass

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break foo',
    ...     'break bar',
    ...     'break foobar',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_on_annotated_function_def[3]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break foo
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_on_annotated_function_def[0]>:2
    (Pdb) break bar
    Breakpoint 2 at <doctest test.test_pdb.test_pdb_breakpoint_on_annotated_function_def[1]>:2
    (Pdb) break foobar
    Breakpoint 3 at <doctest test.test_pdb.test_pdb_breakpoint_on_annotated_function_def[2]>:2
    (Pdb) continue
    """

def test_pdb_commands():
    """Test the commands command of pdb.

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     print(1)
    ...     print(2)
    ...     print(3)

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'b 3',
    ...     'commands',
    ...     'silent',      # suppress the frame status output
    ...     'p "hello"',
    ...     'end',
    ...     'b 4',
    ...     'commands',
    ...     'until 5',     # no output, should stop at line 5
    ...     'continue',    # hit breakpoint at line 3
    ...     '',            # repeat continue, hit breakpoint at line 4 then `until` to line 5
    ...     '',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_commands[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) b 3
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_commands[0]>:3
    (Pdb) commands
    (com) silent
    (com) p "hello"
    (com) end
    (Pdb) b 4
    Breakpoint 2 at <doctest test.test_pdb.test_pdb_commands[0]>:4
    (Pdb) commands
    (com) until 5
    (Pdb) continue
    'hello'
    (Pdb)
    1
    2
    > <doctest test.test_pdb.test_pdb_commands[0]>(5)test_function()
    -> print(3)
    (Pdb)
    3
    """

def test_pdb_breakpoint_with_filename():
    """Breakpoints with filename:lineno

    >>> def test_function():
    ...     # inspect_fodder2 is a great module as the line number is stable
    ...     from test.test_inspect import inspect_fodder2 as mod2
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     mod2.func88()
    ...     mod2.func114()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE +ELLIPSIS
    ...     'break test.test_inspect.inspect_fodder2:90',
    ...     'continue', # will stop at func88
    ...     'break test/test_inspect/inspect_fodder2.py:115',
    ...     'continue', # will stop at func114
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_with_filename[0]>(4)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break test.test_inspect.inspect_fodder2:90
    Breakpoint 1 at ...inspect_fodder2.py:90
    (Pdb) continue
    > ...inspect_fodder2.py(90)func88()
    -> return 90
    (Pdb) break test/test_inspect/inspect_fodder2.py:115
    Breakpoint 2 at ...inspect_fodder2.py:115
    (Pdb) continue
    > ...inspect_fodder2.py(115)func114()
    -> return 115
    (Pdb) continue
    """

def test_pdb_breakpoint_on_disabled_line():
    """New breakpoint on once disabled line should work

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     for i in range(3):
    ...         j = i * 2
    ...         print(j)

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 5',
    ...     'c',
    ...     'clear 1',
    ...     'break 4',
    ...     'c',
    ...     'clear 2',
    ...     'c'
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break 5
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>:5
    (Pdb) c
    > <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>(5)test_function()
    -> print(j)
    (Pdb) clear 1
    Deleted breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>:5
    (Pdb) break 4
    Breakpoint 2 at <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>:4
    (Pdb) c
    0
    > <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>(4)test_function()
    -> j = i * 2
    (Pdb) clear 2
    Deleted breakpoint 2 at <doctest test.test_pdb.test_pdb_breakpoint_on_disabled_line[0]>:4
    (Pdb) c
    2
    4
    """

def test_pdb_breakpoints_preserved_across_interactive_sessions():
    """Breakpoints are remembered between interactive sessions

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...    'import test.test_pdb',
    ...    'break test.test_pdb.do_something',
    ...    'break test.test_pdb.do_nothing',
    ...    'break',
    ...    'continue',
    ... ]):
    ...    pdb.run('print()')
    > <string>(1)<module>()...
    (Pdb) import test.test_pdb
    (Pdb) break test.test_pdb.do_something
    Breakpoint 1 at ...test_pdb.py:...
    (Pdb) break test.test_pdb.do_nothing
    Breakpoint 2 at ...test_pdb.py:...
    (Pdb) break
    Num Type         Disp Enb   Where
    1   breakpoint   keep yes   at ...test_pdb.py:...
    2   breakpoint   keep yes   at ...test_pdb.py:...
    (Pdb) continue

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...    'break',
    ...    'break pdb.find_function',
    ...    'break',
    ...    'clear 1',
    ...    'continue',
    ... ]):
    ...    pdb.run('print()')
    > <string>(1)<module>()...
    (Pdb) break
    Num Type         Disp Enb   Where
    1   breakpoint   keep yes   at ...test_pdb.py:...
    2   breakpoint   keep yes   at ...test_pdb.py:...
    (Pdb) break pdb.find_function
    Breakpoint 3 at ...pdb.py:...
    (Pdb) break
    Num Type         Disp Enb   Where
    1   breakpoint   keep yes   at ...test_pdb.py:...
    2   breakpoint   keep yes   at ...test_pdb.py:...
    3   breakpoint   keep yes   at ...pdb.py:...
    (Pdb) clear 1
    Deleted breakpoint 1 at ...test_pdb.py:...
    (Pdb) continue

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...    'break',
    ...    'clear 2',
    ...    'clear 3',
    ...    'continue',
    ... ]):
    ...    pdb.run('print()')
    > <string>(1)<module>()...
    (Pdb) break
    Num Type         Disp Enb   Where
    2   breakpoint   keep yes   at ...test_pdb.py:...
    3   breakpoint   keep yes   at ...pdb.py:...
    (Pdb) clear 2
    Deleted breakpoint 2 at ...test_pdb.py:...
    (Pdb) clear 3
    Deleted breakpoint 3 at ...pdb.py:...
    (Pdb) continue
    """

def test_pdb_break_anywhere():
    """Test break_anywhere() method of Pdb.

    >>> def outer():
    ...     def inner():
    ...         import pdb
    ...         import sys
    ...         p = pdb.Pdb(nosigint=True, readrc=False)
    ...         p.set_trace()
    ...         frame = sys._getframe()
    ...         print(p.break_anywhere(frame))  # inner
    ...         print(p.break_anywhere(frame.f_back))  # outer
    ...         print(p.break_anywhere(frame.f_back.f_back))  # caller
    ...     inner()

    >>> def caller():
    ...     outer()

    >>> def test_function():
    ...     caller()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'b 3',
    ...     'c',
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_break_anywhere[0]>(6)inner()
    -> p.set_trace()
    (Pdb) b 3
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_break_anywhere[0]>:3
    (Pdb) c
    True
    False
    False
    """

def test_pdb_pp_repr_exc():
    """Test that do_p/do_pp do not swallow exceptions.

    >>> class BadRepr:
    ...     def __repr__(self):
    ...         raise Exception('repr_exc')
    >>> obj = BadRepr()

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'p obj',
    ...     'pp obj',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_pp_repr_exc[2]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) p obj
    *** Exception: repr_exc
    (Pdb) pp obj
    *** Exception: repr_exc
    (Pdb) continue
    """

def test_pdb_empty_line():
    """Test that empty line repeats the last command.

    >>> def test_function():
    ...     x = 1
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     y = 2

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'p x',
    ...     '',  # Should repeat p x
    ...     'n ;; p 0 ;; p x',  # Fill cmdqueue with multiple commands
    ...     '',  # Should still repeat p x
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_empty_line[0]>(3)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) p x
    1
    (Pdb)
    1
    (Pdb) n ;; p 0 ;; p x
    0
    1
    > <doctest test.test_pdb.test_pdb_empty_line[0]>(4)test_function()
    -> y = 2
    (Pdb)
    1
    (Pdb) continue
    """

def do_nothing():
    pass

def do_something():
    print(42)

def test_list_commands():
    """Test the list and source commands of pdb.

    >>> def test_function_2(foo):
    ...     import test.test_pdb
    ...     test.test_pdb.do_nothing()
    ...     'some...'
    ...     'more...'
    ...     'code...'
    ...     'to...'
    ...     'make...'
    ...     'a...'
    ...     'long...'
    ...     'listing...'
    ...     'useful...'
    ...     '...'
    ...     '...'
    ...     return foo

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     ret = test_function_2('baz')

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',      # go to the test function line
    ...     'list',      # list first function
    ...     'step',      # step into second function
    ...     'list',      # list second function
    ...     'list',      # continue listing to EOF
    ...     'list 1,3',  # list specific lines
    ...     'list x',    # invalid argument
    ...     'next',      # step to import
    ...     'next',      # step over import
    ...     'step',      # step into do_nothing
    ...     'longlist',  # list all lines
    ...     'source do_something',  # list all lines of function
    ...     'source fooxxx',        # something that doesn't exit
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_list_commands[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_list_commands[1]>(3)test_function()
    -> ret = test_function_2('baz')
    (Pdb) list
      1         def test_function():
      2             import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
      3  ->         ret = test_function_2('baz')
    [EOF]
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_list_commands[0]>(1)test_function_2()
    -> def test_function_2(foo):
    (Pdb) list
      1  ->     def test_function_2(foo):
      2             import test.test_pdb
      3             test.test_pdb.do_nothing()
      4             'some...'
      5             'more...'
      6             'code...'
      7             'to...'
      8             'make...'
      9             'a...'
     10             'long...'
     11             'listing...'
    (Pdb) list
     12             'useful...'
     13             '...'
     14             '...'
     15             return foo
    [EOF]
    (Pdb) list 1,3
      1  ->     def test_function_2(foo):
      2             import test.test_pdb
      3             test.test_pdb.do_nothing()
    (Pdb) list x
    *** ...
    (Pdb) next
    > <doctest test.test_pdb.test_list_commands[0]>(2)test_function_2()
    -> import test.test_pdb
    (Pdb) next
    > <doctest test.test_pdb.test_list_commands[0]>(3)test_function_2()
    -> test.test_pdb.do_nothing()
    (Pdb) step
    --Call--
    > ...test_pdb.py(...)do_nothing()
    -> def do_nothing():
    (Pdb) longlist
    ...  ->     def do_nothing():
    ...             pass
    (Pdb) source do_something
    ...         def do_something():
    ...             print(42)
    (Pdb) source fooxxx
    *** ...
    (Pdb) continue
    """

def test_pdb_whatis_command():
    """Test the whatis command

    >>> myvar = (1,2)
    >>> def myfunc():
    ...     pass

    >>> class MyClass:
    ...    def mymethod(self):
    ...        pass

    >>> def test_function():
    ...   import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...    'whatis myvar',
    ...    'whatis myfunc',
    ...    'whatis MyClass',
    ...    'whatis MyClass()',
    ...    'whatis MyClass.mymethod',
    ...    'whatis MyClass().mymethod',
    ...    'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_whatis_command[3]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) whatis myvar
    <class 'tuple'>
    (Pdb) whatis myfunc
    Function myfunc
    (Pdb) whatis MyClass
    Class test.test_pdb.MyClass
    (Pdb) whatis MyClass()
    <class 'test.test_pdb.MyClass'>
    (Pdb) whatis MyClass.mymethod
    Function mymethod
    (Pdb) whatis MyClass().mymethod
    Method mymethod
    (Pdb) continue
    """

def test_pdb_display_command():
    """Test display command

    >>> def test_function():
    ...     a = 0
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     a = 1
    ...     a = 2
    ...     a = 3
    ...     a = 4

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     's',
    ...     'display +',
    ...     'display',
    ...     'display a',
    ...     'n',
    ...     'display',
    ...     'undisplay a',
    ...     'n',
    ...     'display a',
    ...     'undisplay',
    ...     'display a < 1',
    ...     'n',
    ...     'display undefined',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_display_command[0]>(3)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) s
    > <doctest test.test_pdb.test_pdb_display_command[0]>(4)test_function()
    -> a = 1
    (Pdb) display +
    *** Unable to display +: SyntaxError: invalid syntax
    (Pdb) display
    No expression is being displayed
    (Pdb) display a
    display a: 0
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_display_command[0]>(5)test_function()
    -> a = 2
    display a: 1  [old: 0]
    (Pdb) display
    Currently displaying:
    a: 1
    (Pdb) undisplay a
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_display_command[0]>(6)test_function()
    -> a = 3
    (Pdb) display a
    display a: 2
    (Pdb) undisplay
    (Pdb) display a < 1
    display a < 1: False
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_display_command[0]>(7)test_function()
    -> a = 4
    (Pdb) display undefined
    display undefined: ** raised NameError: name 'undefined' is not defined **
    (Pdb) continue
    """

def test_pdb_alias_command():
    """Test alias command

    >>> class A:
    ...     def __init__(self):
    ...         self.attr1 = 10
    ...         self.attr2 = 'str'
    ...     def method(self):
    ...         pass

    >>> def test_function():
    ...     o = A()
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     o.method()

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     's',
    ...     'alias pi',
    ...     'alias pi for k in %1.__dict__.keys(): print(f"%1.{k} = {%1.__dict__[k]}")',
    ...     'alias ps pi self',
    ...     'alias ps',
    ...     'pi o',
    ...     's',
    ...     'ps',
    ...     'alias myp p %2',
    ...     'alias myp',
    ...     'alias myp p %1',
    ...     'myp',
    ...     'myp 1',
    ...     'myp 1 2',
    ...     'alias repeat_second_arg p "%* %2"',
    ...     'repeat_second_arg 1 2 3',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_alias_command[1]>(3)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) s
    > <doctest test.test_pdb.test_pdb_alias_command[1]>(4)test_function()
    -> o.method()
    (Pdb) alias pi
    *** Unknown alias 'pi'
    (Pdb) alias pi for k in %1.__dict__.keys(): print(f"%1.{k} = {%1.__dict__[k]}")
    (Pdb) alias ps pi self
    (Pdb) alias ps
    ps = pi self
    (Pdb) pi o
    o.attr1 = 10
    o.attr2 = str
    (Pdb) s
    --Call--
    > <doctest test.test_pdb.test_pdb_alias_command[0]>(5)method()
    -> def method(self):
    (Pdb) ps
    self.attr1 = 10
    self.attr2 = str
    (Pdb) alias myp p %2
    *** Replaceable parameters must be consecutive
    (Pdb) alias myp
    *** Unknown alias 'myp'
    (Pdb) alias myp p %1
    (Pdb) myp
    *** Not enough arguments for alias 'myp'
    (Pdb) myp 1
    1
    (Pdb) myp 1 2
    *** Too many arguments for alias 'myp'
    (Pdb) alias repeat_second_arg p "%* %2"
    (Pdb) repeat_second_arg 1 2 3
    '1 2 3 2'
    (Pdb) continue
    """

def test_pdb_where_command():
    """Test where command

    >>> def g():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> def f():
    ...     g()

    >>> def test_function():
    ...     f()

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     'w',
    ...     'where',
    ...     'w 1',
    ...     'w invalid',
    ...     'u',
    ...     'w',
    ...     'w 0',
    ...     'w 100',
    ...     'w -100',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) w
    ...
      <doctest test.test_pdb.test_pdb_where_command[3]>(13)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_where_command[2]>(2)test_function()
    -> f()
      <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
    > <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) where
    ...
      <doctest test.test_pdb.test_pdb_where_command[3]>(13)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_where_command[2]>(2)test_function()
    -> f()
      <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
    > <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) w 1
    > <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) w invalid
    *** Invalid count (invalid)
    (Pdb) u
    > <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
    (Pdb) w
    ...
      <doctest test.test_pdb.test_pdb_where_command[3]>(13)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_where_command[2]>(2)test_function()
    -> f()
    > <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
      <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) w 0
    > <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
    (Pdb) w 100
    ...
      <doctest test.test_pdb.test_pdb_where_command[3]>(13)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_where_command[2]>(2)test_function()
    -> f()
    > <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
      <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) w -100
    ...
      <doctest test.test_pdb.test_pdb_where_command[3]>(13)<module>()
    -> test_function()
      <doctest test.test_pdb.test_pdb_where_command[2]>(2)test_function()
    -> f()
    > <doctest test.test_pdb.test_pdb_where_command[1]>(2)f()
    -> g()
      <doctest test.test_pdb.test_pdb_where_command[0]>(2)g()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) continue
    """

def test_pdb_restart_command():
    """Test restart command

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False, mode='inline').set_trace()
    ...     x = 1

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     'restart',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_restart_command[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False, mode='inline').set_trace()
    (Pdb) restart
    *** run/restart command is disabled when pdb is running in inline mode.
    Use the command line interface to enable restarting your program
    e.g. "python -m pdb myscript.py"
    (Pdb) continue
    """

def test_pdb_commands_with_set_trace():
    """Test that commands can be passed to Pdb.set_trace()

    >>> def test_function():
    ...     x = 1
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace(commands=['p x', 'c'])

    >>> test_function()
    1
    """


# skip this test if sys.flags.no_site = True;
# exit() isn't defined unless there's a site module.
if not sys.flags.no_site:
    def test_pdb_interact_command():
        """Test interact command

        >>> g = 0
        >>> dict_g = {}

        >>> def test_function():
        ...     x = 1
        ...     lst_local = []
        ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

        >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
        ...     'interact',
        ...     'x',
        ...     'g',
        ...     'x = 2',
        ...     'g = 3',
        ...     'dict_g["a"] = True',
        ...     'lst_local.append(x)',
        ...     'exit()',
        ...     'p x',
        ...     'p g',
        ...     'p dict_g',
        ...     'p lst_local',
        ...     'continue',
        ... ]):
        ...    test_function()
        > <doctest test.test_pdb.test_pdb_interact_command[2]>(4)test_function()
        -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        (Pdb) interact
        *pdb interact start*
        ... x
        1
        ... g
        0
        ... x = 2
        ... g = 3
        ... dict_g["a"] = True
        ... lst_local.append(x)
        ... exit()
        *exit from pdb interact command*
        (Pdb) p x
        1
        (Pdb) p g
        0
        (Pdb) p dict_g
        {'a': True}
        (Pdb) p lst_local
        [2]
        (Pdb) continue
        """

def test_convenience_variables():
    """Test convenience variables

    >>> def util_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     try:
    ...         raise Exception('test')
    ...     except Exception:
    ...         pass
    ...     return 1

    >>> def test_function():
    ...     util_function()

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',             # Step to try statement
    ...     '$_frame.f_lineno', # Check frame convenience variable
    ...     '$ _frame',         # This should be a syntax error
    ...     '$a = 10',          # Set a convenience variable
    ...     '$a',               # Print its value
    ...     'p "$a"',           # Print the string $a
    ...     'p $a + 2',         # Do some calculation
    ...     'p f"$a = {$a}"',   # Make sure $ in string is not converted and f-string works
    ...     'u',                # Switch frame
    ...     '$_frame.f_lineno', # Make sure the frame changed
    ...     '$a',               # Make sure the value persists
    ...     'd',                # Go back to the original frame
    ...     'next',
    ...     '$a',               # The value should be gone
    ...     'next',
    ...     '$_exception',      # Check exception convenience variable
    ...     'next',
    ...     '$_exception',      # Exception should be gone
    ...     'return',
    ...     '$_retval',         # Check return convenience variable
    ...     'continue',
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_convenience_variables[0]>(2)util_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_convenience_variables[0]>(3)util_function()
    -> try:
    (Pdb) $_frame.f_lineno
    3
    (Pdb) $ _frame
    *** SyntaxError: invalid syntax
    (Pdb) $a = 10
    (Pdb) $a
    10
    (Pdb) p "$a"
    '$a'
    (Pdb) p $a + 2
    12
    (Pdb) p f"$a = {$a}"
    '$a = 10'
    (Pdb) u
    > <doctest test.test_pdb.test_convenience_variables[1]>(2)test_function()
    -> util_function()
    (Pdb) $_frame.f_lineno
    2
    (Pdb) $a
    10
    (Pdb) d
    > <doctest test.test_pdb.test_convenience_variables[0]>(3)util_function()
    -> try:
    (Pdb) next
    > <doctest test.test_pdb.test_convenience_variables[0]>(4)util_function()
    -> raise Exception('test')
    (Pdb) $a
    *** KeyError: 'a'
    (Pdb) next
    Exception: test
    > <doctest test.test_pdb.test_convenience_variables[0]>(4)util_function()
    -> raise Exception('test')
    (Pdb) $_exception
    Exception('test')
    (Pdb) next
    > <doctest test.test_pdb.test_convenience_variables[0]>(5)util_function()
    -> except Exception:
    (Pdb) $_exception
    *** KeyError: '_exception'
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_convenience_variables[0]>(7)util_function()->1
    -> return 1
    (Pdb) $_retval
    1
    (Pdb) continue
    """


def test_post_mortem_chained():
    """Test post mortem traceback debugging of chained exception

    >>> def test_function_2():
    ...     try:
    ...         1/0
    ...     finally:
    ...         print('Exception!')

    >>> def test_function_reraise():
    ...     try:
    ...         test_function_2()
    ...     except ZeroDivisionError as e:
    ...         raise ZeroDivisionError('reraised') from e

    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         test_function_reraise()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'exceptions',
    ...     'exceptions 0',
    ...     '$_exception',
    ...     'up',
    ...     'down',
    ...     'exceptions 1',
    ...     '$_exception',
    ...     'up',
    ...     'down',
    ...     'exceptions -1',
    ...     'exceptions 3',
    ...     'up',
    ...     'exit',
    ... ]):
    ...    try:
    ...        test_function()
    ...    except ZeroDivisionError:
    ...        print('Correctly reraised.')
    Exception!
    > <doctest test.test_pdb.test_post_mortem_chained[1]>(5)test_function_reraise()
    -> raise ZeroDivisionError('reraised') from e
    (Pdb) exceptions
      0 ZeroDivisionError('division by zero')
    > 1 ZeroDivisionError('reraised')
    (Pdb) exceptions 0
    > <doctest test.test_pdb.test_post_mortem_chained[0]>(3)test_function_2()
    -> 1/0
    (Pdb) $_exception
    ZeroDivisionError('division by zero')
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_chained[1]>(3)test_function_reraise()
    -> test_function_2()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_chained[0]>(3)test_function_2()
    -> 1/0
    (Pdb) exceptions 1
    > <doctest test.test_pdb.test_post_mortem_chained[1]>(5)test_function_reraise()
    -> raise ZeroDivisionError('reraised') from e
    (Pdb) $_exception
    ZeroDivisionError('reraised')
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_chained[2]>(5)test_function()
    -> test_function_reraise()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_chained[1]>(5)test_function_reraise()
    -> raise ZeroDivisionError('reraised') from e
    (Pdb) exceptions -1
    *** No exception with that number
    (Pdb) exceptions 3
    *** No exception with that number
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_chained[2]>(5)test_function()
    -> test_function_reraise()
    (Pdb) exit
    """


def test_post_mortem_cause_no_context():
    """Test post mortem traceback debugging of chained exception

    >>> def make_exc_with_stack(type_, *content, from_=None):
    ...     try:
    ...         raise type_(*content) from from_
    ...     except Exception as out:
    ...         return out
    ...

    >>> def main():
    ...     try:
    ...         raise ValueError('Context Not Shown')
    ...     except Exception as e1:
    ...         raise ValueError("With Cause") from make_exc_with_stack(TypeError,'The Cause')

    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         main()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'exceptions',
    ...     'exceptions 0',
    ...     'exceptions 1',
    ...     'up',
    ...     'down',
    ...     'exit',
    ... ]):
    ...    try:
    ...        test_function()
    ...    except ValueError:
    ...        print('Ok.')
    > <doctest test.test_pdb.test_post_mortem_cause_no_context[1]>(5)main()
    -> raise ValueError("With Cause") from make_exc_with_stack(TypeError,'The Cause')
    (Pdb) exceptions
        0 TypeError('The Cause')
    >   1 ValueError('With Cause')
    (Pdb) exceptions 0
    > <doctest test.test_pdb.test_post_mortem_cause_no_context[0]>(3)make_exc_with_stack()
    -> raise type_(*content) from from_
    (Pdb) exceptions 1
    > <doctest test.test_pdb.test_post_mortem_cause_no_context[1]>(5)main()
    -> raise ValueError("With Cause") from make_exc_with_stack(TypeError,'The Cause')
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_cause_no_context[2]>(5)test_function()
    -> main()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_cause_no_context[1]>(5)main()
    -> raise ValueError("With Cause") from make_exc_with_stack(TypeError,'The Cause')
    (Pdb) exit"""


def test_post_mortem_context_of_the_cause():
    """Test post mortem traceback debugging of chained exception


    >>> def main():
    ...     try:
    ...         raise TypeError('Context of the cause')
    ...     except Exception as e1:
    ...         try:
    ...             raise ValueError('Root Cause')
    ...         except Exception as e2:
    ...             ex = e2
    ...         raise ValueError("With Cause, and cause has context") from ex

    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         main()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'exceptions',
    ...     'exceptions 2',
    ...     'up',
    ...     'down',
    ...     'exceptions 3',
    ...     'up',
    ...     'down',
    ...     'exceptions 4',
    ...     'up',
    ...     'down',
    ...     'exit',
    ... ]):
    ...    try:
    ...        test_function()
    ...    except ValueError:
    ...        print('Correctly reraised.')
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[0]>(9)main()
    -> raise ValueError("With Cause, and cause has context") from ex
    (Pdb) exceptions
      0 TypeError('Context of the cause')
      1 ValueError('Root Cause')
    > 2 ValueError('With Cause, and cause has context')
    (Pdb) exceptions 2
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[0]>(9)main()
    -> raise ValueError("With Cause, and cause has context") from ex
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[1]>(5)test_function()
    -> main()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[0]>(9)main()
    -> raise ValueError("With Cause, and cause has context") from ex
    (Pdb) exceptions 3
    *** No exception with that number
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[1]>(5)test_function()
    -> main()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[0]>(9)main()
    -> raise ValueError("With Cause, and cause has context") from ex
    (Pdb) exceptions 4
    *** No exception with that number
    (Pdb) up
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[1]>(5)test_function()
    -> main()
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem_context_of_the_cause[0]>(9)main()
    -> raise ValueError("With Cause, and cause has context") from ex
    (Pdb) exit
    """


def test_post_mortem_from_none():
    """Test post mortem traceback debugging of chained exception

    In particular that cause from None (which sets __suppress_context__ to True)
    does not show context.


    >>> def main():
    ...     try:
    ...         raise TypeError('Context of the cause')
    ...     except Exception as e1:
    ...         raise ValueError("With Cause, and cause has context") from None

    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         main()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'exceptions',
    ...     'exit',
    ... ]):
    ...    try:
    ...        test_function()
    ...    except ValueError:
    ...        print('Correctly reraised.')
    > <doctest test.test_pdb.test_post_mortem_from_none[0]>(5)main()
    -> raise ValueError("With Cause, and cause has context") from None
    (Pdb) exceptions
    > 0 ValueError('With Cause, and cause has context')
    (Pdb) exit
    """


def test_post_mortem_from_no_stack():
    """Test post mortem traceback debugging of chained exception

    especially when one exception has no stack.

    >>> def main():
    ...     raise Exception() from Exception()


    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         main()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput(  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     ["exceptions",
    ...      "exceptions 0",
    ...     "exit"],
    ... ):
    ...    try:
    ...        test_function()
    ...    except ValueError:
    ...        print('Correctly reraised.')
    > <doctest test.test_pdb.test_post_mortem_from_no_stack[0]>(2)main()
    -> raise Exception() from Exception()
    (Pdb) exceptions
        - Exception()
    >   1 Exception()
    (Pdb) exceptions 0
    *** This exception does not have a traceback, cannot jump to it
    (Pdb) exit
    """


def test_post_mortem_single_no_stack():
    """Test post mortem called when origin exception has no stack


    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     import sys
    ...     sys.last_exc = Exception()
    ...     pdb._post_mortem(sys.last_exc, instance)

    >>> with PdbTestInput(  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     []
    ... ):
    ...    try:
    ...        test_function()
    ...    except ValueError as e:
    ...        print(e)
    A valid traceback must be passed if no exception is being handled
    """

def test_post_mortem_complex():
    """Test post mortem traceback debugging of chained exception

    Test with simple and complex cycles, exception groups,...

    >>> def make_ex_with_stack(type_, *content, from_=None):
    ...     try:
    ...         raise type_(*content) from from_
    ...     except Exception as out:
    ...         return out
    ...

    >>> def cycle():
    ...     try:
    ...         raise ValueError("Cycle Leaf")
    ...     except Exception as e:
    ...         raise e from e
    ...

    >>> def tri_cycle():
    ...     a = make_ex_with_stack(ValueError, "Cycle1")
    ...     b = make_ex_with_stack(ValueError, "Cycle2")
    ...     c = make_ex_with_stack(ValueError, "Cycle3")
    ...
    ...     a.__cause__ = b
    ...     b.__cause__ = c
    ...
    ...     raise c from a
    ...

    >>> def cause():
    ...     try:
    ...         raise ValueError("Cause Leaf")
    ...     except Exception as e:
    ...         raise e
    ...

    >>> def context(n=10):
    ...     try:
    ...         raise ValueError(f"Context Leaf {n}")
    ...     except Exception as e:
    ...         if n == 0:
    ...             raise ValueError(f"With Context {n}") from e
    ...         else:
    ...             context(n - 1)
    ...

    >>> def main():
    ...     try:
    ...         cycle()
    ...     except Exception as e1:
    ...         try:
    ...             tri_cycle()
    ...         except Exception as e2:
    ...             ex = e2
    ...         raise ValueError("With Context and With Cause") from ex


    >>> def test_function():
    ...     import pdb;
    ...     instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     try:
    ...         main()
    ...     except Exception as e:
    ...         pdb._post_mortem(e, instance)

    >>> with PdbTestInput(  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     ["exceptions",
    ...     "exceptions 0",
    ...     "exceptions 1",
    ...     "exceptions 2",
    ...     "exceptions 3",
    ...     "exit"],
    ... ):
    ...    try:
    ...        test_function()
    ...    except ValueError:
    ...        print('Correctly reraised.')
        > <doctest test.test_pdb.test_post_mortem_complex[5]>(9)main()
    -> raise ValueError("With Context and With Cause") from ex
    (Pdb) exceptions
        0 ValueError('Cycle2')
        1 ValueError('Cycle1')
        2 ValueError('Cycle3')
    >   3 ValueError('With Context and With Cause')
    (Pdb) exceptions 0
    > <doctest test.test_pdb.test_post_mortem_complex[0]>(3)make_ex_with_stack()
    -> raise type_(*content) from from_
    (Pdb) exceptions 1
    > <doctest test.test_pdb.test_post_mortem_complex[0]>(3)make_ex_with_stack()
    -> raise type_(*content) from from_
    (Pdb) exceptions 2
    > <doctest test.test_pdb.test_post_mortem_complex[0]>(3)make_ex_with_stack()
    -> raise type_(*content) from from_
    (Pdb) exceptions 3
    > <doctest test.test_pdb.test_post_mortem_complex[5]>(9)main()
    -> raise ValueError("With Context and With Cause") from ex
    (Pdb) exit
    """


def test_post_mortem():
    """Test post mortem traceback debugging.

    >>> def test_function_2():
    ...     try:
    ...         1/0
    ...     finally:
    ...         print('Exception!')

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     test_function_2()
    ...     print('Not reached.')

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',      # step to test_function_2() line
    ...     'next',      # step over exception-raising call
    ...     'bt',        # get a backtrace
    ...     'list',      # list code of test_function()
    ...     'down',      # step into test_function_2()
    ...     'list',      # list code of test_function_2()
    ...     'continue',
    ... ]):
    ...    try:
    ...        test_function()
    ...    except ZeroDivisionError:
    ...        print('Correctly reraised.')
    > <doctest test.test_pdb.test_post_mortem[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_post_mortem[1]>(3)test_function()
    -> test_function_2()
    (Pdb) next
    Exception!
    ZeroDivisionError: division by zero
    > <doctest test.test_pdb.test_post_mortem[1]>(3)test_function()
    -> test_function_2()
    (Pdb) bt
    ...
      <doctest test.test_pdb.test_post_mortem[2]>(11)<module>()
    -> test_function()
    > <doctest test.test_pdb.test_post_mortem[1]>(3)test_function()
    -> test_function_2()
      <doctest test.test_pdb.test_post_mortem[0]>(3)test_function_2()
    -> 1/0
    (Pdb) list
      1         def test_function():
      2             import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
      3  ->         test_function_2()
      4             print('Not reached.')
    [EOF]
    (Pdb) down
    > <doctest test.test_pdb.test_post_mortem[0]>(3)test_function_2()
    -> 1/0
    (Pdb) list
      1         def test_function_2():
      2             try:
      3  >>             1/0
      4             finally:
      5  ->             print('Exception!')
    [EOF]
    (Pdb) continue
    Correctly reraised.
    """


def test_pdb_return_to_different_file():
    """When pdb returns to a different file, it should not skip if f_trace is
       not already set

    >>> import pprint

    >>> class A:
    ...    def __repr__(self):
    ...        return 'A'

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     pprint.pprint(A())

    >>> with PdbTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'b A.__repr__',
    ...     'continue',
    ...     'return',
    ...     'next',
    ...     'return',
    ...     'return',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_return_to_different_file[2]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) b A.__repr__
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_return_to_different_file[1]>:3
    (Pdb) continue
    > <doctest test.test_pdb.test_pdb_return_to_different_file[1]>(3)__repr__()
    -> return 'A'
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_pdb_return_to_different_file[1]>(3)__repr__()->'A'
    -> return 'A'
    (Pdb) next
    > ...pprint.py..._safe_repr()
    -> return rep,...
    (Pdb) return
    --Return--
    > ...pprint.py..._safe_repr()->('A'...)
    -> return rep,...
    (Pdb) return
    --Return--
    > ...pprint.py...format()->('A'...)
    -> return...
    (Pdb) continue
    A
    """


def test_pdb_skip_modules():
    """This illustrates the simple case of module skipping.

    >>> def skip_module():
    ...     import string
    ...     import pdb; pdb.Pdb(skip=['stri*'], nosigint=True, readrc=False).set_trace()
    ...     string.capwords('FOO')

    >>> with PdbTestInput([
    ...     'step',
    ...     'step',
    ...     'continue',
    ... ]):
    ...     skip_module()
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(3)skip_module()
    -> import pdb; pdb.Pdb(skip=['stri*'], nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()
    -> string.capwords('FOO')
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()->None
    -> string.capwords('FOO')
    (Pdb) continue
    """

def test_pdb_invalid_arg():
    """This tests pdb commands that have invalid arguments

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'a = 3',
    ...     'll 4',
    ...     'step 1',
    ...     'p',
    ...     'enable ',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_invalid_arg[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) a = 3
    *** Invalid argument: = 3
          Usage: a(rgs)
    (Pdb) ll 4
    *** Invalid argument: 4
          Usage: ll | longlist
    (Pdb) step 1
    *** Invalid argument: 1
          Usage: s(tep)
    (Pdb) p
    *** Argument is required for this command
          Usage: p expression
    (Pdb) enable
    *** Argument is required for this command
          Usage: enable bpnumber [bpnumber ...]
    (Pdb) continue
    """


# Module for testing skipping of module that makes a callback
mod = types.ModuleType('module_to_skip')
exec('def foo_pony(callback): x = 1; callback(); return None', mod.__dict__)


def test_pdb_skip_modules_with_callback():
    """This illustrates skipping of modules that call into other code.

    >>> def skip_module():
    ...     def callback():
    ...         return None
    ...     import pdb; pdb.Pdb(skip=['module_to_skip*'], nosigint=True, readrc=False).set_trace()
    ...     mod.foo_pony(callback)

    >>> with PdbTestInput([
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'step',
    ...     'continue',
    ... ]):
    ...     skip_module()
    ...     pass  # provides something to "step" to
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(4)skip_module()
    -> import pdb; pdb.Pdb(skip=['module_to_skip*'], nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(5)skip_module()
    -> mod.foo_pony(callback)
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(2)callback()
    -> def callback():
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(3)callback()
    -> return None
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(3)callback()->None
    -> return None
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[0]>(5)skip_module()->None
    -> mod.foo_pony(callback)
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[1]>(11)<module>()
    -> pass  # provides something to "step" to
    (Pdb) continue
    """


def test_pdb_continue_in_bottomframe():
    """Test that "continue" and "next" work properly in bottom frame (issue #5294).

    >>> def test_function():
    ...     import pdb, sys; inst = pdb.Pdb(nosigint=True, readrc=False)
    ...     inst.set_trace()
    ...     inst.botframe = sys._getframe()  # hackery to get the right botframe
    ...     print(1)
    ...     print(2)
    ...     print(3)
    ...     print(4)

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     'step',
    ...     'next',
    ...     'break 7',
    ...     'continue',
    ...     'next',
    ...     'continue',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>(3)test_function()
    -> inst.set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>(4)test_function()
    -> inst.botframe = sys._getframe()  # hackery to get the right botframe
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>(5)test_function()
    -> print(1)
    (Pdb) break 7
    Breakpoint ... at <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>:7
    (Pdb) continue
    1
    2
    > <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>(7)test_function()
    -> print(3)
    (Pdb) next
    3
    > <doctest test.test_pdb.test_pdb_continue_in_bottomframe[0]>(8)test_function()
    -> print(4)
    (Pdb) continue
    4
    """


def pdb_invoke(method, arg):
    """Run pdb.method(arg)."""
    getattr(pdb.Pdb(nosigint=True, readrc=False), method)(arg)


def test_pdb_run_with_incorrect_argument():
    """Testing run and runeval with incorrect first argument.

    >>> pti = PdbTestInput(['continue',])
    >>> with pti:
    ...     pdb_invoke('run', lambda x: x)
    Traceback (most recent call last):
    TypeError: exec() arg 1 must be a string, bytes or code object

    >>> with pti:
    ...     pdb_invoke('runeval', lambda x: x)
    Traceback (most recent call last):
    TypeError: eval() arg 1 must be a string, bytes or code object
    """


def test_pdb_run_with_code_object():
    """Testing run and runeval with code object as a first argument.

    >>> with PdbTestInput(['step','x', 'continue']):  # doctest: +ELLIPSIS
    ...     pdb_invoke('run', compile('x=1', '<string>', 'exec'))
    > <string>(1)<module>()...
    (Pdb) step
    --Return--
    > <string>(1)<module>()->None
    (Pdb) x
    1
    (Pdb) continue

    >>> with PdbTestInput(['x', 'continue']):
    ...     x=0
    ...     pdb_invoke('runeval', compile('x+1', '<string>', 'eval'))
    > <string>(1)<module>()->None
    (Pdb) x
    1
    (Pdb) continue
    """

def test_next_until_return_at_return_event():
    """Test that pdb stops after a next/until/return issued at a return debug event.

    >>> def test_function_2():
    ...     x = 1
    ...     x = 2

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     test_function_2()
    ...     test_function_2()
    ...     test_function_2()
    ...     end = 1

    >>> with PdbTestInput(['break test_function_2',
    ...                    'continue',
    ...                    'return',
    ...                    'next',
    ...                    'continue',
    ...                    'return',
    ...                    'until',
    ...                    'continue',
    ...                    'return',
    ...                    'return',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_next_until_return_at_return_event[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break test_function_2
    Breakpoint 1 at <doctest test.test_pdb.test_next_until_return_at_return_event[0]>:2
    (Pdb) continue
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(2)test_function_2()
    -> x = 1
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(3)test_function_2()->None
    -> x = 2
    (Pdb) next
    > <doctest test.test_pdb.test_next_until_return_at_return_event[1]>(4)test_function()
    -> test_function_2()
    (Pdb) continue
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(2)test_function_2()
    -> x = 1
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(3)test_function_2()->None
    -> x = 2
    (Pdb) until
    > <doctest test.test_pdb.test_next_until_return_at_return_event[1]>(5)test_function()
    -> test_function_2()
    (Pdb) continue
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(2)test_function_2()
    -> x = 1
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_next_until_return_at_return_event[0]>(3)test_function_2()->None
    -> x = 2
    (Pdb) return
    > <doctest test.test_pdb.test_next_until_return_at_return_event[1]>(6)test_function()
    -> end = 1
    (Pdb) continue
    """

def test_pdb_next_command_for_generator():
    """Testing skip unwinding stack on yield for generators for "next" command

    >>> def test_gen():
    ...     yield 0
    ...     return 1
    ...     yield 2

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     it = test_gen()
    ...     try:
    ...         if next(it) != 0:
    ...             raise AssertionError
    ...         next(it)
    ...     except StopIteration as ex:
    ...         if ex.value != 1:
    ...             raise AssertionError
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'step',
    ...                    'step',
    ...                    'next',
    ...                    'next',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[1]>(3)test_function()
    -> it = test_gen()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[1]>(4)test_function()
    -> try:
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[1]>(5)test_function()
    -> if next(it) != 0:
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[0]>(1)test_gen()
    -> def test_gen():
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[0]>(2)test_gen()
    -> yield 0
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[0]>(3)test_gen()
    -> return 1
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[0]>(3)test_gen()->1
    -> return 1
    (Pdb) step
    StopIteration: 1
    > <doctest test.test_pdb.test_pdb_next_command_for_generator[1]>(7)test_function()
    -> next(it)
    (Pdb) continue
    finished
    """

if not SKIP_CORO_TESTS:
    if has_socket_support:
        def test_pdb_asynctask():
            """Testing $_asynctask is accessible in async context

            >>> import asyncio

            >>> async def test():
            ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

            >>> def test_function():
            ...     asyncio.run(test(), loop_factory=asyncio.EventLoop)

            >>> with PdbTestInput([  # doctest: +ELLIPSIS
            ...     '$_asynctask',
            ...     'continue',
            ... ]):
            ...     test_function()
            > <doctest test.test_pdb.test_pdb_asynctask[1]>(2)test()
            -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
            (Pdb) $_asynctask
            <Task pending name=... coro=<test() running at <doctest test.test_pdb.test_pdb_asynctask[1]>:2> ...
            (Pdb) continue
            """

        def test_pdb_await_support():
            """Testing await support in pdb

            >>> import asyncio

            >>> async def test():
            ...     print("hello")
            ...     await asyncio.sleep(0)
            ...     print("world")
            ...     return 42

            >>> async def main():
            ...     import pdb
            ...     task = asyncio.create_task(test())
            ...     await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            ...     pass

            >>> def test_function():
            ...     asyncio.run(main(), loop_factory=asyncio.EventLoop)

            >>> with PdbTestInput([  # doctest: +ELLIPSIS
            ...     'x = await task',
            ...     'p x',
            ...     'x = await test()',
            ...     'p x',
            ...     'new_task = asyncio.create_task(test())',
            ...     'await new_task',
            ...     'await non_exist()',
            ...     's',
            ...     'continue',
            ... ]):
            ...     test_function()
            > <doctest test.test_pdb.test_pdb_await_support[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) x = await task
            hello
            world
            > <doctest test.test_pdb.test_pdb_await_support[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p x
            42
            (Pdb) x = await test()
            hello
            world
            > <doctest test.test_pdb.test_pdb_await_support[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p x
            42
            (Pdb) new_task = asyncio.create_task(test())
            (Pdb) await new_task
            hello
            world
            > <doctest test.test_pdb.test_pdb_await_support[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) await non_exist()
            *** NameError: name 'non_exist' is not defined
            > <doctest test.test_pdb.test_pdb_await_support[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) s
            > <doctest test.test_pdb.test_pdb_await_support[2]>(5)main()
            -> pass
            (Pdb) continue
            """

        def test_pdb_await_with_breakpoint():
            """Testing await support with breakpoints set in tasks

            >>> import asyncio

            >>> async def test():
            ...     x = 2
            ...     await asyncio.sleep(0)
            ...     return 42

            >>> async def main():
            ...     import pdb
            ...     task = asyncio.create_task(test())
            ...     await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()

            >>> def test_function():
            ...     asyncio.run(main(), loop_factory=asyncio.EventLoop)

            >>> with PdbTestInput([  # doctest: +ELLIPSIS
            ...     'b test',
            ...     'k = await task',
            ...     'n',
            ...     'p x',
            ...     'continue',
            ...     'p k',
            ...     'continue',
            ... ]):
            ...     test_function()
            > <doctest test.test_pdb.test_pdb_await_with_breakpoint[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) b test
            Breakpoint 1 at <doctest test.test_pdb.test_pdb_await_with_breakpoint[1]>:2
            (Pdb) k = await task
            > <doctest test.test_pdb.test_pdb_await_with_breakpoint[1]>(2)test()
            -> x = 2
            (Pdb) n
            > <doctest test.test_pdb.test_pdb_await_with_breakpoint[1]>(3)test()
            -> await asyncio.sleep(0)
            (Pdb) p x
            2
            (Pdb) continue
            > <doctest test.test_pdb.test_pdb_await_with_breakpoint[2]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p k
            42
            (Pdb) continue
            """

        def test_pdb_await_contextvar():
            """Testing await support context vars

            >>> import asyncio
            >>> import contextvars

            >>> var = contextvars.ContextVar('var')

            >>> async def get_var():
            ...     return var.get()

            >>> async def set_var(val):
            ...     var.set(val)
            ...     return var.get()

            >>> async def main():
            ...     var.set(42)
            ...     import pdb
            ...     await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()

            >>> def test_function():
            ...     asyncio.run(main(), loop_factory=asyncio.EventLoop)

            >>> with PdbTestInput([
            ...     'p var.get()',
            ...     'print(await get_var())',
            ...     'print(await asyncio.create_task(set_var(100)))',
            ...     'p var.get()',
            ...     'print(await set_var(99))',
            ...     'p var.get()',
            ...     'print(await get_var())',
            ...     'continue',
            ... ]):
            ...     test_function()
            > <doctest test.test_pdb.test_pdb_await_contextvar[5]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p var.get()
            42
            (Pdb) print(await get_var())
            42
            > <doctest test.test_pdb.test_pdb_await_contextvar[5]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) print(await asyncio.create_task(set_var(100)))
            100
            > <doctest test.test_pdb.test_pdb_await_contextvar[5]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p var.get()
            42
            (Pdb) print(await set_var(99))
            99
            > <doctest test.test_pdb.test_pdb_await_contextvar[5]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) p var.get()
            99
            (Pdb) print(await get_var())
            99
            > <doctest test.test_pdb.test_pdb_await_contextvar[5]>(4)main()
            -> await pdb.Pdb(nosigint=True, readrc=False).set_trace_async()
            (Pdb) continue
            """

    def test_pdb_next_command_for_coroutine():
        """Testing skip unwinding stack on yield for coroutines for "next" command

        >>> from test.support import run_yielding_async_fn, async_yield

        >>> async def test_coro():
        ...     await async_yield(0)
        ...     await async_yield(0)
        ...     await async_yield(0)

        >>> async def test_main():
        ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        ...     await test_coro()

        >>> def test_function():
        ...     run_yielding_async_fn(test_main)
        ...     print("finished")

        >>> with PdbTestInput(['step',
        ...                    'step',
        ...                    'step',
        ...                    'next',
        ...                    'next',
        ...                    'next',
        ...                    'step',
        ...                    'continue']):
        ...     test_function()
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[2]>(2)test_main()
        -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[2]>(3)test_main()
        -> await test_coro()
        (Pdb) step
        --Call--
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(1)test_coro()
        -> async def test_coro():
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(2)test_coro()
        -> await async_yield(0)
        (Pdb) next
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(3)test_coro()
        -> await async_yield(0)
        (Pdb) next
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(4)test_coro()
        -> await async_yield(0)
        (Pdb) next
        Internal StopIteration
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[2]>(3)test_main()
        -> await test_coro()
        (Pdb) step
        --Return--
        > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[2]>(3)test_main()->None
        -> await test_coro()
        (Pdb) continue
        finished
        """

    def test_pdb_next_command_for_asyncgen():
        """Testing skip unwinding stack on yield for coroutines for "next" command

        >>> from test.support import run_yielding_async_fn, async_yield

        >>> async def agen():
        ...     yield 1
        ...     await async_yield(0)
        ...     yield 2

        >>> async def test_coro():
        ...     async for x in agen():
        ...         print(x)

        >>> async def test_main():
        ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        ...     await test_coro()

        >>> def test_function():
        ...     run_yielding_async_fn(test_main)
        ...     print("finished")

        >>> with PdbTestInput(['step',
        ...                    'step',
        ...                    'step',
        ...                    'next',
        ...                    'next',
        ...                    'step',
        ...                    'next',
        ...                    'continue']):
        ...     test_function()
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[3]>(2)test_main()
        -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[3]>(3)test_main()
        -> await test_coro()
        (Pdb) step
        --Call--
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[2]>(1)test_coro()
        -> async def test_coro():
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[2]>(2)test_coro()
        -> async for x in agen():
        (Pdb) next
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[2]>(3)test_coro()
        -> print(x)
        (Pdb) next
        1
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[2]>(2)test_coro()
        -> async for x in agen():
        (Pdb) step
        --Call--
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[1]>(2)agen()
        -> yield 1
        (Pdb) next
        > <doctest test.test_pdb.test_pdb_next_command_for_asyncgen[1]>(3)agen()
        -> await async_yield(0)
        (Pdb) continue
        2
        finished
        """

def test_pdb_return_command_for_generator():
    """Testing no unwinding stack on yield for generators
       for "return" command

    >>> def test_gen():
    ...     yield 0
    ...     return 1
    ...     yield 2

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     it = test_gen()
    ...     try:
    ...         if next(it) != 0:
    ...             raise AssertionError
    ...         next(it)
    ...     except StopIteration as ex:
    ...         if ex.value != 1:
    ...             raise AssertionError
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'step',
    ...                    'step',
    ...                    'return',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(3)test_function()
    -> it = test_gen()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(4)test_function()
    -> try:
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(5)test_function()
    -> if next(it) != 0:
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[0]>(1)test_gen()
    -> def test_gen():
    (Pdb) return
    StopIteration: 1
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(7)test_function()
    -> next(it)
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(8)test_function()
    -> except StopIteration as ex:
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_generator[1]>(9)test_function()
    -> if ex.value != 1:
    (Pdb) continue
    finished
    """

if not SKIP_CORO_TESTS:
    def test_pdb_return_command_for_coroutine():
        """Testing no unwinding stack on yield for coroutines for "return" command

        >>> from test.support import run_yielding_async_fn, async_yield

        >>> async def test_coro():
        ...     await async_yield(0)
        ...     await async_yield(0)
        ...     await async_yield(0)

        >>> async def test_main():
        ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        ...     await test_coro()

        >>> def test_function():
        ...     run_yielding_async_fn(test_main)
        ...     print("finished")

        >>> with PdbTestInput(['step',
        ...                    'step',
        ...                    'step',
        ...                    'next',
        ...                    'continue']):
        ...     test_function()
        > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[2]>(2)test_main()
        -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[2]>(3)test_main()
        -> await test_coro()
        (Pdb) step
        --Call--
        > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(1)test_coro()
        -> async def test_coro():
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(2)test_coro()
        -> await async_yield(0)
        (Pdb) next
        > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(3)test_coro()
        -> await async_yield(0)
        (Pdb) continue
        finished
        """

def test_pdb_until_command_for_generator():
    """Testing no unwinding stack on yield for generators
       for "until" command if target breakpoint is not reached

    >>> def test_gen():
    ...     yield 0
    ...     yield 1
    ...     yield 2

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     for i in test_gen():
    ...         print(i)
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'until 4',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[1]>(3)test_function()
    -> for i in test_gen():
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[0]>(1)test_gen()
    -> def test_gen():
    (Pdb) until 4
    0
    1
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[0]>(4)test_gen()
    -> yield 2
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[0]>(4)test_gen()->2
    -> yield 2
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_until_command_for_generator[1]>(4)test_function()
    -> print(i)
    (Pdb) continue
    2
    finished
    """

if not SKIP_CORO_TESTS:
    def test_pdb_until_command_for_coroutine():
        """Testing no unwinding stack for coroutines
        for "until" command if target breakpoint is not reached

        >>> from test.support import run_yielding_async_fn, async_yield

        >>> async def test_coro():
        ...     print(0)
        ...     await async_yield(0)
        ...     print(1)
        ...     await async_yield(0)
        ...     print(2)
        ...     await async_yield(0)
        ...     print(3)

        >>> async def test_main():
        ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        ...     await test_coro()

        >>> def test_function():
        ...     run_yielding_async_fn(test_main)
        ...     print("finished")

        >>> with PdbTestInput(['step',
        ...                    'step',
        ...                    'until 8',
        ...                    'continue']):
        ...     test_function()
        > <doctest test.test_pdb.test_pdb_until_command_for_coroutine[2]>(2)test_main()
        -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
        (Pdb) step
        > <doctest test.test_pdb.test_pdb_until_command_for_coroutine[2]>(3)test_main()
        -> await test_coro()
        (Pdb) step
        --Call--
        > <doctest test.test_pdb.test_pdb_until_command_for_coroutine[1]>(1)test_coro()
        -> async def test_coro():
        (Pdb) until 8
        0
        1
        2
        > <doctest test.test_pdb.test_pdb_until_command_for_coroutine[1]>(8)test_coro()
        -> print(3)
        (Pdb) continue
        3
        finished
        """

def test_pdb_next_command_in_generator_for_loop():
    """The next command on returning from a generator controlled by a for loop.

    >>> def test_gen():
    ...     yield 0
    ...     return 1

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     for i in test_gen():
    ...         print('value', i)
    ...     x = 123

    >>> with PdbTestInput(['break test_gen',
    ...                    'continue',
    ...                    'next',
    ...                    'next',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break test_gen
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[0]>:2
    (Pdb) continue
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[0]>(2)test_gen()
    -> yield 0
    (Pdb) next
    value 0
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[0]>(3)test_gen()
    -> return 1
    (Pdb) next
    Internal StopIteration: 1
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[1]>(3)test_function()
    -> for i in test_gen():
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[1]>(5)test_function()
    -> x = 123
    (Pdb) continue
    """

def test_pdb_next_command_subiterator():
    """The next command in a generator with a subiterator.

    >>> def test_subgenerator():
    ...     yield 0
    ...     return 1

    >>> def test_gen():
    ...     x = yield from test_subgenerator()
    ...     return x

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     for i in test_gen():
    ...         print('value', i)
    ...     x = 123

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'step',
    ...                    'next',
    ...                    'next',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[2]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[2]>(3)test_function()
    -> for i in test_gen():
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[1]>(1)test_gen()
    -> def test_gen():
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[1]>(2)test_gen()
    -> x = yield from test_subgenerator()
    (Pdb) next
    value 0
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[1]>(3)test_gen()
    -> return x
    (Pdb) next
    Internal StopIteration: 1
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[2]>(3)test_function()
    -> for i in test_gen():
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_subiterator[2]>(5)test_function()
    -> x = 123
    (Pdb) continue
    """

def test_pdb_breakpoint_with_throw():
    """GH-132536: PY_THROW event should not be turned off

    >>> def gen():
    ...    yield 0

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     g = gen()
    ...     try:
    ...         g.throw(TypeError)
    ...     except TypeError:
    ...         pass

    >>> with PdbTestInput([
    ...     'b 7',
    ...     'continue',
    ...     'clear 1',
    ...     'continue',
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_with_throw[1]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) b 7
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_with_throw[1]>:7
    (Pdb) continue
    > <doctest test.test_pdb.test_pdb_breakpoint_with_throw[1]>(7)test_function()
    -> pass
    (Pdb) clear 1
    Deleted breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_with_throw[1]>:7
    (Pdb) continue
    """

def test_pdb_multiline_statement():
    """Test for multiline statement

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'def f(x):',
    ...     '  return x * 2',
    ...     '',
    ...     'val = 2',
    ...     'if val > 0:',
    ...     '  val = f(val)',
    ...     '',
    ...     '',  # empty line should repeat the multi-line statement
    ...     'val',
    ...     'c'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_multiline_statement[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) def f(x):
    ...     return x * 2
    ...
    (Pdb) val = 2
    (Pdb) if val > 0:
    ...     val = f(val)
    ...
    (Pdb)
    (Pdb) val
    8
    (Pdb) c
    """

def test_pdb_closure():
    """Test for all expressions/statements that involve closure

    >>> k = 0
    >>> g = 1
    >>> def test_function():
    ...     x = 2
    ...     g = 3
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'k',
    ...     'g',
    ...     'y = y',
    ...     'global g; g',
    ...     'global g; (lambda: g)()',
    ...     '(lambda: x)()',
    ...     '(lambda: g)()',
    ...     'lst = [n for n in range(10) if (n % x) == 0]',
    ...     'lst',
    ...     'sum(n for n in lst if n > x)',
    ...     'x = 1; raise Exception()',
    ...     'x',
    ...     'def f():',
    ...     '  return x',
    ...     '',
    ...     'f()',
    ...     'c'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_closure[2]>(4)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) k
    0
    (Pdb) g
    3
    (Pdb) y = y
    *** NameError: name 'y' is not defined
    (Pdb) global g; g
    1
    (Pdb) global g; (lambda: g)()
    1
    (Pdb) (lambda: x)()
    2
    (Pdb) (lambda: g)()
    3
    (Pdb) lst = [n for n in range(10) if (n % x) == 0]
    (Pdb) lst
    [0, 2, 4, 6, 8]
    (Pdb) sum(n for n in lst if n > x)
    18
    (Pdb) x = 1; raise Exception()
    *** Exception
    (Pdb) x
    1
    (Pdb) def f():
    ...     return x
    ...
    (Pdb) f()
    1
    (Pdb) c
    """

def test_pdb_show_attribute_and_item():
    """Test for expressions with command prefix

    >>> def test_function():
    ...     n = lambda x: x
    ...     c = {"a": 1}
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'c["a"]',
    ...     'c.get("a")',
    ...     'n(1)',
    ...     'j=1',
    ...     'j+1',
    ...     'r"a"',
    ...     'next(iter([1]))',
    ...     'list((0, 1))',
    ...     'c'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_show_attribute_and_item[0]>(4)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) c["a"]
    1
    (Pdb) c.get("a")
    1
    (Pdb) n(1)
    1
    (Pdb) j=1
    (Pdb) j+1
    2
    (Pdb) r"a"
    'a'
    (Pdb) next(iter([1]))
    1
    (Pdb) list((0, 1))
    [0, 1]
    (Pdb) c
    """

# doctest will modify pdb.set_trace during the test, so we need to backup
# the original function to use it in the test
original_pdb_settrace = pdb.set_trace

def test_pdb_with_inline_breakpoint():
    """Hard-coded breakpoint() calls should invoke the same debugger instance

    >>> def test_function():
    ...     x = 1
    ...     import pdb; pdb.Pdb().set_trace()
    ...     original_pdb_settrace()
    ...     x = 2

    >>> with PdbTestInput(['display x',
    ...                    'n',
    ...                    'n',
    ...                    'n',
    ...                    'n',
    ...                    'undisplay',
    ...                    'c']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_with_inline_breakpoint[0]>(3)test_function()
    -> import pdb; pdb.Pdb().set_trace()
    (Pdb) display x
    display x: 1
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_with_inline_breakpoint[0]>(4)test_function()
    -> original_pdb_settrace()
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_with_inline_breakpoint[0]>(4)test_function()
    -> original_pdb_settrace()
    (Pdb) n
    > <doctest test.test_pdb.test_pdb_with_inline_breakpoint[0]>(5)test_function()
    -> x = 2
    (Pdb) n
    --Return--
    > <doctest test.test_pdb.test_pdb_with_inline_breakpoint[0]>(5)test_function()->None
    -> x = 2
    display x: 2  [old: 1]
    (Pdb) undisplay
    (Pdb) c
    """

def test_pdb_issue_20766():
    """Test for reference leaks when the SIGINT handler is set.

    >>> def test_function():
    ...     i = 1
    ...     while i <= 2:
    ...         sess = pdb.Pdb()
    ...         sess.set_trace(sys._getframe())
    ...         print('pdb %d: %s' % (i, sess._previous_sigint_handler))
    ...         i += 1

    >>> with PdbTestInput(['continue',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_20766[0]>(5)test_function()
    -> sess.set_trace(sys._getframe())
    (Pdb) continue
    pdb 1: <built-in function default_int_handler>
    > <doctest test.test_pdb.test_pdb_issue_20766[0]>(5)test_function()
    -> sess.set_trace(sys._getframe())
    (Pdb) continue
    pdb 2: <built-in function default_int_handler>
    """

def test_pdb_issue_43318():
    """echo breakpoints cleared with filename:lineno

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     print(1)
    ...     print(2)
    ...     print(3)
    ...     print(4)
    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 3',
    ...     'clear <doctest test.test_pdb.test_pdb_issue_43318[0]>:3',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_43318[0]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break 3
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_issue_43318[0]>:3
    (Pdb) clear <doctest test.test_pdb.test_pdb_issue_43318[0]>:3
    Deleted breakpoint 1 at <doctest test.test_pdb.test_pdb_issue_43318[0]>:3
    (Pdb) continue
    1
    2
    3
    4
    """

def test_pdb_issue_gh_91742():
    """See GH-91742

    >>> def test_function():
    ...    __author__ = "pi"
    ...    __version__ = "3.14"
    ...
    ...    def about():
    ...        '''About'''
    ...        print(f"Author: {__author__!r}",
    ...            f"Version: {__version__!r}",
    ...            sep=" ")
    ...
    ...    import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...    about()


    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'step',
    ...     'step',
    ...     'next',
    ...     'next',
    ...     'jump 5',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(11)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(12)test_function()
    -> about()
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(5)about()
    -> def about():
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(7)about()
    -> print(f"Author: {__author__!r}",
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(8)about()
    -> f"Version: {__version__!r}",
    (Pdb) jump 5
    > <doctest test.test_pdb.test_pdb_issue_gh_91742[0]>(5)about()
    -> def about():
    (Pdb) continue
    Author: 'pi' Version: '3.14'
    """

def test_pdb_issue_gh_94215():
    """See GH-94215

    Check that frame_setlineno() does not leak references.

    >>> def test_function():
    ...    def func():
    ...        def inner(v): pass
    ...        inner(
    ...             42
    ...        )
    ...
    ...    import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...    func()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'step',
    ...     'step',
    ...     'next',
    ...     'next',
    ...     'jump 3',
    ...     'next',
    ...     'next',
    ...     'jump 3',
    ...     'next',
    ...     'next',
    ...     'jump 3',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(8)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(9)test_function()
    -> func()
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(2)func()
    -> def func():
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(3)func()
    -> def inner(v): pass
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(4)func()
    -> inner(
    (Pdb) jump 3
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(3)func()
    -> def inner(v): pass
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(4)func()
    -> inner(
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(5)func()
    -> 42
    (Pdb) jump 3
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(3)func()
    -> def inner(v): pass
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(4)func()
    -> inner(
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(5)func()
    -> 42
    (Pdb) jump 3
    > <doctest test.test_pdb.test_pdb_issue_gh_94215[0]>(3)func()
    -> def inner(v): pass
    (Pdb) continue
    """

def test_pdb_issue_gh_101673():
    """See GH-101673

    Make sure ll and switching frames won't revert local variable assignment

    >>> def test_function():
    ...    a = 1
    ...    import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     '!a = 2',
    ...     'll',
    ...     'p a',
    ...     'u',
    ...     'p a',
    ...     'd',
    ...     'p a',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_101673[0]>(3)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) !a = 2
    (Pdb) ll
      1         def test_function():
      2            a = 1
      3  ->        import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) p a
    2
    (Pdb) u
    > <doctest test.test_pdb.test_pdb_issue_gh_101673[1]>(11)<module>()
    -> test_function()
    (Pdb) p a
    *** NameError: name 'a' is not defined
    (Pdb) d
    > <doctest test.test_pdb.test_pdb_issue_gh_101673[0]>(3)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) p a
    2
    (Pdb) continue
    """

def test_pdb_issue_gh_103225():
    """See GH-103225

    Make sure longlist uses 1-based line numbers in frames that correspond to a module

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'longlist',
    ...     'continue'
    ... ]):
    ...     a = 1
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     b = 2
    > <doctest test.test_pdb.test_pdb_issue_gh_103225[0]>(6)<module>()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) longlist
      1     with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
      2         'longlist',
      3         'continue'
      4     ]):
      5         a = 1
      6 ->      import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
      7         b = 2
    (Pdb) continue
    """

def test_pdb_issue_gh_101517():
    """See GH-101517

    Make sure pdb doesn't crash when the exception is caught in a try/except* block

    >>> def test_function():
    ...     try:
    ...         raise KeyError
    ...     except* Exception as e:
    ...         import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'continue'
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_101517[0]>(5)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) continue
    """

def test_pdb_issue_gh_108976():
    """See GH-108976
    Make sure setting f_trace_opcodes = True won't crash pdb
    >>> def test_function():
    ...     import sys
    ...     sys._getframe().f_trace_opcodes = True
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     a = 1
    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'continue'
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_108976[0]>(4)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) continue
    """

def test_pdb_issue_gh_127321():
    """See GH-127321
    breakpoint() should stop at a opcode that has a line number
    >>> def test_function():
    ...     import pdb; pdb_instance = pdb.Pdb(nosigint=True, readrc=False)
    ...     [1, 2] and pdb_instance.set_trace()
    ...     a = 1
    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'continue'
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_127321[0]>(4)test_function()
    -> a = 1
    (Pdb) continue
    """


def test_pdb_issue_gh_80731():
    """See GH-80731

    pdb should correctly print exception info if in an except block.

    >>> with PdbTestInput([  # doctest: +ELLIPSIS
    ...     'import sys',
    ...     'sys.exc_info()',
    ...     'continue'
    ... ]):
    ...     try:
    ...         raise ValueError('Correct')
    ...     except ValueError:
    ...         import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    > <doctest test.test_pdb.test_pdb_issue_gh_80731[0]>(9)<module>()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) import sys
    (Pdb) sys.exc_info()
    (<class 'ValueError'>, ValueError('Correct'), <traceback object at ...>)
    (Pdb) continue
    """


def test_pdb_ambiguous_statements():
    """See GH-104301

    Make sure that ambiguous statements prefixed by '!' are properly disambiguated

    >>> with PdbTestInput([
    ...     's',         # step to the print line
    ...     '! n = 42',  # disambiguated statement: reassign the name n
    ...     'n',         # advance the debugger into the print()
    ...     'continue'
    ... ]):
    ...     n = -1
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     print(f"The value of n is {n}")
    > <doctest test.test_pdb.test_pdb_ambiguous_statements[0]>(8)<module>()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) s
    > <doctest test.test_pdb.test_pdb_ambiguous_statements[0]>(9)<module>()
    -> print(f"The value of n is {n}")
    (Pdb) ! n = 42
    (Pdb) n
    The value of n is 42
    > <doctest test.test_pdb.test_pdb_ambiguous_statements[0]>(1)<module>()
    -> with PdbTestInput([
    (Pdb) continue
    """

def test_pdb_f_trace_lines():
    """GH-80675

    pdb should work even if f_trace_lines is set to False on some frames.

    >>> def test_function():
    ...     import sys
    ...     frame = sys._getframe()
    ...     frame.f_trace_lines = False
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     if frame.f_trace_lines != False:
    ...         print("f_trace_lines is not reset after continue!")

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'continue'
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_f_trace_lines[0]>(5)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) continue
    """

def test_pdb_frame_refleak():
    """
    pdb should not leak reference to frames

    >>> def frame_leaker(container):
    ...     import sys
    ...     container.append(sys._getframe())
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     pass

    >>> def test_function():
    ...     import gc
    ...     container = []
    ...     frame_leaker(container)  # c
    ...     print(len(gc.get_referrers(container[0])))
    ...     container = []
    ...     frame_leaker(container)  # n c
    ...     print(len(gc.get_referrers(container[0])))
    ...     container = []
    ...     frame_leaker(container)  # r c
    ...     print(len(gc.get_referrers(container[0])))

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'continue',
    ...     'next',
    ...     'continue',
    ...     'return',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_frame_refleak[0]>(4)frame_leaker()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) continue
    1
    > <doctest test.test_pdb.test_pdb_frame_refleak[0]>(4)frame_leaker()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_frame_refleak[0]>(5)frame_leaker()
    -> pass
    (Pdb) continue
    1
    > <doctest test.test_pdb.test_pdb_frame_refleak[0]>(4)frame_leaker()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) return
    --Return--
    > <doctest test.test_pdb.test_pdb_frame_refleak[0]>(5)frame_leaker()->None
    -> pass
    (Pdb) continue
    1
    """

def test_pdb_function_break():
    """Testing the line number of break on function

    >>> def foo(): pass

    >>> def bar():
    ...
    ...     pass

    >>> def boo():
    ...     # comments
    ...     global x
    ...     x = 1

    >>> def gen():
    ...     yield 42

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()

    >>> with PdbTestInput([  # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
    ...     'break foo',
    ...     'break bar',
    ...     'break boo',
    ...     'break gen',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_function_break[4]>(2)test_function()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) break foo
    Breakpoint ... at <doctest test.test_pdb.test_pdb_function_break[0]>:1
    (Pdb) break bar
    Breakpoint ... at <doctest test.test_pdb.test_pdb_function_break[1]>:3
    (Pdb) break boo
    Breakpoint ... at <doctest test.test_pdb.test_pdb_function_break[2]>:4
    (Pdb) break gen
    Breakpoint ... at <doctest test.test_pdb.test_pdb_function_break[3]>:2
    (Pdb) continue
    """

def test_pdb_issue_gh_65052():
    """See GH-65052

    args, retval and display should not crash if the object is not displayable
    >>> class A:
    ...     def __new__(cls):
    ...         import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...         return object.__new__(cls)
    ...     def __init__(self):
    ...         import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...         self.a = 1
    ...     def __repr__(self):
    ...         return self.a

    >>> def test_function():
    ...     A()
    >>> with PdbTestInput([  # doctest: +ELLIPSIS +NORMALIZE_WHITESPACE
    ...     's',
    ...     's',
    ...     'retval',
    ...     'continue',
    ...     'args',
    ...     'display self',
    ...     'display',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_issue_gh_65052[0]>(3)__new__()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) s
    > <doctest test.test_pdb.test_pdb_issue_gh_65052[0]>(4)__new__()
    -> return object.__new__(cls)
    (Pdb) s
    --Return--
    > <doctest test.test_pdb.test_pdb_issue_gh_65052[0]>(4)__new__()-><A instance at ...>
    -> return object.__new__(cls)
    (Pdb) retval
    *** repr(retval) failed: AttributeError: 'A' object has no attribute 'a' ***
    (Pdb) continue
    > <doctest test.test_pdb.test_pdb_issue_gh_65052[0]>(6)__init__()
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) args
    self = *** repr(self) failed: AttributeError: 'A' object has no attribute 'a' ***
    (Pdb) display self
    display self: *** repr(self) failed: AttributeError: 'A' object has no attribute 'a' ***
    (Pdb) display
    Currently displaying:
    self: *** repr(self) failed: AttributeError: 'A' object has no attribute 'a' ***
    (Pdb) continue
    """


@support.force_not_colorized_test_class
@support.requires_subprocess()
class PdbTestCase(unittest.TestCase):
    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    @unittest.skipIf(sys.flags.safe_path,
                     'PYTHONSAFEPATH changes default sys.path')
    def _run_pdb(self, pdb_args, commands,
                 expected_returncode=0,
                 extra_env=None):
        self.addCleanup(os_helper.rmtree, '__pycache__')
        cmd = [sys.executable, '-m', 'pdb'] + pdb_args
        if extra_env is not None:
            env = os.environ | extra_env
        else:
            env = os.environ
        with subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                stderr=subprocess.PIPE,
                env = {**env, 'PYTHONIOENCODING': 'utf-8'}
        ) as proc:
            stdout, stderr = proc.communicate(str.encode(commands))
        stdout = bytes.decode(stdout) if isinstance(stdout, bytes) else stdout
        stderr = bytes.decode(stderr) if isinstance(stderr, bytes) else stderr
        self.assertEqual(
            proc.returncode,
            expected_returncode,
            f"Unexpected return code\nstdout: {stdout}\nstderr: {stderr}"
        )
        return stdout, stderr

    def run_pdb_script(self, script, commands,
                       expected_returncode=0,
                       extra_env=None,
                       script_args=None,
                       pdbrc=None,
                       remove_home=False):
        """Run 'script' lines with pdb and the pdb 'commands'."""
        filename = 'main.py'
        with open(filename, 'w') as f:
            f.write(textwrap.dedent(script))

        if pdbrc is not None:
            with open('.pdbrc', 'w') as f:
                f.write(textwrap.dedent(pdbrc))
            self.addCleanup(os_helper.unlink, '.pdbrc')
        self.addCleanup(os_helper.unlink, filename)

        with os_helper.EnvironmentVarGuard() as env:
            if remove_home:
                env.unset('HOME')
            if script_args is None:
                script_args = []
            stdout, stderr = self._run_pdb([filename] + script_args, commands, expected_returncode, extra_env)
        return stdout, stderr

    def run_pdb_module(self, script, commands):
        """Runs the script code as part of a module"""
        self.module_name = 't_main'
        os_helper.rmtree(self.module_name)
        main_file = self.module_name + '/__main__.py'
        init_file = self.module_name + '/__init__.py'
        os.mkdir(self.module_name)
        with open(init_file, 'w') as f:
            pass
        with open(main_file, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.rmtree, self.module_name)
        return self._run_pdb(['-m', self.module_name], commands)

    def _assert_find_function(self, file_content, func_name, expected):
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(file_content)

        expected = None if not expected else (
            expected[0], os_helper.TESTFN, expected[1])
        self.assertEqual(
            expected, pdb.find_function(func_name, os_helper.TESTFN))

    def test_find_function_empty_file(self):
        self._assert_find_function(b'', 'foo', None)

    def test_find_function_found(self):
        self._assert_find_function(
            """\
def foo():
    pass

def bœr():
    pass

def quux():
    pass
""".encode(),
            'bœr',
            ('bœr', 5),
        )

    def test_find_function_found_with_encoding_cookie(self):
        self._assert_find_function(
            """\
# coding: iso-8859-15
def foo():
    pass

def bœr():
    pass

def quux():
    pass
""".encode('iso-8859-15'),
            'bœr',
            ('bœr', 6),
        )

    def test_find_function_found_with_bom(self):
        self._assert_find_function(
            codecs.BOM_UTF8 + """\
def bœr():
    pass
""".encode(),
            'bœr',
            ('bœr', 2),
        )

    def test_spec(self):
        # Test that __main__.__spec__ is set to None when running a script
        script = """
            import __main__
            print(__main__.__spec__)
        """

        commands = "continue"

        stdout, _ = self.run_pdb_script(script, commands)
        self.assertIn('None', stdout)

    def test_find_function_first_executable_line(self):
        code = textwrap.dedent("""\
            def foo(): pass

            def bar():
                pass  # line 4

            def baz():
                # comment
                pass  # line 8

            def mul():
                # code on multiple lines
                code = compile(   # line 12
                    'def f()',
                    '<string>',
                    'exec',
                )
        """).encode()

        self._assert_find_function(code, 'foo', ('foo', 1))
        self._assert_find_function(code, 'bar', ('bar', 4))
        self._assert_find_function(code, 'baz', ('baz', 8))
        self._assert_find_function(code, 'mul', ('mul', 12))

    def test_issue7964(self):
        # open the file as binary so we can force \r\n newline
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'print("testing my pdb")\r\n')
        cmd = [sys.executable, '-m', 'pdb', os_helper.TESTFN]
        proc = subprocess.Popen(cmd,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            )
        self.addCleanup(proc.stdout.close)
        stdout, stderr = proc.communicate(b'quit\n')
        self.assertNotIn(b'SyntaxError', stdout,
                         "Got a syntax error running test script under PDB")

    def test_issue46434(self):
        # Temporarily patch in an extra help command which doesn't have a
        # docstring to emulate what happens in an embeddable distribution
        script = """
            def do_testcmdwithnodocs(self, arg):
                pass

            import pdb
            pdb.Pdb.do_testcmdwithnodocs = do_testcmdwithnodocs
        """
        commands = """
            continue
            help testcmdwithnodocs
        """
        stdout, stderr = self.run_pdb_script(script, commands)
        output = (stdout or '') + (stderr or '')
        self.assertNotIn('AttributeError', output,
                         'Calling help on a command with no docs should be handled gracefully')
        self.assertIn("*** No help for 'testcmdwithnodocs'; __doc__ string missing", output,
                      'Calling help on a command with no docs should print an error')

    def test_issue13183(self):
        script = """
            from bar import bar

            def foo():
                bar()

            def nope():
                pass

            def foobar():
                foo()
                nope()

            foobar()
        """
        commands = """
            from bar import bar
            break bar
            continue
            step
            step
            quit
        """
        bar = """
            def bar():
                pass
        """
        with open('bar.py', 'w') as f:
            f.write(textwrap.dedent(bar))
        self.addCleanup(os_helper.unlink, 'bar.py')
        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertTrue(
            any('main.py(5)foo()->None' in l for l in stdout.splitlines()),
            'Fail to step into the caller after a return')

    def test_issue13120(self):
        # Invoking "continue" on a non-main thread triggered an exception
        # inside signal.signal.

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(textwrap.dedent("""
                import threading
                import pdb

                def start_pdb():
                    pdb.Pdb(readrc=False).set_trace()
                    x = 1
                    y = 1

                t = threading.Thread(target=start_pdb)
                t.start()""").encode('ascii'))
        cmd = [sys.executable, '-u', os_helper.TESTFN]
        proc = subprocess.Popen(cmd,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env={**os.environ, 'PYTHONIOENCODING': 'utf-8'}
            )
        self.addCleanup(proc.stdout.close)
        stdout, stderr = proc.communicate(b'cont\n')
        self.assertNotIn(b'Error', stdout,
                         "Got an error running test script under PDB")

    def test_issue36250(self):

        with open(os_helper.TESTFN, 'wb') as f:
            f.write(textwrap.dedent("""
                import threading
                import pdb

                evt = threading.Event()

                def start_pdb():
                    evt.wait()
                    pdb.Pdb(readrc=False).set_trace()

                t = threading.Thread(target=start_pdb)
                t.start()
                pdb.Pdb(readrc=False).set_trace()
                evt.set()
                t.join()""").encode('ascii'))
        cmd = [sys.executable, '-u', os_helper.TESTFN]
        proc = subprocess.Popen(cmd,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env = {**os.environ, 'PYTHONIOENCODING': 'utf-8'}
            )
        self.addCleanup(proc.stdout.close)
        stdout, stderr = proc.communicate(b'cont\ncont\n')
        self.assertNotIn(b'Error', stdout,
                         "Got an error running test script under PDB")

    def test_issue16180(self):
        # A syntax error in the debuggee.
        script = "def f: pass\n"
        commands = ''
        expected = "SyntaxError:"
        stdout, stderr = self.run_pdb_script(
            script, commands
        )
        self.assertIn(expected, stderr,
            '\n\nExpected:\n{}\nGot:\n{}\n'
            'Fail to handle a syntax error in the debuggee.'
            .format(expected, stderr))

    def test_issue84583(self):
        # A syntax error from ast.literal_eval should not make pdb exit.
        script = "import ast; ast.literal_eval('')\n"
        commands = """
            continue
            where
            quit
        """
        stdout, stderr = self.run_pdb_script(script, commands)
        # The code should appear 3 times in the stdout/stderr:
        # 1. when pdb starts (stdout)
        # 2. when the exception is raised, in trackback (stderr)
        # 3. in where command (stdout)
        self.assertEqual(stdout.count("ast.literal_eval('')"), 2)
        self.assertEqual(stderr.count("ast.literal_eval('')"), 1)

    def test_issue26053(self):
        # run command of pdb prompt echoes the correct args
        script = "print('hello')"
        commands = """
            continue
            run a b c
            run d e f
            quit
        """
        stdout, stderr = self.run_pdb_script(script, commands)
        res = '\n'.join([x.strip() for x in stdout.splitlines()])
        self.assertRegex(res, "Restarting .* with arguments:\na b c")
        self.assertRegex(res, "Restarting .* with arguments:\nd e f")

    def test_issue58956(self):
        # Set a breakpoint in a function that already exists on the call stack
        # should enable the trace function for the frame.
        script = """
            import bar
            def foo():
                ret = bar.bar()
                pass
            foo()
        """
        commands = """
            b bar.bar
            c
            b main.py:5
            c
            p ret
            quit
        """
        bar = """
            def bar():
                return 42
        """
        with open('bar.py', 'w') as f:
            f.write(textwrap.dedent(bar))
        self.addCleanup(os_helper.unlink, 'bar.py')
        stdout, stderr = self.run_pdb_script(script, commands)
        lines = stdout.splitlines()
        self.assertIn('-> pass', lines)
        self.assertIn('(Pdb) 42', lines)

    def test_step_into_botframe(self):
        # gh-125422
        # pdb should not be able to step into the botframe (bdb.py)
        script = "x = 1"
        commands = """
            step
            step
            step
            quit
        """
        stdout, _ = self.run_pdb_script(script, commands)
        self.assertIn("The program finished", stdout)
        self.assertNotIn("bdb.py", stdout)

    def test_pdbrc_basic(self):
        script = textwrap.dedent("""
            a = 1
            b = 2
        """)

        pdbrc = textwrap.dedent("""
            # Comments should be fine
            n
            p f"{a+8=}"
        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc=pdbrc, remove_home=True)
        self.assertNotIn("SyntaxError", stdout)
        self.assertIn("a+8=9", stdout)
        self.assertIn("-> b = 2", stdout)

    def test_pdbrc_empty_line(self):
        """Test that empty lines in .pdbrc are ignored."""

        script = textwrap.dedent("""
            a = 1
            b = 2
            c = 3
        """)

        pdbrc = textwrap.dedent("""
            n

        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc=pdbrc, remove_home=True)
        self.assertIn("b = 2", stdout)
        self.assertNotIn("c = 3", stdout)

    def test_pdbrc_alias(self):
        script = textwrap.dedent("""
            class A:
                def __init__(self):
                    self.attr = 1
            a = A()
            b = 2
        """)

        pdbrc = textwrap.dedent("""
            alias pi for k in %1.__dict__.keys(): print(f"%1.{k} = {%1.__dict__[k]}")
            until 6
            pi a
        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc=pdbrc, remove_home=True)
        self.assertIn("a.attr = 1", stdout)

    def test_pdbrc_semicolon(self):
        script = textwrap.dedent("""
            class A:
                def __init__(self):
                    self.attr = 1
            a = A()
            b = 2
        """)

        pdbrc = textwrap.dedent("""
            b 5;;c;;n
        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc=pdbrc, remove_home=True)
        self.assertIn("-> b = 2", stdout)

    def test_pdbrc_commands(self):
        script = textwrap.dedent("""
            class A:
                def __init__(self):
                    self.attr = 1
            a = A()
            b = 2
        """)

        pdbrc = textwrap.dedent("""
            b 6
            commands 1 ;; p a;; end
            c
        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc=pdbrc, remove_home=True)
        self.assertIn("<__main__.A object at", stdout)

    def test_readrc_kwarg(self):
        script = textwrap.dedent("""
            print('hello')
        """)

        stdout, stderr = self.run_pdb_script(script, 'q\n', pdbrc='invalid', remove_home=True)
        self.assertIn("NameError: name 'invalid' is not defined", stdout)

    def test_readrc_homedir(self):
        with os_helper.EnvironmentVarGuard() as env:
            env.unset("HOME")
            with os_helper.temp_dir() as temp_dir, patch("os.path.expanduser"):
                rc_path = os.path.join(temp_dir, ".pdbrc")
                os.path.expanduser.return_value = rc_path
                with open(rc_path, "w") as f:
                    f.write("invalid")
                self.assertEqual(pdb.Pdb().rcLines[0], "invalid")

    def test_header(self):
        stdout = StringIO()
        header = 'Nobody expects... blah, blah, blah'
        with ExitStack() as resources:
            resources.enter_context(patch('sys.stdout', stdout))
            # patch pdb.Pdb.set_trace() to avoid entering the debugger
            resources.enter_context(patch.object(pdb.Pdb, 'set_trace'))
            # We need to manually clear pdb.Pdb._last_pdb_instance so a
            # new instance with stdout redirected could be created when
            # pdb.set_trace() is called.
            pdb.Pdb._last_pdb_instance = None
            pdb.set_trace(header=header)
        self.assertEqual(stdout.getvalue(), header + '\n')

    def test_run_module(self):
        script = """print("SUCCESS")"""
        commands = """
            continue
            quit
        """
        stdout, stderr = self.run_pdb_module(script, commands)
        self.assertTrue(any("SUCCESS" in l for l in stdout.splitlines()), stdout)

    def test_module_is_run_as_main(self):
        script = """
            if __name__ == '__main__':
                print("SUCCESS")
        """
        commands = """
            continue
            quit
        """
        stdout, stderr = self.run_pdb_module(script, commands)
        self.assertTrue(any("SUCCESS" in l for l in stdout.splitlines()), stdout)

    def test_run_module_with_args(self):
        commands = """
            continue
        """
        self._run_pdb(["calendar", "-m"], commands, expected_returncode=2)

        stdout, _ = self._run_pdb(["-m", "calendar", "1"], commands)
        self.assertIn("December", stdout)

        stdout, _ = self._run_pdb(["-m", "calendar", "--type", "text"], commands)
        self.assertIn("December", stdout)

    def test_run_script_with_args(self):
        script = """
            import sys
            print(sys.argv[1:])
        """
        commands = """
            continue
            quit
        """

        stdout, stderr = self.run_pdb_script(script, commands, script_args=["--bar", "foo"])
        self.assertIn("['--bar', 'foo']", stdout)

    def test_breakpoint(self):
        script = """
            if __name__ == '__main__':
                pass
                print("SUCCESS")
                pass
        """
        commands = """
            b 3
            quit
        """
        stdout, stderr = self.run_pdb_module(script, commands)
        self.assertTrue(any("Breakpoint 1 at" in l for l in stdout.splitlines()), stdout)
        self.assertTrue(all("SUCCESS" not in l for l in stdout.splitlines()), stdout)

    def test_run_pdb_with_pdb(self):
        commands = """
            c
            quit
        """
        stdout, stderr = self._run_pdb(["-m", "pdb"], commands)
        self.assertIn(
            pdb._usage,
            stdout.replace('\r', '')  # remove \r for windows
        )

    def test_module_without_a_main(self):
        module_name = 't_main'
        os_helper.rmtree(module_name)
        init_file = module_name + '/__init__.py'
        os.mkdir(module_name)
        with open(init_file, 'w'):
            pass
        self.addCleanup(os_helper.rmtree, module_name)
        stdout, stderr = self._run_pdb(
            ['-m', module_name], "", expected_returncode=1
        )
        self.assertIn("ImportError: No module named t_main.__main__;", stdout)

    def test_package_without_a_main(self):
        pkg_name = 't_pkg'
        module_name = 't_main'
        os_helper.rmtree(pkg_name)
        modpath = pkg_name + '/' + module_name
        os.makedirs(modpath)
        with open(modpath + '/__init__.py', 'w'):
            pass
        self.addCleanup(os_helper.rmtree, pkg_name)
        stdout, stderr = self._run_pdb(
            ['-m', modpath.replace('/', '.')], "", expected_returncode=1
        )
        self.assertIn(
            "'t_pkg.t_main' is a package and cannot be directly executed",
            stdout)

    def test_nonexistent_module(self):
        assert not os.path.exists(os_helper.TESTFN)
        stdout, stderr = self._run_pdb(["-m", os_helper.TESTFN], "", expected_returncode=1)
        self.assertIn(f"ImportError: No module named {os_helper.TESTFN}", stdout)

    def test_dir_as_script(self):
        with os_helper.temp_dir() as temp_dir:
            stdout, stderr = self._run_pdb([temp_dir], "", expected_returncode=1)
            self.assertIn(f"Error: {temp_dir} is a directory", stdout)

    def test_invalid_cmd_line_options(self):
        stdout, stderr = self._run_pdb(["-c"], "", expected_returncode=2)
        self.assertIn(f"pdb: error: argument -c/--command: expected one argument", stderr.split('\n')[1])
        stdout, stderr = self._run_pdb(["--spam", "-m", "pdb"], "", expected_returncode=2)
        self.assertIn(f"pdb: error: unrecognized arguments: --spam", stderr.split('\n')[1])

    def test_blocks_at_first_code_line(self):
        script = """
                #This is a comment, on line 2

                print("SUCCESS")
        """
        commands = """
            quit
        """
        stdout, stderr = self.run_pdb_module(script, commands)
        self.assertTrue(any("__main__.py(4)<module>()"
                            in l for l in stdout.splitlines()), stdout)

    def test_file_modified_after_execution(self):
        script = """
            print("hello")
        """

        # the time.sleep is needed for low-resolution filesystems like HFS+
        commands = """
            filename = $_frame.f_code.co_filename
            f = open(filename, "w")
            f.write("print('goodbye')")
            import time; time.sleep(1)
            f.close()
            ll
        """

        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertIn("WARNING:", stdout)
        self.assertIn("was edited", stdout)

    def test_file_modified_and_immediately_restarted(self):
        script = """
            print("hello")
        """

        # the time.sleep is needed for low-resolution filesystems like HFS+
        commands = """
            filename = $_frame.f_code.co_filename
            f = open(filename, "w")
            f.write("print('goodbye')")
            import time; time.sleep(1)
            f.close()
            restart
        """

        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertNotIn("WARNING:", stdout)
        self.assertNotIn("was edited", stdout)

    def test_file_modified_after_execution_with_multiple_instances(self):
        # the time.sleep is needed for low-resolution filesystems like HFS+
        script = """
            import pdb; pdb.Pdb().set_trace()
            with open(__file__, "w") as f:
                f.write("print('goodbye')\\n" * 5)
                import time; time.sleep(1)
            import pdb; pdb.Pdb().set_trace()
        """

        commands = """
            continue
            continue
        """

        filename = 'main.py'
        with open(filename, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.unlink, filename)
        self.addCleanup(os_helper.rmtree, '__pycache__')
        cmd = [sys.executable, filename]
        with subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                stderr=subprocess.PIPE,
                env = {**os.environ, 'PYTHONIOENCODING': 'utf-8'},
        ) as proc:
            stdout, _ = proc.communicate(str.encode(commands))
        stdout = stdout and bytes.decode(stdout)

        self.assertEqual(proc.returncode, 0)
        self.assertIn("WARNING:", stdout)
        self.assertIn("was edited", stdout)

    def test_file_modified_after_execution_with_restart(self):
        script = """
            import random
            # Any code with a source to step into so this script is not checked
            # for changes when it's being changed
            random.randint(1, 4)
            print("hello")
        """

        commands = """
            ll
            n
            s
            filename = $_frame.f_back.f_code.co_filename
            def change_file(content, filename):
                with open(filename, "w") as f:
                    f.write(f"print({content})")

            change_file('world', filename)
            restart
            ll
        """

        stdout, stderr = self.run_pdb_script(script, commands)
        # Make sure the code is running correctly and the file is edited
        self.assertIn("hello", stdout)
        self.assertIn("world", stdout)
        # The file was edited, but restart should clear the state and consider
        # the file as up to date
        self.assertNotIn("WARNING:", stdout)

    def test_post_mortem_restart(self):
        script = """
            def foo():
                raise ValueError("foo")
            foo()
        """

        commands = """
            continue
            restart
            continue
            quit
        """

        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertIn("Restarting", stdout)

    def test_relative_imports(self):
        self.module_name = 't_main'
        os_helper.rmtree(self.module_name)
        main_file = self.module_name + '/__main__.py'
        init_file = self.module_name + '/__init__.py'
        module_file = self.module_name + '/module.py'
        self.addCleanup(os_helper.rmtree, self.module_name)
        os.mkdir(self.module_name)
        with open(init_file, 'w') as f:
            f.write(textwrap.dedent("""
                top_var = "VAR from top"
            """))
        with open(main_file, 'w') as f:
            f.write(textwrap.dedent("""
                from . import top_var
                from .module import var
                from . import module
                pass # We'll stop here and print the vars
            """))
        with open(module_file, 'w') as f:
            f.write(textwrap.dedent("""
                var = "VAR from module"
                var2 = "second var"
            """))
        commands = """
            b 5
            c
            p top_var
            p var
            p module.var2
            quit
        """
        stdout, _ = self._run_pdb(['-m', self.module_name], commands)
        self.assertTrue(any("VAR from module" in l for l in stdout.splitlines()), stdout)
        self.assertTrue(any("VAR from top" in l for l in stdout.splitlines()))
        self.assertTrue(any("second var" in l for l in stdout.splitlines()))

    def test_relative_imports_on_plain_module(self):
        # Validates running a plain module. See bpo32691
        self.module_name = 't_main'
        os_helper.rmtree(self.module_name)
        main_file = self.module_name + '/runme.py'
        init_file = self.module_name + '/__init__.py'
        module_file = self.module_name + '/module.py'
        self.addCleanup(os_helper.rmtree, self.module_name)
        os.mkdir(self.module_name)
        with open(init_file, 'w') as f:
            f.write(textwrap.dedent("""
                top_var = "VAR from top"
            """))
        with open(main_file, 'w') as f:
            f.write(textwrap.dedent("""
                from . import module
                pass # We'll stop here and print the vars
            """))
        with open(module_file, 'w') as f:
            f.write(textwrap.dedent("""
                var = "VAR from module"
            """))
        commands = """
            b 3
            c
            p module.var
            quit
        """
        stdout, _ = self._run_pdb(['-m', self.module_name + '.runme'], commands)
        self.assertTrue(any("VAR from module" in l for l in stdout.splitlines()), stdout)

    def test_errors_in_command(self):
        commands = "\n".join([
            'print(]',
            'debug print(',
            'debug doesnotexist',
            'c',
        ])
        stdout, _ = self.run_pdb_script('pass', commands + '\n')

        self.assertEqual(stdout.splitlines()[1:], [
            '-> pass',
            "(Pdb) *** SyntaxError: closing parenthesis ']' does not match opening "
            "parenthesis '('",

            '(Pdb) ENTERING RECURSIVE DEBUGGER',
            '*** SyntaxError: \'(\' was never closed',
            'LEAVING RECURSIVE DEBUGGER',

            '(Pdb) ENTERING RECURSIVE DEBUGGER',
            '> <string>(1)<module>()',
            "((Pdb)) *** NameError: name 'doesnotexist' is not defined",
            'LEAVING RECURSIVE DEBUGGER',
            '(Pdb) ',
        ])

    def test_issue34266(self):
        '''do_run handles exceptions from parsing its arg'''
        def check(bad_arg, msg):
            commands = "\n".join([
                f'run {bad_arg}',
                'q',
            ])
            stdout, _ = self.run_pdb_script('pass', commands + '\n')
            self.assertEqual(stdout.splitlines()[1:], [
                '-> pass',
                f'(Pdb) *** Cannot run {bad_arg}: {msg}',
                '(Pdb) ',
            ])
        check('\\', 'No escaped character')
        check('"', 'No closing quotation')

    def test_issue42384(self):
        '''When running `python foo.py` sys.path[0] is an absolute path. `python -m pdb foo.py` should behave the same'''
        script = textwrap.dedent("""
            import sys
            print('sys.path[0] is', sys.path[0])
        """)
        commands = 'c\nq'

        with os_helper.temp_cwd() as cwd:
            expected = f'(Pdb) sys.path[0] is {os.path.realpath(cwd)}'

            stdout, stderr = self.run_pdb_script(script, commands)

            self.assertEqual(stdout.split('\n')[2].rstrip('\r'), expected)

    @os_helper.skip_unless_symlink
    def test_issue42384_symlink(self):
        '''When running `python foo.py` sys.path[0] resolves symlinks. `python -m pdb foo.py` should behave the same'''
        script = textwrap.dedent("""
            import sys
            print('sys.path[0] is', sys.path[0])
        """)
        commands = 'c\nq'

        with os_helper.temp_cwd() as cwd:
            cwd = os.path.realpath(cwd)
            dir_one = os.path.join(cwd, 'dir_one')
            dir_two = os.path.join(cwd, 'dir_two')
            expected = f'(Pdb) sys.path[0] is {dir_one}'

            os.mkdir(dir_one)
            with open(os.path.join(dir_one, 'foo.py'), 'w') as f:
                f.write(script)
            os.mkdir(dir_two)
            os.symlink(os.path.join(dir_one, 'foo.py'), os.path.join(dir_two, 'foo.py'))

            stdout, stderr = self._run_pdb([os.path.join('dir_two', 'foo.py')], commands)

            self.assertEqual(stdout.split('\n')[2].rstrip('\r'), expected)

    def test_safe_path(self):
        """ With safe_path set, pdb should not mangle sys.path[0]"""

        script = textwrap.dedent("""
            import sys
            import random
            print('sys.path[0] is', sys.path[0])
        """)
        commands = 'c\n'


        with os_helper.temp_cwd() as cwd:
            stdout, _ = self.run_pdb_script(script, commands, extra_env={'PYTHONSAFEPATH': '1'})

            unexpected = f'sys.path[0] is {os.path.realpath(cwd)}'
            self.assertNotIn(unexpected, stdout)

    def test_issue42383(self):
        with os_helper.temp_cwd() as cwd:
            with open('foo.py', 'w') as f:
                s = textwrap.dedent("""
                    print('The correct file was executed')

                    import os
                    os.chdir("subdir")
                """)
                f.write(s)

            subdir = os.path.join(cwd, 'subdir')
            os.mkdir(subdir)
            os.mkdir(os.path.join(subdir, 'subdir'))
            wrong_file = os.path.join(subdir, 'foo.py')

            with open(wrong_file, 'w') as f:
                f.write('print("The wrong file was executed")')

            stdout, stderr = self._run_pdb(['foo.py'], 'c\nc\nq')
            expected = '(Pdb) The correct file was executed'
            self.assertEqual(stdout.split('\n')[6].rstrip('\r'), expected)

    def test_gh_94215_crash(self):
        script = """\
            def func():
                def inner(v): pass
                inner(
                    42
                )
            func()
        """
        commands = textwrap.dedent("""
            break func
            continue
            next
            next
            jump 2
        """)
        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertFalse(stderr)

    def test_gh_93696_frozen_list(self):
        frozen_src = """
        def func():
            x = "Sentinel string for gh-93696"
            print(x)
        """
        host_program = """
        import os
        import sys

        def _create_fake_frozen_module():
            with open('gh93696.py') as f:
                src = f.read()

            # this function has a co_filename as if it were in a frozen module
            dummy_mod = compile(src, "<frozen gh93696>", "exec")
            func_code = dummy_mod.co_consts[0]

            mod = type(sys)("gh93696")
            mod.func = type(lambda: None)(func_code, mod.__dict__)
            mod.__file__ = 'gh93696.py'

            return mod

        mod = _create_fake_frozen_module()
        mod.func()
        """
        commands_list = """
            break 20
            continue
            step
            break 4
            list
            quit
        """
        commands_longlist = """
            break 20
            continue
            step
            break 4
            longlist
            quit
        """
        with open('gh93696.py', 'w') as f:
            f.write(textwrap.dedent(frozen_src))

        with open('gh93696_host.py', 'w') as f:
            f.write(textwrap.dedent(host_program))

        self.addCleanup(os_helper.unlink, 'gh93696.py')
        self.addCleanup(os_helper.unlink, 'gh93696_host.py')

        # verify that pdb found the source of the "frozen" function and it
        # shows the breakpoint at the correct line for both list and longlist
        for commands in (commands_list, commands_longlist):
            stdout, _ = self._run_pdb(["gh93696_host.py"], commands)
            self.assertIn('x = "Sentinel string for gh-93696"', stdout, "Sentinel statement not found")
            self.assertIn('4 B', stdout, "breakpoint not found")
            self.assertIn('-> def func():', stdout, "stack entry not found")

    def test_empty_file(self):
        script = ''
        commands = 'q\n'
        # We check that pdb stopped at line 0, but anything reasonable
        # is acceptable here, as long as it does not halt
        stdout, _ = self.run_pdb_script(script, commands)
        self.assertIn('main.py(0)', stdout)
        stdout, _ = self.run_pdb_module(script, commands)
        self.assertIn('__main__.py(0)', stdout)

    def test_non_utf8_encoding(self):
        script_dir = os.path.join(os.path.dirname(__file__), 'encoded_modules')
        for filename in os.listdir(script_dir):
            if filename.endswith(".py"):
                self._run_pdb([os.path.join(script_dir, filename)], 'q')

    def test_zipapp(self):
        with os_helper.temp_dir() as temp_dir:
            os.mkdir(os.path.join(temp_dir, 'source'))
            script = textwrap.dedent(
                """
                def f(x):
                    return x + 1
                f(21 + 21)
                """
            )
            with open(os.path.join(temp_dir, 'source', '__main__.py'), 'w') as f:
                f.write(script)
            zipapp.create_archive(os.path.join(temp_dir, 'source'),
                                  os.path.join(temp_dir, 'zipapp.pyz'))
            stdout, _ = self._run_pdb([os.path.join(temp_dir, 'zipapp.pyz')], '\n'.join([
                'b f',
                'c',
                'p x',
                'q'
            ]))
            self.assertIn('42', stdout)
            self.assertIn('return x + 1', stdout)

    def test_zipimport(self):
        with os_helper.temp_dir() as temp_dir:
            os.mkdir(os.path.join(temp_dir, 'source'))
            zipmodule = textwrap.dedent(
                """
                def bar():
                    pass
                """
            )
            script = textwrap.dedent(
                f"""
                import sys; sys.path.insert(0, {repr(os.path.join(temp_dir, 'zipmodule.zip'))})
                import foo
                foo.bar()
                """
            )

            with zipfile.ZipFile(os.path.join(temp_dir, 'zipmodule.zip'), 'w') as zf:
                zf.writestr('foo.py', zipmodule)
            with open(os.path.join(temp_dir, 'script.py'), 'w') as f:
                f.write(script)

            stdout, _ = self._run_pdb([os.path.join(temp_dir, 'script.py')], '\n'.join([
                'n',
                'n',
                'b foo.bar',
                'c',
                'p f"break in {$_frame.f_code.co_name}"',
                'q'
            ]))
            self.assertIn('break in bar', stdout)


class ChecklineTests(unittest.TestCase):
    def setUp(self):
        linecache.clearcache()  # Pdb.checkline() uses linecache.getline()

    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    def test_checkline_before_debugging(self):
        with open(os_helper.TESTFN, "w") as f:
            f.write("print(123)")
        db = pdb.Pdb()
        self.assertEqual(db.checkline(os_helper.TESTFN, 1), 1)

    def test_checkline_after_reset(self):
        with open(os_helper.TESTFN, "w") as f:
            f.write("print(123)")
        db = pdb.Pdb()
        db.reset()
        self.assertEqual(db.checkline(os_helper.TESTFN, 1), 1)

    def test_checkline_is_not_executable(self):
        # Test for comments, docstrings and empty lines
        s = textwrap.dedent("""
            # Comment
            \"\"\" docstring \"\"\"
            ''' docstring '''

        """)
        with open(os_helper.TESTFN, "w") as f:
            f.write(s)
        num_lines = len(s.splitlines()) + 2  # Test for EOF
        with redirect_stdout(StringIO()):
            db = pdb.Pdb()
            for lineno in range(num_lines):
                self.assertFalse(db.checkline(os_helper.TESTFN, lineno))


@support.requires_subprocess()
class PdbTestInline(unittest.TestCase):
    @unittest.skipIf(sys.flags.safe_path,
                     'PYTHONSAFEPATH changes default sys.path')
    def _run_script(self, script, commands,
                    expected_returncode=0,
                    extra_env=None):
        self.addCleanup(os_helper.rmtree, '__pycache__')
        filename = 'main.py'
        with open(filename, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.unlink, filename)

        commands = textwrap.dedent(commands)

        cmd = [sys.executable, 'main.py']
        if extra_env is not None:
            env = os.environ | extra_env
        else:
            env = os.environ
        with subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                stderr=subprocess.PIPE,
                env = {**env, 'PYTHONIOENCODING': 'utf-8'}
        ) as proc:
            stdout, stderr = proc.communicate(str.encode(commands))
        stdout = bytes.decode(stdout) if isinstance(stdout, bytes) else stdout
        stderr = bytes.decode(stderr) if isinstance(stderr, bytes) else stderr
        self.assertEqual(
            proc.returncode,
            expected_returncode,
            f"Unexpected return code\nstdout: {stdout}\nstderr: {stderr}"
        )
        return stdout, stderr

    def test_quit(self):
        script = """
            x = 1
            breakpoint()
        """

        commands = """
            quit
            n
            p x + 1
            quit
            y
        """

        stdout, stderr = self._run_script(script, commands, expected_returncode=1)
        self.assertIn("2", stdout)
        self.assertIn("Quit anyway", stdout)
        # Closing stdin will quit the debugger anyway so we need to confirm
        # it's the quit command that does the job
        # call/return event will print --Call-- and --Return--
        self.assertNotIn("--", stdout)
        # Normal exit should not print anything to stderr
        self.assertEqual(stderr, "")
        # The quit prompt should be printed exactly twice
        self.assertEqual(stdout.count("Quit anyway"), 2)

    def test_quit_after_interact(self):
        """
        interact command will set sys.ps1 temporarily, we need to make sure
        that it's restored and pdb does not believe it's in interactive mode
        after interact is done.
        """
        script = """
            x = 1
            breakpoint()
        """

        commands = """
            interact
            quit()
            q
            y
        """

        stdout, stderr = self._run_script(script, commands, expected_returncode=1)
        # Normal exit should not print anything to stderr
        self.assertEqual(stderr, "")
        # The quit prompt should be printed exactly once
        self.assertEqual(stdout.count("Quit anyway"), 1)
        # BdbQuit should not be printed
        self.assertNotIn("BdbQuit", stdout)

    def test_set_trace_with_skip(self):
        """GH-82897
        Inline set_trace() should break unconditionally. This example is a
        bit oversimplified, but as `pdb.set_trace()` uses the previous Pdb
        instance, it's possible that we had a previous pdb instance with
        skip values when we use `pdb.set_trace()` - it would be confusing
        to users when such inline breakpoints won't break immediately.
        """
        script = textwrap.dedent("""
            import pdb
            def foo():
                x = 40 + 2
                pdb.Pdb(skip=['__main__']).set_trace()
            foo()
        """)
        commands = """
            p x
            c
        """
        stdout, _ = self._run_script(script, commands)
        self.assertIn("42", stdout)


@support.force_colorized_test_class
class PdbTestColorize(unittest.TestCase):
    def setUp(self):
        self._original_can_colorize = _colorize.can_colorize
        # Force colorize to be enabled because we are sending data
        # to a StringIO
        _colorize.can_colorize = lambda *args, **kwargs: True

    def tearDown(self):
        _colorize.can_colorize = self._original_can_colorize

    def test_code_display(self):
        output = io.StringIO()
        p = pdb.Pdb(stdout=output, colorize=True)
        p.set_trace(commands=['ll', 'c'])
        self.assertIn("\x1b", output.getvalue())

        output = io.StringIO()
        p = pdb.Pdb(stdout=output, colorize=False)
        p.set_trace(commands=['ll', 'c'])
        self.assertNotIn("\x1b", output.getvalue())

        output = io.StringIO()
        p = pdb.Pdb(stdout=output)
        p.set_trace(commands=['ll', 'c'])
        self.assertNotIn("\x1b", output.getvalue())

    def test_stack_entry(self):
        output = io.StringIO()
        p = pdb.Pdb(stdout=output, colorize=True)
        p.set_trace(commands=['w', 'c'])
        self.assertIn("\x1b", output.getvalue())


@support.force_not_colorized_test_class
@support.requires_subprocess()
class TestREPLSession(unittest.TestCase):
    def test_return_from_inline_mode_to_REPL(self):
        # GH-124703: Raise BdbQuit when exiting pdb in REPL session.
        # This allows the REPL session to continue.
        from test.test_repl import spawn_repl
        p = spawn_repl()
        user_input = """
            x = 'Spam'
            import pdb
            pdb.set_trace(commands=['x + "During"', 'q'])
            x + 'After'
        """
        p.stdin.write(textwrap.dedent(user_input))
        output = kill_python(p)
        self.assertIn('SpamDuring', output)
        self.assertNotIn("Quit anyway", output)
        self.assertIn('BdbQuit', output)
        self.assertIn('SpamAfter', output)
        self.assertEqual(p.returncode, 0)


@support.force_not_colorized_test_class
@support.requires_subprocess()
class PdbTestReadline(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        # Ensure that the readline module is loaded
        # If this fails, the test is skipped because SkipTest will be raised
        readline = import_module('readline')
        if readline.backend == "editline":
            raise unittest.SkipTest("libedit readline is not supported for pdb")

    def test_basic_completion(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
            # Concatenate strings so that the output doesn't appear in the source
            print('hello' + '!')
        """)

        # List everything starting with 'co', there should be multiple matches
        # then add ntin and complete 'contin' to 'continue'
        input = b"co\t\tntin\t\n"

        output = run_pty(script, input)

        self.assertIn(b'commands', output)
        self.assertIn(b'condition', output)
        self.assertIn(b'continue', output)
        self.assertIn(b'hello!', output)

    def test_expression_completion(self):
        script = textwrap.dedent("""
            value = "speci"
            import pdb; pdb.Pdb().set_trace()
        """)

        # Complete: value + 'al'
        input = b"val\t + 'al'\n"
        # Complete: p value + 'es'
        input += b"p val\t + 'es'\n"
        # Complete: $_frame
        input += b"$_fra\t\n"
        # Continue
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'special', output)
        self.assertIn(b'species', output)
        self.assertIn(b'$_frame', output)

    def test_builtin_completion(self):
        script = textwrap.dedent("""
            value = "speci"
            import pdb; pdb.Pdb().set_trace()
        """)

        # Complete: print(value + 'al')
        input = b"pri\tval\t + 'al')\n"

        # Continue
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'special', output)

    def test_convvar_completion(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
        """)

        # Complete: $_frame
        input = b"$_fram\t\n"

        # Complete: $_frame.f_lineno + 100
        input += b"$_frame.f_line\t + 100\n"

        # Continue
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'<frame at 0x', output)
        self.assertIn(b'102', output)

    def test_local_namespace(self):
        script = textwrap.dedent("""
            def f():
                original = "I live Pythin"
                import pdb; pdb.Pdb().set_trace()
            f()
        """)

        # Complete: original.replace('i', 'o')
        input = b"orig\t.repl\t('i', 'o')\n"

        # Continue
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'I love Python', output)

    @unittest.skipIf(sys.platform.startswith('freebsd'),
                     '\\x08 is not interpreted as backspace on FreeBSD')
    def test_multiline_auto_indent(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
        """)

        input = b"def f(x):\n"
        input += b"if x > 0:\n"
        input += b"x += 1\n"
        input += b"return x\n"
        # We need to do backspaces to remove the auto-indentation
        input += b"\x08\x08\x08\x08else:\n"
        input += b"return -x\n"
        input += b"\n"
        input += b"f(-21-21)\n"
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'42', output)

    def test_multiline_completion(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
        """)

        input = b"def func():\n"
        # Auto-indent
        # Complete: return 40 + 2
        input += b"ret\t 40 + 2\n"
        input += b"\n"
        # Complete: func()
        input += b"fun\t()\n"
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b'42', output)

    @unittest.skipIf(sys.platform.startswith('freebsd'),
                     '\\x08 is not interpreted as backspace on FreeBSD')
    def test_multiline_indent_completion(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
        """)

        # \t should always complete a 4-space indent
        # This piece of code will raise an IndentationError or a SyntaxError
        # if the completion is not working as expected
        input = textwrap.dedent("""\
            def func():
            a = 1
            \x08\ta += 1
            \x08\x08\ta += 1
            \x08\x08\x08\ta += 1
            \x08\x08\x08\x08\tif a > 0:
            a += 1
            \x08\x08\x08\x08return a

            func()
            c
        """).encode()

        output = run_pty(script, input)

        self.assertIn(b'5', output)
        self.assertNotIn(b'Error', output)

    def test_interact_completion(self):
        script = textwrap.dedent("""
            value = "speci"
            import pdb; pdb.Pdb().set_trace()
        """)

        # Enter interact mode
        input = b"interact\n"
        # Should fail to complete 'display' because that's a pdb command
        input += b"disp\t\n"
        # 'value' should still work
        input += b"val\t + 'al'\n"
        # Let's define a function to test <tab>
        input += b"def f():\n"
        input += b"\treturn 42\n"
        input += b"\n"
        input += b"f() * 2\n"
        # Exit interact mode
        input += b"exit()\n"
        # continue
        input += b"c\n"

        output = run_pty(script, input)

        self.assertIn(b"'disp' is not defined", output)
        self.assertIn(b'special', output)
        self.assertIn(b'84', output)


def load_tests(loader, tests, pattern):
    from test import test_pdb

    def setUpPdbBackend(backend):
        def setUp(test):
            import pdb
            pdb.set_default_backend(backend)
        return setUp

    def tearDown(test):
        # Ensure that asyncio state has been cleared at the end of the test.
        # This prevents a "test altered the execution environment" warning if
        # asyncio features are used.
        _set_event_loop_policy(None)

        # A doctest of pdb could have residues. For example, pdb could still
        # be running, or breakpoints might be left uncleared. These residues
        # could potentially interfere with the following test, especially
        # when we switch backends. Here we clear all the residues to restore
        # to its pre-test state.

        # clear all the breakpoints left
        import bdb
        bdb.Breakpoint.clearBreakpoints()

        # Stop tracing and clear the pdb instance cache
        if pdb.Pdb._last_pdb_instance:
            pdb.Pdb._last_pdb_instance.stop_trace()
            pdb.Pdb._last_pdb_instance = None

        # If garbage objects are collected right after we start tracing, we
        # could stop at __del__ of the object which would fail the test.
        gc.collect()

    tests.addTest(
        doctest.DocTestSuite(
            test_pdb,
            setUp=setUpPdbBackend('monitoring'),
            tearDown=tearDown,
        )
    )
    tests.addTest(
        doctest.DocTestSuite(
            test_pdb,
            setUp=setUpPdbBackend('settrace'),
            tearDown=tearDown,
        )
    )
    return tests


if __name__ == '__main__':
    unittest.main()
