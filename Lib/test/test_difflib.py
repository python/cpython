import difflib
from test import test_support
import unittest

class TestSFbugs(unittest.TestCase):

    def test_ratio_for_null_seqn(self):
        # Check clearing of SF bug 763023
        s = difflib.SequenceMatcher(None, [], [])
        self.assertEqual(s.ratio(), 1)
        self.assertEqual(s.quick_ratio(), 1)
        self.assertEqual(s.real_quick_ratio(), 1)

test_support.run_unittest(TestSFbugs)
test_support.run_doctest(difflib)
