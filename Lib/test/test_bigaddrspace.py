from test import test_support
from test.test_support import bigaddrspacetest, MAX_Py_ssize_t

import unittest
import operator
import sys


class StrTest(unittest.TestCase):

    @bigaddrspacetest
    def test_concat(self):
        s1 = 'x' * MAX_Py_ssize_t
        self.assertRaises(OverflowError, operator.add, s1, '?')

    @bigaddrspacetest
    def test_optimized_concat(self):
        x = 'x' * MAX_Py_ssize_t
        try:
            x = x + '?'     # this statement uses a fast path in ceval.c
        except OverflowError:
            pass
        else:
            self.fail("should have raised OverflowError")
        try:
            x += '?'        # this statement uses a fast path in ceval.c
        except OverflowError:
            pass
        else:
            self.fail("should have raised OverflowError")
        self.assertEquals(len(x), MAX_Py_ssize_t)

    ### the following test is pending a patch
    #   (http://mail.python.org/pipermail/python-dev/2006-July/067774.html)
    #@bigaddrspacetest
    #def test_repeat(self):
    #    self.assertRaises(OverflowError, operator.mul, 'x', MAX_Py_ssize_t + 1)


def test_main():
    test_support.run_unittest(StrTest)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        test_support.set_memlimit(sys.argv[1])
    test_main()
