import unittest
from test import test_support

class Empty:
    def __repr__(self):
        return '<Empty>'

class Coerce:
    def __init__(self, arg):
        self.arg = arg

    def __repr__(self):
        return '<Coerce %s>' % self.arg

    def __coerce__(self, other):
        if isinstance(other, Coerce):
            return self.arg, other.arg
        else:
            return self.arg, other

class Cmp:
    def __init__(self,arg):
        self.arg = arg

    def __repr__(self):
        return '<Cmp %s>' % self.arg

    def __cmp__(self, other):
        return cmp(self.arg, other)

class ComparisonTest(unittest.TestCase):
    set1 = [2, 2.0, 2L, 2+0j, Coerce(2), Cmp(2.0)]
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
                self.assertEqual(cmp(a, b), cmp(id(a), id(b)),
                                 'a=%r, b=%r' % (a, b))

def test_main():
    test_support.run_unittest(ComparisonTest)

if __name__ == '__main__':
    test_main()
