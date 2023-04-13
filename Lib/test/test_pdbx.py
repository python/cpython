from bdbx import Pdbx
import doctest
import sys
from test.test_doctest import _FakeInput

class PdbxTestInput(object):
    """Context manager that makes testing Pdb in doctests easier."""

    def __init__(self, input):
        self.input = input

    def __enter__(self):
        self.real_stdin = sys.stdin
        sys.stdin = _FakeInput(self.input)

    def __exit__(self, *exc):
        sys.stdin = self.real_stdin

def test_pdbx_basic_commands():
    """Test the basic commands of pdb.

    >>> def f(x):
    ...     x = x + 1
    ...     return x

    >>> def test_function():
    ...     import bdbx; bdbx.break_here()
    ...     for i in range(5):
    ...         n = f(i)
    ...         pass

    >>> with PdbxTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'step',
    ...     'step',
    ...     'return',
    ...     'next',
    ...     'n',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdbx.test_pdbx_basic_commands[1]>(3)test_function()
    -> for i in range(5):
    (Pdbx) step
    > <doctest test.test_pdbx.test_pdbx_basic_commands[1]>(4)test_function()
    -> n = f(i)
    (Pdbx) step
    > <doctest test.test_pdbx.test_pdbx_basic_commands[0]>(2)f()
    -> x = x + 1
    (Pdbx) return
    > <doctest test.test_pdbx.test_pdbx_basic_commands[1]>(5)test_function()
    -> pass
    (Pdbx) next
    > <doctest test.test_pdbx.test_pdbx_basic_commands[1]>(4)test_function()
    -> n = f(i)
    (Pdbx) n
    > <doctest test.test_pdbx.test_pdbx_basic_commands[1]>(5)test_function()
    -> pass
    (Pdbx) continue
    """

def test_pdbx_basic_breakpoint():
    """Test the breakpoints of pdbx.

    >>> def f(x):
    ...     x = x + 1
    ...     return x

    >>> def test_function():
    ...     import bdbx; bdbx.break_here()
    ...     for i in range(5):
    ...         n = f(i)
    ...         pass
    ...     a = 3

    >>> with PdbxTestInput([  # doctest: +ELLIPSIS, +NORMALIZE_WHITESPACE
    ...     'break f',
    ...     'continue',
    ...     'clear',
    ...     'return',
    ...     'break 6',
    ...     'continue',
    ...     'continue',
    ... ]):
    ...    test_function()
    > <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>(3)test_function()
    -> for i in range(5):
    (Pdbx) break f
    (Pdbx) continue
    ----call----
    > <doctest test.test_pdbx.test_pdbx_basic_breakpoint[0]>(1)f()
    -> def f(x):
    (Pdbx) clear
    (Pdbx) return
    > <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>(5)test_function()
    -> pass
    (Pdbx) break 6
    (Pdbx) continue
    > <doctest test.test_pdbx.test_pdbx_basic_breakpoint[1]>(6)test_function()
    -> a = 3
    (Pdbx) continue
    """

def load_tests(loader, tests, pattern):
    from test import test_pdbx
    tests.addTest(doctest.DocTestSuite(test_pdbx))
    return tests
