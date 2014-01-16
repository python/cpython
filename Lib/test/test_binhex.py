"""Test script for the binhex C module

   Uses the mechanism of the python binhex module
   Based on an original test by Roger E. Masse.
"""
import binhex
import os
import unittest
from test import test_support


class BinHexTestCase(unittest.TestCase):

    def setUp(self):
        self.fname1 = test_support.TESTFN + "1"
        self.fname2 = test_support.TESTFN + "2"

    def tearDown(self):
        try: os.unlink(self.fname1)
        except OSError: pass

        try: os.unlink(self.fname2)
        except OSError: pass

    DATA = 'Jack is my hero'

    def test_binhex(self):
        f = open(self.fname1, 'w')
        f.write(self.DATA)
        f.close()

        binhex.binhex(self.fname1, self.fname2)

        binhex.hexbin(self.fname2, self.fname1)

        f = open(self.fname1, 'r')
        finish = f.readline()
        f.close()

        self.assertEqual(self.DATA, finish)


def test_main():
    test_support.run_unittest(BinHexTestCase)


if __name__ == "__main__":
    test_main()
