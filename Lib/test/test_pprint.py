import pprint
import unittest

import test_support


class QueryTestCase(unittest.TestCase):

    def setUp(self):
        self.a = range(100)
        self.b = range(200)
        self.a[-12] = self.b

    def test_basic(self):
        # Verify that .isrecursive() and .isreadable() work.
        verify = self.assert_
        for safe in (2, 2.0, 2j, "abc", [3], (2,2), {3: 3}, u"yaddayadda",
                     self.a, self.b):
            verify(not pprint.isrecursive(safe),
                   "expected not isrecursive for " + `safe`)
            verify(pprint.isreadable(safe),
                   "expected isreadable for " + `safe`)

    def test_knotted(self):
        # Tie a knot.
        self.b[67] = self.a
        # Messy dict.
        self.d = {}
        self.d[0] = self.d[1] = self.d[2] = self.d

        verify = self.assert_

        for icky in self.a, self.b, self.d, (self.d, self.d):
            verify(pprint.isrecursive(icky), "expected isrecursive")
            verify(not pprint.isreadable(icky),  "expected not isreadable")

        # Break the cycles.
        self.d.clear()
        del self.a[:]
        del self.b[:]

        for safe in self.a, self.b, self.d, (self.d, self.d):
            verify(not pprint.isrecursive(safe),
                   "expected not isrecursive for " + `safe`)
            verify(pprint.isreadable(safe),
                   "expected isreadable for " + `safe`)

    def test_unreadable(self):
        """Not recursive but not readable anyway."""
        verify = self.assert_
        for unreadable in type(3), pprint, pprint.isrecursive:
            verify(not pprint.isrecursive(unreadable),
                   "expected not isrecursive for " + `unreadable`)
            verify(not pprint.isreadable(unreadable),
                   "expected not isreadable for " + `unreadable`)


test_support.run_unittest(QueryTestCase)
