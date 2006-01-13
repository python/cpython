#
# This file is for everybody to add tests for bugs that aren't
# fixed yet. Please add a test case and appropriate bug description.
#
# When you fix one of the bugs, please move the test to the correct
# test_ module.
#

import unittest
from test import test_support

class TestBug1385040(unittest.TestCase):
    def testSyntaxError(self):
        import compiler

        # The following snippet gives a SyntaxError in the interpreter
        #
        # If you compile and exec it, the call foo(7) returns (7, 1)
        self.assertRaises(SyntaxError, compiler.compile,
                          "def foo(a=1, b): return a, b\n\n", "<string>", "exec")


def test_main():
    test_support.run_unittest(TestBug1385040)

if __name__ == "__main__":
    test_main()
