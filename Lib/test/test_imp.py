import gc
import importlib
import importlib.util
import os
import os.path
import py_compile
import sys
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import script_helper
from test.support import warnings_helper
from test.support import is_wasi
import unittest
import warnings
imp = warnings_helper.import_deprecated('imp')
import _imp


OS_PATH_NAME = os.path.__name__


def requires_load_dynamic(meth):
    """Decorator to skip a test if not running under CPython or lacking
    imp.load_dynamic()."""
    meth = support.cpython_only(meth)
    if is_wasi:
        return unittest.skipIf(True, 'Not supoorted in WASI')(meth)
    return unittest.skipIf(getattr(imp, 'load_dynamic', None) is None,
                           'imp.load_dynamic() required')(meth)


class LockTests(unittest.TestCase):

    """Very basic test of import lock functions."""

    def verify_lock_state(self, expected):
        self.assertEqual(imp.lock_held(), expected,
                             "expected imp.lock_held() to be %r" % expected)
    def testLock(self):
        LOOPS = 50

        # The import lock may already be held, e.g. if the test suite is run
        # via "import test.autotest".
        lock_held_at_start = imp.lock_held()
        self.verify_lock_state(lock_held_at_start)

        for i in range(LOOPS):
            imp.acquire_lock()
            self.verify_lock_state(True)

        for i in range(LOOPS):
            imp.release_lock()

        # The original state should be restored now.
        self.verify_lock_state(lock_held_at_start)

        if not lock_held_at_start:
            try:
                imp.release_lock()
            except RuntimeError:
                pass
            else:
                self.fail("release_lock() without lock should raise "
                            "RuntimeError")

class ImportTests(unittest.TestCase):
    def setUp(self):
        mod = importlib.import_module('test.encoded_modules')
        self.test_strings = mod.test_strings
        self.test_path = mod.__path__

    def test_import_encoded_module(self):
        for modname, encoding, teststr in self.test_strings:
            mod = importlib.import_module('test.encoded_modules.'
                                          'module_' + modname)
            self.assertEqual(teststr, mod.test)

    def test_find_module_encoding(self):
        for mod, encoding, _ in self.test_strings:
            with imp.find_module('module_' + mod, self.test_path)[0] as fd:
                self.assertEqual(fd.encoding, encoding)

        path = [os.path.join(os.path.dirname(__file__), 'tokenizedata')]
        with self.assertRaises(SyntaxError):
            imp.find_module('badsyntax_pep3120', path)

    def test_issue1267(self):
        for mod, encoding, _ in self.test_strings:
            fp, filename, info  = imp.find_module('module_' + mod,
                                                  self.test_path)
            with fp:
                self.assertNotEqual(fp, None)
                self.assertEqual(fp.encoding, encoding)
                self.assertEqual(fp.tell(), 0)
                self.assertEqual(fp.readline(), '# test %s encoding\n'
                                 % encoding)

        fp, filename, info = imp.find_module("tokenize")
        with fp:
            self.assertNotEqual(fp, None)
            self.assertEqual(fp.encoding, "utf-8")
            self.assertEqual(fp.tell(), 0)
            self.assertEqual(fp.readline(),
                             '"""Tokenization help for Python programs.\n')

    def test_issue3594(self):
        temp_mod_name = 'test_imp_helper'
        sys.path.insert(0, '.')
        try:
            with open(temp_mod_name + '.py', 'w', encoding="latin-1") as file:
                file.write("# coding: cp1252\nu = 'test.test_imp'\n")
            file, filename, info = imp.find_module(temp_mod_name)
            file.close()
            self.assertEqual(file.encoding, 'cp1252')
        finally:
            del sys.path[0]
            os_helper.unlink(temp_mod_name + '.py')
            os_helper.unlink(temp_mod_name + '.pyc')

    def test_issue5604(self):
        # Test cannot cover imp.load_compiled function.
        # Martin von Loewis note what shared library cannot have non-ascii
        # character because init_xxx function cannot be compiled
        # and issue never happens for dynamic modules.
        # But sources modified to follow generic way for processing paths.

        # the return encoding could be uppercase or None
        fs_encoding = sys.getfilesystemencoding()

        # covers utf-8 and Windows ANSI code pages
        # one non-space symbol from every page
        # (http://en.wikipedia.org/wiki/Code_page)
        known_locales = {
            'utf-8' : b'\xc3\xa4',
            'cp1250' : b'\x8C',
            'cp1251' : b'\xc0',
            'cp1252' : b'\xc0',
            'cp1253' : b'\xc1',
            'cp1254' : b'\xc0',
            'cp1255' : b'\xe0',
            'cp1256' : b'\xe0',
            'cp1257' : b'\xc0',
            'cp1258' : b'\xc0',
            }

        if sys.platform == 'darwin':
            self.assertEqual(fs_encoding, 'utf-8')
            # Mac OS X uses the Normal Form D decomposition
            # http://developer.apple.com/mac/library/qa/qa2001/qa1173.html
            special_char = b'a\xcc\x88'
        else:
            special_char = known_locales.get(fs_encoding)

        if not special_char:
            self.skipTest("can't run this test with %s as filesystem encoding"
                          % fs_encoding)
        decoded_char = special_char.decode(fs_encoding)
        temp_mod_name = 'test_imp_helper_' + decoded_char
        test_package_name = 'test_imp_helper_package_' + decoded_char
        init_file_name = os.path.join(test_package_name, '__init__.py')
        try:
            # if the curdir is not in sys.path the test fails when run with
            # ./python ./Lib/test/regrtest.py test_imp
            sys.path.insert(0, os.curdir)
            with open(temp_mod_name + '.py', 'w', encoding="utf-8") as file:
                file.write('a = 1\n')
            file, filename, info = imp.find_module(temp_mod_name)
            with file:
                self.assertIsNotNone(file)
                self.assertTrue(filename[:-3].endswith(temp_mod_name))
                self.assertEqual(info[0], '.py')
                self.assertEqual(info[1], 'r')
                self.assertEqual(info[2], imp.PY_SOURCE)

                mod = imp.load_module(temp_mod_name, file, filename, info)
                self.assertEqual(mod.a, 1)

            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                mod = imp.load_source(temp_mod_name, temp_mod_name + '.py')
            self.assertEqual(mod.a, 1)

            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                if not sys.dont_write_bytecode:
                    mod = imp.load_compiled(
                        temp_mod_name,
                        imp.cache_from_source(temp_mod_name + '.py'))
            self.assertEqual(mod.a, 1)

            if not os.path.exists(test_package_name):
                os.mkdir(test_package_name)
            with open(init_file_name, 'w', encoding="utf-8") as file:
                file.write('b = 2\n')
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                package = imp.load_package(test_package_name, test_package_name)
            self.assertEqual(package.b, 2)
        finally:
            del sys.path[0]
            for ext in ('.py', '.pyc'):
                os_helper.unlink(temp_mod_name + ext)
                os_helper.unlink(init_file_name + ext)
            os_helper.rmtree(test_package_name)
            os_helper.rmtree('__pycache__')

    def test_issue9319(self):
        path = os.path.join(os.path.dirname(__file__), "tokenizedata")
        self.assertRaises(SyntaxError,
                          imp.find_module,
                          "badsyntax_pep3120",
                          [path])

    def test_load_from_source(self):
        # Verify that the imp module can correctly load and find .py files
        # XXX (ncoghlan): It would be nice to use import_helper.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with import_helper.CleanImport('os', 'os.path', OS_PATH_NAME):
            import os
            orig_path = os.path
            orig_getenv = os.getenv
            with os_helper.EnvironmentVarGuard():
                x = imp.find_module("os")
                self.addCleanup(x[0].close)
                new_os = imp.load_module("os", *x)
                self.assertIs(os, new_os)
                self.assertIs(orig_path, new_os.path)
                self.assertIsNot(orig_getenv, new_os.getenv)

    @requires_load_dynamic
    def test_issue15828_load_extensions(self):
        # Issue 15828 picked up that the adapter between the old imp API
        # and importlib couldn't handle C extensions
        example = "_heapq"
        x = imp.find_module(example)
        file_ = x[0]
        if file_ is not None:
            self.addCleanup(file_.close)
        mod = imp.load_module(example, *x)
        self.assertEqual(mod.__name__, example)

    @requires_load_dynamic
    def test_issue16421_multiple_modules_in_one_dll(self):
        # Issue 16421: loading several modules from the same compiled file fails
        m = '_testimportmultiple'
        fileobj, pathname, description = imp.find_module(m)
        fileobj.close()
        mod0 = imp.load_dynamic(m, pathname)
        mod1 = imp.load_dynamic('_testimportmultiple_foo', pathname)
        mod2 = imp.load_dynamic('_testimportmultiple_bar', pathname)
        self.assertEqual(mod0.__name__, m)
        self.assertEqual(mod1.__name__, '_testimportmultiple_foo')
        self.assertEqual(mod2.__name__, '_testimportmultiple_bar')
        with self.assertRaises(ImportError):
            imp.load_dynamic('nonexistent', pathname)

    @requires_load_dynamic
    def test_load_dynamic_ImportError_path(self):
        # Issue #1559549 added `name` and `path` attributes to ImportError
        # in order to provide better detail. Issue #10854 implemented those
        # attributes on import failures of extensions on Windows.
        path = 'bogus file path'
        name = 'extension'
        with self.assertRaises(ImportError) as err:
            imp.load_dynamic(name, path)
        self.assertIn(path, err.exception.path)
        self.assertEqual(name, err.exception.name)

    @requires_load_dynamic
    def test_load_module_extension_file_is_None(self):
        # When loading an extension module and the file is None, open one
        # on the behalf of imp.load_dynamic().
        # Issue #15902
        name = '_testimportmultiple'
        found = imp.find_module(name)
        if found[0] is not None:
            found[0].close()
        if found[2][2] != imp.C_EXTENSION:
            self.skipTest("found module doesn't appear to be a C extension")
        imp.load_module(name, None, *found[1:])

    @requires_load_dynamic
    def test_issue24748_load_module_skips_sys_modules_check(self):
        name = 'test.imp_dummy'
        try:
            del sys.modules[name]
        except KeyError:
            pass
        try:
            module = importlib.import_module(name)
            spec = importlib.util.find_spec('_testmultiphase')
            module = imp.load_dynamic(name, spec.origin)
            self.assertEqual(module.__name__, name)
            self.assertEqual(module.__spec__.name, name)
            self.assertEqual(module.__spec__.origin, spec.origin)
            self.assertRaises(AttributeError, getattr, module, 'dummy_name')
            self.assertEqual(module.int_const, 1969)
            self.assertIs(sys.modules[name], module)
        finally:
            try:
                del sys.modules[name]
            except KeyError:
                pass

    @unittest.skipIf(sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")
    def test_bug7732(self):
        with os_helper.temp_cwd():
            source = os_helper.TESTFN + '.py'
            os.mkdir(source)
            self.assertRaisesRegex(ImportError, '^No module',
                imp.find_module, os_helper.TESTFN, ["."])

    def test_multiple_calls_to_get_data(self):
        # Issue #18755: make sure multiple calls to get_data() can succeed.
        loader = imp._LoadSourceCompatibility('imp', imp.__file__,
                                              open(imp.__file__, encoding="utf-8"))
        loader.get_data(imp.__file__)  # File should be closed
        loader.get_data(imp.__file__)  # Will need to create a newly opened file

    def test_load_source(self):
        # Create a temporary module since load_source(name) modifies
        # sys.modules[name] attributes like __loader___
        modname = f"tmp{__name__}"
        mod = type(sys.modules[__name__])(modname)
        with support.swap_item(sys.modules, modname, mod):
            with self.assertRaisesRegex(ValueError, 'embedded null'):
                imp.load_source(modname, __file__ + "\0")

    @support.cpython_only
    def test_issue31315(self):
        # There shouldn't be an assertion failure in imp.create_dynamic(),
        # when spec.name is not a string.
        create_dynamic = support.get_attribute(imp, 'create_dynamic')
        class BadSpec:
            name = None
            origin = 'foo'
        with self.assertRaises(TypeError):
            create_dynamic(BadSpec())

    def test_issue_35321(self):
        # Both _frozen_importlib and _frozen_importlib_external
        # should have a spec origin of "frozen" and
        # no need to clean up imports in this case.

        import _frozen_importlib_external
        self.assertEqual(_frozen_importlib_external.__spec__.origin, "frozen")

        import _frozen_importlib
        self.assertEqual(_frozen_importlib.__spec__.origin, "frozen")

    def test_source_hash(self):
        self.assertEqual(_imp.source_hash(42, b'hi'), b'\xfb\xd9G\x05\xaf$\x9b~')
        self.assertEqual(_imp.source_hash(43, b'hi'), b'\xd0/\x87C\xccC\xff\xe2')

    def test_pyc_invalidation_mode_from_cmdline(self):
        cases = [
            ([], "default"),
            (["--check-hash-based-pycs", "default"], "default"),
            (["--check-hash-based-pycs", "always"], "always"),
            (["--check-hash-based-pycs", "never"], "never"),
        ]
        for interp_args, expected in cases:
            args = interp_args + [
                "-c",
                "import _imp; print(_imp.check_hash_based_pycs)",
            ]
            res = script_helper.assert_python_ok(*args)
            self.assertEqual(res.out.strip().decode('utf-8'), expected)

    def test_find_and_load_checked_pyc(self):
        # issue 34056
        with os_helper.temp_cwd():
            with open('mymod.py', 'wb') as fp:
                fp.write(b'x = 42\n')
            py_compile.compile(
                'mymod.py',
                doraise=True,
                invalidation_mode=py_compile.PycInvalidationMode.CHECKED_HASH,
            )
            file, path, description = imp.find_module('mymod', path=['.'])
            mod = imp.load_module('mymod', file, path, description)
        self.assertEqual(mod.x, 42)


    @support.cpython_only
    def test_create_builtin_subinterp(self):
        # gh-99578: create_builtin() behavior changes after the creation of the
        # first sub-interpreter. Test both code paths, before and after the
        # creation of a sub-interpreter. Previously, create_builtin() had
        # a reference leak after the creation of the first sub-interpreter.

        import builtins
        create_builtin = support.get_attribute(_imp, "create_builtin")
        class Spec:
            name = "builtins"
        spec = Spec()

        def check_get_builtins():
            refcnt = sys.getrefcount(builtins)
            mod = _imp.create_builtin(spec)
            self.assertIs(mod, builtins)
            self.assertEqual(sys.getrefcount(builtins), refcnt + 1)
            # Check that a GC collection doesn't crash
            gc.collect()

        check_get_builtins()

        ret = support.run_in_subinterp("import builtins")
        self.assertEqual(ret, 0)

        check_get_builtins()


class ReloadTests(unittest.TestCase):

    """Very basic tests to make sure that imp.reload() operates just like
    reload()."""

    def test_source(self):
        # XXX (ncoghlan): It would be nice to use test.import_helper.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with os_helper.EnvironmentVarGuard():
            import os
            imp.reload(os)

    def test_extension(self):
        with import_helper.CleanImport('time'):
            import time
            imp.reload(time)

    def test_builtin(self):
        with import_helper.CleanImport('marshal'):
            import marshal
            imp.reload(marshal)

    def test_with_deleted_parent(self):
        # see #18681
        from html import parser
        html = sys.modules.pop('html')
        def cleanup():
            sys.modules['html'] = html
        self.addCleanup(cleanup)
        with self.assertRaisesRegex(ImportError, 'html'):
            imp.reload(parser)


class PEP3147Tests(unittest.TestCase):
    """Tests of PEP 3147."""

    tag = imp.get_tag()

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag not be None')
    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        path = os.path.join('foo', 'bar', 'baz', 'qux.py')
        expect = os.path.join('foo', 'bar', 'baz', '__pycache__',
                              'qux.{}.pyc'.format(self.tag))
        self.assertEqual(imp.cache_from_source(path, True), expect)

    @unittest.skipUnless(sys.implementation.cache_tag is not None,
                         'requires sys.implementation.cache_tag to not be '
                         'None')
    def test_source_from_cache(self):
        # Given the path to a PEP 3147 defined .pyc file, return the path to
        # its source.  This tests the good path.
        path = os.path.join('foo', 'bar', 'baz', '__pycache__',
                            'qux.{}.pyc'.format(self.tag))
        expect = os.path.join('foo', 'bar', 'baz', 'qux.py')
        self.assertEqual(imp.source_from_cache(path), expect)


class NullImporterTests(unittest.TestCase):
    @unittest.skipIf(os_helper.TESTFN_UNENCODABLE is None,
                     "Need an undecodeable filename")
    def test_unencodeable(self):
        name = os_helper.TESTFN_UNENCODABLE
        os.mkdir(name)
        try:
            self.assertRaises(ImportError, imp.NullImporter, name)
        finally:
            os.rmdir(name)


if __name__ == "__main__":
    unittest.main()
