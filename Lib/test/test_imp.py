import imp
import os
import os.path
import sys
import unittest
from test import support


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

    def test_find_module_encoding(self):
        fd = imp.find_module("heapq")[0]
        self.assertEqual(fd.encoding, "iso-8859-1")

    def test_issue1267(self):
        fp, filename, info  = imp.find_module("pydoc")
        self.assertNotEqual(fp, None)
        self.assertEqual(fp.encoding, "iso-8859-1")
        self.assertEqual(fp.tell(), 0)
        self.assertEqual(fp.readline(), '#!/usr/bin/env python3\n')
        fp.close()

        fp, filename, info = imp.find_module("tokenize")
        self.assertNotEqual(fp, None)
        self.assertEqual(fp.encoding, "utf-8")
        self.assertEqual(fp.tell(), 0)
        self.assertEqual(fp.readline(),
                         '"""Tokenization help for Python programs.\n')
        fp.close()

    def test_issue3594(self):
        temp_mod_name = 'test_imp_helper'
        sys.path.insert(0, '.')
        try:
            with open(temp_mod_name + '.py', 'w') as file:
                file.write("# coding: cp1252\nu = 'test.test_imp'\n")
            file, filename, info = imp.find_module(temp_mod_name)
            file.close()
            self.assertEquals(file.encoding, 'cp1252')
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
        fs_encoding = fs_encoding.lower() if fs_encoding else 'ascii'

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
            self.assertIsNotNone(file)
            self.assertTrue(filename[:-3].endswith(temp_mod_name))
            self.assertEqual(info[0], '.py')
            self.assertEqual(info[1], 'U')
            self.assertEqual(info[2], imp.PY_SOURCE)

            mod = imp.load_module(temp_mod_name, file, filename, info)
            self.assertEqual(mod.a, 1)
            file.close()

            mod = imp.load_source(temp_mod_name, temp_mod_name + '.py')
            self.assertEqual(mod.a, 1)

            mod = imp.load_compiled(temp_mod_name, temp_mod_name + '.pyc')
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


class ReloadTests(unittest.TestCase):

    """Very basic tests to make sure that imp.reload() operates just like
    reload()."""

    def test_source(self):
        # XXX (ncoghlan): It would be nice to use test_support.CleanImport
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


def test_main():
    tests = [
        ImportTests,
        ReloadTests,
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
