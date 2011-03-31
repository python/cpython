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

    def ndiffAssertEqual(self, first, second):
        """Like assertEqual except use ndiff for readable output."""
        if first != second:
            sfirst = str(first)
            ssecond = str(second)
            rfirst = [repr(line) for line in sfirst.splitlines()]
            rsecond = [repr(line) for line in ssecond.splitlines()]
            diff = difflib.ndiff(rfirst, rsecond)
            raise self.failureException(NL + NL.join(diff))

    def _msgobj(self, filename):
        with openfile(filename) as fp:
            return email.message_from_file(fp)
