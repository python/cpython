import unittest
from test import test_support
from bisect import bisect_right, bisect_left, insort_left, insort_right, insort, bisect
from UserList import UserList

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
        for func, data, elem, expected in self.precomputedCases:
            self.assertEqual(func(data, elem), expected)
            self.assertEqual(func(UserList(data), elem), expected)

    def test_random(self, n=25):
        from random import randrange
        for i in xrange(n):
            data = [randrange(0, n, 2) for j in xrange(i)]
            data.sort()
            elem = randrange(-1, n+1)
            ip = bisect_left(data, elem)
            if ip < len(data):
                self.failUnless(elem <= data[ip])
            if ip > 0:
                self.failUnless(data[ip-1] < elem)
            ip = bisect_right(data, elem)
            if ip < len(data):
                self.failUnless(elem < data[ip])
            if ip > 0:
                self.failUnless(data[ip-1] <= elem)

    def test_optionalSlicing(self):
        for func, data, elem, expected in self.precomputedCases:
            for lo in xrange(4):
                lo = min(len(data), lo)
                for hi in xrange(3,8):
                    hi = min(len(data), hi)
                    ip = func(data, elem, lo, hi)
                    self.failUnless(lo <= ip <= hi)
                    if func is bisect_left and ip < hi:
                        self.failUnless(elem <= data[ip])
                    if func is bisect_left and ip > lo:
                        self.failUnless(data[ip-1] < elem)
                    if func is bisect_right and ip < hi:
                        self.failUnless(elem < data[ip])
                    if func is bisect_right and ip > lo:
                        self.failUnless(data[ip-1] <= elem)
                    self.assertEqual(ip, max(lo, min(hi, expected)))

    def test_backcompatibility(self):
        self.assertEqual(bisect, bisect_right)

    def test_keyword_args(self):
        data = [10, 20, 30, 40, 50]
        self.assertEqual(bisect_left(a=data, x=25, lo=1, hi=3), 2)
        self.assertEqual(bisect_right(a=data, x=25, lo=1, hi=3), 2)
        self.assertEqual(bisect(a=data, x=25, lo=1, hi=3), 2)
        insort_left(a=data, x=25, lo=1, hi=3)
        insort_right(a=data, x=25, lo=1, hi=3)
        insort(a=data, x=25, lo=1, hi=3)
        self.assertEqual(data, [10, 20, 25, 25, 25, 30, 40, 50])

#==============================================================================

class TestInsort(unittest.TestCase):

    def test_vsBuiltinSort(self, n=500):
        from random import choice
        for insorted in (list(), UserList()):
            for i in xrange(n):
                digit = choice("0123456789")
                if digit in "02468":
                    f = insort_left
                else:
                    f = insort_right
                f(insorted, digit)
        self.assertEqual(sorted(insorted), insorted)

    def test_backcompatibility(self):
        self.assertEqual(insort, insort_right)

#==============================================================================


class LenOnly:
    "Dummy sequence class defining __len__ but not __getitem__."
    def __len__(self):
        return 10

class GetOnly:
    "Dummy sequence class defining __getitem__ but not __len__."
    def __getitem__(self, ndx):
        return 10

class CmpErr:
    "Dummy element that always raises an error during comparison"
    def __cmp__(self, other):
        raise ZeroDivisionError

class TestErrorHandling(unittest.TestCase):

    def test_non_sequence(self):
        for f in (bisect_left, bisect_right, insort_left, insort_right):
            self.assertRaises(TypeError, f, 10, 10)

    def test_len_only(self):
        for f in (bisect_left, bisect_right, insort_left, insort_right):
            self.assertRaises(AttributeError, f, LenOnly(), 10)

    def test_get_only(self):
        for f in (bisect_left, bisect_right, insort_left, insort_right):
            self.assertRaises(AttributeError, f, GetOnly(), 10)

    def test_cmp_err(self):
        seq = [CmpErr(), CmpErr(), CmpErr()]
        for f in (bisect_left, bisect_right, insort_left, insort_right):
            self.assertRaises(ZeroDivisionError, f, seq, 10)

    def test_arg_parsing(self):
        for f in (bisect_left, bisect_right, insort_left, insort_right):
            self.assertRaises(TypeError, f, 10)

#==============================================================================

libreftest = """
Example from the Library Reference:  Doc/lib/libbisect.tex

The bisect() function is generally useful for categorizing numeric data.
This example uses bisect() to look up a letter grade for an exam total
(say) based on a set of ordered numeric breakpoints: 85 and up is an `A',
75..84 is a `B', etc.

    >>> grades = "FEDCBA"
    >>> breakpoints = [30, 44, 66, 75, 85]
    >>> from bisect import bisect
    >>> def grade(total):
    ...           return grades[bisect(breakpoints, total)]
    ...
    >>> grade(66)
    'C'
    >>> map(grade, [33, 99, 77, 44, 12, 88])
    ['E', 'A', 'B', 'D', 'F', 'A']

"""

#------------------------------------------------------------------------------

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    from test import test_bisect
    from types import BuiltinFunctionType
    import sys

    test_classes = [TestBisect, TestInsort]
    if isinstance(bisect_left, BuiltinFunctionType):
        test_classes.append(TestErrorHandling)

    test_support.run_unittest(*test_classes)
    test_support.run_doctest(test_bisect, verbose)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
