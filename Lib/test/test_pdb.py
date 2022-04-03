# A test suite for pdb; not very comprehensive at the moment.

import doctest
import os
import pdb
import sys
import types
import codecs
import unittest
import subprocess
import textwrap
import linecache

from contextlib import ExitStack, redirect_stdout
from io import StringIO
from test.support import os_helper
# This little helper class is essential for testing pdb under doctest.
from test.test_doctest import _FakeInput
from unittest.mock import patch


class PdbTestInput(object):
    """Context manager that makes testing Pdb in doctests easier."""

    def __init__(self, input):
        self.input = input

    def __enter__(self):
        self.real_stdin = sys.stdin
        sys.stdin = _FakeInput(self.input)
        self.orig_trace = sys.gettrace() if hasattr(sys, 'gettrace') else None

    def __exit__(self, *exc):
        sys.stdin = self.real_stdin
        if self.orig_trace:
            sys.settrace(self.orig_trace)


def test_pdb_displayhook():
    """This tests the custom displayhook for pdb.

    >>> def test_function(foo, bar):
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     pass

    >>> with PdbTestInput([
    ...     'foo',
    ...     'bar',
    ...     'for i in range(5): print(i)',
    ...     'continue',
    ... ]):
    ...     test_function(1, None)
    > <doctest test.test_pdb.test_pdb_displayhook[0]>(3)test_function()
    -> pass
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
      <doctest test.test_pdb.test_pdb_basic_commands[4]>(25)<module>()
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

def reset_Breakpoint():
    import bdb
    bdb.Breakpoint.clearBreakpoints()

def test_pdb_breakpoint_commands():
    """Test basic commands related to breakpoints.

    >>> def test_function():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     print(1)
    ...     print(2)
    ...     print(3)
    ...     print(4)

    First, need to clear bdb state that might be left over from previous tests.
    Otherwise, the new breakpoints might get assigned different numbers.

    >>> reset_Breakpoint()

    Now test the breakpoint commands.  NORMALIZE_WHITESPACE is needed because
    the breakpoint list outputs a tab for the "stop only" and "ignore next"
    lines, which we don't want to put in here.

    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 3',
    ...     'disable 1',
    ...     'ignore 1 10',
    ...     'condition 1 1 < 2',
    ...     'break 4',
    ...     'break 4',
    ...     'break',
    ...     'clear 3',
    ...     'break',
    ...     'condition 1',
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
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>(3)test_function()
    -> print(1)
    (Pdb) break 3
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) disable 1
    Disabled breakpoint 1 at <doctest test.test_pdb.test_pdb_breakpoint_commands[0]>:3
    (Pdb) ignore 1 10
    Will ignore next 10 crossings of breakpoint 1.
    (Pdb) condition 1 1 < 2
    New condition set for breakpoint 1.
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
    *** Usage: commands [bnum]
            ...
            end
    (Pdb) commands 4
    *** cannot set commands: Breakpoint 4 already deleted
    (Pdb) continue
    3
    4
    """

def test_pdb_breakpoints_preserved_across_interactive_sessions():
    """Breakpoints are remembered between interactive sessions

    >>> reset_Breakpoint()
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
    Breakpoint 3 at ...pdb.py:97
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
    --Return--
    > <doctest test.test_pdb.test_pdb_pp_repr_exc[2]>(2)test_function()->None
    -> import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    (Pdb) p obj
    *** Exception: repr_exc
    (Pdb) pp obj
    *** Exception: repr_exc
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
    --Return--
    > <doctest test.test_pdb.test_pdb_whatis_command[3]>(2)test_function()->None
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
    > <doctest test.test_pdb.test_post_mortem[1]>(3)test_function()
    -> test_function_2()
    (Pdb) next
    Exception!
    ZeroDivisionError: division by zero
    > <doctest test.test_pdb.test_post_mortem[1]>(3)test_function()
    -> test_function_2()
    (Pdb) bt
    ...
      <doctest test.test_pdb.test_post_mortem[2]>(10)<module>()
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


def test_pdb_skip_modules():
    """This illustrates the simple case of module skipping.

    >>> def skip_module():
    ...     import string
    ...     import pdb; pdb.Pdb(skip=['stri*'], nosigint=True, readrc=False).set_trace()
    ...     string.capwords('FOO')

    >>> with PdbTestInput([
    ...     'step',
    ...     'continue',
    ... ]):
    ...     skip_module()
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()
    -> string.capwords('FOO')
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()->None
    -> string.capwords('FOO')
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
    ...     'continue',
    ... ]):
    ...     skip_module()
    ...     pass  # provides something to "step" to
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
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[1]>(10)<module>()
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
    ...     'next',
    ...     'break 7',
    ...     'continue',
    ...     'next',
    ...     'continue',
    ...     'continue',
    ... ]):
    ...    test_function()
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

    >>> reset_Breakpoint()
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
    > <doctest test.test_pdb.test_next_until_return_at_return_event[1]>(3)test_function()
    -> test_function_2()
    (Pdb) break test_function_2
    Breakpoint 1 at <doctest test.test_pdb.test_next_until_return_at_return_event[0]>:1
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
    """Testing skip unwindng stack on yield for generators for "next" command

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
    ...                    'next',
    ...                    'next',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
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

def test_pdb_next_command_for_coroutine():
    """Testing skip unwindng stack on yield for coroutines for "next" command

    >>> import asyncio

    >>> async def test_coro():
    ...     await asyncio.sleep(0)
    ...     await asyncio.sleep(0)
    ...     await asyncio.sleep(0)

    >>> async def test_main():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     await test_coro()

    >>> def test_function():
    ...     loop = asyncio.new_event_loop()
    ...     loop.run_until_complete(test_main())
    ...     loop.close()
    ...     asyncio.set_event_loop_policy(None)
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'next',
    ...                    'next',
    ...                    'next',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[2]>(3)test_main()
    -> await test_coro()
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(1)test_coro()
    -> async def test_coro():
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(2)test_coro()
    -> await asyncio.sleep(0)
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(3)test_coro()
    -> await asyncio.sleep(0)
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_next_command_for_coroutine[1]>(4)test_coro()
    -> await asyncio.sleep(0)
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
    """Testing skip unwindng stack on yield for coroutines for "next" command

    >>> import asyncio

    >>> async def agen():
    ...     yield 1
    ...     await asyncio.sleep(0)
    ...     yield 2

    >>> async def test_coro():
    ...     async for x in agen():
    ...         print(x)

    >>> async def test_main():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     await test_coro()

    >>> def test_function():
    ...     loop = asyncio.new_event_loop()
    ...     loop.run_until_complete(test_main())
    ...     loop.close()
    ...     asyncio.set_event_loop_policy(None)
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'next',
    ...                    'next',
    ...                    'step',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
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
    -> await asyncio.sleep(0)
    (Pdb) continue
    2
    finished
    """

def test_pdb_return_command_for_generator():
    """Testing no unwindng stack on yield for generators
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
    ...                    'return',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
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

def test_pdb_return_command_for_coroutine():
    """Testing no unwindng stack on yield for coroutines for "return" command

    >>> import asyncio

    >>> async def test_coro():
    ...     await asyncio.sleep(0)
    ...     await asyncio.sleep(0)
    ...     await asyncio.sleep(0)

    >>> async def test_main():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     await test_coro()

    >>> def test_function():
    ...     loop = asyncio.new_event_loop()
    ...     loop.run_until_complete(test_main())
    ...     loop.close()
    ...     asyncio.set_event_loop_policy(None)
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'step',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[2]>(3)test_main()
    -> await test_coro()
    (Pdb) step
    --Call--
    > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(1)test_coro()
    -> async def test_coro():
    (Pdb) step
    > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(2)test_coro()
    -> await asyncio.sleep(0)
    (Pdb) next
    > <doctest test.test_pdb.test_pdb_return_command_for_coroutine[1]>(3)test_coro()
    -> await asyncio.sleep(0)
    (Pdb) continue
    finished
    """

def test_pdb_until_command_for_generator():
    """Testing no unwindng stack on yield for generators
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
    ...                    'until 4',
    ...                    'step',
    ...                    'step',
    ...                    'continue']):
    ...     test_function()
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

def test_pdb_until_command_for_coroutine():
    """Testing no unwindng stack for coroutines
       for "until" command if target breakpoint is not reached

    >>> import asyncio

    >>> async def test_coro():
    ...     print(0)
    ...     await asyncio.sleep(0)
    ...     print(1)
    ...     await asyncio.sleep(0)
    ...     print(2)
    ...     await asyncio.sleep(0)
    ...     print(3)

    >>> async def test_main():
    ...     import pdb; pdb.Pdb(nosigint=True, readrc=False).set_trace()
    ...     await test_coro()

    >>> def test_function():
    ...     loop = asyncio.new_event_loop()
    ...     loop.run_until_complete(test_main())
    ...     loop.close()
    ...     asyncio.set_event_loop_policy(None)
    ...     print("finished")

    >>> with PdbTestInput(['step',
    ...                    'until 8',
    ...                    'continue']):
    ...     test_function()
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

    >>> reset_Breakpoint()
    >>> with PdbTestInput(['break test_gen',
    ...                    'continue',
    ...                    'next',
    ...                    'next',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[1]>(3)test_function()
    -> for i in test_gen():
    (Pdb) break test_gen
    Breakpoint 1 at <doctest test.test_pdb.test_pdb_next_command_in_generator_for_loop[0]>:1
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
    ...                    'next',
    ...                    'next',
    ...                    'next',
    ...                    'continue']):
    ...     test_function()
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

def test_pdb_issue_20766():
    """Test for reference leaks when the SIGINT handler is set.

    >>> def test_function():
    ...     i = 1
    ...     while i <= 2:
    ...         sess = pdb.Pdb()
    ...         sess.set_trace(sys._getframe())
    ...         print('pdb %d: %s' % (i, sess._previous_sigint_handler))
    ...         i += 1

    >>> reset_Breakpoint()
    >>> with PdbTestInput(['continue',
    ...                    'continue']):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_20766[0]>(6)test_function()
    -> print('pdb %d: %s' % (i, sess._previous_sigint_handler))
    (Pdb) continue
    pdb 1: <built-in function default_int_handler>
    > <doctest test.test_pdb.test_pdb_issue_20766[0]>(6)test_function()
    -> print('pdb %d: %s' % (i, sess._previous_sigint_handler))
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
    >>> reset_Breakpoint()
    >>> with PdbTestInput([  # doctest: +NORMALIZE_WHITESPACE
    ...     'break 3',
    ...     'clear <doctest test.test_pdb.test_pdb_issue_43318[0]>:3',
    ...     'continue'
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_issue_43318[0]>(3)test_function()
    -> print(1)
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


class PdbTestCase(unittest.TestCase):
    def tearDown(self):
        os_helper.unlink(os_helper.TESTFN)

    def _run_pdb(self, pdb_args, commands):
        self.addCleanup(os_helper.rmtree, '__pycache__')
        cmd = [sys.executable, '-m', 'pdb'] + pdb_args
        with subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stdin=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env = {**os.environ, 'PYTHONIOENCODING': 'utf-8'}
        ) as proc:
            stdout, stderr = proc.communicate(str.encode(commands))
        stdout = stdout and bytes.decode(stdout)
        stderr = stderr and bytes.decode(stderr)
        return stdout, stderr

    def run_pdb_script(self, script, commands):
        """Run 'script' lines with pdb and the pdb 'commands'."""
        filename = 'main.py'
        with open(filename, 'w') as f:
            f.write(textwrap.dedent(script))
        self.addCleanup(os_helper.unlink, filename)
        return self._run_pdb([filename], commands)

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
            ('bœr', 4),
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
            ('bœr', 5),
        )

    def test_find_function_found_with_bom(self):
        self._assert_find_function(
            codecs.BOM_UTF8 + """\
def bœr():
    pass
""".encode(),
            'bœr',
            ('bœr', 1),
        )

    def test_issue7964(self):
        # open the file as binary so we can force \r\n newline
        with open(os_helper.TESTFN, 'wb') as f:
            f.write(b'print("testing my pdb")\r\n')
        cmd = [sys.executable, '-m', 'pdb', os_helper.TESTFN]
        proc = subprocess.Popen(cmd,
            stdout=subprocess.PIPE,
            stdin=subprocess.PIPE,
            stderr=subprocess.STDOUT,
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
            stderr=subprocess.STDOUT,
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
            stderr=subprocess.STDOUT,
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
        stdout, stderr = self.run_pdb_script(script, commands)
        self.assertIn(expected, stdout,
            '\n\nExpected:\n{}\nGot:\n{}\n'
            'Fail to handle a syntax error in the debuggee.'
            .format(expected, stdout))

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

    def test_readrc_kwarg(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb(readrc=False).set_trace()

            print('hello')
        """)

        save_home = os.environ.pop('HOME', None)
        try:
            with os_helper.temp_cwd():
                with open('.pdbrc', 'w') as f:
                    f.write("invalid\n")

                with open('main.py', 'w') as f:
                    f.write(script)

                cmd = [sys.executable, 'main.py']
                proc = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stdin=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                )
                with proc:
                    stdout, stderr = proc.communicate(b'q\n')
                    self.assertNotIn(b"NameError: name 'invalid' is not defined",
                                  stdout)

        finally:
            if save_home is not None:
                os.environ['HOME'] = save_home

    def test_readrc_homedir(self):
        save_home = os.environ.pop("HOME", None)
        with os_helper.temp_dir() as temp_dir, patch("os.path.expanduser"):
            rc_path = os.path.join(temp_dir, ".pdbrc")
            os.path.expanduser.return_value = rc_path
            try:
                with open(rc_path, "w") as f:
                    f.write("invalid")
                self.assertEqual(pdb.Pdb().rcLines[0], "invalid")
            finally:
                if save_home is not None:
                    os.environ["HOME"] = save_home

    def test_read_pdbrc_with_ascii_encoding(self):
        script = textwrap.dedent("""
            import pdb; pdb.Pdb().set_trace()
            print('hello')
        """)
        save_home = os.environ.pop('HOME', None)
        try:
            with os_helper.temp_cwd():
                with open('.pdbrc', 'w', encoding='utf-8') as f:
                    f.write("Fran\u00E7ais")

                with open('main.py', 'w', encoding='utf-8') as f:
                    f.write(script)

                cmd = [sys.executable, 'main.py']
                env = {'PYTHONIOENCODING': 'ascii'}
                if sys.platform == 'win32':
                    env['PYTHONLEGACYWINDOWSSTDIO'] = 'non-empty-string'
                proc = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stdin=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    env={**os.environ, **env}
                )
                with proc:
                    stdout, stderr = proc.communicate(b'c\n')
                    self.assertIn(b"UnicodeEncodeError: \'ascii\' codec can\'t encode character "
                                  b"\'\\xe7\' in position 21: ordinal not in range(128)", stderr)

        finally:
            if save_home is not None:
                os.environ['HOME'] = save_home

    def test_header(self):
        stdout = StringIO()
        header = 'Nobody expects... blah, blah, blah'
        with ExitStack() as resources:
            resources.enter_context(patch('sys.stdout', stdout))
            resources.enter_context(patch.object(pdb.Pdb, 'set_trace'))
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
        stdout, stderr = self._run_pdb(['-m', module_name], "")
        self.assertIn("ImportError: No module named t_main.__main__",
                      stdout.splitlines())

    def test_package_without_a_main(self):
        pkg_name = 't_pkg'
        module_name = 't_main'
        os_helper.rmtree(pkg_name)
        modpath = pkg_name + '/' + module_name
        os.makedirs(modpath)
        with open(modpath + '/__init__.py', 'w'):
            pass
        self.addCleanup(os_helper.rmtree, pkg_name)
        stdout, stderr = self._run_pdb(['-m', modpath.replace('/', '.')], "")
        self.assertIn(
            "'t_pkg.t_main' is a package and cannot be directly executed",
            stdout)

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
            'print(',
            'debug print(',
            'debug doesnotexist',
            'c',
        ])
        stdout, _ = self.run_pdb_script('pass', commands + '\n')

        self.assertEqual(stdout.splitlines()[1:], [
            '-> pass',
            '(Pdb) *** SyntaxError: \'(\' was never closed',

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


def load_tests(loader, tests, pattern):
    from test import test_pdb
    tests.addTest(doctest.DocTestSuite(test_pdb))
    return tests


if __name__ == '__main__':
    unittest.main()
