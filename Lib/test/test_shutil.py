# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import sys
import stat
import os
import os.path
from test import test_support
from test.test_support import TESTFN

class TestShutil(unittest.TestCase):
    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp()
        self.assertRaises(OSError, shutil.rmtree, filename)

    # See bug #1071513 for why we don't run this on cygwin
    # and bug #1076467 for why we don't run this as root.
    if (hasattr(os, 'chmod') and sys.platform[:6] != 'cygwin'
        and not (hasattr(os, 'geteuid') and os.geteuid() == 0)):
        def test_on_error(self):
            self.errorState = 0
            os.mkdir(TESTFN)
            self.childpath = os.path.join(TESTFN, 'a')
            f = open(self.childpath, 'w')
            f.close()
            old_dir_mode = os.stat(TESTFN).st_mode
            old_child_mode = os.stat(self.childpath).st_mode
            # Make unwritable.
            os.chmod(self.childpath, stat.S_IREAD)
            os.chmod(TESTFN, stat.S_IREAD)

            shutil.rmtree(TESTFN, onerror=self.check_args_to_onerror)
            # Test whether onerror has actually been called.
            self.assertEqual(self.errorState, 2,
                             "Expected call to onerror function did not happen.")

            # Make writable again.
            os.chmod(TESTFN, old_dir_mode)
            os.chmod(self.childpath, old_child_mode)

            # Clean up.
            shutil.rmtree(TESTFN)

    def check_args_to_onerror(self, func, arg, exc):
        if self.errorState == 0:
            self.assertEqual(func, os.remove)
            self.assertEqual(arg, self.childpath)
            self.failUnless(issubclass(exc[0], OSError))
            self.errorState = 1
        else:
            self.assertEqual(func, os.rmdir)
            self.assertEqual(arg, TESTFN)
            self.failUnless(issubclass(exc[0], OSError))
            self.errorState = 2

    def test_rmtree_dont_delete_file(self):
        # When called on a file instead of a directory, don't delete it.
        handle, path = tempfile.mkstemp()
        os.fdopen(handle).close()
        self.assertRaises(OSError, shutil.rmtree, path)
        os.remove(path)

    def test_dont_move_dir_in_itself(self):
        src_dir = tempfile.mkdtemp()
        try:
            dst = os.path.join(src_dir, 'foo')
            self.assertRaises(shutil.Error, shutil.move, src_dir, dst)
        finally:
            try:
                os.rmdir(src_dir)
            except:
                pass

    def test_copytree_simple(self):
        def write_data(path, data):
            f = open(path, "w")
            f.write(data)
            f.close()

        def read_data(path):
            f = open(path)
            data = f.read()
            f.close()
            return data

        src_dir = tempfile.mkdtemp()
        dst_dir = os.path.join(tempfile.mkdtemp(), 'destination')

        write_data(os.path.join(src_dir, 'test.txt'), '123')

        os.mkdir(os.path.join(src_dir, 'test_dir'))
        write_data(os.path.join(src_dir, 'test_dir', 'test.txt'), '456')

        try:
            shutil.copytree(src_dir, dst_dir)
            self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test.txt')))
            self.assertTrue(os.path.isdir(os.path.join(dst_dir, 'test_dir')))
            self.assertTrue(os.path.isfile(os.path.join(dst_dir, 'test_dir',
                                                        'test.txt')))
            actual = read_data(os.path.join(dst_dir, 'test.txt'))
            self.assertEqual(actual, '123')
            actual = read_data(os.path.join(dst_dir, 'test_dir', 'test.txt'))
            self.assertEqual(actual, '456')
        finally:
            for path in (
                    os.path.join(src_dir, 'test.txt'),
                    os.path.join(dst_dir, 'test.txt'),
                    os.path.join(src_dir, 'test_dir', 'test.txt'),
                    os.path.join(dst_dir, 'test_dir', 'test.txt'),
                ):
                if os.path.exists(path):
                    os.remove(path)
            for path in (src_dir,
                    os.path.abspath(os.path.join(dst_dir, os.path.pardir))
                ):
                if os.path.exists(path):
                    shutil.rmtree(path)


    if hasattr(os, "symlink"):
        def test_dont_copy_file_onto_link_to_itself(self):
            # bug 851123.
            os.mkdir(TESTFN)
            src = os.path.join(TESTFN, 'cheese')
            dst = os.path.join(TESTFN, 'shop')
            try:
                f = open(src, 'w')
                f.write('cheddar')
                f.close()

                os.link(src, dst)
                self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
                self.assertEqual(open(src,'r').read(), 'cheddar')
                os.remove(dst)

                # Using `src` here would mean we end up with a symlink pointing
                # to TESTFN/TESTFN/cheese, while it should point at
                # TESTFN/cheese.
                os.symlink('cheese', dst)
                self.assertRaises(shutil.Error, shutil.copyfile, src, dst)
                self.assertEqual(open(src,'r').read(), 'cheddar')
                os.remove(dst)
            finally:
                try:
                    shutil.rmtree(TESTFN)
                except OSError:
                    pass

def test_main():
    test_support.run_unittest(TestShutil)

if __name__ == '__main__':
    test_main()
