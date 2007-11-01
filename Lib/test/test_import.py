from test.test_support import TESTFN, run_unittest, catch_warning

import unittest
import os
import random
import shutil
import sys
import py_compile
import warnings
from test.test_support import unlink, TESTFN, unload


def remove_files(name):
    for f in (name + ".py",
              name + ".pyc",
              name + ".pyo",
              name + ".pyw",
              name + "$py.class"):
        if os.path.exists(f):
            os.remove(f)


class ImportTest(unittest.TestCase):

    def testCaseSensitivity(self):
        # Brief digression to test that import is case-sensitive:  if we got this
        # far, we know for sure that "random" exists.
        try:
            import RAnDoM
        except ImportError:
            pass
        else:
            self.fail("import of RAnDoM should have failed (case mismatch)")

    def testDoubleConst(self):
        # Another brief digression to test the accuracy of manifest float constants.
        from test import double_const  # don't blink -- that *was* the test

    def testImport(self):
        def test_with_extension(ext):
            # ext normally ".py"; perhaps ".pyw"
            source = TESTFN + ext
            pyo = TESTFN + ".pyo"
            if sys.platform.startswith('java'):
                pyc = TESTFN + "$py.class"
            else:
                pyc = TESTFN + ".pyc"

            f = open(source, "w")
            print("# This tests Python's ability to import a", ext, "file.", file=f)
            a = random.randrange(1000)
            b = random.randrange(1000)
            print("a =", a, file=f)
            print("b =", b, file=f)
            f.close()

            try:
                try:
                    mod = __import__(TESTFN)
                except ImportError as err:
                    self.fail("import from %s failed: %s" % (ext, err))

                self.assertEquals(mod.a, a,
                    "module loaded (%s) but contents invalid" % mod)
                self.assertEquals(mod.b, b,
                    "module loaded (%s) but contents invalid" % mod)
            finally:
                unlink(source)
                unlink(pyc)
                unlink(pyo)
                del sys.modules[TESTFN]

        sys.path.insert(0, os.curdir)
        try:
            test_with_extension(".py")
            if sys.platform.startswith("win"):
                for ext in ".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw":
                    test_with_extension(ext)
        finally:
            del sys.path[0]

    def testImpModule(self):
        # Verify that the imp module can correctly load and find .py files
        import imp
        x = imp.find_module("os")
        os = imp.load_module("os", *x)

    def test_module_with_large_stack(self, module='longlist'):
        # create module w/list of 65000 elements to test bug #561858
        filename = module + '.py'

        # create a file with a list of 65000 elements
        f = open(filename, 'w+')
        f.write('d = [\n')
        for i in range(65000):
            f.write('"",\n')
        f.write(']')
        f.close()

        # compile & remove .py file, we only need .pyc (or .pyo)
        f = open(filename, 'r')
        py_compile.compile(filename)
        f.close()
        os.unlink(filename)

        # need to be able to load from current dir
        sys.path.append('')

        # this used to crash
        exec('import ' + module)

        # cleanup
        del sys.path[-1]
        for ext in '.pyc', '.pyo':
            fname = module + ext
            if os.path.exists(fname):
                os.unlink(fname)

    def test_failing_import_sticks(self):
        source = TESTFN + ".py"
        f = open(source, "w")
        print("a = 1/0", file=f)
        f.close()

        # New in 2.4, we shouldn't be able to import that no matter how often
        # we try.
        sys.path.insert(0, os.curdir)
        if TESTFN in sys.modules:
            del sys.modules[TESTFN]
        try:
            for i in 1, 2, 3:
                try:
                    mod = __import__(TESTFN)
                except ZeroDivisionError:
                    if TESTFN in sys.modules:
                        self.fail("damaged module in sys.modules on %i. try" % i)
                else:
                    self.fail("was able to import a damaged module on %i. try" % i)
        finally:
            sys.path.pop(0)
            remove_files(TESTFN)

    def test_import_name_binding(self):
        # import x.y.z binds x in the current namespace
        import test as x
        import test.test_support
        self.assert_(x is test, x.__name__)
        self.assert_(hasattr(test.test_support, "__file__"))

        # import x.y.z as w binds z as w
        import test.test_support as y
        self.assert_(y is test.test_support, y.__name__)

    def test_import_initless_directory_warning(self):
        with catch_warning():
            # Just a random non-package directory we always expect to be
            # somewhere in sys.path...
            warnings.simplefilter('error', ImportWarning)
            self.assertRaises(ImportWarning, __import__, "site-packages")

class UnicodePathsTests(unittest.TestCase):
    SAMPLES = ('test', 'testäöüß', 'testéè', 'test°³²')
    path = TESTFN

    def setUp(self):
        os.mkdir(self.path)
        self.syspath = sys.path[:]

    def tearDown(self):
        shutil.rmtree(self.path)
        sys.path = self.syspath

    def test_sys_path(self):
        for i, subpath in enumerate(self.SAMPLES):
            path = os.path.join(self.path, subpath)
            os.mkdir(path)
            self.failUnless(os.path.exists(path), os.listdir(self.path))
            f = open(os.path.join(path, 'testimport%i.py' % i), 'w')
            f.write("testdata = 'unicode path %i'\n" % i)
            f.close()
            sys.path.append(path)
            try:
                mod = __import__("testimport%i" % i)
            except ImportError:
                print(path, file=sys.stderr)
                raise
            self.assertEqual(mod.testdata, 'unicode path %i' % i)
            unload("testimport%i" % i)

def test_main(verbose=None):
    run_unittest(ImportTest, UnicodePathsTests)

if __name__ == '__main__':
    test_main()
