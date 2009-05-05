# A test suite for pdb; at the moment, this only validates skipping of
# specified test modules (RFE #5142).

import imp
import os
import sys
import doctest
import tempfile

from test import support
# This little helper class is essential for testing pdb under doctest.
from test.test_doctest import _FakeInput


def test_pdb_skip_modules():
    """This illustrates the simple case of module skipping.

    >>> def skip_module():
    ...     import string
    ...     import pdb; pdb.Pdb(skip=['stri*']).set_trace()
    ...     string.capwords('FOO')
    >>> real_stdin = sys.stdin
    >>> sys.stdin = _FakeInput([
    ...    'step',
    ...    'continue',
    ...    ])

    >>> try:
    ...     skip_module()
    ... finally:
    ...     sys.stdin = real_stdin
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()
    -> string.capwords('FOO')
    (Pdb) step
    --Return--
    > <doctest test.test_pdb.test_pdb_skip_modules[0]>(4)skip_module()->None
    -> string.capwords('FOO')
    (Pdb) continue
"""


# Module for testing skipping of module that makes a callback
mod = imp.new_module('module_to_skip')
exec('def foo_pony(callback): x = 1; callback(); return None', mod.__dict__)


def test_pdb_skip_modules_with_callback():
    """This illustrates skipping of modules that call into other code.

    >>> def skip_module():
    ...     def callback():
    ...         return None
    ...     import pdb;pdb.Pdb(skip=['module_to_skip*']).set_trace()
    ...     mod.foo_pony(callback)
    >>> real_stdin = sys.stdin
    >>> sys.stdin = _FakeInput([
    ...    'step',
    ...    'step',
    ...    'step',
    ...    'step',
    ...    'step',
    ...    'continue',
    ...    ])

    >>> try:
    ...     skip_module()
    ... finally:
    ...     sys.stdin = real_stdin
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
    > <doctest test.test_pdb.test_pdb_skip_modules_with_callback[3]>(4)<module>()
    -> sys.stdin = real_stdin
    (Pdb) continue
"""


def test_main():
    from test import test_pdb
    support.run_doctest(test_pdb, verbosity=True)


if __name__ == '__main__':
    test_main()
