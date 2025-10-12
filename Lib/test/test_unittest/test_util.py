import unittest
from unittest.util import safe_repr, sorted_list_difference, unorderable_list_difference


class TestUtil(unittest.TestCase):
    def test_safe_repr(self):
        class RaisingRepr:
            def __repr__(self):
                raise ValueError("Invalid repr()")

        class LongRepr:
            def __repr__(self):
                return 'x' * 100

        safe_repr(RaisingRepr())
        self.assertEqual(safe_repr('foo'), "'foo'")
        self.assertEqual(safe_repr(LongRepr(), short=True), 'x'*80 + ' [truncated]...')

    def test_sorted_list_difference(self):
        self.assertEqual(sorted_list_difference([], []), ([], []))
        self.assertEqual(sorted_list_difference([1, 2], [2, 3]), ([1], [3]))
        self.assertEqual(sorted_list_difference([1, 2], [1, 3]), ([2], [3]))
        self.assertEqual(sorted_list_difference([1, 1, 1], [1, 2, 3]), ([], [2, 3]))
        self.assertEqual(sorted_list_difference([4], [1, 2, 3, 4]), ([], [1, 2, 3]))
        self.assertEqual(sorted_list_difference([1, 1], [2]), ([1], [2]))
        self.assertEqual(sorted_list_difference([2], [1, 1]), ([2], [1]))
        self.assertEqual(sorted_list_difference([1, 2], [1, 1]), ([2], []))

    def test_unorderable_list_difference(self):
        self.assertEqual(unorderable_list_difference([], []), ([], []))
        self.assertEqual(unorderable_list_difference([1, 2], []), ([2, 1], []))
        self.assertEqual(unorderable_list_difference([], [1, 2]), ([], [1, 2]))
        self.assertEqual(unorderable_list_difference([1, 2], [1, 3]), ([2], [3]))
