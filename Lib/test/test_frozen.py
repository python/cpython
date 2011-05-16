# Test the frozen module defined in frozen.c.

from test.support import captured_stdout, run_unittest
import unittest
import sys

class FrozenTests(unittest.TestCase):
    def test_frozen(self):
        with captured_stdout() as stdout:
            try:
                import __hello__
            except ImportError as x:
                self.fail("import __hello__ failed:" + str(x))
            self.assertEqual(__hello__.initialized, True)
            self.assertEqual(len(dir(__hello__)), 7, dir(__hello__))
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

        with captured_stdout() as stdout:
            try:
                import __phello__
            except ImportError as x:
                self.fail("import __phello__ failed:" + str(x))
            self.assertEqual(__phello__.initialized, True)
            if not "__phello__.spam" in sys.modules:
                self.assertEqual(len(dir(__phello__)), 8, dir(__phello__))
            else:
                self.assertEqual(len(dir(__phello__)), 9, dir(__phello__))
            self.assertEqual(__phello__.__path__, [__phello__.__name__])
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

        with captured_stdout() as stdout:
            try:
                import __phello__.spam
            except ImportError as x:
                self.fail("import __phello__.spam failed:" + str(x))
            self.assertEqual(__phello__.spam.initialized, True)
            self.assertEqual(len(dir(__phello__.spam)), 7)
            self.assertEqual(len(dir(__phello__)), 9)
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

        try:
            import __phello__.foo
        except ImportError:
            pass
        else:
            self.fail("import __phello__.foo should have failed")

            try:
                import __phello__.foo
            except ImportError:
                pass
            else:
                self.fail("import __phello__.foo should have failed")

        del sys.modules['__hello__']
        del sys.modules['__phello__']
        del sys.modules['__phello__.spam']

def test_main():
    run_unittest(FrozenTests)

if __name__ == "__main__":
    test_main()
