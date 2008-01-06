import sys
import unittest
from test import test_support

class Empty:
    def __repr__(self):
        return '<Empty>'

class Cmp:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<Cmp %s>' % self.arg

    def __eq__(self, other):
        return self.arg == other

class Anything:
    def __eq__(self, other):
        return True

    def __ne__(self, other):
        return False

class ComparisonTest(unittest.TestCase):
    set1 = [2, 2.0, 2, 2+0j, Cmp(2.0)]
    set2 = [[1], (3,), None, Empty()]
    candidates = set1 + set2

    def test_comparisons(self):
        for a in self.candidates:
            for b in self.candidates:
                if ((a in self.set1) and (b in self.set1)) or a is b:
                    self.assertEqual(a, b)
                else:
                    self.assertNotEqual(a, b)

    def test_id_comparisons(self):
        # Ensure default comparison compares id() of args
        L = []
        for i in range(10):
            L.insert(len(L)//2, Empty())
        for a in L:
            for b in L:
                self.assertEqual(a == b, id(a) == id(b),
                                 'a=%r, b=%r' % (a, b))

    def test_ne_defaults_to_not_eq(self):
        a = Cmp(1)
        b = Cmp(1)
        self.assertTrue(a == b)
        self.assertFalse(a != b)

    def test_issue_1393(self):
        x = lambda: None
        self.assertEqual(x, Anything())
        self.assertEqual(Anything(), x)
        y = object()
        self.assertEqual(y, Anything())
        self.assertEqual(Anything(), y)


def test_main():
    test_support.run_unittest(ComparisonTest)

if __name__ == '__main__':
    test_main()
