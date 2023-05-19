"Test equality and order comparisons."
import unittest
from test.support import ALWAYS_EQ
from fractions import Fraction
from decimal import Decimal


class ComparisonSimpleTest(unittest.TestCase):
    """Test equality and order comparisons for some simple cases."""
    # TODO Does addition of FullTest below obsolete any of these?

    class Empty:
        def __repr__(self):
            return '<Empty>'

    class Cmp:
        def __init__(self, arg):
            self.arg = arg

        def __repr__(self):
            return '<Cmp %s>' % self.arg

        def __eq__(self, other):
            return self.arg == other

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
            L.insert(len(L)//2, self.Empty())
        for a in L:
            for b in L:
                self.assertEqual(a == b, a is b, 'a=%r, b=%r' % (a, b))

    def test_ne_defaults_to_not_eq(self):
        a = self.Cmp(1)
        b = self.Cmp(1)
        c = self.Cmp(2)
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
        self.assertEqual(x, ALWAYS_EQ)
        self.assertEqual(ALWAYS_EQ, x)
        y = object()
        self.assertEqual(y, ALWAYS_EQ)
        self.assertEqual(ALWAYS_EQ, y)


class ComparisonFullTest(unittest.TestCase):
    """Test equality and ordering comparisons for built-in types and
    user-defined classes that implement relevant combinations of rich
    comparison methods.
    """

    class CompBase:
        """Base class for classes with rich comparison methods.

        The "x" attribute should be set to an underlying value to compare.

        Derived classes have a "meth" tuple attribute listing names of
        comparison methods implemented. See assert_total_order().
        """

    # Class without any rich comparison methods.
    class CompNone(CompBase):
        meth = ()

    # Classes with all combinations of value-based equality comparison methods.
    class CompEq(CompBase):
        meth = ("eq",)
        def __eq__(self, other):
            return self.x == other.x

    class CompNe(CompBase):
        meth = ("ne",)
        def __ne__(self, other):
            return self.x != other.x

    class CompEqNe(CompBase):
        meth = ("eq", "ne")
        def __eq__(self, other):
            return self.x == other.x
        def __ne__(self, other):
            return self.x != other.x

    # Classes with all combinations of value-based less/greater-than order
    # comparison methods.
    class CompLt(CompBase):
        meth = ("lt",)
        def __lt__(self, other):
            return self.x < other.x

    class CompGt(CompBase):
        meth = ("gt",)
        def __gt__(self, other):
            return self.x > other.x

    class CompLtGt(CompBase):
        meth = ("lt", "gt")
        def __lt__(self, other):
            return self.x < other.x
        def __gt__(self, other):
            return self.x > other.x

    # Classes with all combinations of value-based less/greater-or-equal-than
    # order comparison methods
    class CompLe(CompBase):
        meth = ("le",)
        def __le__(self, other):
            return self.x <= other.x

    class CompGe(CompBase):
        meth = ("ge",)
        def __ge__(self, other):
            return self.x >= other.x

    class CompLeGe(CompBase):
        meth = ("le", "ge")
        def __le__(self, other):
            return self.x <= other.x
        def __ge__(self, other):
            return self.x >= other.x

    # It should be sufficient to combine the comparison methods only within
    # each group.
    all_comp_classes = (
            CompNone,
            CompEq, CompNe, CompEqNe,  # equal group
            CompLt, CompGt, CompLtGt,  # less/greater-than group
            CompLe, CompGe, CompLeGe)  # less/greater-or-equal group

    def create_sorted_instances(self, class_, values):
        """Create objects of type `class_` and return them in a list.

        `values` is a list of values that determines the value of data
        attribute `x` of each object.

        Objects in the returned list are sorted by their identity.  They
        assigned values in `values` list order.  By assign decreasing
        values to objects with increasing identities, testcases can assert
        that order comparison is performed by value and not by identity.
        """

        instances = [class_() for __ in range(len(values))]
        instances.sort(key=id)
        # Assign the provided values to the instances.
        for inst, value in zip(instances, values):
            inst.x = value
        return instances

    def assert_equality_only(self, a, b, equal):
        """ Assert equality result and that ordering is not implemented.

        a, b: Instances to be tested (of same or different type).

        equal: Boolean indicating the expected equality comparison result:
                 True means: a == b
                 False means: a != b
        """
        self.assertEqual(a == b, equal)
        self.assertEqual(b == a, equal)
        self.assertEqual(a != b, not equal)
        self.assertEqual(b != a, not equal)
        with self.assertRaisesRegex(TypeError, "not supported"):
            a < b
        with self.assertRaisesRegex(TypeError, "not supported"):
            a <= b
        with self.assertRaisesRegex(TypeError, "not supported"):
            a > b
        with self.assertRaisesRegex(TypeError, "not supported"):
            a >= b
        with self.assertRaisesRegex(TypeError, "not supported"):
            b < a
        with self.assertRaisesRegex(TypeError, "not supported"):
            b <= a
        with self.assertRaisesRegex(TypeError, "not supported"):
            b > a
        with self.assertRaisesRegex(TypeError, "not supported"):
            b >= a

    def assert_total_order(self, a, b, comp, a_meth=None, b_meth=None):
        """
        Perform assertions on total ordering comparison of two instances.

        This function implements the knowledge about how equality and ordering
        comparisons are supposed to work.

        a, b: Instances to be tested (of same or different type).

        comp: Integer indicating the expected order comparison result,
              for operations that are supported by the classes:
                       <0 means: a < b
                       0 means: a == b
                       >0 means: a > b

        a_meth, b_meth:  Tuple of rich comparison method names (without
                    leading and trailing underscores) that are expected to be
                    available for the corresponding instance. This information
                    is only needed for instances of user-defined classes,
                    when some operations are not implemented.
                    Possible values for the tuple items are:
                    "eq", "ne", "lt", "le", "gt", "ge".
        """

        args = dict(a=a, b=b, comp=comp, a_meth=a_meth, b_meth=b_meth)
        self.assert_eq_comparison_subtest(**args)
        self.assert_ne_comparison_subtest(**args)
        self.assert_lt_comparison_subtest(**args)
        self.assert_le_comparison_subtest(**args)
        self.assert_gt_comparison_subtest(**args)
        self.assert_ge_comparison_subtest(**args)

    # The body of each subtest has form
        # if value-based comparison methods:
            # expect what the testcase defined;
        # else:  no value-based comparison
            # expect default behavior of object.

    def assert_eq_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test "==" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or "eq" in a_meth or "eq" in b_meth:
            self.assertEqual(a == b, comp == 0)
            self.assertEqual(b == a, comp == 0)
        else:
            self.assertEqual(a == b, a is b)
            self.assertEqual(b == a, a is b)

    def assert_ne_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test "!=" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or not {"ne", "eq"}.isdisjoint(a_meth + b_meth):
            self.assertEqual(a != b, comp != 0)
            self.assertEqual(b != a, comp != 0)
        else:
            self.assertEqual(a != b, a is not b)
            self.assertEqual(b != a, a is not b)

    def assert_lt_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test "<" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or "lt" in a_meth or "gt" in b_meth:
            self.assertEqual(a < b, comp < 0)
            self.assertEqual(b > a, comp < 0)
        else:
            with self.assertRaisesRegex(TypeError, "not supported"):
                a < b
            with self.assertRaisesRegex(TypeError, "not supported"):
                b > a

    def assert_le_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test "<=" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or "le" in a_meth or "ge" in b_meth:
            self.assertEqual(a <= b, comp <= 0)
            self.assertEqual(b >= a, comp <= 0)
        else:
            with self.assertRaisesRegex(TypeError, "not supported"):
                a <= b
            with self.assertRaisesRegex(TypeError, "not supported"):
                b >= a

    def assert_gt_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test ">" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or "gt" in a_meth or "lt" in b_meth:
            self.assertEqual(a > b, comp > 0)
            self.assertEqual(b < a, comp > 0)
        else:
            with self.assertRaisesRegex(TypeError, "not supported"):
                a > b
            with self.assertRaisesRegex(TypeError, "not supported"):
                b < a

    def assert_ge_comparison_subtest(self, *, a, b, comp, a_meth, b_meth):
        """ Test ">=" comparison.
        The comparison is performed in both directions of the operands.
        """
        if a_meth is None or "ge" in a_meth or "le" in b_meth:
            self.assertEqual(a >= b, comp >= 0)
            self.assertEqual(b <= a, comp >= 0)
        else:
            with self.assertRaisesRegex(TypeError, "not supported"):
                a >= b
            with self.assertRaisesRegex(TypeError, "not supported"):
                b <= a

    def test_objects(self):
        """ Compare instances of type 'object'."""
        a = object()
        b = object()
        self.assert_equality_only(a, a, True)
        self.assert_equality_only(a, b, False)

    def test_comp_classes_same(self):
        """ Compare same-class instances with comparison methods."""

        for cls in self.all_comp_classes:
            with self.subTest(cls):
                instances = self.create_sorted_instances(cls, (1, 2, 1))

                # Same object.
                self.assert_total_order(instances[0], instances[0], 0,
                                        cls.meth, cls.meth)

                # Different objects, same value.
                self.assert_total_order(instances[0], instances[2], 0,
                                        cls.meth, cls.meth)

                # Different objects, value ascending for ascending identities.
                self.assert_total_order(instances[0], instances[1], -1,
                                        cls.meth, cls.meth)

                # different objects, value descending for ascending identities.
                # This is the interesting case to assert that order comparison
                # is performed based on the value and not based on the identity.
                self.assert_total_order(instances[1], instances[2], +1,
                                        cls.meth, cls.meth)

    def test_comp_classes_different(self):
        """ Compare different-class instances with comparison methods."""

        for cls_a in self.all_comp_classes:
            for cls_b in self.all_comp_classes:
                with self.subTest(a=cls_a, b=cls_b):
                    a1 = cls_a()
                    a1.x = 1
                    b1 = cls_b()
                    b1.x = 1
                    b2 = cls_b()
                    b2.x = 2

                    self.assert_total_order(
                        a1, b1, 0, cls_a.meth, cls_b.meth)
                    self.assert_total_order(
                        a1, b2, -1, cls_a.meth, cls_b.meth)

    def test_str_subclass(self):
        """  Compare instances of str and a subclass."""
        class StrSubclass(str):
            pass

        s1 = str("a")
        s2 = str("b")
        c1 = StrSubclass("a")
        c2 = StrSubclass("b")
        c3 = StrSubclass("b")

        self.assert_total_order(s1, s1,   0)
        self.assert_total_order(s1, s2, -1)
        self.assert_total_order(c1, c1,   0)
        self.assert_total_order(c1, c2, -1)
        self.assert_total_order(c2, c3,   0)

        self.assert_total_order(s1, c2, -1)
        self.assert_total_order(s2, c3,   0)
        self.assert_total_order(c1, s2, -1)
        self.assert_total_order(c2, s2,   0)

    def test_numbers(self):
        """ Compare number types."""

        i1 = 10001
        i2 = 10002

        f1 = 1.1
        f2 = 2.1
        f3 = f1 + 1  # Same value, different instance than f2.
        f4 = f2 - 1  # Same value, different instance than f1.
        f5 = 42.0
        self.assertIsNot(f2, f3, "Testcase error: f2 is f3")
        self.assertIsNot(f1, f4, "Testcase error: f1 is f4")

        c1 = 1+1j
        c2 = 2+2j
        c3 = 2+2j  # Same value, different instance than c2.
        c4 = 1+1j  # Same value, different instance than c1.
        c5 = 42+0j
        self.assertIsNot(c2, c3, "Testcase error: c2 is c3")
        self.assertIsNot(c1, c4, "Testcase error: c1 is c4")

        q1 = Fraction(1, 2)
        q2 = Fraction(2, 3)
        q3 = Fraction(2, 3)  # Same value, different instance than q2.
        q4 = Fraction(1, 2)  # Same value, different instance than q1.
        q5 = Fraction(84, 2)
        self.assertIsNot(q2, q3, "Testcase error: q2 is q3")
        self.assertIsNot(q1, q4, "Testcase error: q1 is q4")

        d1 = Decimal('1.2')
        d2 = Decimal('2.3')
        d3 = Decimal('2.3')  # Same value, different instance than d2.
        d4 = Decimal('1.2')  # Same value, different instance than d1.
        d5 = Decimal('42.0')
        self.assertIsNot(d2, d3, "Testcase error: d2 is d3")
        self.assertIsNot(d1, d4, "Testcase error: d1 is d4")

        # Same types.
        self.assert_total_order(i1, i1, 0)
        self.assert_total_order(i1, i2, -1)

        self.assert_total_order(f1, f1, 0)
        self.assert_total_order(f1, f2, -1)
        self.assert_total_order(f2, f3, 0)
        self.assert_total_order(f3, f4, +1)

        self.assert_equality_only(c1, c1, True)
        self.assert_equality_only(c1, c2, False)
        self.assert_equality_only(c2, c3, True)
        self.assert_equality_only(c3, c4, False)

        self.assert_total_order(q1, q1, 0)
        self.assert_total_order(q1, q2, -1)
        self.assert_total_order(q2, q3, 0)
        self.assert_total_order(q3, q4, +1)

        self.assert_total_order(d1, d1, 0)
        self.assert_total_order(d1, d2, -1)
        self.assert_total_order(d2, d3, 0)
        self.assert_total_order(d3, d4, +1)

        # Mixing types.
        self.assert_total_order(i5, f5, 0)
        self.assert_equality_only(i5, c5, True)
        self.assert_total_order(i5, q5, 0)
        self.assert_total_order(i5, d5, 0)

        self.assert_equality_only(f5, c5, True)
        self.assert_total_order(f5, q5, 0)
        self.assert_total_order(f5, d5, 0)

        self.assert_equality_only(c5, q5, True)
        self.assert_equality_only(c5, d5, True)

        self.assert_total_order(q5, d5, 0)

        self.assert_total_order(i1, f1, +1)
        self.assert_equality_only(i1, c1, False)
        self.assert_total_order(i1, q1, +1)
        self.assert_total_order(i1, d1, +1)

        self.assert_equality_only(f1, c1, False)
        self.assert_total_order(f1, q1, +1)
        self.assert_total_order(f1, d1, -1)

        self.assert_equality_only(c1, q1, False)
        self.assert_equality_only(c1, d1, False)

        self.assert_total_order(q1, d1, -1)

    def test_sequences(self):
        """ Test comparison for sequences (list, tuple, range).
        """
        l1 = [1, 2]
        l2 = [2, 3]
        l3 = [2, 3]  # Same value, different instance than l2.
        l4 = [1, 2]  # Same value, different instance than l1.
        self.assertIsNot(l2, l3, "Testcase error: l2 is l3")
        self.assertIsNot(l1, l4, "Testcase error: l1 is l4")

        t1 = (1, 2)
        t2 = (2, 3)
        t3 = (2, 3)  # Same value, different instance than t2.
        t4 = (1, 2)  # Same value, different instance than t1.
        self.assertIsNot(t2, t3, "Testcase error: t2 is t3")
        self.assertIsNot(t1, t4, "Testcase error: t1 is t4")

        r1 = range(1, 2)
        r2 = range(2, 2)
        r3 = range(2, 2)  # Same value, different instance than r2.
        r4 = range(1, 2)  # Same value, different instance than r1.
        self.assertIsNot(r2, r3, "Testcase error: r2 is r3")
        self.assertIsNot(r1, r4, "Testcase error: r1 is r4")

        # Same types.
        self.assert_total_order(t1, t1, 0)
        self.assert_total_order(t1, t2, -1)
        self.assert_total_order(t2, t3, 0)
        self.assert_total_order(t3, t4, +1)

        self.assert_total_order(l1, l1, 0)
        self.assert_total_order(l1, l2, -1)
        self.assert_total_order(l2, l3, 0)
        self.assert_total_order(l3, l4, +1)

        self.assert_equality_only(r1, r1, True)
        self.assert_equality_only(r1, r2, False)
        self.assert_equality_only(r2, r3, True)
        self.assert_equality_only(r3, r4, False)

        # Mixing types.
        self.assert_equality_only(t1, l1, False)
        self.assert_equality_only(l1, r1, False)
        self.assert_equality_only(r1, t1, False)

    def test_binary_sequences(self):
        """ Test comparison for binary sequences (bytes, bytearray).
        """
        bs1 = b'a1'
        bs2 = b'b2'
        bs3 = b'b' + b'2'  # Same value, different instance than bs2.
        bs4 = b'a' + b'1'  # Same value, different instance than bs1.
        self.assertIsNot(bs2, bs3, "Testcase error: bs2 is bs3")
        self.assertIsNot(bs1, bs4, "Testcase error: bs1 is bs4")

        ba1 = bytearray(b'a1')
        ba2 = bytearray(b'b2')
        ba3 = bytearray(b'b2')  # Same value, different instance than ba2.
        ba4 = bytearray(b'a1')  # Same value, different instance than ba1.
        self.assertIsNot(ba2, ba3, "Testcase error: ba2 is ba3")
        self.assertIsNot(ba1, ba4, "Testcase error: ba1 is ba4")

        # Same types.
        self.assert_total_order(bs1, bs1, 0)
        self.assert_total_order(bs1, bs2, -1)
        self.assert_total_order(bs2, bs3, 0)
        self.assert_total_order(bs3, bs4, +1)

        self.assert_total_order(ba1, ba1,  0)
        self.assert_total_order(ba1, ba2, -1)
        self.assert_total_order(ba2, ba3, 0)
        self.assert_total_order(ba3, ba4, +1)

        # Mixing types.
        self.assert_total_order(bs1, ba1, 0)
        self.assert_total_order(bs1, ba2, -1)
        self.assert_total_order(bs2, ba3, 0)
        self.assert_total_order(bs3, ba4, +1)

        self.assert_total_order(ba1, bs1, 0)
        self.assert_total_order(ba1, bs2, -1)
        self.assert_total_order(ba2, bs3, 0)
        self.assert_total_order(ba3, bs4, +1)

    def test_sets(self):
        """ Test comparison for sets (set, frozenset).
        """
        s1 = {1, 2}
        s2 = {1, 2, 3}
        s3 = {1, 2, 3}  # Same value, different instance than s2.
        s4 = {1, 2}     # Same value, different instance than s1.
        self.assertIsNot(s2, s3, "Testcase error: s2 is s3")
        self.assertIsNot(s1, s4, "Testcase error: s1 is s4")

        f1 = frozenset({1, 2})
        f2 = frozenset({1, 2, 3})
        f3 = frozenset({1, 2, 3})  # Same value, different instance than f2.
        f4 = frozenset({1, 2})     # Same value, different instance than f1.
        self.assertIsNot(f2, f3, "Testcase error: f2 is f3")
        self.assertIsNot(f1, f4, "Testcase error: f1 is f4")

        # Same types.
        self.assert_total_order(s1, s1, 0)
        self.assert_total_order(s1, s2, -1)
        self.assert_total_order(s2, s3, 0)
        self.assert_total_order(s3, s4, +1)

        self.assert_total_order(f1, f1,  0)
        self.assert_total_order(f1, f2, -1)
        self.assert_total_order(f2, f3, 0)
        self.assert_total_order(f3, f4, +1)

        # Mixing types.
        self.assert_total_order(s1, f1, 0)
        self.assert_total_order(s1, f2, -1)
        self.assert_total_order(s2, f3, 0)
        self.assert_total_order(s3, f4, +1)

        self.assert_total_order(f1, s1, 0)
        self.assert_total_order(f1, s2, -1)
        self.assert_total_order(f2, s3, 0)
        self.assert_total_order(f3, s4, +1)

    def test_mappings(self):
        """ Test comparison for mappings (dict).
        """
        d1 = {1: "a", 2: "b"}
        d2 = {2: "b", 3: "c"}
        d3 = {3: "c", 2: "b"}  # Same value, different instance than d2.
        d4 = {1: "a", 2: "b"}  # Same value, different instance than d1.
        self.assertIsNot(d2, d3, "Testcase error: d2 is d3")
        self.assertIsNot(d1, d4, "Testcase error: d1 is d4")

        self.assert_equality_only(d1, d1, True)
        self.assert_equality_only(d1, d2, False)
        self.assert_equality_only(d2, d3, True)
        self.assert_equality_only(d3, d4, False)


if __name__ == '__main__':
    unittest.main()
