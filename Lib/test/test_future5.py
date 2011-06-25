# Check that multiple features can be enabled.
from __future__ import unicode_literals, print_function

import sys
import unittest
from test import support


class TestMultipleFeatures(unittest.TestCase):

    def test_unicode_literals(self):
        self.assertIsInstance("", str)

    def test_print_function(self):
        with support.captured_output("stderr") as s:
            print("foo", file=sys.stderr)
        self.assertEqual(s.getvalue(), "foo\n")


def test_main():
    support.run_unittest(TestMultipleFeatures)
