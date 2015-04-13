import unittest

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
        c = Cmp(2)
        self.assertIs(a == b, True)
        self.assertIs(a != b, False)
        self.assertIs(a != c, True)

    def test_ne_high_priority(self):
        """object.__ne__() should allow reflected __ne__() to be tried"""
        calls = []
        class Left:
            # Inherits object.__ne__()
            def __eq__(*args):
                calls.append('Left.__eq__')
                return NotImplemented
        class Right:
            def __eq__(*args):
                calls.append('Right.__eq__')
                return NotImplemented
            def __ne__(*args):
                calls.append('Right.__ne__')
                return NotImplemented
        Left() != Right()
        self.assertSequenceEqual(calls, ['Left.__eq__', 'Right.__ne__'])

    def test_ne_low_priority(self):
        """object.__ne__() should not invoke reflected __eq__()"""
        calls = []
        class Base:
            # Inherits object.__ne__()
            def __eq__(*args):
                calls.append('Base.__eq__')
                return NotImplemented
        class Derived(Base):  # Subclassing forces higher priority
            def __eq__(*args):
                calls.append('Derived.__eq__')
                return NotImplemented
            def __ne__(*args):
                calls.append('Derived.__ne__')
                return NotImplemented
        Base() != Derived()
        self.assertSequenceEqual(calls, ['Derived.__ne__', 'Base.__eq__'])

    def test_other_delegation(self):
        """No default delegation between operations except __ne__()"""
        ops = (
            ('__eq__', lambda a, b: a == b),
            ('__lt__', lambda a, b: a < b),
            ('__le__', lambda a, b: a <= b),
            ('__gt__', lambda a, b: a > b),
            ('__ge__', lambda a, b: a >= b),
        )
        for name, func in ops:
            with self.subTest(name):
                def unexpected(*args):
                    self.fail('Unexpected operator method called')
                class C:
                    __ne__ = unexpected
                for other, _ in ops:
                    if other != name:
                        setattr(C, other, unexpected)
                if name == '__eq__':
                    self.assertIs(func(C(), object()), False)
                else:
                    self.assertRaises(TypeError, func, C(), object())

    def test_issue_1393(self):
        x = lambda: None
        self.assertEqual(x, Anything())
        self.assertEqual(Anything(), x)
        y = object()
        self.assertEqual(y, Anything())
        self.assertEqual(Anything(), y)


if __name__ == '__main__':
    unittest.main()
