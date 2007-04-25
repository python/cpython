from __future__ import division
# When true division is the default, get rid of this and add it to
# test_long.py instead.  In the meantime, it's too obscure to try to
# trick just part of test_long into using future division.

import unittest
from test.test_support import run_unittest

class TrueDivisionTests(unittest.TestCase):
    def test(self):
        huge = 1L << 40000
        mhuge = -huge
        self.assertEqual(huge / huge, 1.0)
        self.assertEqual(mhuge / mhuge, 1.0)
        self.assertEqual(huge / mhuge, -1.0)
        self.assertEqual(mhuge / huge, -1.0)
        self.assertEqual(1 / huge, 0.0)
        self.assertEqual(1L / huge, 0.0)
        self.assertEqual(1 / mhuge, 0.0)
        self.assertEqual(1L / mhuge, 0.0)
        self.assertEqual((666 * huge + (huge >> 1)) / huge, 666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / mhuge, 666.5)
        self.assertEqual((666 * huge + (huge >> 1)) / mhuge, -666.5)
        self.assertEqual((666 * mhuge + (mhuge >> 1)) / huge, -666.5)
        self.assertEqual(huge / (huge << 1), 0.5)
        self.assertEqual((1000000 * huge) / huge, 1000000)

        namespace = {'huge': huge, 'mhuge': mhuge}

        for overflow in ["float(huge)", "float(mhuge)",
                         "huge / 1", "huge / 2L", "huge / -1", "huge / -2L",
                         "mhuge / 100", "mhuge / 100L"]:
            # XXX(cwinter) this test doesn't pass when converted to
            # use assertRaises.
            try:
                eval(overflow, namespace)
                self.fail("expected OverflowError from %r" % overflow)
            except OverflowError:
                pass

        for underflow in ["1 / huge", "2L / huge", "-1 / huge", "-2L / huge",
                         "100 / mhuge", "100L / mhuge"]:
            result = eval(underflow, namespace)
            self.assertEqual(result, 0.0,
                             "expected underflow to 0 from %r" % underflow)

        for zero in ["huge / 0", "huge / 0L", "mhuge / 0", "mhuge / 0L"]:
            self.assertRaises(ZeroDivisionError, eval, zero, namespace)


def test_main():
    run_unittest(TrueDivisionTests)

if __name__ == "__main__":
    test_main()
