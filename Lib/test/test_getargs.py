"""
Test the internal getargs.c implementation

 PyArg_ParseTuple() is defined here.

The test here is not intended to test all of the module, just the
single case that failed between 2.1 and 2.2a2.
"""

# marshal.loads() uses PyArg_ParseTuple(args, "s#:loads")
# The s code will cause a Unicode conversion to occur.  This test
# verify that the error is propagated properly from the C code back to
# Python.

import marshal
import unittest
from test import test_support

class GetArgsTest(unittest.TestCase):
    # If the encoding succeeds using the current default encoding,
    # this test will fail because it does not test the right part of the
    # PyArg_ParseTuple() implementation.
    def test_with_marshal(self):
        if not test_support.have_unicode:
            return

        arg = unicode(r'\222', 'unicode-escape')
        self.assertRaises(UnicodeError, marshal.loads, arg)

def test_main():
    test_support.run_unittest(GetArgsTest)

if __name__ == '__main__':
    test_main()
