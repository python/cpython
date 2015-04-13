try:
    import _thread
except ImportError:
    _thread = None
import importlib
import os
import os.path
import shutil
import sys
from test import support
import unittest
import warnings
with warnings.catch_warnings():
    warnings.simplefilter('ignore', PendingDeprecationWarning)
    import imp


def requires_load_dynamic(meth):
    """Decorator to skip a test if not running under CPython or lacking
    imp.load_dynamic()."""
    meth = support.cpython_only(meth)
    return unittest.skipIf(not hasattr(imp, 'load_dynamic'),
                           'imp.load_dynamic() required')(meth)


@unittest.skipIf(_thread is None, '_thread module is required')
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

        path = [os.path.dirname(__file__)]
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
            with open(temp_mod_name + '.py', 'w') as file:
                file.write("# coding: cp1252\nu = 'test.test_imp'\n")
            file, filename, info = imp.find_module(temp_mod_name)
            file.close()
            self.assertEqual(file.encoding, 'cp1252')
        finally:
            del sys.path[0]
            support.unlink(temp_mod_name + '.py')
            support.unlink(temp_mod_name + '.pyc')

    def test_issue5604(self):
        # Test cannot cover imp.load_compiled function.
        # Martin von Loewis note what shared library cannot have non-ascii
        # character because init_xxx function cannot be compiled
        # and issue never happens for dynamic modules.
        # But sources modified to follow generic way for processing pathes.

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
            with open(temp_mod_name + '.py', 'w') as file:
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
            with open(init_file_name, 'w') as file:
                file.write('b = 2\n')
            with warnings.catch_warnings():
                warnings.simplefilter('ignore')
                package = imp.load_package(test_package_name, test_package_name)
            self.assertEqual(package.b, 2)
        finally:
            del sys.path[0]
            for ext in ('.py', '.pyc'):
                support.unlink(temp_mod_name + ext)
                support.unlink(init_file_name + ext)
            support.rmtree(test_package_name)
            support.rmtree('__pycache__')

    def test_issue9319(self):
        path = os.path.dirname(__file__)
        self.assertRaises(SyntaxError,
                          imp.find_module, "badsyntax_pep3120", [path])

    def test_load_from_source(self):
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
        with support.EnvironmentVarGuard():
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

    @unittest.skipIf(sys.dont_write_bytecode,
        "test meaningful only when writing bytecode")
    def test_bug7732(self):
        with support.temp_cwd():
            source = support.TESTFN + '.py'
            os.mkdir(source)
            self.assertRaisesRegex(ImportError, '^No module',
                imp.find_module, support.TESTFN, ["."])

    def test_multiple_calls_to_get_data(self):
        # Issue #18755: make sure multiple calls to get_data() can succeed.
        loader = imp._LoadSourceCompatibility('imp', imp.__file__,
                                              open(imp.__file__))
        loader.get_data(imp.__file__)  # File should be closed
        loader.get_data(imp.__file__)  # Will need to create a newly opened file


class ReloadTests(unittest.TestCase):

    """Very basic tests to make sure that imp.reload() operates just like
    reload()."""

    def test_source(self):
        # XXX (ncoghlan): It would be nice to use test.support.CleanImport
        # here, but that breaks because the os module registers some
        # handlers in copy_reg on import. Since CleanImport doesn't
        # revert that registration, the module is left in a broken
        # state after reversion. Reinitialising the module contents
        # and just reverting os.environ to its previous state is an OK
        # workaround
        with support.EnvironmentVarGuard():
            import os
            imp.reload(os)

    def test_extension(self):
        with support.CleanImport('time'):
            import time
            imp.reload(time)

    def test_builtin(self):
        with support.CleanImport('marshal'):
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
    @unittest.skipIf(support.TESTFN_UNENCODABLE is None,
                     "Need an undecodeable filename")
    def test_unencodeable(self):
        name = support.TESTFN_UNENCODABLE
        os.mkdir(name)
        try:
            self.assertRaises(ImportError, imp.NullImporter, name)
        finally:
            os.rmdir(name)


if __name__ == "__main__":
    unittest.main()
