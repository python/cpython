# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
import os
import os.path
from test import test_support

class TestShutil(unittest.TestCase):
    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp()
        self.assertRaises(OSError, shutil.rmtree, filename)
        self.assertEqual(shutil.rmtree(filename, True), None)
        shutil.rmtree(filename, False, lambda func, arg, exc: None)

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



def test_main():
    test_support.run_unittest(TestShutil)

if __name__ == '__main__':
    test_main()
