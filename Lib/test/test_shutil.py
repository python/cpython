# Copyright (C) 2003 Python Software Foundation

import unittest
import shutil
import tempfile
from test import test_support

class TestShutil(unittest.TestCase):
    def test_rmtree_errors(self):
        # filename is guaranteed not to exist
        filename = tempfile.mktemp()
        self.assertRaises(OSError, shutil.rmtree, filename)
        self.assertEqual(shutil.rmtree(filename, True), None)



def test_main():
    test_support.run_unittest(TestShutil)


if __name__ == '__main__':
    test_main()
