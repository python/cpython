import imp
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
