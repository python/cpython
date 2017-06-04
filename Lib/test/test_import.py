import errno
import imp
import marshal
import os
import py_compile
import random
import stat
import struct
import sys
import unittest
import textwrap
import shutil

from test.test_support import (unlink, TESTFN, unload, run_unittest, rmtree,
                               is_jython, check_warnings, EnvironmentVarGuard)
from test import symlink_support
from test import script_helper

def _files(name):
    return (name + os.extsep + "py",
            name + os.extsep + "pyc",
            name + os.extsep + "pyo",
            name + os.extsep + "pyw",
            name + "$py.class")

def chmod_files(name):
    for f in _files(name):
        try:
            os.chmod(f, 0600)
        except OSError as exc:
            if exc.errno != errno.ENOENT:
                raise

def remove_files(name):
    for f in _files(name):
        unlink(f)


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
            pyo = TESTFN + os.extsep + "pyo"
            if is_jython:
                pyc = TESTFN + "$py.class"
            else:
                pyc = TESTFN + os.extsep + "pyc"

            with open(source, "w") as f:
                print >> f, ("# This tests Python's ability to import a", ext,
                             "file.")
                a = random.randrange(1000)
                b = random.randrange(1000)
                print >> f, "a =", a
                print >> f, "b =", b

            try:
                mod = __import__(TESTFN)
            except ImportError, err:
                self.fail("import from %s failed: %s" % (ext, err))
            else:
                self.assertEqual(mod.a, a,
                    "module loaded (%s) but contents invalid" % mod)
                self.assertEqual(mod.b, b,
                    "module loaded (%s) but contents invalid" % mod)
            finally:
                unlink(source)

            try:
                if not sys.dont_write_bytecode:
                    imp.reload(mod)
            except ImportError, err:
                self.fail("import from .pyc/.pyo failed: %s" % err)
            finally:
                unlink(pyc)
                unlink(pyo)
                unload(TESTFN)

        sys.path.insert(0, os.curdir)
        try:
            test_with_extension(os.extsep + "py")
            if sys.platform.startswith("win"):
                for ext in [".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw"]:
                    test_with_extension(ext)
        finally:
            del sys.path[0]

    @unittest.skipUnless(os.name == 'posix',
        "test meaningful only on posix systems")
    @unittest.skipIf(sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")
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
                if not os.path.exists(fn):
                    self.fail("__import__ did not result in creation of "
                              "either a .pyc or .pyo file")
            s = os.stat(fn)
            self.assertEqual(stat.S_IMODE(s.st_mode),
                             stat.S_IRUSR | stat.S_IRGRP | stat.S_IROTH)
        finally:
            os.umask(oldmask)
            remove_files(TESTFN)
            unload(TESTFN)
            del sys.path[0]

    @unittest.skipIf(sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")
    def test_rewrite_pyc_with_read_only_source(self):
        # Issue 6074: a long time ago on posix, and more recently on Windows,
        # a read only source file resulted in a read only pyc file, which
        # led to problems with updating it later
        sys.path.insert(0, os.curdir)
        fname = TESTFN + os.extsep + "py"
        try:
            # Write a Python file, make it read-only and import it
            with open(fname, 'w') as f:
                f.write("x = 'original'\n")
            # Tweak the mtime of the source to ensure pyc gets updated later
            s = os.stat(fname)
            os.utime(fname, (s.st_atime, s.st_mtime-100000000))
            os.chmod(fname, 0400)
            m1 = __import__(TESTFN)
            self.assertEqual(m1.x, 'original')
            # Change the file and then reimport it
            os.chmod(fname, 0600)
            with open(fname, 'w') as f:
                f.write("x = 'rewritten'\n")
            unload(TESTFN)
            m2 = __import__(TESTFN)
            self.assertEqual(m2.x, 'rewritten')
            # Now delete the source file and check the pyc was rewritten
            unlink(fname)
            unload(TESTFN)
            m3 = __import__(TESTFN)
            self.assertEqual(m3.x, 'rewritten')
        finally:
            chmod_files(TESTFN)
            remove_files(TESTFN)
            unload(TESTFN)
            del sys.path[0]

    def test_imp_module(self):
        # Verify that the imp module can correctly load and find .py files

        # XXX (ncoghlan): It would be nice to use test_support.CleanImport
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
        filename = module + os.extsep + 'py'

        # Create a file with a list of 65000 elements.
        with open(filename, 'w+') as f:
            f.write('d = [\n')
            for i in range(65000):
                f.write('"",\n')
            f.write(']')

        # Compile & remove .py file, we only need .pyc (or .pyo).
        with open(filename, 'r') as f:
            py_compile.compile(filename)
        unlink(filename)

        # Need to be able to load from current dir.
        sys.path.append('')

        # This used to crash.
        exec 'import ' + module

        # Cleanup.
        del sys.path[-1]
        unlink(filename + 'c')
        unlink(filename + 'o')

    def test_failing_import_sticks(self):
        source = TESTFN + os.extsep + "py"
        with open(source, "w") as f:
            print >> f, "a = 1 // 0"

        # New in 2.4, we shouldn't be able to import that no matter how often
        # we try.
        sys.path.insert(0, os.curdir)
        try:
            for i in [1, 2, 3]:
                self.assertRaises(ZeroDivisionError, __import__, TESTFN)
                self.assertNotIn(TESTFN, sys.modules,
                                 "damaged module in sys.modules on %i try" % i)
        finally:
            del sys.path[0]
            remove_files(TESTFN)

    def test_failing_reload(self):
        # A failing reload should leave the module object in sys.modules.
        source = TESTFN + os.extsep + "py"
        with open(source, "w") as f:
            print >> f, "a = 1"
            print >> f, "b = 2"

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
                print >> f, "a = 10"
                print >> f, "b = 20//0"

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

    def test_infinite_reload(self):
        # http://bugs.python.org/issue742342 reports that Python segfaults
        # (infinite recursion in C) when faced with self-recursive reload()ing.

        sys.path.insert(0, os.path.dirname(__file__))
        try:
            import infinite_reload
        finally:
            del sys.path[0]

    def test_import_name_binding(self):
        # import x.y.z binds x in the current namespace.
        import test as x
        import test.test_support
        self.assertIs(x, test, x.__name__)
        self.assertTrue(hasattr(test.test_support, "__file__"))

        # import x.y.z as w binds z as w.
        import test.test_support as y
        self.assertIs(y, test.test_support, y.__name__)

    def test_import_initless_directory_warning(self):
        with check_warnings(('', ImportWarning)):
            # Just a random non-package directory we always expect to be
            # somewhere in sys.path...
            self.assertRaises(ImportError, __import__, "site-packages")

    def test_import_by_filename(self):
        path = os.path.abspath(TESTFN)
        with self.assertRaises(ImportError) as c:
            __import__(path)
        self.assertEqual("Import by filename is not supported.",
                         c.exception.args[0])

    def test_import_in_del_does_not_crash(self):
        # Issue 4236
        testfn = script_helper.make_script('', TESTFN, textwrap.dedent("""\
            import sys
            class C:
               def __del__(self):
                  import imp
            sys.argv.insert(0, C())
            """))
        try:
            script_helper.assert_python_ok(testfn)
        finally:
            unlink(testfn)

    def test_bug7732(self):
        source = TESTFN + '.py'
        os.mkdir(source)
        try:
            self.assertRaises((ImportError, IOError),
                              imp.find_module, TESTFN, ["."])
        finally:
            os.rmdir(source)

    def test_timestamp_overflow(self):
        # A modification timestamp larger than 2**32 should not be a problem
        # when importing a module (issue #11235).
        sys.path.insert(0, os.curdir)
        try:
            source = TESTFN + ".py"
            compiled = source + ('c' if __debug__ else 'o')
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

    def test_pyc_mtime(self):
        # Test for issue #13863: .pyc timestamp sometimes incorrect on Windows.
        sys.path.insert(0, os.curdir)
        try:
            # Jan 1, 2012; Jul 1, 2012.
            mtimes = 1325376000, 1341100800

            # Different names to avoid running into import caching.
            tails = "spam", "eggs"
            for mtime, tail in zip(mtimes, tails):
                module = TESTFN + tail
                source = module + ".py"
                compiled = source + ('c' if __debug__ else 'o')

                # Create a new Python file with the given mtime.
                with open(source, 'w') as f:
                    f.write("# Just testing\nx=1, 2, 3\n")
                os.utime(source, (mtime, mtime))

                # Generate the .pyc/o file; if it couldn't be created
                # for some reason, skip the test.
                m = __import__(module)
                if not os.path.exists(compiled):
                    unlink(source)
                    self.skipTest("Couldn't create .pyc/.pyo file.")

                # Actual modification time of .py file.
                mtime1 = int(os.stat(source).st_mtime) & 0xffffffff

                # mtime that was encoded in the .pyc file.
                with open(compiled, 'rb') as f:
                    mtime2 = struct.unpack('<L', f.read(8)[4:])[0]

                unlink(compiled)
                unlink(source)

                self.assertEqual(mtime1, mtime2)
        finally:
            sys.path.pop(0)

    def test_replace_parent_in_sys_modules(self):
        dir_name = os.path.abspath(TESTFN)
        os.mkdir(dir_name)
        self.addCleanup(rmtree, dir_name)
        pkg_dir = os.path.join(dir_name, 'sa')
        os.mkdir(pkg_dir)
        with open(os.path.join(pkg_dir, '__init__.py'), 'w') as init_file:
            init_file.write("import v1")
        with open(os.path.join(pkg_dir, 'v1.py'), 'w') as v1_file:
            v1_file.write("import sys;"
                          "sys.modules['sa'] = sys.modules[__name__];"
                          "import sa")
        sys.path.insert(0, dir_name)
        self.addCleanup(sys.path.pop, 0)
        # a segfault means the test failed!
        import sa

    def test_fromlist_type(self):
        with self.assertRaises(TypeError) as cm:
            __import__('encodings', fromlist=[u'aliases'])
        self.assertIn('must be str, not unicode', str(cm.exception))
        with self.assertRaises(TypeError) as cm:
            __import__('encodings', fromlist=[1])
        self.assertIn('must be str, not int', str(cm.exception))


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
        if not sys.dont_write_bytecode:
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
        try:
            os.listdir(unc)
        except OSError as e:
            if e.errno in (errno.EPERM, errno.EACCES, errno.ENOENT):
                # See issue #15338
                self.skipTest("cannot access administrative share %r" % (unc,))
            raise
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
        # Regression test for http://bugs.python.org/issue3221.
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
        with check_warnings(('.+foo', RuntimeWarning)):
            check_absolute()
        self.assertRaises(SystemError, check_relative)

        # Check relative fails with __package__ and __name__ wrong
        ns = dict(__package__='foo', __name__='notarealpkg.notarealmodule')
        with check_warnings(('.+foo', RuntimeWarning)):
            check_absolute()
        self.assertRaises(SystemError, check_relative)

        # Check both fail with package set to a non-string
        ns = dict(__package__=object())
        self.assertRaises(ValueError, check_absolute)
        self.assertRaises(ValueError, check_relative)

    def test_absolute_import_without_future(self):
        # If explicit relative import syntax is used, then do not try
        # to perform an absolute import in the face of failure.
        # Issue #7902.
        with self.assertRaises(ImportError):
            from .os import sep
            self.fail("explicit relative import triggered an "
                      "implicit absolute import")


class TestSymbolicallyLinkedPackage(unittest.TestCase):
    package_name = 'sample'

    def setUp(self):
        if os.path.exists(self.tagged):
            shutil.rmtree(self.tagged)
        if os.path.exists(self.package_name):
            symlink_support.remove_symlink(self.package_name)
        self.orig_sys_path = sys.path[:]

        # create a sample package; imagine you have a package with a tag and
        #  you want to symbolically link it from its untagged name.
        os.mkdir(self.tagged)
        init_file = os.path.join(self.tagged, '__init__.py')
        open(init_file, 'w').close()
        assert os.path.exists(init_file)

        # now create a symlink to the tagged package
        # sample -> sample-tagged
        symlink_support.symlink(self.tagged, self.package_name)

        assert os.path.isdir(self.package_name)
        assert os.path.isfile(os.path.join(self.package_name, '__init__.py'))

    @property
    def tagged(self):
        return self.package_name + '-tagged'

    # regression test for issue6727
    @unittest.skipUnless(
        not hasattr(sys, 'getwindowsversion')
        or sys.getwindowsversion() >= (6, 0),
        "Windows Vista or later required")
    @symlink_support.skip_unless_symlink
    def test_symlinked_dir_importable(self):
        # make sure sample can only be imported from the current directory.
        sys.path[:] = ['.']

        # and try to import the package
        __import__(self.package_name)

    def tearDown(self):
        # now cleanup
        if os.path.exists(self.package_name):
            symlink_support.remove_symlink(self.package_name)
        if os.path.exists(self.tagged):
            shutil.rmtree(self.tagged)
        sys.path[:] = self.orig_sys_path

def test_main(verbose=None):
    run_unittest(ImportTests, PycRewritingTests, PathsTests,
        RelativeImportTests, TestSymbolicallyLinkedPackage)

if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    from test.test_import import test_main
    test_main()
