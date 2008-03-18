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

        def test_rmtree_on_symlink(self):
            # bug 1669.
            os.mkdir(TESTFN)
            try:
                src = os.path.join(TESTFN, 'cheese')
                dst = os.path.join(TESTFN, 'shop')
                os.mkdir(src)
                os.symlink(src, dst)
                self.assertRaises(OSError, shutil.rmtree, dst)
            finally:
                shutil.rmtree(TESTFN, ignore_errors=True)


class TestMove(unittest.TestCase):

    def setUp(self):
        filename = "foo"
        self.src_dir = tempfile.mkdtemp()
        self.dst_dir = tempfile.mkdtemp()
        self.src_file = os.path.join(self.src_dir, filename)
        self.dst_file = os.path.join(self.dst_dir, filename)
        # Try to create a dir in the current directory, hoping that it is
        # not located on the same filesystem as the system tmp dir.
        try:
            self.dir_other_fs = tempfile.mkdtemp(
                dir=os.path.dirname(__file__))
            self.file_other_fs = os.path.join(self.dir_other_fs,
                filename)
        except OSError:
            self.dir_other_fs = None
        with open(self.src_file, "wb") as f:
            f.write(b"spam")

    def tearDown(self):
        for d in (self.src_dir, self.dst_dir, self.dir_other_fs):
            try:
                if d:
                    shutil.rmtree(d)
            except:
                pass

    def _check_move_file(self, src, dst, real_dst):
        contents = open(src, "rb").read()
        shutil.move(src, dst)
        self.assertEqual(contents, open(real_dst, "rb").read())
        self.assertFalse(os.path.exists(src))

    def _check_move_dir(self, src, dst, real_dst):
        contents = sorted(os.listdir(src))
        shutil.move(src, dst)
        self.assertEqual(contents, sorted(os.listdir(real_dst)))
        self.assertFalse(os.path.exists(src))

    def test_move_file(self):
        # Move a file to another location on the same filesystem.
        self._check_move_file(self.src_file, self.dst_file, self.dst_file)

    def test_move_file_to_dir(self):
        # Move a file inside an existing dir on the same filesystem.
        self._check_move_file(self.src_file, self.dst_dir, self.dst_file)

    def test_move_file_other_fs(self):
        # Move a file to an existing dir on another filesystem.
        if not self.dir_other_fs:
            # skip
            return
        self._check_move_file(self.src_file, self.file_other_fs,
            self.file_other_fs)

    def test_move_file_to_dir_other_fs(self):
        # Move a file to another location on another filesystem.
        if not self.dir_other_fs:
            # skip
            return
        self._check_move_file(self.src_file, self.dir_other_fs,
            self.file_other_fs)

    def test_move_dir(self):
        # Move a dir to another location on the same filesystem.
        dst_dir = tempfile.mktemp()
        try:
            self._check_move_dir(self.src_dir, dst_dir, dst_dir)
        finally:
            try:
                shutil.rmtree(dst_dir)
            except:
                pass

    def test_move_dir_other_fs(self):
        # Move a dir to another location on another filesystem.
        if not self.dir_other_fs:
            # skip
            return
        dst_dir = tempfile.mktemp(dir=self.dir_other_fs)
        try:
            self._check_move_dir(self.src_dir, dst_dir, dst_dir)
        finally:
            try:
                shutil.rmtree(dst_dir)
            except:
                pass

    def test_move_dir_to_dir(self):
        # Move a dir inside an existing dir on the same filesystem.
        self._check_move_dir(self.src_dir, self.dst_dir,
            os.path.join(self.dst_dir, os.path.basename(self.src_dir)))

    def test_move_dir_to_dir_other_fs(self):
        # Move a dir inside an existing dir on another filesystem.
        if not self.dir_other_fs:
            # skip
            return
        self._check_move_dir(self.src_dir, self.dir_other_fs,
            os.path.join(self.dir_other_fs, os.path.basename(self.src_dir)))

    def test_existing_file_inside_dest_dir(self):
        # A file with the same name inside the destination dir already exists.
        with open(self.dst_file, "wb"):
            pass
        self.assertRaises(shutil.Error, shutil.move, self.src_file, self.dst_dir)

    def test_dont_move_dir_in_itself(self):
        # Moving a dir inside itself raises an Error.
        dst = os.path.join(self.src_dir, "bar")
        self.assertRaises(shutil.Error, shutil.move, self.src_dir, dst)



def test_main():
    test_support.run_unittest(TestShutil, TestMove)

if __name__ == '__main__':
    test_main()
