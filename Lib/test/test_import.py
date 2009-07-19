import unittest
import os
import stat
import random
import shutil
import sys
import py_compile
import warnings
import marshal
from test.test_support import (unlink, TESTFN, unload, run_unittest,
    check_warnings, TestFailed)


def remove_files(name):
    for f in (name + os.extsep + "py",
              name + os.extsep + "pyc",
              name + os.extsep + "pyo",
              name + os.extsep + "pyw",
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
            pyo = TESTFN + os.extsep + "pyo"
            if sys.platform.startswith('java'):
                pyc = TESTFN + "$py.class"
            else:
                pyc = TESTFN + os.extsep + "pyc"

            f = open(source, "w")
            print >> f, "# This tests Python's ability to import a", ext, "file."
            a = random.randrange(1000)
            b = random.randrange(1000)
            print >> f, "a =", a
            print >> f, "b =", b
            f.close()

            try:
                try:
                    mod = __import__(TESTFN)
                except ImportError, err:
                    self.fail("import from %s failed: %s" % (ext, err))

                self.assertEquals(mod.a, a,
                    "module loaded (%s) but contents invalid" % mod)
                self.assertEquals(mod.b, b,
                    "module loaded (%s) but contents invalid" % mod)
            finally:
                os.unlink(source)

            try:
                try:
                    reload(mod)
                except ImportError, err:
                    self.fail("import from .pyc/.pyo failed: %s" % err)
            finally:
                try:
                    os.unlink(pyc)
                except OSError:
                    pass
                try:
                    os.unlink(pyo)
                except OSError:
                    pass
                del sys.modules[TESTFN]

        sys.path.insert(0, os.curdir)
        try:
            test_with_extension(os.extsep + "py")
            if sys.platform.startswith("win"):
                for ext in ".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw":
                    test_with_extension(ext)
        finally:
            del sys.path[0]

    if os.name == 'posix':
        def test_execute_bit_not_copied(self):
            # Issue 6070: under posix .pyc files got their execute bit set if
            # the .py file had the execute bit set, but they aren't executable.
            oldmask = os.umask(022)
            sys.path.insert(0, os.curdir)
            try:
                fname = TESTFN + os.extsep + "py"
                f = open(fname, 'w').close()
                os.chmod(fname, (stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH |
                                 stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH))
                __import__(TESTFN)
                fn = fname + 'c'
                if not os.path.exists(fn):
                    fn = fname + 'o'
                    if not os.path.exists(fn): raise TestFailed("__import__ did "
                        "not result in creation of either a .pyc or .pyo file")
                s = os.stat(fn)
                self.assertEquals(stat.S_IMODE(s.st_mode),
                                  stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
            finally:
                os.umask(oldmask)
                remove_files(TESTFN)
                if TESTFN in sys.modules: del sys.modules[TESTFN]
                del sys.path[0]

    def testImpModule(self):
        # Verify that the imp module can correctly load and find .py files
        import imp
        x = imp.find_module("os")
        os = imp.load_module("os", *x)

    def test_module_with_large_stack(self, module='longlist'):
        # create module w/list of 65000 elements to test bug #561858
        filename = module + os.extsep + 'py'

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
        exec 'import ' + module

        # cleanup
        del sys.path[-1]
        for ext in 'pyc', 'pyo':
            fname = module + os.extsep + ext
            if os.path.exists(fname):
                os.unlink(fname)

    def test_failing_import_sticks(self):
        source = TESTFN + os.extsep + "py"
        f = open(source, "w")
        print >> f, "a = 1/0"
        f.close()

        # New in 2.4, we shouldn't be able to import that no matter how often
        # we try.
        sys.path.insert(0, os.curdir)
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

    def test_failing_reload(self):
        # A failing reload should leave the module object in sys.modules.
        source = TESTFN + os.extsep + "py"
        f = open(source, "w")
        print >> f, "a = 1"
        print >> f, "b = 2"
        f.close()

        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assert_(TESTFN in sys.modules, "expected module in sys.modules")
            self.assertEquals(mod.a, 1, "module has wrong attribute values")
            self.assertEquals(mod.b, 2, "module has wrong attribute values")

            # On WinXP, just replacing the .py file wasn't enough to
            # convince reload() to reparse it.  Maybe the timestamp didn't
            # move enough.  We force it to get reparsed by removing the
            # compiled file too.
            remove_files(TESTFN)

            # Now damage the module.
            f = open(source, "w")
            print >> f, "a = 10"
            print >> f, "b = 20//0"
            f.close()

            self.assertRaises(ZeroDivisionError, reload, mod)

            # But we still expect the module to be in sys.modules.
            mod = sys.modules.get(TESTFN)
            self.failIf(mod is None, "expected module to still be in sys.modules")

            # We should have replaced a w/ 10, but the old b value should
            # stick.
            self.assertEquals(mod.a, 10, "module has wrong attribute values")
            self.assertEquals(mod.b, 2, "module has wrong attribute values")

        finally:
            sys.path.pop(0)
            remove_files(TESTFN)
            if TESTFN in sys.modules:
                del sys.modules[TESTFN]

    def test_infinite_reload(self):
        # Bug #742342 reports that Python segfaults (infinite recursion in C)
        #  when faced with self-recursive reload()ing.

        sys.path.insert(0, os.path.dirname(__file__))
        try:
            import infinite_reload
        finally:
            sys.path.pop(0)

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
        with warnings.catch_warnings():
            # Just a random non-package directory we always expect to be
            # somewhere in sys.path...
            warnings.simplefilter('error', ImportWarning)
            self.assertRaises(ImportWarning, __import__, "site-packages")

    def test_importbyfilename(self):
        path = os.path.abspath(TESTFN)
        try:
            __import__(path)
        except ImportError, err:
            self.assertEqual("Import by filename is not supported.",
                              err.args[0])
        else:
            self.fail("import by path didn't raise an exception")


class TestPycRewriting(unittest.TestCase):
    # Test that the `co_filename` attribute on code objects always points
    # to the right file, even when various things happen (e.g. both the .py
    # and the .pyc file are renamed).

    module_name = "unlikely_module_name"
    module_source = """
import sys
code_filename = sys._getframe().f_code.co_filename
module_filename = __file__
constant = 1
def func():
    pass
func_filename = func.func_code.co_filename
"""
    dir_name = os.path.abspath(TESTFN)
    file_name = os.path.join(dir_name, module_name) + os.extsep + "py"
    compiled_name = file_name + ("c" if __debug__ else "o")

    def setUp(self):
        self.sys_path = sys.path[:]
        self.orig_module = sys.modules.pop(self.module_name, None)
        os.mkdir(self.dir_name)
        with open(self.file_name, "w") as f:
            f.write(self.module_source)
        sys.path.insert(0, self.dir_name)

    def tearDown(self):
        sys.path[:] = self.sys_path
        if self.orig_module is not None:
            sys.modules[self.module_name] = self.orig_module
        else:
            del sys.modules[self.module_name]
        for file_name in self.file_name, self.compiled_name:
            if os.path.exists(file_name):
                os.remove(file_name)
        if os.path.exists(self.dir_name):
            shutil.rmtree(self.dir_name)

    def import_module(self):
        ns = globals()
        __import__(self.module_name, ns, ns)
        return sys.modules[self.module_name]

    def test_basics(self):
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.file_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)
        del sys.modules[self.module_name]
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.compiled_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)

    def test_incorrect_code_name(self):
        py_compile.compile(self.file_name, dfile="another_module.py")
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.compiled_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)

    def test_module_without_source(self):
        target = "another_module.py"
        py_compile.compile(self.file_name, dfile=target)
        os.remove(self.file_name)
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.compiled_name)
        self.assertEqual(mod.code_filename, target)
        self.assertEqual(mod.func_filename, target)

    def test_foreign_code(self):
        py_compile.compile(self.file_name)
        with open(self.compiled_name, "rb") as f:
            header = f.read(8)
            code = marshal.load(f)
        constants = list(code.co_consts)
        foreign_code = test_main.func_code
        pos = constants.index(1)
        constants[pos] = foreign_code
        code = type(code)(code.co_argcount, code.co_nlocals, code.co_stacksize,
                          code.co_flags, code.co_code, tuple(constants),
                          code.co_names, code.co_varnames, code.co_filename,
                          code.co_name, code.co_firstlineno, code.co_lnotab,
                          code.co_freevars, code.co_cellvars)
        with open(self.compiled_name, "wb") as f:
            f.write(header)
            marshal.dump(code, f)
        mod = self.import_module()
        self.assertEqual(mod.constant.co_filename, foreign_code.co_filename)

class PathsTests(unittest.TestCase):
    path = TESTFN

    def setUp(self):
        os.mkdir(self.path)
        self.syspath = sys.path[:]

    def tearDown(self):
        shutil.rmtree(self.path)
        sys.path = self.syspath

    # http://bugs.python.org/issue1293
    def test_trailing_slash(self):
        f = open(os.path.join(self.path, 'test_trailing_slash.py'), 'w')
        f.write("testdata = 'test_trailing_slash'")
        f.close()
        sys.path.append(self.path+'/')
        mod = __import__("test_trailing_slash")
        self.assertEqual(mod.testdata, 'test_trailing_slash')
        unload("test_trailing_slash")

class RelativeImport(unittest.TestCase):
    def tearDown(self):
        try:
            del sys.modules["test.relimport"]
        except:
            pass

    def test_relimport_star(self):
        # This will import * from .test_import.
        from . import relimport
        self.assertTrue(hasattr(relimport, "RelativeImport"))

    def test_issue3221(self):
        def check_absolute():
            exec "from os import path" in ns
        def check_relative():
            exec "from . import relimport" in ns
        # Check both OK with __package__ and __name__ correct
        ns = dict(__package__='test', __name__='test.notarealmodule')
        check_absolute()
        check_relative()
        # Check both OK with only __name__ wrong
        ns = dict(__package__='test', __name__='notarealpkg.notarealmodule')
        check_absolute()
        check_relative()
        # Check relative fails with only __package__ wrong
        ns = dict(__package__='foo', __name__='test.notarealmodule')
        with check_warnings() as w:
            check_absolute()
            self.assert_('foo' in str(w.message))
            self.assertEqual(w.category, RuntimeWarning)
        self.assertRaises(SystemError, check_relative)
        # Check relative fails with __package__ and __name__ wrong
        ns = dict(__package__='foo', __name__='notarealpkg.notarealmodule')
        with check_warnings() as w:
            check_absolute()
            self.assert_('foo' in str(w.message))
            self.assertEqual(w.category, RuntimeWarning)
        self.assertRaises(SystemError, check_relative)
        # Check both fail with package set to a non-string
        ns = dict(__package__=object())
        self.assertRaises(ValueError, check_absolute)
        self.assertRaises(ValueError, check_relative)

def test_main(verbose=None):
    run_unittest(ImportTest, TestPycRewriting, PathsTests, RelativeImport)

if __name__ == '__main__':
    # test needs to be a package, so we can do relative import
    from test.test_import import test_main
    test_main()
