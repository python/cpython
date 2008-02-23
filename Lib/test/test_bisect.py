import sys
import unittest
from test import test_support
from UserList import UserList

# We do a bit of trickery here to be able to test both the C implementation
# and the Python implementation of the module.

# Make it impossible to import the C implementation anymore.
sys.modules['_bisect'] = 0
# We must also handle the case that bisect was imported before.
if 'bisect' in sys.modules:
    del sys.modules['bisect']

# Now we can import the module and get the pure Python implementation.
import bisect as py_bisect

# Restore everything to normal.
del sys.modules['_bisect']
del sys.modules['bisect']

# This is now the module with the C implementation.
import bisect as c_bisect


class TestBisect(unittest.TestCase):
    module = None

    def setUp(self):
        self.precomputedCases = [
            (self.module.bisect_right, [], 1, 0),
            (self.module.bisect_right, [1], 0, 0),
            (self.module.bisect_right, [1], 1, 1),
            (self.module.bisect_right, [1], 2, 1),
            (self.module.bisect_right, [1, 1], 0, 0),
            (self.module.bisect_right, [1, 1], 1, 2),
            (self.module.bisect_right, [1, 1], 2, 2),
            (self.module.bisect_right, [1, 1, 1], 0, 0),
            (self.module.bisect_right, [1, 1, 1], 1, 3),
            (self.module.bisect_right, [1, 1, 1], 2, 3),
            (self.module.bisect_right, [1, 1, 1, 1], 0, 0),
            (self.module.bisect_right, [1, 1, 1, 1], 1, 4),
            (self.module.bisect_right, [1, 1, 1, 1], 2, 4),
            (self.module.bisect_right, [1, 2], 0, 0),
            (self.module.bisect_right, [1, 2], 1, 1),
            (self.module.bisect_right, [1, 2], 1.5, 1),
            (self.module.bisect_right, [1, 2], 2, 2),
            (self.module.bisect_right, [1, 2], 3, 2),
            (self.module.bisect_right, [1, 1, 2, 2], 0, 0),
            (self.module.bisect_right, [1, 1, 2, 2], 1, 2),
            (self.module.bisect_right, [1, 1, 2, 2], 1.5, 2),
            (self.module.bisect_right, [1, 1, 2, 2], 2, 4),
            (self.module.bisect_right, [1, 1, 2, 2], 3, 4),
            (self.module.bisect_right, [1, 2, 3], 0, 0),
            (self.module.bisect_right, [1, 2, 3], 1, 1),
            (self.module.bisect_right, [1, 2, 3], 1.5, 1),
            (self.module.bisect_right, [1, 2, 3], 2, 2),
            (self.module.bisect_right, [1, 2, 3], 2.5, 2),
            (self.module.bisect_right, [1, 2, 3], 3, 3),
            (self.module.bisect_right, [1, 2, 3], 4, 3),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 1),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 3),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 6),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 10),
            (self.module.bisect_right, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10),

            (self.module.bisect_left, [], 1, 0),
            (self.module.bisect_left, [1], 0, 0),
            (self.module.bisect_left, [1], 1, 0),
            (self.module.bisect_left, [1], 2, 1),
            (self.module.bisect_left, [1, 1], 0, 0),
            (self.module.bisect_left, [1, 1], 1, 0),
            (self.module.bisect_left, [1, 1], 2, 2),
            (self.module.bisect_left, [1, 1, 1], 0, 0),
            (self.module.bisect_left, [1, 1, 1], 1, 0),
            (self.module.bisect_left, [1, 1, 1], 2, 3),
            (self.module.bisect_left, [1, 1, 1, 1], 0, 0),
            (self.module.bisect_left, [1, 1, 1, 1], 1, 0),
            (self.module.bisect_left, [1, 1, 1, 1], 2, 4),
            (self.module.bisect_left, [1, 2], 0, 0),
            (self.module.bisect_left, [1, 2], 1, 0),
            (self.module.bisect_left, [1, 2], 1.5, 1),
            (self.module.bisect_left, [1, 2], 2, 1),
            (self.module.bisect_left, [1, 2], 3, 2),
            (self.module.bisect_left, [1, 1, 2, 2], 0, 0),
            (self.module.bisect_left, [1, 1, 2, 2], 1, 0),
            (self.module.bisect_left, [1, 1, 2, 2], 1.5, 2),
            (self.module.bisect_left, [1, 1, 2, 2], 2, 2),
            (self.module.bisect_left, [1, 1, 2, 2], 3, 4),
            (self.module.bisect_left, [1, 2, 3], 0, 0),
            (self.module.bisect_left, [1, 2, 3], 1, 0),
            (self.module.bisect_left, [1, 2, 3], 1.5, 1),
            (self.module.bisect_left, [1, 2, 3], 2, 1),
            (self.module.bisect_left, [1, 2, 3], 2.5, 2),
            (self.module.bisect_left, [1, 2, 3], 3, 2),
            (self.module.bisect_left, [1, 2, 3], 4, 3),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 0, 0),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1, 0),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 1.5, 1),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2, 1),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 2.5, 3),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3, 3),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 3.5, 6),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 4, 6),
            (self.module.bisect_left, [1, 2, 2, 3, 3, 3, 4, 4, 4, 4], 5, 10)
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
            ip = self.module.bisect_left(data, elem)
            if ip < len(data):
                self.failUnless(elem <= data[ip])
            if ip > 0:
                self.failUnless(data[ip-1] < elem)
            ip = self.module.bisect_right(data, elem)
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
                    if func is self.module.bisect_left and ip < hi:
                        self.failUnless(elem <= data[ip])
                    if func is self.module.bisect_left and ip > lo:
                        self.failUnless(data[ip-1] < elem)
                    if func is self.module.bisect_right and ip < hi:
                        self.failUnless(elem < data[ip])
                    if func is self.module.bisect_right and ip > lo:
                        self.failUnless(data[ip-1] <= elem)
                    self.assertEqual(ip, max(lo, min(hi, expected)))

    def test_backcompatibility(self):
        self.assertEqual(self.module.bisect, self.module.bisect_right)

    def test_keyword_args(self):
        data = [10, 20, 30, 40, 50]
        self.assertEqual(self.module.bisect_left(a=data, x=25, lo=1, hi=3), 2)
        self.assertEqual(self.module.bisect_right(a=data, x=25, lo=1, hi=3), 2)
        self.assertEqual(self.module.bisect(a=data, x=25, lo=1, hi=3), 2)
        self.module.insort_left(a=data, x=25, lo=1, hi=3)
        self.module.insort_right(a=data, x=25, lo=1, hi=3)
        self.module.insort(a=data, x=25, lo=1, hi=3)
        self.assertEqual(data, [10, 20, 25, 25, 25, 30, 40, 50])

class TestBisectPython(TestBisect):
    module = py_bisect

class TestBisectC(TestBisect):
    module = c_bisect

#==============================================================================

class TestInsort(unittest.TestCase):
    module = None

    def test_vsBuiltinSort(self, n=500):
        from random import choice
        for insorted in (list(), UserList()):
            for i in xrange(n):
                digit = choice("0123456789")
                if digit in "02468":
                    f = self.module.insort_left
                else:
                    f = self.module.insort_right
                f(insorted, digit)
        self.assertEqual(sorted(insorted), insorted)

    def test_backcompatibility(self):
        self.assertEqual(self.module.insort, self.module.insort_right)

class TestInsortPython(TestInsort):
    module = py_bisect

class TestInsortC(TestInsort):
    module = c_bisect

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
    module = None

    def test_non_sequence(self):
        for f in (self.module.bisect_left, self.module.bisect_right,
                  self.module.insort_left, self.module.insort_right):
            self.assertRaises(TypeError, f, 10, 10)

    def test_len_only(self):
        for f in (self.module.bisect_left, self.module.bisect_right,
                  self.module.insort_left, self.module.insort_right):
            self.assertRaises(AttributeError, f, LenOnly(), 10)

    def test_get_only(self):
        for f in (self.module.bisect_left, self.module.bisect_right,
                  self.module.insort_left, self.module.insort_right):
            self.assertRaises(AttributeError, f, GetOnly(), 10)

    def test_cmp_err(self):
        seq = [CmpErr(), CmpErr(), CmpErr()]
        for f in (self.module.bisect_left, self.module.bisect_right,
                  self.module.insort_left, self.module.insort_right):
            self.assertRaises(ZeroDivisionError, f, seq, 10)

    def test_arg_parsing(self):
        for f in (self.module.bisect_left, self.module.bisect_right,
                  self.module.insort_left, self.module.insort_right):
            self.assertRaises(TypeError, f, 10)

class TestErrorHandlingPython(TestErrorHandling):
    module = py_bisect

class TestErrorHandlingC(TestErrorHandling):
    module = c_bisect

#==============================================================================

libreftest = """
Example from the Library Reference:  Doc/library/bisect.rst

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

    test_classes = [TestBisectPython, TestBisectC,
                    TestInsortPython, TestInsortC,
                    TestErrorHandlingPython, TestErrorHandlingC]

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
