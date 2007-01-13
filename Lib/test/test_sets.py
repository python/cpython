#!/usr/bin/env python

import warnings
warnings.filterwarnings("ignore", "the sets module is deprecated",
                        DeprecationWarning, "test\.test_sets")

import unittest, operator, copy, pickle, random
from sets import Set, ImmutableSet
from test import test_support

empty_set = Set()

#==============================================================================

class TestBasicOps(unittest.TestCase):

    def test_repr(self):
        if self.repr is not None:
            self.assertEqual(repr(self.set), self.repr)

    def test_length(self):
        self.assertEqual(len(self.set), self.length)

    def test_self_equality(self):
        self.assertEqual(self.set, self.set)

    def test_equivalent_equality(self):
        self.assertEqual(self.set, self.dup)

    def test_copy(self):
        self.assertEqual(self.set.copy(), self.dup)

    def test_self_union(self):
        result = self.set | self.set
        self.assertEqual(result, self.dup)

    def test_empty_union(self):
        result = self.set | empty_set
        self.assertEqual(result, self.dup)

    def test_union_empty(self):
        result = empty_set | self.set
        self.assertEqual(result, self.dup)

    def test_self_intersection(self):
        result = self.set & self.set
        self.assertEqual(result, self.dup)

    def test_empty_intersection(self):
        result = self.set & empty_set
        self.assertEqual(result, empty_set)

    def test_intersection_empty(self):
        result = empty_set & self.set
        self.assertEqual(result, empty_set)

    def test_self_symmetric_difference(self):
        result = self.set ^ self.set
        self.assertEqual(result, empty_set)

    def checkempty_symmetric_difference(self):
        result = self.set ^ empty_set
        self.assertEqual(result, self.set)

    def test_self_difference(self):
        result = self.set - self.set
        self.assertEqual(result, empty_set)

    def test_empty_difference(self):
        result = self.set - empty_set
        self.assertEqual(result, self.dup)

    def test_empty_difference_rev(self):
        result = empty_set - self.set
        self.assertEqual(result, empty_set)

    def test_iteration(self):
        for v in self.set:
            self.assert_(v in self.values)

    def test_pickling(self):
        p = pickle.dumps(self.set)
        copy = pickle.loads(p)
        self.assertEqual(self.set, copy,
                         "%s != %s" % (self.set, copy))

#------------------------------------------------------------------------------

class TestBasicOpsEmpty(TestBasicOps):
    def setUp(self):
        self.case   = "empty set"
        self.values = []
        self.set    = Set(self.values)
        self.dup    = Set(self.values)
        self.length = 0
        self.repr   = "Set([])"

#------------------------------------------------------------------------------

class TestBasicOpsSingleton(TestBasicOps):
    def setUp(self):
        self.case   = "unit set (number)"
        self.values = [3]
        self.set    = Set(self.values)
        self.dup    = Set(self.values)
        self.length = 1
        self.repr   = "Set([3])"

    def test_in(self):
        self.failUnless(3 in self.set)

    def test_not_in(self):
        self.failUnless(2 not in self.set)

#------------------------------------------------------------------------------

class TestBasicOpsTuple(TestBasicOps):
    def setUp(self):
        self.case   = "unit set (tuple)"
        self.values = [(0, "zero")]
        self.set    = Set(self.values)
        self.dup    = Set(self.values)
        self.length = 1
        self.repr   = "Set([(0, 'zero')])"

    def test_in(self):
        self.failUnless((0, "zero") in self.set)

    def test_not_in(self):
        self.failUnless(9 not in self.set)

#------------------------------------------------------------------------------

class TestBasicOpsTriple(TestBasicOps):
    def setUp(self):
        self.case   = "triple set"
        self.values = [0, "zero", operator.add]
        self.set    = Set(self.values)
        self.dup    = Set(self.values)
        self.length = 3
        self.repr   = None

#==============================================================================

def baditer():
    raise TypeError
    yield True

def gooditer():
    yield True

class TestExceptionPropagation(unittest.TestCase):
    """SF 628246:  Set constructor should not trap iterator TypeErrors"""

    def test_instanceWithException(self):
        self.assertRaises(TypeError, Set, baditer())

    def test_instancesWithoutException(self):
        # All of these iterables should load without exception.
        Set([1,2,3])
        Set((1,2,3))
        Set({'one':1, 'two':2, 'three':3})
        Set(xrange(3))
        Set('abc')
        Set(gooditer())

#==============================================================================

class TestSetOfSets(unittest.TestCase):
    def test_constructor(self):
        inner = Set([1])
        outer = Set([inner])
        element = outer.pop()
        self.assertEqual(type(element), ImmutableSet)
        outer.add(inner)        # Rebuild set of sets with .add method
        outer.remove(inner)
        self.assertEqual(outer, Set())   # Verify that remove worked
        outer.discard(inner)    # Absence of KeyError indicates working fine

#==============================================================================

class TestBinaryOps(unittest.TestCase):
    def setUp(self):
        self.set = Set((2, 4, 6))

    def test_eq(self):              # SF bug 643115
        self.assertEqual(self.set, Set({2:1,4:3,6:5}))

    def test_union_subset(self):
        result = self.set | Set([2])
        self.assertEqual(result, Set((2, 4, 6)))

    def test_union_superset(self):
        result = self.set | Set([2, 4, 6, 8])
        self.assertEqual(result, Set([2, 4, 6, 8]))

    def test_union_overlap(self):
        result = self.set | Set([3, 4, 5])
        self.assertEqual(result, Set([2, 3, 4, 5, 6]))

    def test_union_non_overlap(self):
        result = self.set | Set([8])
        self.assertEqual(result, Set([2, 4, 6, 8]))

    def test_intersection_subset(self):
        result = self.set & Set((2, 4))
        self.assertEqual(result, Set((2, 4)))

    def test_intersection_superset(self):
        result = self.set & Set([2, 4, 6, 8])
        self.assertEqual(result, Set([2, 4, 6]))

    def test_intersection_overlap(self):
        result = self.set & Set([3, 4, 5])
        self.assertEqual(result, Set([4]))

    def test_intersection_non_overlap(self):
        result = self.set & Set([8])
        self.assertEqual(result, empty_set)

    def test_sym_difference_subset(self):
        result = self.set ^ Set((2, 4))
        self.assertEqual(result, Set([6]))

    def test_sym_difference_superset(self):
        result = self.set ^ Set((2, 4, 6, 8))
        self.assertEqual(result, Set([8]))

    def test_sym_difference_overlap(self):
        result = self.set ^ Set((3, 4, 5))
        self.assertEqual(result, Set([2, 3, 5, 6]))

    def test_sym_difference_non_overlap(self):
        result = self.set ^ Set([8])
        self.assertEqual(result, Set([2, 4, 6, 8]))

    def test_cmp(self):
        a, b = Set('a'), Set('b')
        self.assertRaises(TypeError, cmp, a, b)

        # You can view this as a buglet:  cmp(a, a) does not raise TypeError,
        # because __eq__ is tried before __cmp__, and a.__eq__(a) returns True,
        # which Python thinks is good enough to synthesize a cmp() result
        # without calling __cmp__.
        self.assertEqual(cmp(a, a), 0)

        self.assertRaises(TypeError, cmp, a, 12)
        self.assertRaises(TypeError, cmp, "abc", a)

    def test_inplace_on_self(self):
        t = self.set.copy()
        t |= t
        self.assertEqual(t, self.set)
        t &= t
        self.assertEqual(t, self.set)
        t -= t
        self.assertEqual(len(t), 0)
        t = self.set.copy()
        t ^= t
        self.assertEqual(len(t), 0)


#==============================================================================

class TestUpdateOps(unittest.TestCase):
    def setUp(self):
        self.set = Set((2, 4, 6))

    def test_union_subset(self):
        self.set |= Set([2])
        self.assertEqual(self.set, Set((2, 4, 6)))

    def test_union_superset(self):
        self.set |= Set([2, 4, 6, 8])
        self.assertEqual(self.set, Set([2, 4, 6, 8]))

    def test_union_overlap(self):
        self.set |= Set([3, 4, 5])
        self.assertEqual(self.set, Set([2, 3, 4, 5, 6]))

    def test_union_non_overlap(self):
        self.set |= Set([8])
        self.assertEqual(self.set, Set([2, 4, 6, 8]))

    def test_union_method_call(self):
        self.set.union_update(Set([3, 4, 5]))
        self.assertEqual(self.set, Set([2, 3, 4, 5, 6]))

    def test_intersection_subset(self):
        self.set &= Set((2, 4))
        self.assertEqual(self.set, Set((2, 4)))

    def test_intersection_superset(self):
        self.set &= Set([2, 4, 6, 8])
        self.assertEqual(self.set, Set([2, 4, 6]))

    def test_intersection_overlap(self):
        self.set &= Set([3, 4, 5])
        self.assertEqual(self.set, Set([4]))

    def test_intersection_non_overlap(self):
        self.set &= Set([8])
        self.assertEqual(self.set, empty_set)

    def test_intersection_method_call(self):
        self.set.intersection_update(Set([3, 4, 5]))
        self.assertEqual(self.set, Set([4]))

    def test_sym_difference_subset(self):
        self.set ^= Set((2, 4))
        self.assertEqual(self.set, Set([6]))

    def test_sym_difference_superset(self):
        self.set ^= Set((2, 4, 6, 8))
        self.assertEqual(self.set, Set([8]))

    def test_sym_difference_overlap(self):
        self.set ^= Set((3, 4, 5))
        self.assertEqual(self.set, Set([2, 3, 5, 6]))

    def test_sym_difference_non_overlap(self):
        self.set ^= Set([8])
        self.assertEqual(self.set, Set([2, 4, 6, 8]))

    def test_sym_difference_method_call(self):
        self.set.symmetric_difference_update(Set([3, 4, 5]))
        self.assertEqual(self.set, Set([2, 3, 5, 6]))

    def test_difference_subset(self):
        self.set -= Set((2, 4))
        self.assertEqual(self.set, Set([6]))

    def test_difference_superset(self):
        self.set -= Set((2, 4, 6, 8))
        self.assertEqual(self.set, Set([]))

    def test_difference_overlap(self):
        self.set -= Set((3, 4, 5))
        self.assertEqual(self.set, Set([2, 6]))

    def test_difference_non_overlap(self):
        self.set -= Set([8])
        self.assertEqual(self.set, Set([2, 4, 6]))

    def test_difference_method_call(self):
        self.set.difference_update(Set([3, 4, 5]))
        self.assertEqual(self.set, Set([2, 6]))

#==============================================================================

class TestMutate(unittest.TestCase):
    def setUp(self):
        self.values = ["a", "b", "c"]
        self.set = Set(self.values)

    def test_add_present(self):
        self.set.add("c")
        self.assertEqual(self.set, Set("abc"))

    def test_add_absent(self):
        self.set.add("d")
        self.assertEqual(self.set, Set("abcd"))

    def test_add_until_full(self):
        tmp = Set()
        expected_len = 0
        for v in self.values:
            tmp.add(v)
            expected_len += 1
            self.assertEqual(len(tmp), expected_len)
        self.assertEqual(tmp, self.set)

    def test_remove_present(self):
        self.set.remove("b")
        self.assertEqual(self.set, Set("ac"))

    def test_remove_absent(self):
        try:
            self.set.remove("d")
            self.fail("Removing missing element should have raised LookupError")
        except LookupError:
            pass

    def test_remove_until_empty(self):
        expected_len = len(self.set)
        for v in self.values:
            self.set.remove(v)
            expected_len -= 1
            self.assertEqual(len(self.set), expected_len)

    def test_discard_present(self):
        self.set.discard("c")
        self.assertEqual(self.set, Set("ab"))

    def test_discard_absent(self):
        self.set.discard("d")
        self.assertEqual(self.set, Set("abc"))

    def test_clear(self):
        self.set.clear()
        self.assertEqual(len(self.set), 0)

    def test_pop(self):
        popped = {}
        while self.set:
            popped[self.set.pop()] = None
        self.assertEqual(len(popped), len(self.values))
        for v in self.values:
            self.failUnless(v in popped)

    def test_update_empty_tuple(self):
        self.set.union_update(())
        self.assertEqual(self.set, Set(self.values))

    def test_update_unit_tuple_overlap(self):
        self.set.union_update(("a",))
        self.assertEqual(self.set, Set(self.values))

    def test_update_unit_tuple_non_overlap(self):
        self.set.union_update(("a", "z"))
        self.assertEqual(self.set, Set(self.values + ["z"]))

#==============================================================================

class TestSubsets(unittest.TestCase):

    case2method = {"<=": "issubset",
                   ">=": "issuperset",
                  }

    reverse = {"==": "==",
               "!=": "!=",
               "<":  ">",
               ">":  "<",
               "<=": ">=",
               ">=": "<=",
              }

    def test_issubset(self):
        x = self.left
        y = self.right
        for case in "!=", "==", "<", "<=", ">", ">=":
            expected = case in self.cases
            # Test the binary infix spelling.
            result = eval("x" + case + "y", locals())
            self.assertEqual(result, expected)
            # Test the "friendly" method-name spelling, if one exists.
            if case in TestSubsets.case2method:
                method = getattr(x, TestSubsets.case2method[case])
                result = method(y)
                self.assertEqual(result, expected)

            # Now do the same for the operands reversed.
            rcase = TestSubsets.reverse[case]
            result = eval("y" + rcase + "x", locals())
            self.assertEqual(result, expected)
            if rcase in TestSubsets.case2method:
                method = getattr(y, TestSubsets.case2method[rcase])
                result = method(x)
                self.assertEqual(result, expected)
#------------------------------------------------------------------------------

class TestSubsetEqualEmpty(TestSubsets):
    left  = Set()
    right = Set()
    name  = "both empty"
    cases = "==", "<=", ">="

#------------------------------------------------------------------------------

class TestSubsetEqualNonEmpty(TestSubsets):
    left  = Set([1, 2])
    right = Set([1, 2])
    name  = "equal pair"
    cases = "==", "<=", ">="

#------------------------------------------------------------------------------

class TestSubsetEmptyNonEmpty(TestSubsets):
    left  = Set()
    right = Set([1, 2])
    name  = "one empty, one non-empty"
    cases = "!=", "<", "<="

#------------------------------------------------------------------------------

class TestSubsetPartial(TestSubsets):
    left  = Set([1])
    right = Set([1, 2])
    name  = "one a non-empty proper subset of other"
    cases = "!=", "<", "<="

#------------------------------------------------------------------------------

class TestSubsetNonOverlap(TestSubsets):
    left  = Set([1])
    right = Set([2])
    name  = "neither empty, neither contains"
    cases = "!="

#==============================================================================

class TestOnlySetsInBinaryOps(unittest.TestCase):

    def test_eq_ne(self):
        # Unlike the others, this is testing that == and != *are* allowed.
        self.assertEqual(self.other == self.set, False)
        self.assertEqual(self.set == self.other, False)
        self.assertEqual(self.other != self.set, True)
        self.assertEqual(self.set != self.other, True)

    def test_ge_gt_le_lt(self):
        self.assertRaises(TypeError, lambda: self.set < self.other)
        self.assertRaises(TypeError, lambda: self.set <= self.other)
        self.assertRaises(TypeError, lambda: self.set > self.other)
        self.assertRaises(TypeError, lambda: self.set >= self.other)

        self.assertRaises(TypeError, lambda: self.other < self.set)
        self.assertRaises(TypeError, lambda: self.other <= self.set)
        self.assertRaises(TypeError, lambda: self.other > self.set)
        self.assertRaises(TypeError, lambda: self.other >= self.set)

    def test_union_update_operator(self):
        try:
            self.set |= self.other
        except TypeError:
            pass
        else:
            self.fail("expected TypeError")

    def test_union_update(self):
        if self.otherIsIterable:
            self.set.union_update(self.other)
        else:
            self.assertRaises(TypeError, self.set.union_update, self.other)

    def test_union(self):
        self.assertRaises(TypeError, lambda: self.set | self.other)
        self.assertRaises(TypeError, lambda: self.other | self.set)
        if self.otherIsIterable:
            self.set.union(self.other)
        else:
            self.assertRaises(TypeError, self.set.union, self.other)

    def test_intersection_update_operator(self):
        try:
            self.set &= self.other
        except TypeError:
            pass
        else:
            self.fail("expected TypeError")

    def test_intersection_update(self):
        if self.otherIsIterable:
            self.set.intersection_update(self.other)
        else:
            self.assertRaises(TypeError,
                              self.set.intersection_update,
                              self.other)

    def test_intersection(self):
        self.assertRaises(TypeError, lambda: self.set & self.other)
        self.assertRaises(TypeError, lambda: self.other & self.set)
        if self.otherIsIterable:
            self.set.intersection(self.other)
        else:
            self.assertRaises(TypeError, self.set.intersection, self.other)

    def test_sym_difference_update_operator(self):
        try:
            self.set ^= self.other
        except TypeError:
            pass
        else:
            self.fail("expected TypeError")

    def test_sym_difference_update(self):
        if self.otherIsIterable:
            self.set.symmetric_difference_update(self.other)
        else:
            self.assertRaises(TypeError,
                              self.set.symmetric_difference_update,
                              self.other)

    def test_sym_difference(self):
        self.assertRaises(TypeError, lambda: self.set ^ self.other)
        self.assertRaises(TypeError, lambda: self.other ^ self.set)
        if self.otherIsIterable:
            self.set.symmetric_difference(self.other)
        else:
            self.assertRaises(TypeError, self.set.symmetric_difference, self.other)

    def test_difference_update_operator(self):
        try:
            self.set -= self.other
        except TypeError:
            pass
        else:
            self.fail("expected TypeError")

    def test_difference_update(self):
        if self.otherIsIterable:
            self.set.difference_update(self.other)
        else:
            self.assertRaises(TypeError,
                              self.set.difference_update,
                              self.other)

    def test_difference(self):
        self.assertRaises(TypeError, lambda: self.set - self.other)
        self.assertRaises(TypeError, lambda: self.other - self.set)
        if self.otherIsIterable:
            self.set.difference(self.other)
        else:
            self.assertRaises(TypeError, self.set.difference, self.other)

#------------------------------------------------------------------------------

class TestOnlySetsNumeric(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = 19
        self.otherIsIterable = False

#------------------------------------------------------------------------------

class TestOnlySetsDict(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = {1:2, 3:4}
        self.otherIsIterable = True

#------------------------------------------------------------------------------

class TestOnlySetsOperator(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = operator.add
        self.otherIsIterable = False

#------------------------------------------------------------------------------

class TestOnlySetsTuple(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = (2, 4, 6)
        self.otherIsIterable = True

#------------------------------------------------------------------------------

class TestOnlySetsString(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = 'abc'
        self.otherIsIterable = True

#------------------------------------------------------------------------------

class TestOnlySetsGenerator(TestOnlySetsInBinaryOps):
    def setUp(self):
        def gen():
            for i in xrange(0, 10, 2):
                yield i
        self.set   = Set((1, 2, 3))
        self.other = gen()
        self.otherIsIterable = True

#------------------------------------------------------------------------------

class TestOnlySetsofSets(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = [Set('ab'), ImmutableSet('cd')]
        self.otherIsIterable = True

#==============================================================================

class TestCopying(unittest.TestCase):

    def test_copy(self):
        dup = self.set.copy()
        dup_list = list(dup); dup_list.sort()
        set_list = list(self.set); set_list.sort()
        self.assertEqual(len(dup_list), len(set_list))
        for i in range(len(dup_list)):
            self.failUnless(dup_list[i] is set_list[i])

    def test_deep_copy(self):
        dup = copy.deepcopy(self.set)
        ##print type(dup), repr(dup)
        dup_list = list(dup); dup_list.sort()
        set_list = list(self.set); set_list.sort()
        self.assertEqual(len(dup_list), len(set_list))
        for i in range(len(dup_list)):
            self.assertEqual(dup_list[i], set_list[i])

#------------------------------------------------------------------------------

class TestCopyingEmpty(TestCopying):
    def setUp(self):
        self.set = Set()

#------------------------------------------------------------------------------

class TestCopyingSingleton(TestCopying):
    def setUp(self):
        self.set = Set(["hello"])

#------------------------------------------------------------------------------

class TestCopyingTriple(TestCopying):
    def setUp(self):
        self.set = Set(["zero", 0, None])

#------------------------------------------------------------------------------

class TestCopyingTuple(TestCopying):
    def setUp(self):
        self.set = Set([(1, 2)])

#------------------------------------------------------------------------------

class TestCopyingNested(TestCopying):
    def setUp(self):
        self.set = Set([((1, 2), (3, 4))])

#==============================================================================

class TestIdentities(unittest.TestCase):
    def setUp(self):
        self.a = Set([random.randrange(100) for i in xrange(50)])
        self.b = Set([random.randrange(100) for i in xrange(50)])

    def test_binopsVsSubsets(self):
        a, b = self.a, self.b
        self.assert_(a - b <= a)
        self.assert_(b - a <= b)
        self.assert_(a & b <= a)
        self.assert_(a & b <= b)
        self.assert_(a | b >= a)
        self.assert_(a | b >= b)
        self.assert_(a ^ b <= a | b)

    def test_commutativity(self):
        a, b = self.a, self.b
        self.assertEqual(a&b, b&a)
        self.assertEqual(a|b, b|a)
        self.assertEqual(a^b, b^a)
        if a != b:
            self.assertNotEqual(a-b, b-a)

    def test_reflexsive_relations(self):
        a, zero = self.a, Set()
        self.assertEqual(a ^ a, zero)
        self.assertEqual(a - a, zero)
        self.assertEqual(a | a, a)
        self.assertEqual(a & a, a)
        self.assert_(a <= a)
        self.assert_(a >= a)
        self.assert_(a == a)

    def test_summations(self):
        # check that sums of parts equal the whole
        a, b = self.a, self.b
        self.assertEqual((a-b)|(a&b)|(b-a), a|b)
        self.assertEqual((a&b)|(a^b), a|b)
        self.assertEqual(a|(b-a), a|b)
        self.assertEqual((a-b)|b, a|b)
        self.assertEqual((a-b)|(a&b), a)
        self.assertEqual((b-a)|(a&b), b)
        self.assertEqual((a-b)|(b-a), a^b)

    def test_exclusion(self):
        # check that inverse operations do not overlap
        a, b, zero = self.a, self.b, Set()
        self.assertEqual((a-b)&b, zero)
        self.assertEqual((b-a)&a, zero)
        self.assertEqual((a&b)&(a^b), zero)

    def test_cardinality_relations(self):
        a, b = self.a, self.b
        self.assertEqual(len(a), len(a-b) + len(a&b))
        self.assertEqual(len(b), len(b-a) + len(a&b))
        self.assertEqual(len(a^b), len(a-b) + len(b-a))
        self.assertEqual(len(a|b), len(a-b) + len(a&b) + len(b-a))
        self.assertEqual(len(a^b) + len(a&b), len(a|b))

#==============================================================================

libreftest = """
Example from the Library Reference:  Doc/lib/libsets.tex

>>> from sets import Set as Base  # override _repr to get sorted output
>>> class Set(Base):
...     def _repr(self):
...         return Base._repr(self, sorted=True)
>>> engineers = Set(['John', 'Jane', 'Jack', 'Janice'])
>>> programmers = Set(['Jack', 'Sam', 'Susan', 'Janice'])
>>> managers = Set(['Jane', 'Jack', 'Susan', 'Zack'])
>>> employees = engineers | programmers | managers           # union
>>> engineering_management = engineers & managers            # intersection
>>> fulltime_management = managers - engineers - programmers # difference
>>> engineers.add('Marvin')
>>> print engineers
Set(['Jack', 'Jane', 'Janice', 'John', 'Marvin'])
>>> employees.issuperset(engineers)           # superset test
False
>>> employees.union_update(engineers)         # update from another set
>>> employees.issuperset(engineers)
True
>>> for group in [engineers, programmers, managers, employees]:
...     group.discard('Susan')                # unconditionally remove element
...     print group
...
Set(['Jack', 'Jane', 'Janice', 'John', 'Marvin'])
Set(['Jack', 'Janice', 'Sam'])
Set(['Jack', 'Jane', 'Zack'])
Set(['Jack', 'Jane', 'Janice', 'John', 'Marvin', 'Sam', 'Zack'])
"""

#==============================================================================

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    import doctest
    from test import test_sets
    test_support.run_unittest(
        TestSetOfSets,
        TestExceptionPropagation,
        TestBasicOpsEmpty,
        TestBasicOpsSingleton,
        TestBasicOpsTuple,
        TestBasicOpsTriple,
        TestBinaryOps,
        TestUpdateOps,
        TestMutate,
        TestSubsetEqualEmpty,
        TestSubsetEqualNonEmpty,
        TestSubsetEmptyNonEmpty,
        TestSubsetPartial,
        TestSubsetNonOverlap,
        TestOnlySetsNumeric,
        TestOnlySetsDict,
        TestOnlySetsOperator,
        TestOnlySetsTuple,
        TestOnlySetsString,
        TestOnlySetsGenerator,
        TestOnlySetsofSets,
        TestCopyingEmpty,
        TestCopyingSingleton,
        TestCopyingTriple,
        TestCopyingTuple,
        TestCopyingNested,
        TestIdentities,
        doctest.DocTestSuite(test_sets),
    )

if __name__ == "__main__":
    test_main(verbose=True)
