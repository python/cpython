# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import os
import os.path
from test import test_support
from test.test_support import TESTFN

class TestShutil(unittest.TestCase):
    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp()
        self.assertRaises(OSError, shutil.rmtree, filename)
        self.assertEqual(shutil.rmtree(filename, True), None)
        shutil.rmtree(filename, False, lambda func, arg, exc: None)

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
