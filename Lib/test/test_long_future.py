from __future__ import division
# When true division is the default, get rid of this and add it to
# test_long.py instead.  In the meantime, it's too obscure to try to
# trick just part of test_long into using future division.

from test_support import TestFailed, verify, verbose

def test_true_division():
    if verbose:
        print "long true division"
    huge = 1L << 40000
    mhuge = -huge
    verify(huge / huge == 1.0)
    verify(mhuge / mhuge == 1.0)
    verify(huge / mhuge == -1.0)
    verify(mhuge / huge == -1.0)
    verify(1 / huge == 0.0)
    verify(1L / huge == 0.0)
    verify(1 / mhuge == 0.0)
    verify(1L / mhuge == 0.0)
    verify((666 * huge + (huge >> 1)) / huge == 666.5)
    verify((666 * mhuge + (mhuge >> 1)) / mhuge == 666.5)
    verify((666 * huge + (huge >> 1)) / mhuge == -666.5)
    verify((666 * mhuge + (mhuge >> 1)) / huge == -666.5)
    verify(huge / (huge << 1) == 0.5)
    verify((1000000 * huge) / huge == 1000000)

    namespace = {'huge': huge, 'mhuge': mhuge}

    for overflow in ["float(huge)", "float(mhuge)",
                     "huge / 1", "huge / 2L", "huge / -1", "huge / -2L",
                     "mhuge / 100", "mhuge / 100L"]:
        try:
            eval(overflow, namespace)
        except OverflowError:
            pass
        else:
            raise TestFailed("expected OverflowError from %r" % overflow)

    for underflow in ["1 / huge", "2L / huge", "-1 / huge", "-2L / huge",
                     "100 / mhuge", "100L / mhuge"]:
        result = eval(underflow, namespace)
        if result != 0.0:
            raise TestFailed("expected underflow to 0 from %r" % underflow)

    for zero in ["huge / 0", "huge / 0L",
                 "mhuge / 0", "mhuge / 0L"]:
        try:
            eval(zero, namespace)
        except ZeroDivisionError:
            pass
        else:
            raise TestFailed("expected ZeroDivisionError from %r" % zero)

test_true_division()
