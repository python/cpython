#!/usr/bin/env python

import unittest, operator, copy
from sets import Set, ImmutableSet
from test import test_support

empty_set = Set()

#==============================================================================

class TestBasicOps(unittest.TestCase):

    def test_repr(self):
        if self.repr is not None:
            self.assertEqual(`self.set`, self.repr)

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
            assert v in self.values, "Missing item in iteration for " + self.case

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
        assert 3 in self.set, "Valueship for unit set"

    def test_not_in(self):
        assert 2 not in self.set, "Non-valueship for unit set"

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
        assert (0, "zero") in self.set, "Valueship for tuple set"

    def test_not_in(self):
        assert 9 not in self.set, "Non-valueship for tuple set"

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

class TestSetOfSets(unittest.TestCase):
    def test_constructor(self):
        inner = Set([1])
        outer = Set([inner])
        element = outer.pop()
        assert type(element) == ImmutableSet, "Construct set of sets"
        outer.add(inner)        # Rebuild set of sets with .add method
        outer.remove(inner)
        assert outer == Set()   # Verify that remove worked
        outer.discard(inner)    # Absence of KeyError indicates working fine

#==============================================================================

class TestBinaryOps(unittest.TestCase):
    def setUp(self):
        self.set = Set((2, 4, 6))

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
            assert 0, "Removing missing element"
        except LookupError:
            pass

    def test_remove_until_empty(self):
        expected_len = len(self.set)
        for v in self.values:
            self.set.remove(v)
            expected_len -= 1
            assert len(self.set) == expected_len, "Removing values one by one"

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
            assert v in popped, "Popping items"

    def test_update_empty_tuple(self):
        self.set.update(())
        self.assertEqual(self.set, Set(self.values))

    def test_update_unit_tuple_overlap(self):
        self.set.update(("a",))
        self.assertEqual(self.set, Set(self.values))

    def test_update_unit_tuple_non_overlap(self):
        self.set.update(("a", "z"))
        self.assertEqual(self.set, Set(self.values + ["z"]))

#==============================================================================

class TestSubsets(unittest.TestCase):

    def test_issubset(self):
        result = self.left.issubset(self.right)
        if "<" in self.cases:
            assert result, "subset: " + self.name
        else:
            assert not result, "non-subset: " + self.name

#------------------------------------------------------------------------------

class TestSubsetEqualEmpty(TestSubsets):
    left  = Set()
    right = Set()
    name  = "both empty"
    cases = "<>"

#------------------------------------------------------------------------------

class TestSubsetEqualNonEmpty(TestSubsets):
    left  = Set([1, 2])
    right = Set([1, 2])
    name  = "equal pair"
    cases = "<>"

#------------------------------------------------------------------------------

class TestSubsetEmptyNonEmpty(TestSubsets):
    left  = Set()
    right = Set([1, 2])
    name  = "one empty, one non-empty"
    cases = "<"

#------------------------------------------------------------------------------

class TestSubsetPartial(TestSubsets):
   left  = Set([1])
   right = Set([1, 2])
   name  = "one a non-empty subset of other"
   cases = "<"

#------------------------------------------------------------------------------

class TestSubsetNonOverlap(TestSubsets):
    left  = Set([1])
    right = Set([2])
    name  = "neither empty, neither contains"
    cases = ""

#==============================================================================

class TestOnlySetsInBinaryOps(unittest.TestCase):

    def test_cmp(self):
        try:
            self.other == self.set
            assert 0, "Comparison with non-set on left"
        except TypeError:
            pass
        try:
            self.set != self.other
            assert 0, "Comparison with non-set on right"
        except TypeError:
            pass

    def test_union_update(self):
        try:
            self.set |= self.other
            assert 0, "Union update with non-set"
        except TypeError:
            pass

    def test_union(self):
        try:
            self.other | self.set
            assert 0, "Union with non-set on left"
        except TypeError:
            pass
        try:
            self.set | self.other
            assert 0, "Union with non-set on right"
        except TypeError:
            pass

    def test_intersection_update(self):
        try:
            self.set &= self.other
            assert 0, "Intersection update with non-set"
        except TypeError:
            pass

    def test_intersection(self):
        try:
            self.other & self.set
            assert 0, "Intersection with non-set on left"
        except TypeError:
            pass
        try:
            self.set & self.other
            assert 0, "Intersection with non-set on right"
        except TypeError:
            pass

    def test_sym_difference_update(self):
        try:
            self.set ^= self.other
            assert 0, "Symmetric difference update with non-set"
        except TypeError:
            pass

    def test_sym_difference(self):
        try:
            self.other ^ self.set
            assert 0, "Symmetric difference with non-set on left"
        except TypeError:
            pass
        try:
            self.set ^ self.other
            assert 0, "Symmetric difference with non-set on right"
        except TypeError:
            pass

    def test_difference_update(self):
        try:
            self.set -= self.other
            assert 0, "Symmetric difference update with non-set"
        except TypeError:
            pass

    def test_difference(self):
        try:
            self.other - self.set
            assert 0, "Symmetric difference with non-set on left"
        except TypeError:
            pass
        try:
            self.set - self.other
            assert 0, "Symmetric difference with non-set on right"
        except TypeError:
            pass

#------------------------------------------------------------------------------

class TestOnlySetsNumeric(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = 19

#------------------------------------------------------------------------------

class TestOnlySetsDict(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = {1:2, 3:4}

#------------------------------------------------------------------------------

class TestOnlySetsOperator(TestOnlySetsInBinaryOps):
    def setUp(self):
        self.set   = Set((1, 2, 3))
        self.other = operator.add

#==============================================================================

class TestCopying(unittest.TestCase):

    def test_copy(self):
        dup = self.set.copy()
        dup_list = list(dup); dup_list.sort()
        set_list = list(self.set); set_list.sort()
        assert len(dup_list) == len(set_list), "Unequal lengths after copy"
        for i in range(len(dup_list)):
            assert dup_list[i] is set_list[i], "Non-identical items after copy"

    def test_deep_copy(self):
        dup = copy.deepcopy(self.set)
        ##print type(dup), `dup`
        dup_list = list(dup); dup_list.sort()
        set_list = list(self.set); set_list.sort()
        assert len(dup_list) == len(set_list), "Unequal lengths after deep copy"
        for i in range(len(dup_list)):
            assert dup_list[i] == set_list[i], "Unequal items after deep copy"

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

def makeAllTests():
    suite = unittest.TestSuite()
    for klass in (TestSetOfSets,
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
                  TestCopyingEmpty,
                  TestCopyingSingleton,
                  TestCopyingTriple,
                  TestCopyingTuple,
                  TestCopyingNested,
                 ):
        suite.addTest(unittest.makeSuite(klass))
    return suite

#------------------------------------------------------------------------------

def test_main():
    suite = makeAllTests()
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
