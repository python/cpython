import difflib
from test import test_support
import unittest
import doctest

class TestSFbugs(unittest.TestCase):

    def test_ratio_for_null_seqn(self):
        # Check clearing of SF bug 763023
        s = difflib.SequenceMatcher(None, [], [])
        self.assertEqual(s.ratio(), 1)
        self.assertEqual(s.quick_ratio(), 1)
        self.assertEqual(s.real_quick_ratio(), 1)

    def test_comparing_empty_lists(self):
        # Check fix for bug #979794
        group_gen = difflib.SequenceMatcher(None, [], []).get_grouped_opcodes()
        self.assertRaises(StopIteration, group_gen.next)
        diff_gen = difflib.unified_diff([], [])
        self.assertRaises(StopIteration, diff_gen.next)

Doctests = doctest.DocTestSuite(difflib)

test_support.run_unittest(TestSFbugs, Doctests)
