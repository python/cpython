import os, sys, string, random, tempfile, unittest

from test.test_support import run_unittest

class TestImport(unittest.TestCase):

    def __init__(self, *args, **kw):
        self.package_name = 'PACKAGE_'
        while self.package_name in sys.modules:
            self.package_name += random.choose(string.letters)
        self.module_name = self.package_name + '.foo'
        unittest.TestCase.__init__(self, *args, **kw)

    def remove_modules(self):
        for module_name in (self.package_name, self.module_name):
            if module_name in sys.modules:
                del sys.modules[module_name]

    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        sys.path.append(self.test_dir)
        self.package_dir = os.path.join(self.test_dir,
                                        self.package_name)
        os.mkdir(self.package_dir)
        open(os.path.join(
                self.package_dir, '__init__'+os.extsep+'py'), 'w').close()
        self.module_path = os.path.join(self.package_dir, 'foo'+os.extsep+'py')

    def tearDown(self):
        for file in os.listdir(self.package_dir):
            os.remove(os.path.join(self.package_dir, file))
        os.rmdir(self.package_dir)
        os.rmdir(self.test_dir)
        self.assertNotEqual(sys.path.count(self.test_dir), 0)
        sys.path.remove(self.test_dir)
        self.remove_modules()

    def rewrite_file(self, contents):
        for extension in "co":
            compiled_path = self.module_path + extension
            if os.path.exists(compiled_path):
                os.remove(compiled_path)
        f = open(self.module_path, 'w')
        f.write(contents)
        f.close()

    def test_package_import__semantics(self):

        # Generate a couple of broken modules to try importing.

        # ...try loading the module when there's a SyntaxError
        self.rewrite_file('for')
        try: __import__(self.module_name)
        except SyntaxError: pass
        else: raise RuntimeError, 'Failed to induce SyntaxError'
        self.assertNotIn(self.module_name, sys.modules)
        self.assertFalse(hasattr(sys.modules[self.package_name], 'foo'))

        # ...make up a variable name that isn't bound in __builtins__
        var = 'a'
        while var in dir(__builtins__):
            var += random.choose(string.letters)

        # ...make a module that just contains that
        self.rewrite_file(var)

        try: __import__(self.module_name)
        except NameError: pass
        else: raise RuntimeError, 'Failed to induce NameError.'

        # ...now  change  the module  so  that  the NameError  doesn't
        # happen
        self.rewrite_file('%s = 1' % var)
        module = __import__(self.module_name).foo
        self.assertEqual(getattr(module, var), 1)


def test_main():
    run_unittest(TestImport)


if __name__ == "__main__":
    test_main()
