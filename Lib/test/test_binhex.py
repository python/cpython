#! /usr/bin/env python3
"""Test script for the binhex C module

   Uses the mechanism of the python binhex module
   Based on an original test by Roger E. Masse.
"""
import binhex
import os
import unittest
from test import support


class BinHexTestCase(unittest.TestCase):

    def setUp(self):
        self.fname1 = support.TESTFN + "1"
        self.fname2 = support.TESTFN + "2"
        self.fname3 = support.TESTFN + "very_long_filename__very_long_filename__very_long_filename__very_long_filename__"

    def tearDown(self):
        support.unlink(self.fname1)
        support.unlink(self.fname2)
        support.unlink(self.fname3)

    DATA = b'Jack is my hero'

    def test_binhex(self):
        f = open(self.fname1, 'wb')
        f.write(self.DATA)
        f.close()

        binhex.binhex(self.fname1, self.fname2)

        binhex.hexbin(self.fname2, self.fname1)

        f = open(self.fname1, 'rb')
        finish = f.readline()
        f.close()

        self.assertEqual(self.DATA, finish)

    def test_binhex_error_on_long_filename(self):
        """
        The testcase fails if no exception is raised when a filename parameter provided to binhex.binhex()
        is too long, or if the exception raised in binhex.binhex() is not an instance of binhex.Error.
        """
        f3 = open(self.fname3, 'wb')
        f3.close()

        self.assertRaises(binhex.Error, binhex.binhex, self.fname3, self.fname2)

def test_main():
    support.run_unittest(BinHexTestCase)


if __name__ == "__main__":
    test_main()
