import imp
import locale
import os
import os.path
import sys
import unittest
from test import support


class LockTests(unittest.TestCase):

    """Very basic test of import lock functions."""

    def verify_lock_state(self, expected):
        self.failUnlessEqual(imp.lock_held(), expected,
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
        self.assertEqual(fp.readline(), '#!/usr/bin/env python\n')
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

        locale_encoding = locale.getpreferredencoding()

        # covers utf-8 and Windows ANSI code pages
        # one non-space symbol from every page
        # (http://en.wikipedia.org/wiki/Code_page)
        known_locales = {
            'utf-8' : b'\xe4',
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

        special_char = known_locales.get(locale_encoding)
        if special_char:
            encoded_char = special_char.decode(locale_encoding)
            temp_mod_name = 'test_imp_helper_' + encoded_char
            test_package_name = 'test_imp_helper_package_' + encoded_char
            init_file_name = os.path.join(test_package_name, '__init__.py')
            try:
                with open(temp_mod_name + '.py', 'w') as file:
                    file.write('a = 1\n')
                file, filename, info = imp.find_module(temp_mod_name)
                self.assertNotEquals(None, file)
                self.assertTrue(filename[:-3].endswith(temp_mod_name))
                self.assertEquals('.py', info[0])
                self.assertEquals('U', info[1])
                self.assertEquals(imp.PY_SOURCE, info[2])

                mod = imp.load_module(temp_mod_name, file, filename, info)
                self.assertEquals(1, mod.a)
                file.close()

                mod = imp.load_source(temp_mod_name, temp_mod_name + '.py')
                self.assertEquals(1, mod.a)

                mod = imp.load_compiled(temp_mod_name, temp_mod_name + '.pyc')
                self.assertEquals(1, mod.a)

                if not os.path.exists(test_package_name):
                    os.mkdir(test_package_name)
                with open(init_file_name, 'w') as file:
                    file.write('b = 2\n')
                package = imp.load_package(test_package_name, test_package_name)
                self.assertEquals(2, package.b)
            finally:
                support.unlink(temp_mod_name + '.py')
                support.unlink(temp_mod_name + '.pyc')
                support.unlink(temp_mod_name + '.pyo')

                support.unlink(init_file_name + '.py')
                support.unlink(init_file_name + '.pyc')
                support.unlink(init_file_name + '.pyo')
                support.rmtree(test_package_name)


    def test_reload(self):
        import marshal
        imp.reload(marshal)
        import string
        imp.reload(string)
        ## import sys
        ## self.assertRaises(ImportError, reload, sys)


def test_main():
    tests = [
        ImportTests,
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
