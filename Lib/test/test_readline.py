"""
Very minimal unittests for parts of the readline module.
"""
import os
import unittest
from test.test_support import run_unittest, import_module
from test.script_helper import assert_python_ok

# Skip tests if there is no readline module
readline = import_module('readline')

class TestHistoryManipulation (unittest.TestCase):
    """These tests were added to check that the libedit emulation on OSX and
    the "real" readline have the same interface for history manipulation.
    That's why the tests cover only a small subset of the interface.
    """

    @unittest.skipUnless(hasattr(readline, "clear_history"),
                         "The history update test cannot be run because the "
                         "clear_history method is not available.")
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


class TestReadline(unittest.TestCase):

    @unittest.skipIf(readline._READLINE_VERSION < 0x0600
                     and "libedit" not in readline.__doc__,
                     "not supported in this library version")
    def test_init(self):
        # Issue #19884: Ensure that the ANSI sequence "\033[1034h" is not
        # written into stdout when the readline module is imported and stdout
        # is redirected to a pipe.
        rc, stdout, stderr = assert_python_ok('-c', 'import readline',
                                              TERM='xterm-256color')
        self.assertEqual(stdout, b'')


def test_main():
    run_unittest(TestHistoryManipulation, TestReadline)

if __name__ == "__main__":
    test_main()
