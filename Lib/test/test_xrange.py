# Python test set -- built-in functions

import test.test_support, unittest
import sys
import pickle
import itertools

import warnings
warnings.filterwarnings("ignore", "integer argument expected",
                        DeprecationWarning, "unittest")

# pure Python implementations (3 args only), for comparison
def pyrange(start, stop, step):
    if (start - stop) // step < 0:
        # replace stop with next element in the sequence of integers
        # that are congruent to start modulo step.
        stop += (start - stop) % step
        while start != stop:
            yield start
            start += step

def pyrange_reversed(start, stop, step):
    stop += (start - stop) % step
    return pyrange(stop - step, start - step, -step)


class XrangeTest(unittest.TestCase):
    def assert_iterators_equal(self, xs, ys, test_id, limit=None):
        # check that an iterator xs matches the expected results ys,
        # up to a given limit.
        if limit is not None:
            xs = itertools.islice(xs, limit)
            ys = itertools.islice(ys, limit)
        sentinel = object()
        pairs = itertools.izip_longest(xs, ys, fillvalue=sentinel)
        for i, (x, y) in enumerate(pairs):
            if x == y:
                continue
            elif x == sentinel:
                self.fail('{}: iterator ended unexpectedly '
                          'at position {}; expected {}'.format(test_id, i, y))
            elif y == sentinel:
                self.fail('{}: unexpected excess element {} at '
                          'position {}'.format(test_id, x, i))
            else:
                self.fail('{}: wrong element at position {};'
                          'expected {}, got {}'.format(test_id, i, y, x))

    def assert_xranges_equivalent(self, x, y):
        # Check that two xrange objects are equivalent, in the sense of the
        # associated sequences being the same.  We want to use this for large
        # xrange objects, so instead of converting to lists and comparing
        # directly we do a number of indirect checks.
        if len(x) != len(y):
            self.fail('{} and {} have different '
                      'lengths: {} and {} '.format(x, y, len(x), len(y)))
        if len(x) >= 1:
            if x[0] != y[0]:
                self.fail('{} and {} have different initial '
                          'elements: {} and {} '.format(x, y, x[0], y[0]))
            if x[-1] != y[-1]:
                self.fail('{} and {} have different final '
                          'elements: {} and {} '.format(x, y, x[-1], y[-1]))
        if len(x) >= 2:
            x_step = x[1] - x[0]
            y_step = y[1] - y[0]
            if x_step != y_step:
                self.fail('{} and {} have different step: '
                          '{} and {} '.format(x, y, x_step, y_step))

    def test_xrange(self):
        self.assertEqual(list(xrange(3)), [0, 1, 2])
        self.assertEqual(list(xrange(1, 5)), [1, 2, 3, 4])
        self.assertEqual(list(xrange(0)), [])
        self.assertEqual(list(xrange(-3)), [])
        self.assertEqual(list(xrange(1, 10, 3)), [1, 4, 7])
        self.assertEqual(list(xrange(5, -5, -3)), [5, 2, -1, -4])

        a = 10
        b = 100
        c = 50

        self.assertEqual(list(xrange(a, a+2)), [a, a+1])
        self.assertEqual(list(xrange(a+2, a, -1L)), [a+2, a+1])
        self.assertEqual(list(xrange(a+4, a, -2)), [a+4, a+2])

        seq = list(xrange(a, b, c))
        self.assertIn(a, seq)
        self.assertNotIn(b, seq)
        self.assertEqual(len(seq), 2)

        seq = list(xrange(b, a, -c))
        self.assertIn(b, seq)
        self.assertNotIn(a, seq)
        self.assertEqual(len(seq), 2)

        seq = list(xrange(-a, -b, -c))
        self.assertIn(-a, seq)
        self.assertNotIn(-b, seq)
        self.assertEqual(len(seq), 2)

        self.assertRaises(TypeError, xrange)
        self.assertRaises(TypeError, xrange, 1, 2, 3, 4)
        self.assertRaises(ValueError, xrange, 1, 2, 0)

        self.assertRaises(OverflowError, xrange, 10**100, 10**101, 10**101)

        self.assertRaises(TypeError, xrange, 0, "spam")
        self.assertRaises(TypeError, xrange, 0, 42, "spam")

        self.assertEqual(len(xrange(0, sys.maxint, sys.maxint-1)), 2)

        self.assertRaises(OverflowError, xrange, -sys.maxint, sys.maxint)
        self.assertRaises(OverflowError, xrange, 0, 2*sys.maxint)

        r = xrange(-sys.maxint, sys.maxint, 2)
        self.assertEqual(len(r), sys.maxint)
        self.assertRaises(OverflowError, xrange, -sys.maxint-1, sys.maxint, 2)

    def test_pickling(self):
        testcases = [(13,), (0, 11), (-22, 10), (20, 3, -1),
                     (13, 21, 3), (-2, 2, 2)]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            for t in testcases:
                r = xrange(*t)
                self.assertEqual(list(pickle.loads(pickle.dumps(r, proto))),
                                 list(r))

        M = min(sys.maxint, sys.maxsize)
        large_testcases = testcases + [
            (0, M, 1),
            (M, 0, -1),
            (0, M, M - 1),
            (M // 2, M, 1),
            (0, -M, -1),
            (0, -M, 1 - M),
            (-M, M, 2),
            (-M, M, 1024),
            (-M, M, 10585),
            (M, -M, -2),
            (M, -M, -1024),
            (M, -M, -10585),
            ]
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            for t in large_testcases:
                r = xrange(*t)
                r_out = pickle.loads(pickle.dumps(r, proto))
                self.assert_xranges_equivalent(r_out, r)

    def test_repr(self):
        # Check that repr of an xrange is a valid representation
        # of that xrange.

        # Valid xranges have at most min(sys.maxint, sys.maxsize) elements.
        M = min(sys.maxint, sys.maxsize)

        testcases = [
            (13,),
            (0, 11),
            (-22, 10),
            (20, 3, -1),
            (13, 21, 3),
            (-2, 2, 2),
            (0, M, 1),
            (M, 0, -1),
            (0, M, M - 1),
            (M // 2, M, 1),
            (0, -M, -1),
            (0, -M, 1 - M),
            (-M, M, 2),
            (-M, M, 1024),
            (-M, M, 10585),
            (M, -M, -2),
            (M, -M, -1024),
            (M, -M, -10585),
            ]
        for t in testcases:
            r = xrange(*t)
            r_out = eval(repr(r))
            self.assert_xranges_equivalent(r, r_out)

    def test_range_iterators(self):
        # see issue 7298
        limits = [base + jiggle
                  for M in (2**32, 2**64)
                  for base in (-M, -M//2, 0, M//2, M)
                  for jiggle in (-2, -1, 0, 1, 2)]
        test_ranges = [(start, end, step)
                       for start in limits
                       for end in limits
                       for step in (-2**63, -2**31, -2, -1, 1, 2)]

        for start, end, step in test_ranges:
            try:
                iter1 = xrange(start, end, step)
            except OverflowError:
                pass
            else:
                iter2 = pyrange(start, end, step)
                test_id = "xrange({}, {}, {})".format(start, end, step)
                # check first 100 entries
                self.assert_iterators_equal(iter1, iter2, test_id, limit=100)

            try:
                iter1 = reversed(xrange(start, end, step))
            except OverflowError:
                pass
            else:
                iter2 = pyrange_reversed(start, end, step)
                test_id = "reversed(xrange({}, {}, {}))".format(start, end, step)
                self.assert_iterators_equal(iter1, iter2, test_id, limit=100)


def test_main():
    test.test_support.run_unittest(XrangeTest)

if __name__ == "__main__":
    test_main()
