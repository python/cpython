"""
Very minimal unittests for parts of the readline module.

These tests were added to check that the libedit emulation on OSX and
the "real" readline have the same interface for history manipulation. That's
why the tests cover only a small subset of the interface.
"""
import unittest
from test.test_support import run_unittest, import_module

# Skip tests if there is no readline module
readline = import_module('readline')

class TestHistoryManipulation (unittest.TestCase):
    def testHistoryUpdates(self):
        readline.clear_history()

        readline.add_history("first line")
        readline.add_history("second line")

        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "first line")
        self.assertEqual(readline.get_history_item(2), "second line")

        readline.replace_history_item(0, "replaced line")
        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "replaced line")
        self.assertEqual(readline.get_history_item(2), "second line")

        self.assertEqual(readline.get_current_history_length(), 2)

        readline.remove_history_item(0)
        self.assertEqual(readline.get_history_item(0), None)
        self.assertEqual(readline.get_history_item(1), "second line")

        self.assertEqual(readline.get_current_history_length(), 1)


def test_main():
    run_unittest(TestHistoryManipulation)

if __name__ == "__main__":
    test_main()
