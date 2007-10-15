# Test the frozen module defined in frozen.c.
from __future__ import with_statement

from test.test_support import captured_stdout, run_unittest
import unittest
import sys, os

class FrozenTests(unittest.TestCase):
    def test_frozen(self):

        with captured_stdout() as stdout:
            try:
                import __hello__
            except ImportError as x:
                self.fail("import __hello__ failed:" + str(x))

            try:
                import __phello__
            except ImportError as x:
                self.fail("import __phello__ failed:" + str(x))

            try:
                import __phello__.spam
            except ImportError as x:
                self.fail("import __phello__.spam failed:" + str(x))

            if sys.platform != "mac":  # On the Mac this import does succeed.
                try:
                    import __phello__.foo
                except ImportError:
                    pass
                else:
                    self.fail("import __phello__.foo should have failed")

        self.assertEquals(stdout.getvalue(),
                          'Hello world...\nHello world...\nHello world...\n')


def test_main():
    run_unittest(FrozenTests)

if __name__ == "__main__":
    test_main()
