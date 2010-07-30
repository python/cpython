# A test suite for pdb; at the moment, this only validates skipping of
# specified test modules (RFE #5142).

import imp
import sys

from test import test_support
# This little helper class is essential for testing pdb under doctest.
from test_doctest import _FakeInput


class PdbTestInput(object):
    """Context manager that makes testing Pdb in doctests easier."""

    def __init__(self, input):
        self.input = input

    def __enter__(self):
        self.real_stdin = sys.stdin
        sys.stdin = _FakeInput(self.input)

    def __exit__(self, *exc):
        sys.stdin = self.real_stdin


def write(x):
    print x

def test_pdb_displayhook():
    """This tests the custom displayhook for pdb.

    >>> def test_function(foo, bar):
    ...     import pdb; pdb.Pdb().set_trace()
    ...     pass

    >>> with PdbTestInput([
    ...     'foo',
    ...     'bar',
    ...     'for i in range(5): write(i)',
    ...     'continue',
    ... ]):
    ...     test_function(1, None)
    > <doctest test.test_pdb.test_pdb_displayhook[0]>(3)test_function()
    -> pass
    (Pdb) foo
    1
    (Pdb) bar
    (Pdb) for i in range(5): write(i)
    0
    1
    2
    3
    4
    (Pdb) continue
    """


def test_pdb_skip_modules():
    """This illustrates the simple case of module skipping.

    >>> def skip_module():
    ...     import string
    ...     import pdb; pdb.Pdb(skip=['string*']).set_trace()
    ...     string.lower('FOO')

    >>> with PdbTestInput([
    ...     'step',
    ...     'continue',
    ... ]):
    ...     skip_module()
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()
    -> string.lower('FOO')
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()->None
    -> string.lower('FOO')
    (Pdb) continue
    """


# Module for testing skipping of module that makes a callback
mod = imp.new_module('module_to_skip')
exec 'def foo_pony(callback): x = 1; callback(); return None' in mod.__dict__


def test_pdb_skip_modules_with_callback():
    """This illustrates skipping of modules that call into other code.

    >>> def skip_module():
    ...     def callback():
    ...         return None
    ...     import pdb; pdb.Pdb(skip=['module_to_skip*']).set_trace()
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


def test_pdb_unicode_exception():
    r"""This tests exceptions that cannot be displayed due to Unicode issues.
    http://bugs.python.org/issue7539

    >>> def test_function():
    ...     import pdb; pdb.Pdb().set_trace()
    ...     pass

    >>> def raising_function():
    ...     raise ValueError(u"\xff")

    >>> with PdbTestInput([
    ...     'raising_function()',
    ...     'p raising_function()',
    ...     'continue',
    ... ]):
    ...     test_function()
    > <doctest test.test_pdb.test_pdb_unicode_exception[0]>(3)test_function()
    -> pass
    (Pdb) raising_function()
    *** ValueError: ValueError(u'\xff',)
    (Pdb) p raising_function()
    *** ValueError: ValueError(u'\xff',)
    (Pdb) continue
    """


def test_main():
    from test import test_pdb
    test_support.run_doctest(test_pdb, verbosity=True)


if __name__ == '__main__':
    test_main()
