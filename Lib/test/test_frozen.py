# Test the frozen module defined in frozen.c.

from test.support import captured_stdout, run_unittest
import unittest
import sys

class FrozenTests(unittest.TestCase):

    module_attrs = frozenset(['__builtins__', '__cached__', '__doc__',
                              '__loader__', '__name__',
                              '__package__'])
    package_attrs = frozenset(list(module_attrs) + ['__path__'])

    def test_frozen(self):
        with captured_stdout() as stdout:
            try:
                import __hello__
            except ImportError as x:
                self.fail("import __hello__ failed:" + str(x))
            self.assertEqual(__hello__.initialized, True)
            expect = set(self.module_attrs)
            expect.add('initialized')
            self.assertEqual(set(dir(__hello__)), expect)
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

        with captured_stdout() as stdout:
            try:
                import __phello__
            except ImportError as x:
                self.fail("import __phello__ failed:" + str(x))
            self.assertEqual(__phello__.initialized, True)
            expect = set(self.package_attrs)
            expect.add('initialized')
            if not "__phello__.spam" in sys.modules:
                self.assertEqual(set(dir(__phello__)), expect)
            else:
                expect.add('spam')
                self.assertEqual(set(dir(__phello__)), expect)
            self.assertEqual(__phello__.__path__, [__phello__.__name__])
            self.assertEqual(stdout.getvalue(), 'Hello world!\n')

        with captured_stdout() as stdout:
            try:
                import __phello__.spam
            except ImportError as x:
                self.fail("import __phello__.spam failed:" + str(x))
            self.assertEqual(__phello__.spam.initialized, True)
            spam_expect = set(self.module_attrs)
            spam_expect.add('initialized')
            self.assertEqual(set(dir(__phello__.spam)), spam_expect)
            phello_expect = set(self.package_attrs)
            phello_expect.add('initialized')
            phello_expect.add('spam')
            self.assertEqual(set(dir(__phello__)), phello_expect)
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
