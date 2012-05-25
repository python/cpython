import builtins
import imp
from importlib.test.import_ import test_suite as importlib_import_test_suite
from importlib.test.import_ import util as importlib_util
import importlib
import marshal
import os
import platform
import py_compile
import random
import stat
import sys
import unittest
import textwrap
import errno

from test.support import (
    EnvironmentVarGuard, TESTFN, check_warnings, forget, is_jython,
    make_legacy_pyc, rmtree, run_unittest, swap_attr, swap_item, temp_umask,
    unlink, unload, create_empty_file)
from test import script_helper


def remove_files(name):
    for f in (name + ".py",
              name + ".pyc",
              name + ".pyo",
              name + ".pyw",
              name + "$py.class"):
        unlink(f)
    rmtree('__pycache__')


class ImportTests(unittest.TestCase):

    def setUp(self):
        remove_files(TESTFN)
        importlib.invalidate_caches()

    def tearDown(self):
        unload(TESTFN)

    setUp = tearDown

    def test_case_sensitivity(self):
        # Brief digression to test that import is case-sensitive:  if we got
        # this far, we know for sure that "random" exists.
        with self.assertRaises(ImportError):
            import RAnDoM

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
            importlib.invalidate_caches()
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
    def test_creation_mode(self):
        mask = 0o022
        with temp_umask(mask):
            sys.path.insert(0, os.curdir)
            try:
                fname = TESTFN + os.extsep + "py"
                create_empty_file(fname)
                fn = imp.cache_from_source(fname)
                unlink(fn)
                importlib.invalidate_caches()
                __import__(TESTFN)
                if not os.path.exists(fn):
                    self.fail("__import__ did not result in creation of "
                              "either a .pyc or .pyo file")
                s = os.stat(fn)
                # Check that the umask is respected, and the executable bits
                # aren't set.
                self.assertEqual(stat.S_IMODE(s.st_mode), 0o666 & ~mask)
            finally:
                del sys.path[0]
                remove_files(TESTFN)
                unload(TESTFN)

    def test_imp_module(self):
        # Verify that the imp module can correctly load and find .py files
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
            self.addCleanup(x[0].close)
            new_os = imp.load_module("os", *x)
            self.assertIs(os, new_os)
            self.assertIs(orig_path, new_os.path)
            self.assertIsNot(orig_getenv, new_os.getenv)

    def test_bug7732(self):
        source = TESTFN + '.py'
        os.mkdir(source)
        try:
            self.assertRaisesRegex(ImportError, '^No module',
                imp.find_module, TESTFN, ["."])
        finally:
            os.rmdir(source)

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
            # Bytecode must be relocated from the PEP 3147 bytecode-only location.
            py_compile.compile(filename)
        finally:
            unlink(filename)

        # Need to be able to load from current dir.
        sys.path.append('')
        importlib.invalidate_caches()

        try:
            make_legacy_pyc(filename)
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
            make_legacy_pyc(source)
            importlib.invalidate_caches()
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
        encoding = sys.getfilesystemencoding()
        try:
            path.encode(encoding)
        except UnicodeEncodeError:
            self.skipTest('path is not encodable to {}'.format(encoding))
        with self.assertRaises(ImportError) as c:
            __import__(path)

    def test_import_in_del_does_not_crash(self):
        # Issue 4236
        testfn = script_helper.make_script('', TESTFN, textwrap.dedent("""\
            import sys
            class C:
               def __del__(self):
                  import imp
            sys.argv.insert(0, C())
            """))
        script_helper.assert_python_ok(testfn)

    def test_timestamp_overflow(self):
        # A modification timestamp larger than 2**32 should not be a problem
        # when importing a module (issue #11235).
        sys.path.insert(0, os.curdir)
        try:
            source = TESTFN + ".py"
            compiled = imp.cache_from_source(source)
            with open(source, 'w') as f:
                pass
            try:
                os.utime(source, (2 ** 33 - 5, 2 ** 33 - 5))
            except OverflowError:
                self.skipTest("cannot set modification time to large integer")
            except OSError as e:
                if e.errno != getattr(errno, 'EOVERFLOW', None):
                    raise
                self.skipTest("cannot set modification time to large integer ({})".format(e))
            __import__(TESTFN)
            # The pyc file was created.
            os.stat(compiled)
        finally:
            del sys.path[0]
            remove_files(TESTFN)


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
    compiled_name = imp.cache_from_source(file_name)

    def setUp(self):
        self.sys_path = sys.path[:]
        self.orig_module = sys.modules.pop(self.module_name, None)
        os.mkdir(self.dir_name)
        with open(self.file_name, "w") as f:
            f.write(self.module_source)
        sys.path.insert(0, self.dir_name)
        importlib.invalidate_caches()

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
        pyc_file = make_legacy_pyc(self.file_name)
        importlib.invalidate_caches()
        mod = self.import_module()
        self.assertEqual(mod.module_filename, pyc_file)
        self.assertEqual(mod.code_filename, target)
        self.assertEqual(mod.func_filename, target)

    def test_foreign_code(self):
        py_compile.compile(self.file_name)
        with open(self.compiled_name, "rb") as f:
            header = f.read(12)
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
    @unittest.skipUnless(sys.platform == 'win32', 'Windows-specific')
    def test_UNC_path(self):
        with open(os.path.join(self.path, 'test_trailing_slash.py'), 'w') as f:
            f.write("testdata = 'test_trailing_slash'")
        importlib.invalidate_caches()
        # Create the UNC path, like \\myhost\c$\foo\bar.
        path = os.path.abspath(self.path)
        import socket
        hn = socket.gethostname()
        drive = path[0]
        unc = "\\\\%s\\%s$"%(hn, drive)
        unc += path[2:]
        sys.path.append(unc)
        try:
            mod = __import__("test_trailing_slash")
            self.assertEqual(mod.testdata, 'test_trailing_slash')
            unload("test_trailing_slash")
        finally:
            sys.path.remove(unc)


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
        self.assertRaises(TypeError, check_relative)

    def test_absolute_import_without_future(self):
        # If explicit relative import syntax is used, then do not try
        # to perform an absolute import in the face of failure.
        # Issue #7902.
        with self.assertRaises(ImportError):
            from .os import sep
            self.fail("explicit relative import triggered an "
                      "implicit absolute import")


class OverridingImportBuiltinTests(unittest.TestCase):
    def test_override_builtin(self):
        # Test that overriding builtins.__import__ can bypass sys.modules.
        import os

        def foo():
            import os
            return os
        self.assertEqual(foo(), os)  # Quick sanity check.

        with swap_attr(builtins, "__import__", lambda *x: 5):
            self.assertEqual(foo(), 5)

        # Test what happens when we shadow __import__ in globals(); this
        # currently does not impact the import process, but if this changes,
        # other code will need to change, so keep this test as a tripwire.
        with swap_item(globals(), "__import__", lambda *x: 5):
            self.assertEqual(foo(), os)


class PycacheTests(unittest.TestCase):
    # Test the various PEP 3147 related behaviors.

    tag = imp.get_tag()

    def _clean(self):
        forget(TESTFN)
        rmtree('__pycache__')
        unlink(self.source)

    def setUp(self):
        self.source = TESTFN + '.py'
        self._clean()
        with open(self.source, 'w') as fp:
            print('# This is a test file written by test_import.py', file=fp)
        sys.path.insert(0, os.curdir)
        importlib.invalidate_caches()

    def tearDown(self):
        assert sys.path[0] == os.curdir, 'Unexpected sys.path[0]'
        del sys.path[0]
        self._clean()

    def test_import_pyc_path(self):
        self.assertFalse(os.path.exists('__pycache__'))
        __import__(TESTFN)
        self.assertTrue(os.path.exists('__pycache__'))
        self.assertTrue(os.path.exists(os.path.join(
            '__pycache__', '{}.{}.py{}'.format(
            TESTFN, self.tag, __debug__ and 'c' or 'o'))))

    @unittest.skipUnless(os.name == 'posix',
                         "test meaningful only on posix systems")
    @unittest.skipIf(hasattr(os, 'geteuid') and os.geteuid() == 0,
            "due to varying filesystem permission semantics (issue #11956)")
    def test_unwritable_directory(self):
        # When the umask causes the new __pycache__ directory to be
        # unwritable, the import still succeeds but no .pyc file is written.
        with temp_umask(0o222):
            __import__(TESTFN)
        self.assertTrue(os.path.exists('__pycache__'))
        self.assertFalse(os.path.exists(os.path.join(
            '__pycache__', '{}.{}.pyc'.format(TESTFN, self.tag))))

    def test_missing_source(self):
        # With PEP 3147 cache layout, removing the source but leaving the pyc
        # file does not satisfy the import.
        __import__(TESTFN)
        pyc_file = imp.cache_from_source(self.source)
        self.assertTrue(os.path.exists(pyc_file))
        os.remove(self.source)
        forget(TESTFN)
        self.assertRaises(ImportError, __import__, TESTFN)

    def test_missing_source_legacy(self):
        # Like test_missing_source() except that for backward compatibility,
        # when the pyc file lives where the py file would have been (and named
        # without the tag), it is importable.  The __file__ of the imported
        # module is the pyc location.
        __import__(TESTFN)
        # pyc_file gets removed in _clean() via tearDown().
        pyc_file = make_legacy_pyc(self.source)
        os.remove(self.source)
        unload(TESTFN)
        importlib.invalidate_caches()
        m = __import__(TESTFN)
        self.assertEqual(m.__file__,
                         os.path.join(os.curdir, os.path.relpath(pyc_file)))

    def test___cached__(self):
        # Modules now also have an __cached__ that points to the pyc file.
        m = __import__(TESTFN)
        pyc_file = imp.cache_from_source(TESTFN + '.py')
        self.assertEqual(m.__cached__, os.path.join(os.curdir, pyc_file))

    def test___cached___legacy_pyc(self):
        # Like test___cached__() except that for backward compatibility,
        # when the pyc file lives where the py file would have been (and named
        # without the tag), it is importable.  The __cached__ of the imported
        # module is the pyc location.
        __import__(TESTFN)
        # pyc_file gets removed in _clean() via tearDown().
        pyc_file = make_legacy_pyc(self.source)
        os.remove(self.source)
        unload(TESTFN)
        importlib.invalidate_caches()
        m = __import__(TESTFN)
        self.assertEqual(m.__cached__,
                         os.path.join(os.curdir, os.path.relpath(pyc_file)))

    def test_package___cached__(self):
        # Like test___cached__ but for packages.
        def cleanup():
            rmtree('pep3147')
        os.mkdir('pep3147')
        self.addCleanup(cleanup)
        # Touch the __init__.py
        with open(os.path.join('pep3147', '__init__.py'), 'w'):
            pass
        with open(os.path.join('pep3147', 'foo.py'), 'w'):
            pass
        unload('pep3147.foo')
        unload('pep3147')
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        init_pyc = imp.cache_from_source(
            os.path.join('pep3147', '__init__.py'))
        self.assertEqual(m.__cached__, os.path.join(os.curdir, init_pyc))
        foo_pyc = imp.cache_from_source(os.path.join('pep3147', 'foo.py'))
        self.assertEqual(sys.modules['pep3147.foo'].__cached__,
                         os.path.join(os.curdir, foo_pyc))

    def test_package___cached___from_pyc(self):
        # Like test___cached__ but ensuring __cached__ when imported from a
        # PEP 3147 pyc file.
        def cleanup():
            rmtree('pep3147')
        os.mkdir('pep3147')
        self.addCleanup(cleanup)
        unload('pep3147.foo')
        unload('pep3147')
        # Touch the __init__.py
        with open(os.path.join('pep3147', '__init__.py'), 'w'):
            pass
        with open(os.path.join('pep3147', 'foo.py'), 'w'):
            pass
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        unload('pep3147.foo')
        unload('pep3147')
        importlib.invalidate_caches()
        m = __import__('pep3147.foo')
        init_pyc = imp.cache_from_source(
            os.path.join('pep3147', '__init__.py'))
        self.assertEqual(m.__cached__, os.path.join(os.curdir, init_pyc))
        foo_pyc = imp.cache_from_source(os.path.join('pep3147', 'foo.py'))
        self.assertEqual(sys.modules['pep3147.foo'].__cached__,
                         os.path.join(os.curdir, foo_pyc))

    def test_recompute_pyc_same_second(self):
        # Even when the source file doesn't change timestamp, a change in
        # source size is enough to trigger recomputation of the pyc file.
        __import__(TESTFN)
        unload(TESTFN)
        with open(self.source, 'a') as fp:
            print("x = 5", file=fp)
        m = __import__(TESTFN)
        self.assertEqual(m.x, 5)


def test_main(verbose=None):
    flag = importlib_util.using___import__
    try:
        importlib_util.using___import__ = True
        run_unittest(ImportTests, PycacheTests,
                     PycRewritingTests, PathsTests, RelativeImportTests,
                     OverridingImportBuiltinTests,
                     importlib_import_test_suite())
    finally:
        importlib_util.using___import__ = flag


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    from test.test_import import test_main
    test_main()
