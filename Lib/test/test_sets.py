#!/usr/bin/env python

import unittest, operator, copy
from sets import Set, ImmutableSet
from test import test_support

empty_set = Set()

#==============================================================================

class TestBasicOps(unittest.TestCase):

    def test_repr(self):
        if self.repr is not None:
            assert `self.set` == self.repr, "Wrong representation for " + self.case

    def test_length(self):
        assert len(self.set) == self.length, "Wrong length for " + self.case

    def test_self_equality(self):
        assert self.set == self.set, "Self-equality failed for " + self.case

    def test_equivalent_equality(self):
        assert self.set == self.dup, "Equivalent equality failed for " + self.case

    def test_copy(self):
        assert self.set.copy() == self.dup, "Copy and comparison failed for " + self.case

    def test_self_union(self):
        result = self.set | self.set
        assert result == self.dup, "Self-union failed for " + self.case

    def test_empty_union(self):
        result = self.set | empty_set
        assert result == self.dup, "Union with empty failed for " + self.case

    def test_union_empty(self):
        result = empty_set | self.set
        assert result == self.dup, "Union with empty failed for " + self.case

    def test_self_intersection(self):
        result = self.set & self.set
        assert result == self.dup, "Self-intersection failed for " + self.case

    def test_empty_intersection(self):
        result = self.set & empty_set
        assert result == empty_set, "Intersection with empty failed for " + self.case

    def test_intersection_empty(self):
        result = empty_set & self.set
        assert result == empty_set, "Intersection with empty failed for " + self.case

    def test_self_symmetric_difference(self):
        result = self.set ^ self.set
        assert result == empty_set, "Self-symdiff failed for " + self.case

    def checkempty_symmetric_difference(self):
        result = self.set ^ empty_set
        assert result == self.set, "Symdiff with empty failed for " + self.case

    def test_self_difference(self):
        result = self.set - self.set
        assert result == empty_set, "Self-difference failed for " + self.case

    def test_empty_difference(self):
        result = self.set - empty_set
        assert result == self.dup, "Difference with empty failed for " + self.case

    def test_empty_difference_rev(self):
        result = empty_set - self.set
        assert result == empty_set, "Difference from empty failed for " + self.case

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

class TestBinaryOps(unittest.TestCase):
    def setUp(self):
        self.set = Set((2, 4, 6))

    def test_union_subset(self):
        result = self.set | Set([2])
        assert result == Set((2, 4, 6)), "Subset union"

    def test_union_superset(self):
        result = self.set | Set([2, 4, 6, 8])
        assert result == Set([2, 4, 6, 8]), "Superset union"

    def test_union_overlap(self):
        result = self.set | Set([3, 4, 5])
        assert result == Set([2, 3, 4, 5, 6]), "Overlapping union"

    def test_union_non_overlap(self):
        result = self.set | Set([8])
        assert result == Set([2, 4, 6, 8]), "Non-overlapping union"

    def test_intersection_subset(self):
        result = self.set & Set((2, 4))
        assert result == Set((2, 4)), "Subset intersection"

    def test_intersection_superset(self):
        result = self.set & Set([2, 4, 6, 8])
        assert result == Set([2, 4, 6]), "Superset intersection"

    def test_intersection_overlap(self):
        result = self.set & Set([3, 4, 5])
        assert result == Set([4]), "Overlapping intersection"

    def test_intersection_non_overlap(self):
        result = self.set & Set([8])
        assert result == empty_set, "Non-overlapping intersection"

    def test_sym_difference_subset(self):
        result = self.set ^ Set((2, 4))
        assert result == Set([6]), "Subset symmetric difference"

    def test_sym_difference_superset(self):
        result = self.set ^ Set((2, 4, 6, 8))
        assert result == Set([8]), "Superset symmetric difference"

    def test_sym_difference_overlap(self):
        result = self.set ^ Set((3, 4, 5))
        assert result == Set([2, 3, 5, 6]), "Overlapping symmetric difference"

    def test_sym_difference_non_overlap(self):
        result = self.set ^ Set([8])
        assert result == Set([2, 4, 6, 8]), "Non-overlapping symmetric difference"

#==============================================================================

class TestUpdateOps(unittest.TestCase):
    def setUp(self):
        self.set = Set((2, 4, 6))

    def test_union_subset(self):
        self.set |= Set([2])
        assert self.set == Set((2, 4, 6)), "Subset union"

    def test_union_superset(self):
        self.set |= Set([2, 4, 6, 8])
        assert self.set == Set([2, 4, 6, 8]), "Superset union"

    def test_union_overlap(self):
        self.set |= Set([3, 4, 5])
        assert self.set == Set([2, 3, 4, 5, 6]), "Overlapping union"

    def test_union_non_overlap(self):
        self.set |= Set([8])
        assert self.set == Set([2, 4, 6, 8]), "Non-overlapping union"

    def test_intersection_subset(self):
        self.set &= Set((2, 4))
        assert self.set == Set((2, 4)), "Subset intersection"

    def test_intersection_superset(self):
        self.set &= Set([2, 4, 6, 8])
        assert self.set == Set([2, 4, 6]), "Superset intersection"

    def test_intersection_overlap(self):
        self.set &= Set([3, 4, 5])
        assert self.set == Set([4]), "Overlapping intersection"

    def test_intersection_non_overlap(self):
        self.set &= Set([8])
        assert self.set == empty_set, "Non-overlapping intersection"

    def test_sym_difference_subset(self):
        self.set ^= Set((2, 4))
        assert self.set == Set([6]), "Subset symmetric difference"

    def test_sym_difference_superset(self):
        self.set ^= Set((2, 4, 6, 8))
        assert self.set == Set([8]), "Superset symmetric difference"

    def test_sym_difference_overlap(self):
        self.set ^= Set((3, 4, 5))
        assert self.set == Set([2, 3, 5, 6]), "Overlapping symmetric difference"

    def test_sym_difference_non_overlap(self):
        self.set ^= Set([8])
        assert self.set == Set([2, 4, 6, 8]), "Non-overlapping symmetric difference"

#==============================================================================

class TestMutate(unittest.TestCase):
    def setUp(self):
        self.values = ["a", "b", "c"]
        self.set = Set(self.values)

    def test_add_present(self):
        self.set.add("c")
        assert self.set == Set(("a", "b", "c")), "Adding present element"

    def test_add_absent(self):
        self.set.add("d")
        assert self.set == Set(("a", "b", "c", "d")), "Adding missing element"

    def test_add_until_full(self):
        tmp = Set()
        expected_len = 0
        for v in self.values:
            tmp.add(v)
            expected_len += 1
            assert len(tmp) == expected_len, "Adding values one by one to temporary"
        assert tmp == self.set, "Adding values one by one"

    def test_remove_present(self):
        self.set.remove("b")
        assert self.set == Set(("a", "c")), "Removing present element"

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
        assert self.set == Set(("a", "b")), "Discarding present element"

    def test_discard_absent(self):
        self.set.discard("d")
        assert self.set == Set(("a", "b", "c")), "Discarding missing element"

    def test_clear(self):
        self.set.clear()
        assert len(self.set) == 0, "Clearing set"

    def test_pop(self):
        popped = {}
        while self.set:
            popped[self.set.pop()] = None
        assert len(popped) == len(self.values), "Popping items"
        for v in self.values:
            assert v in popped, "Popping items"

    def test_update_empty_tuple(self):
        self.set.update(())
        assert self.set == Set(self.values), "Updating with empty tuple"

    def test_update_unit_tuple_overlap(self):
        self.set.update(("a",))
        assert self.set == Set(self.values), "Updating with overlapping unit tuple"

    def test_update_unit_tuple_non_overlap(self):
        self.set.update(("a", "z"))
        assert self.set == Set(self.values + ["z"]), "Updating with non-overlapping unit tuple"

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
    def setUp(self):
        self.left  = Set()
        self.right = Set()
        self.name  = "both empty"
        self.cases = "<>"

#------------------------------------------------------------------------------

class TestSubsetEqualNonEmpty(TestSubsets):
    def setUp(self):
        self.left  = Set([1, 2])
        self.right = Set([1, 2])
        self.name  = "equal pair"
        self.cases = "<>"

#------------------------------------------------------------------------------

class TestSubsetEmptyNonEmpty(TestSubsets):
    def setUp(self):
        self.left  = Set()
        self.right = Set([1, 2])
        self.name  = "one empty, one non-empty"
        self.cases = "<"

#------------------------------------------------------------------------------

class TestSubsetPartial(TestSubsets):
    def setUp(self):
        self.left  = Set([1])
        self.right = Set([1, 2])
        self.name  = "one a non-empty subset of other"
        self.cases = "<"

#------------------------------------------------------------------------------

class TestSubsetNonOverlap(TestSubsets):
    def setUp(self):
        self.left  = Set([1])
        self.right = Set([2])
        self.name  = "neither empty, neither contains"
        self.cases = ""

#==============================================================================

class TestOnlySetsInBinaryOps(unittest.TestCase):

    def test_cmp(self):
        try:
            self.other < self.set
            assert 0, "Comparison with non-set on left"
        except TypeError:
            pass
        try:
            self.set >= self.other
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
    suite.addTest(unittest.makeSuite(TestBasicOpsEmpty))
    suite.addTest(unittest.makeSuite(TestBasicOpsSingleton))
    suite.addTest(unittest.makeSuite(TestBasicOpsTuple))
    suite.addTest(unittest.makeSuite(TestBasicOpsTriple))
    suite.addTest(unittest.makeSuite(TestBinaryOps))
    suite.addTest(unittest.makeSuite(TestUpdateOps))
    suite.addTest(unittest.makeSuite(TestMutate))
    suite.addTest(unittest.makeSuite(TestSubsetEqualEmpty))
    suite.addTest(unittest.makeSuite(TestSubsetEqualNonEmpty))
    suite.addTest(unittest.makeSuite(TestSubsetEmptyNonEmpty))
    suite.addTest(unittest.makeSuite(TestSubsetPartial))
    suite.addTest(unittest.makeSuite(TestSubsetNonOverlap))
    suite.addTest(unittest.makeSuite(TestOnlySetsNumeric))
    suite.addTest(unittest.makeSuite(TestOnlySetsDict))
    suite.addTest(unittest.makeSuite(TestOnlySetsOperator))
    suite.addTest(unittest.makeSuite(TestCopyingEmpty))
    suite.addTest(unittest.makeSuite(TestCopyingSingleton))
    suite.addTest(unittest.makeSuite(TestCopyingTriple))
    suite.addTest(unittest.makeSuite(TestCopyingTuple))
    suite.addTest(unittest.makeSuite(TestCopyingNested))
    return suite

#------------------------------------------------------------------------------

def test_main():
    suite = makeAllTests()
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
