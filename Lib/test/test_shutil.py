
# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
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

    if hasattr(os, 'chmod'):
        def test_on_error(self):
            self.errorState = 0
            os.mkdir(TESTFN)
            f = open(os.path.join(TESTFN, 'a'), 'w')
            f.close()
            # Make TESTFN unwritable.
            os.chmod(TESTFN, stat.S_IRUSR)

            shutil.rmtree(TESTFN, onerror=self.check_args_to_onerror)

            # Make TESTFN writable again.
            os.chmod(TESTFN, stat.S_IRWXU)
            shutil.rmtree(TESTFN)

    def check_args_to_onerror(self, func, arg, exc):
        if self.errorState == 0:
            self.assertEqual(func, os.remove)
            self.assertEqual(arg, os.path.join(TESTFN, 'a'))
            self.assertEqual(exc[0], OSError)
            self.errorState = 1
        else:
            self.assertEqual(func, os.rmdir)
            self.assertEqual(arg, TESTFN)
            self.assertEqual(exc[0], OSError)

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
