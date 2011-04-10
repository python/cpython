import os
import sys
import unittest
import test.support
import email
from test.test_email import __file__ as landmark

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

    def __init__(self, *args, **kw):
        super().__init__(*args, **kw)
        self.addTypeEqualityFunc(bytes, self.assertBytesEqual)

    ndiffAssertEqual = unittest.TestCase.assertEqual

    def _msgobj(self, filename):
        with openfile(filename) as fp:
            return email.message_from_file(fp)

    def _bytes_repr(self, b):
        return [repr(x) for x in b.splitlines(True)]

    def assertBytesEqual(self, first, second, msg):
        """Our byte strings are really encoded strings; improve diff output"""
        self.assertEqual(self._bytes_repr(first), self._bytes_repr(second))
