import imp
from importlib.test.import_ import test_relative_imports
from importlib.test.import_ import util as importlib_util
import marshal
import os
import py_compile
import random
import stat
import sys
import unittest
import warnings

from test.support import (
    EnvironmentVarGuard, TESTFN, forget, is_jython,
    rmtree, run_unittest, unlink, unload)


def remove_files(name):
    for f in (name + ".py",
              name + ".pyc",
              name + ".pyo",
              name + ".pyw",
              name + "$py.class"):
        unlink(f)
    rmtree('__pycache__')


class ImportTests(unittest.TestCase):

    def tearDown(self):
        unload(TESTFN)

    setUp = tearDown

    def test_case_sensitivity(self):
        # Brief digression to test that import is case-sensitive:  if we got
        # this far, we know for sure that "random" exists.
        try:
            import RAnDoM
        except ImportError:
            pass
        else:
            self.fail("import of RAnDoM should have failed (case mismatch)")

    def test_double_const(self):
        # Another brief digression to test the accuracy of manifest float
        # constants.
        from test import double_const  # don't blink -- that *was* the test

    def test_import(self):
        def test_with_extension(ext):
            # The extension is normally ".py", perhaps ".pyw".
            source = TESTFN + ext
            pyo = TESTFN + ".pyo"
            if is_jython:
                pyc = TESTFN + "$py.class"
            else:
                pyc = TESTFN + ".pyc"

            with open(source, "w") as f:
                print("# This tests Python's ability to import a",
                      ext, "file.", file=f)
                a = random.randrange(1000)
                b = random.randrange(1000)
                print("a =", a, file=f)
                print("b =", b, file=f)

            if TESTFN in sys.modules:
                del sys.modules[TESTFN]
            try:
                try:
                    mod = __import__(TESTFN)
                except ImportError as err:
                    self.fail("import from %s failed: %s" % (ext, err))

                self.assertEqual(mod.a, a,
                    "module loaded (%s) but contents invalid" % mod)
                self.assertEqual(mod.b, b,
                    "module loaded (%s) but contents invalid" % mod)
            finally:
                forget(TESTFN)
                unlink(source)
                unlink(pyc)
                unlink(pyo)

        sys.path.insert(0, os.curdir)
        try:
            test_with_extension(".py")
            if sys.platform.startswith("win"):
                for ext in [".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw"]:
                    test_with_extension(ext)
        finally:
            del sys.path[0]

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    def test_execute_bit_not_copied(self):
        # Issue 6070: under posix .pyc files got their execute bit set if
        # the .py file had the execute bit set, but they aren't executable.
        oldmask = os.umask(0o022)
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
            self.assertEqual(stat.S_IMODE(s.st_mode),
                             stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
        finally:
            os.umask(oldmask)
            remove_files(TESTFN)
            if TESTFN in sys.modules: del sys.modules[TESTFN]
            del sys.path[0]

    def test_imp_module(self):
        # Verify that the imp module can correctly load and find .py files
        import imp, os
        # XXX (ncoghlan): It would be nice to use support.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        orig_path = os.path
        orig_getenv = os.getenv
        with EnvironmentVarGuard():
            x = imp.find_module("os")
            new_os = imp.load_module("os", *x)
            self.assertIs(os, new_os)
            self.assertIs(orig_path, new_os.path)
            self.assertIsNot(orig_getenv, new_os.getenv)

    def test_module_with_large_stack(self, module='longlist'):
        # Regression test for http://bugs.python.org/issue561858.
        filename = module + '.py'

        # Create a file with a list of 65000 elements.
        with open(filename, 'w') as f:
            f.write('d = [\n')
            for i in range(65000):
                f.write('"",\n')
            f.write(']')

        try:
            # Compile & remove .py file; we only need .pyc (or .pyo).
            py_compile.compile(filename)
        finally:
            unlink(filename)

        # Need to be able to load from current dir.
        sys.path.append('')

        try:
            # This used to crash.
            exec('import ' + module)
        finally:
            # Cleanup.
            del sys.path[-1]
            unlink(filename + 'c')
            unlink(filename + 'o')

    def test_failing_import_sticks(self):
        source = TESTFN + ".py"
        with open(source, "w") as f:
            print("a = 1/0", file=f)

        # New in 2.4, we shouldn't be able to import that no matter how often
        # we try.
        sys.path.insert(0, os.curdir)
        if TESTFN in sys.modules:
            del sys.modules[TESTFN]
        try:
            for i in [1, 2, 3]:
                self.assertRaises(ZeroDivisionError, __import__, TESTFN)
                self.assertNotIn(TESTFN, sys.modules,
                                 "damaged module in sys.modules on %i try" % i)
        finally:
            del sys.path[0]
            remove_files(TESTFN)

    def test_import_name_binding(self):
        # import x.y.z binds x in the current namespace
        import test as x
        import test.support
        self.assertTrue(x is test, x.__name__)
        self.assertTrue(hasattr(test.support, "__file__"))

        # import x.y.z as w binds z as w
        import test.support as y
        self.assertTrue(y is test.support, y.__name__)

    def test_import_initless_directory_warning(self):
        with warnings.catch_warnings():
            # Just a random non-package directory we always expect to be
            # somewhere in sys.path...
            warnings.simplefilter('error', ImportWarning)
            self.assertRaises(ImportWarning, __import__, "site-packages")

    def test_failing_reload(self):
        # A failing reload should leave the module object in sys.modules.
        source = TESTFN + os.extsep + "py"
        with open(source, "w") as f:
            f.write("a = 1\nb=2\n")

        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assertIn(TESTFN, sys.modules)
            self.assertEqual(mod.a, 1, "module has wrong attribute values")
            self.assertEqual(mod.b, 2, "module has wrong attribute values")

            # On WinXP, just replacing the .py file wasn't enough to
            # convince reload() to reparse it.  Maybe the timestamp didn't
            # move enough.  We force it to get reparsed by removing the
            # compiled file too.
            remove_files(TESTFN)

            # Now damage the module.
            with open(source, "w") as f:
                f.write("a = 10\nb=20//0\n")

            self.assertRaises(ZeroDivisionError, imp.reload, mod)
            # But we still expect the module to be in sys.modules.
            mod = sys.modules.get(TESTFN)
            self.assertIsNot(mod, None, "expected module to be in sys.modules")

            # We should have replaced a w/ 10, but the old b value should
            # stick.
            self.assertEqual(mod.a, 10, "module has wrong attribute values")
            self.assertEqual(mod.b, 2, "module has wrong attribute values")

        finally:
            del sys.path[0]
            remove_files(TESTFN)
            unload(TESTFN)

    def test_file_to_source(self):
        # check if __file__ points to the source file where available
        source = TESTFN + ".py"
        with open(source, "w") as f:
            f.write("test = None\n")

        sys.path.insert(0, os.curdir)
        try:
            mod = __import__(TESTFN)
            self.assertTrue(mod.__file__.endswith('.py'))
            os.remove(source)
            del sys.modules[TESTFN]
            mod = __import__(TESTFN)
            base, ext = os.path.splitext(mod.__file__)
            self.assertIn(ext, ('.pyc', '.pyo'))
        finally:
            del sys.path[0]
            remove_files(TESTFN)
            if TESTFN in sys.modules:
                del sys.modules[TESTFN]

    def test_import_name_binding(self):
        # import x.y.z binds x in the current namespace.
        import test as x
        import test.support
        self.assertIs(x, test, x.__name__)
        self.assertTrue(hasattr(test.support, "__file__"))

        # import x.y.z as w binds z as w.
        import test.support as y
        self.assertIs(y, test.support, y.__name__)

    def test_import_by_filename(self):
        path = os.path.abspath(TESTFN)
        try:
            __import__(path)
        except ImportError as err:
            self.assertEqual("Import by filename is not supported.",
                              err.args[0])
        else:
            self.fail("import by path didn't raise an exception")


class PycRewritingTests(unittest.TestCase):
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
func_filename = func.__code__.co_filename
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
            unload(self.module_name)
        unlink(self.file_name)
        unlink(self.compiled_name)
        rmtree(self.dir_name)

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
        self.assertEqual(mod.module_filename, self.file_name)
        self.assertEqual(mod.code_filename, self.file_name)
        self.assertEqual(mod.func_filename, self.file_name)

    def test_incorrect_code_name(self):
        py_compile.compile(self.file_name, dfile="another_module.py")
        mod = self.import_module()
        self.assertEqual(mod.module_filename, self.file_name)
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
        foreign_code = test_main.__code__
        pos = constants.index(1)
        constants[pos] = foreign_code
        code = type(code)(code.co_argcount, code.co_kwonlyargcount,
                          code.co_nlocals, code.co_stacksize,
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
    SAMPLES = ('test', 'test\u00e4\u00f6\u00fc\u00df', 'test\u00e9\u00e8',
               'test\u00b0\u00b3\u00b2')
    path = TESTFN

    def setUp(self):
        os.mkdir(self.path)
        self.syspath = sys.path[:]

    def tearDown(self):
        rmtree(self.path)
        sys.path[:] = self.syspath

    # Regression test for http://bugs.python.org/issue1293.
    def test_trailing_slash(self):
        with open(os.path.join(self.path, 'test_trailing_slash.py'), 'w') as f:
            f.write("testdata = 'test_trailing_slash'")
        sys.path.append(self.path+'/')
        mod = __import__("test_trailing_slash")
        self.assertEqual(mod.testdata, 'test_trailing_slash')
        unload("test_trailing_slash")

    # Regression test for http://bugs.python.org/issue3677.
    def _test_UNC_path(self):
        with open(os.path.join(self.path, 'test_trailing_slash.py'), 'w') as f:
            f.write("testdata = 'test_trailing_slash'")
        # Create the UNC path, like \\myhost\c$\foo\bar.
        path = os.path.abspath(self.path)
        import socket
        hn = socket.gethostname()
        drive = path[0]
        unc = "\\\\%s\\%s$"%(hn, drive)
        unc += path[2:]
        sys.path.append(path)
        mod = __import__("test_trailing_slash")
        self.assertEqual(mod.testdata, 'test_trailing_slash')
        unload("test_trailing_slash")

    if sys.platform == "win32":
        test_UNC_path = _test_UNC_path


class RelativeImportTests(unittest.TestCase):

    def tearDown(self):
        unload("test.relimport")
    setUp = tearDown

    def test_relimport_star(self):
        # This will import * from .test_import.
        from . import relimport
        self.assertTrue(hasattr(relimport, "RelativeImportTests"))

    def test_issue3221(self):
        # Note for mergers: the 'absolute' tests from the 2.x branch
        # are missing in Py3k because implicit relative imports are
        # a thing of the past
        #
        # Regression test for http://bugs.python.org/issue3221.
        def check_relative():
            exec("from . import relimport", ns)

        # Check relative import OK with __package__ and __name__ correct
        ns = dict(__package__='test', __name__='test.notarealmodule')
        check_relative()

        # Check relative import OK with only __name__ wrong
        ns = dict(__package__='test', __name__='notarealpkg.notarealmodule')
        check_relative()

        # Check relative import fails with only __package__ wrong
        ns = dict(__package__='foo', __name__='test.notarealmodule')
        self.assertRaises(SystemError, check_relative)

        # Check relative import fails with __package__ and __name__ wrong
        ns = dict(__package__='foo', __name__='notarealpkg.notarealmodule')
        self.assertRaises(SystemError, check_relative)

        # Check relative import fails with package set to a non-string
        ns = dict(__package__=object())
        self.assertRaises(ValueError, check_relative)

    def test_absolute_import_without_future(self):
        # If explicit relative import syntax is used, then do not try
        # to perform an absolute import in the face of failure.
        # Issue #7902.
        with self.assertRaises(ImportError):
            from .os import sep
            self.fail("explicit relative import triggered an "
                      "implicit absolute import")


class RelativeImportFromImportlibTests(test_relative_imports.RelativeImports):

    def setUp(self):
        self._importlib_util_flag = importlib_util.using___import__
        importlib_util.using___import__ = True

    def tearDown(self):
        importlib_util.using___import__ = self._importlib_util_flag


def test_main(verbose=None):
    run_unittest(ImportTests,
                 PycRewritingTests, PathsTests, RelativeImportTests,
                 RelativeImportFromImportlibTests)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    from test.test_import import test_main
    test_main()
