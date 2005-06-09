#! /usr/bin/env python
"""Test script for the gzip module.
"""

import unittest
from test import test_support
import sys, os
import gzip


data1 = """  int length=DEFAULTALLOC, err = Z_OK;
  PyObject *RetVal;
  int flushmode = Z_FINISH;
  unsigned long start_total_out;

"""

data2 = """/* zlibmodule.c -- gzip-compatible data compression */
/* See http://www.gzip.org/zlib/
/* See http://www.winimage.com/zLibDll for Windows */
"""


class TestGzip(unittest.TestCase):
    filename = test_support.TESTFN

    def setUp (self):
        pass

    def tearDown (self):
        try:
            os.unlink(self.filename)
        except os.error:
            pass


    def test_write (self):
        f = gzip.GzipFile(self.filename, 'wb') ; f.write(data1 * 50)

        # Try flush and fileno.
        f.flush()
        f.fileno()
        if hasattr(os, 'fsync'):
            os.fsync(f.fileno())
        f.close()

    def test_read(self):
        self.test_write()
        # Try reading.
        f = gzip.GzipFile(self.filename, 'r') ; d = f.read() ; f.close()
        self.assertEqual(d, data1*50)

    def test_append(self):
        self.test_write()
        # Append to the previous file
        f = gzip.GzipFile(self.filename, 'ab') ; f.write(data2 * 15) ; f.close()

        f = gzip.GzipFile(self.filename, 'rb') ; d = f.read() ; f.close()
        self.assertEqual(d, (data1*50) + (data2*15))

    def test_many_append(self):
        # Bug #1074261 was triggered when reading a file that contained
        # many, many members.  Create such a file and verify that reading it
        # works.
        f = gzip.open(self.filename, 'wb', 9)
        f.write('a')
        f.close()
        for i in range(0,200):
            f = gzip.open(self.filename, "ab", 9) # append
            f.write('a')
            f.close()

        # Try reading the file
        zgfile = gzip.open(self.filename, "rb")
        contents = ""
        while 1:
            ztxt = zgfile.read(8192)
            contents += ztxt
            if not ztxt: break
        zgfile.close()
        self.assertEquals(contents, 'a'*201)


    def test_readline(self):
        self.test_write()
        # Try .readline() with varying line lengths

        f = gzip.GzipFile(self.filename, 'rb')
        line_length = 0
        while 1:
            L = f.readline(line_length)
            if L == "" and line_length != 0: break
            self.assert_(len(L) <= line_length)
            line_length = (line_length + 1) % 50
        f.close()

    def test_readlines(self):
        self.test_write()
        # Try .readlines()

        f = gzip.GzipFile(self.filename, 'rb')
        L = f.readlines()
        f.close()

        f = gzip.GzipFile(self.filename, 'rb')
        while 1:
            L = f.readlines(150)
            if L == []: break
        f.close()

    def test_seek_read(self):
        self.test_write()
        # Try seek, read test

        f = gzip.GzipFile(self.filename)
        while 1:
            oldpos = f.tell()
            line1 = f.readline()
            if not line1: break
            newpos = f.tell()
            f.seek(oldpos)  # negative seek
            if len(line1)>10:
                amount = 10
            else:
                amount = len(line1)
            line2 = f.read(amount)
            self.assertEqual(line1[:amount], line2)
            f.seek(newpos)  # positive seek
        f.close()

    def test_seek_write(self):
        # Try seek, write test
        f = gzip.GzipFile(self.filename, 'w')
        for pos in range(0, 256, 16):
            f.seek(pos)
            f.write('GZ\n')
        f.close()

    def test_mode(self):
        self.test_write()
        f = gzip.GzipFile(self.filename, 'r')
        self.assertEqual(f.myfileobj.mode, 'rb')
        f.close()

def test_main(verbose=None):
    test_support.run_unittest(TestGzip)

if __name__ == "__main__":
    test_main(verbose=True)
