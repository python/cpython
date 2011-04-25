import imp
import os
import os.path
import shutil
import sys
import unittest
from test import support
import importlib

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
        self.assertRaisesRegex(SyntaxError,
            r"Non-UTF-8 code starting with '\\xf6'"
            r" in file .*badsyntax_pep3120.py",
            imp.find_module, 'badsyntax_pep3120', path)

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
            support.unlink(temp_mod_name + '.pyo')

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
                self.assertEqual(info[1], 'U')
                self.assertEqual(info[2], imp.PY_SOURCE)

                mod = imp.load_module(temp_mod_name, file, filename, info)
                self.assertEqual(mod.a, 1)

            mod = imp.load_source(temp_mod_name, temp_mod_name + '.py')
            self.assertEqual(mod.a, 1)

            mod = imp.load_compiled(
                temp_mod_name, imp.cache_from_source(temp_mod_name + '.py'))
            self.assertEqual(mod.a, 1)

            if not os.path.exists(test_package_name):
                os.mkdir(test_package_name)
            with open(init_file_name, 'w') as file:
                file.write('b = 2\n')
            package = imp.load_package(test_package_name, test_package_name)
            self.assertEqual(package.b, 2)
        finally:
            del sys.path[0]
            for ext in ('.py', '.pyc', '.pyo'):
                support.unlink(temp_mod_name + ext)
                support.unlink(init_file_name + ext)
            support.rmtree(test_package_name)

    def test_issue9319(self):
        path = os.path.dirname(__file__)
        self.assertRaises(SyntaxError,
                          imp.find_module, "badsyntax_pep3120", [path])


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


class PEP3147Tests(unittest.TestCase):
    """Tests of PEP 3147."""

    tag = imp.get_tag()

    def test_cache_from_source(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyc file (i.e. under __pycache__).
        self.assertEqual(
            imp.cache_from_source('/foo/bar/baz/qux.py', True),
            '/foo/bar/baz/__pycache__/qux.{}.pyc'.format(self.tag))
        # Directory with a dot, filename without dot
        self.assertEqual(
            imp.cache_from_source('/foo.bar/file', True),
            '/foo.bar/__pycache__/file{}.pyc'.format(self.tag))

    def test_cache_from_source_optimized(self):
        # Given the path to a .py file, return the path to its PEP 3147
        # defined .pyo file (i.e. under __pycache__).
        self.assertEqual(
            imp.cache_from_source('/foo/bar/baz/qux.py', False),
            '/foo/bar/baz/__pycache__/qux.{}.pyo'.format(self.tag))

    def test_cache_from_source_cwd(self):
        self.assertEqual(imp.cache_from_source('foo.py', True),
                         os.sep.join(('__pycache__',
                                      'foo.{}.pyc'.format(self.tag))))

    def test_cache_from_source_override(self):
        # When debug_override is not None, it can be any true-ish or false-ish
        # value.
        self.assertEqual(
            imp.cache_from_source('/foo/bar/baz.py', []),
            '/foo/bar/__pycache__/baz.{}.pyo'.format(self.tag))
        self.assertEqual(
            imp.cache_from_source('/foo/bar/baz.py', [17]),
            '/foo/bar/__pycache__/baz.{}.pyc'.format(self.tag))
        # However if the bool-ishness can't be determined, the exception
        # propagates.
        class Bearish:
            def __bool__(self): raise RuntimeError
        self.assertRaises(
            RuntimeError,
            imp.cache_from_source, '/foo/bar/baz.py', Bearish())

    @unittest.skipIf(os.altsep is None,
                     'test meaningful only where os.altsep is defined')
    def test_altsep_cache_from_source(self):
        # Windows path and PEP 3147.
        self.assertEqual(
            imp.cache_from_source('\\foo\\bar\\baz\\qux.py', True),
            '\\foo\\bar\\baz\\__pycache__\\qux.{}.pyc'.format(self.tag))

    @unittest.skipIf(os.altsep is None,
                     'test meaningful only where os.altsep is defined')
    def test_altsep_and_sep_cache_from_source(self):
        # Windows path and PEP 3147 where altsep is right of sep.
        self.assertEqual(
            imp.cache_from_source('\\foo\\bar/baz\\qux.py', True),
            '\\foo\\bar/baz\\__pycache__\\qux.{}.pyc'.format(self.tag))

    @unittest.skipIf(os.altsep is None,
                     'test meaningful only where os.altsep is defined')
    def test_sep_altsep_and_sep_cache_from_source(self):
        # Windows path and PEP 3147 where sep is right of altsep.
        self.assertEqual(
            imp.cache_from_source('\\foo\\bar\\baz/qux.py', True),
            '\\foo\\bar\\baz/__pycache__/qux.{}.pyc'.format(self.tag))

    def test_source_from_cache(self):
        # Given the path to a PEP 3147 defined .pyc file, return the path to
        # its source.  This tests the good path.
        self.assertEqual(imp.source_from_cache(
            '/foo/bar/baz/__pycache__/qux.{}.pyc'.format(self.tag)),
            '/foo/bar/baz/qux.py')

    def test_source_from_cache_bad_path(self):
        # When the path to a pyc file is not in PEP 3147 format, a ValueError
        # is raised.
        self.assertRaises(
            ValueError, imp.source_from_cache, '/foo/bar/bazqux.pyc')

    def test_source_from_cache_no_slash(self):
        # No slashes at all in path -> ValueError
        self.assertRaises(
            ValueError, imp.source_from_cache, 'foo.cpython-32.pyc')

    def test_source_from_cache_too_few_dots(self):
        # Too few dots in final path component -> ValueError
        self.assertRaises(
            ValueError, imp.source_from_cache, '__pycache__/foo.pyc')

    def test_source_from_cache_too_many_dots(self):
        # Too many dots in final path component -> ValueError
        self.assertRaises(
            ValueError, imp.source_from_cache,
            '__pycache__/foo.cpython-32.foo.pyc')

    def test_source_from_cache_no__pycache__(self):
        # Another problem with the path -> ValueError
        self.assertRaises(
            ValueError, imp.source_from_cache,
            '/foo/bar/foo.cpython-32.foo.pyc')

    def test_package___file__(self):
        # Test that a package's __file__ points to the right source directory.
        os.mkdir('pep3147')
        sys.path.insert(0, os.curdir)
        def cleanup():
            if sys.path[0] == os.curdir:
                del sys.path[0]
            shutil.rmtree('pep3147')
        self.addCleanup(cleanup)
        # Touch the __init__.py file.
        with open('pep3147/__init__.py', 'w'):
            pass
        m = __import__('pep3147')
        # Ensure we load the pyc file.
        support.forget('pep3147')
        m = __import__('pep3147')
        self.assertEqual(m.__file__,
                         os.sep.join(('.', 'pep3147', '__init__.py')))


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


def test_main():
    tests = [
        ImportTests,
        PEP3147Tests,
        ReloadTests,
        NullImporterTests,
        ]
    try:
        import _thread
    except ImportError:
        pass
    else:
        tests.append(LockTests)
    support.run_unittest(*tests)

if __name__ == "__main__":
    test_main()
