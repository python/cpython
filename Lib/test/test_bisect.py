import unittest
from test import test_support
from bisect import bisect_right, bisect_left, insort_left, insort_right

# XXX optional slice arguments need tests.


class TestBisect(unittest.TestCase):

    precomputedCases = [
        (bisect_right, [], 1, 0),
        (bisect_right, [1], 0, 0),
        (bisect_right, [1], 1, 1),
        (bisect_right, [1], 2, 1),
        (bisect_right, [1, 1], 0, 0),
        (bisect_right, [1, 1], 1, 2),
        (bisect_right, [1, 1], 2, 2),
        (bisect_right, [1, 1, 1], 0, 0),
        (bisect_right, [1, 1, 1], 1, 3),
        (bisect_right, [1, 1, 1], 2, 3),
        (bisect_right, [1, 1, 1, 1], 0, 0),
        (bisect_right, [1, 1, 1, 1], 1, 4),
        (bisect_right, [1, 1, 1, 1], 2, 4),
        (bisect_right, [1, 2], 0, 0),
        (bisect_right, [1, 2], 1, 1),
        (bisect_right, [1, 2], 1.5, 1),
        (bisect_right, [1, 2], 2, 2),
        (bisect_right, [1, 2], 3, 2),
        (bisect_right, [1, 1, 2, 2], 0, 0),
        (bisect_right, [1, 1, 2, 2], 1, 2),
        (bisect_right, [1, 1, 2, 2], 1.5, 2),
        (bisect_right, [1, 1, 2, 2], 2, 4),
        (bisect_right, [1, 1, 2, 2], 3, 4),
        (bisect_right, [1, 2, 3], 0, 0),
        (bisect_right, [1, 2, 3], 1, 1),
        (bisect_right, [1, 2, 3], 1.5, 1),
        (bisect_right, [1, 2, 3], 2, 2),
        (bisect_right, [1, 2, 3], 2.5, 2),
        (bisect_right, [1, 2, 3], 3, 3),
        (bisect_right, [1, 2, 3], 4, 3),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 1),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 3),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 6),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 10),
        (bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10),

        (bisect_left, [], 1, 0),
        (bisect_left, [1], 0, 0),
        (bisect_left, [1], 1, 0),
        (bisect_left, [1], 2, 1),
        (bisect_left, [1, 1], 0, 0),
        (bisect_left, [1, 1], 1, 0),
        (bisect_left, [1, 1], 2, 2),
        (bisect_left, [1, 1, 1], 0, 0),
        (bisect_left, [1, 1, 1], 1, 0),
        (bisect_left, [1, 1, 1], 2, 3),
        (bisect_left, [1, 1, 1, 1], 0, 0),
        (bisect_left, [1, 1, 1, 1], 1, 0),
        (bisect_left, [1, 1, 1, 1], 2, 4),
        (bisect_left, [1, 2], 0, 0),
        (bisect_left, [1, 2], 1, 0),
        (bisect_left, [1, 2], 1.5, 1),
        (bisect_left, [1, 2], 2, 1),
        (bisect_left, [1, 2], 3, 2),
        (bisect_left, [1, 1, 2, 2], 0, 0),
        (bisect_left, [1, 1, 2, 2], 1, 0),
        (bisect_left, [1, 1, 2, 2], 1.5, 2),
        (bisect_left, [1, 1, 2, 2], 2, 2),
        (bisect_left, [1, 1, 2, 2], 3, 4),
        (bisect_left, [1, 2, 3], 0, 0),
        (bisect_left, [1, 2, 3], 1, 0),
        (bisect_left, [1, 2, 3], 1.5, 1),
        (bisect_left, [1, 2, 3], 2, 1),
        (bisect_left, [1, 2, 3], 2.5, 2),
        (bisect_left, [1, 2, 3], 3, 2),
        (bisect_left, [1, 2, 3], 4, 3),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 1),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 3),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 6),
        (bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10)
    ]

    def test_precomputed(self):
        for func, list, elt, expected in self.precomputedCases:
            self.assertEqual(func(list, elt), expected)

#==============================================================================

class TestInsort(unittest.TestCase):

    def test_vsListSort(self, n=500):
        from random import choice
        digits = "0123456789"
        raw = []
        insorted = []
        for i in range(n):
            digit = choice(digits)
            raw.append(digit)
            if digit in "02468":
                f = insort_left
            else:
                f = insort_right
            f(insorted, digit)
        sorted = raw[:]
        sorted.sort()
        self.assertEqual(sorted, insorted)

#==============================================================================

def makeAllTests():
    suite = unittest.TestSuite()
    for klass in (TestBisect,
                  TestInsort
                  ):
        suite.addTest(unittest.makeSuite(klass))
    return suite

#------------------------------------------------------------------------------

def test_main(verbose=None):
    from test import test_bisect
    suite = makeAllTests()
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main(verbose=True)

