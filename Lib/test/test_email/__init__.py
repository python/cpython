import os
import sys
import unittest
import test.support
import email
from email.message import Message
from email._policybase import compat32
from test.test_email import __file__ as landmark

# Run all tests in package for '-m unittest test.test_email'
def load_tests(loader, standard_tests, pattern):
    this_dir = os.path.dirname(__file__)
    if pattern is None:
        pattern = "test*"
    package_tests = loader.discover(start_dir=this_dir, pattern=pattern)
    standard_tests.addTests(package_tests)
    return standard_tests


# used by regrtest and __main__.
def test_main():
    here = os.path.dirname(__file__)
    # Unittest mucks with the path, so we have to save and restore
    # it to keep regrtest happy.
    savepath = sys.path[:]
    test.support._run_suite(unittest.defaultTestLoader.discover(here))
    sys.path[:] = savepath


# helper code used by a number of test modules.

def openfile(filename, *args, **kws):
    path = os.path.join(os.path.dirname(landmark), 'data', filename)
    return open(path, *args, **kws)


# Base test class
class TestEmailBase(unittest.TestCase):

    maxDiff = None
    # Currently the default policy is compat32.  By setting that as the default
    # here we make minimal changes in the test_email tests compared to their
    # pre-3.3 state.
    policy = compat32

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self.addTypeEqualityFunc(bytes, self.assertBytesEqual)

    # Backward compatibility to minimize test_email test changes.
    ndiffAssertEqual = unittest.TestCase.assertEqual

    def _msgobj(self, filename):
        with openfile(filename) as fp:
            return email.message_from_file(fp, policy=self.policy)

    def _str_msg(self, string, message=Message, policy=None):
        if policy is None:
            policy = self.policy
        return email.message_from_string(string, message, policy=policy)

    def _bytes_repr(self, b):
        return [repr(x) for x in b.splitlines(keepends=True)]

    def assertBytesEqual(self, first, second, msg):
        """Our byte strings are really encoded strings; improve diff output"""
        self.assertEqual(self._bytes_repr(first), self._bytes_repr(second))
