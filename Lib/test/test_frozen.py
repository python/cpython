# Test the frozen module defined in frozen.c.
from __future__ import with_statement

from test.test_support import captured_stdout, run_unittest
import unittest
import sys, os

class FrozenTests(unittest.TestCase):
    def test_frozen(self):
        try:
            import __hello__
        except ImportError as x:
            self.fail("import __hello__ failed:" + str(x))
        self.assertEqual(__hello__.initialized, True)
        self.assertEqual(len(dir(__hello__)), 5)

        try:
            import __phello__
        except ImportError as x:
            self.fail("import __phello__ failed:" + str(x))
        self.assertEqual(__phello__.initialized, True)
        if not "__phello__.spam" in sys.modules:
            self.assertEqual(len(dir(__phello__)), 6, dir(__phello__))
        else:
            self.assertEqual(len(dir(__phello__)), 7, dir(__phello__))

        try:
            import __phello__.spam
        except ImportError as x:
            self.fail("import __phello__.spam failed:" + str(x))
        self.assertEqual(__phello__.spam.initialized, True)
        self.assertEqual(len(dir(__phello__.spam)), 5)
        self.assertEqual(len(dir(__phello__)), 7)

        try:
            import __phello__.foo
        except ImportError:
            pass
        else:
            self.fail("import __phello__.foo should have failed")

def test_main():
    run_unittest(FrozenTests)

if __name__ == "__main__":
    test_main()
