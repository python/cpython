# Tests universal newline support for both reading and parsing files.
import io
import _pyio as pyio
import unittest
import os
import sys
from test import support

if not hasattr(sys.stdin, 'newlines'):
    raise unittest.SkipTest(
        "This Python does not have universal newline support")

FATX = 'x' * (2**14)

DATA_TEMPLATE = [
    "line1=1",
    "line2='this is a very long line designed to go past any default " +
        "buffer limits that exist in io.py but we also want to test " +
        "the uncommon case, naturally.'",
    "def line3():pass",
    "line4 = '%s'" % FATX,
    ]

DATA_LF = "\n".join(DATA_TEMPLATE) + "\n"
DATA_CR = "\r".join(DATA_TEMPLATE) + "\r"
DATA_CRLF = "\r\n".join(DATA_TEMPLATE) + "\r\n"

# Note that DATA_MIXED also tests the ability to recognize a lone \r
# before end-of-file.
DATA_MIXED = "\n".join(DATA_TEMPLATE) + "\r"
DATA_SPLIT = [x + "\n" for x in DATA_TEMPLATE]

class TestGenericUnivNewlines(unittest.TestCase):
    # use a class variable DATA to define the data to write to the file
    # and a class variable NEWLINE to set the expected newlines value
    READMODE = 'r'
    WRITEMODE = 'wb'

    def setUp(self):
        data = self.DATA
        if "b" in self.WRITEMODE:
            data = data.encode("ascii")
        with self.open(support.TESTFN, self.WRITEMODE) as fp:
            fp.write(data)

    def tearDown(self):
        try:
            os.unlink(support.TESTFN)
        except:
            pass

    def test_read(self):
        with self.open(support.TESTFN, self.READMODE) as fp:
            data = fp.read()
        self.assertEqual(data, DATA_LF)
        self.assertEqual(repr(fp.newlines), repr(self.NEWLINE))

    def test_readlines(self):
        with self.open(support.TESTFN, self.READMODE) as fp:
            data = fp.readlines()
        self.assertEqual(data, DATA_SPLIT)
        self.assertEqual(repr(fp.newlines), repr(self.NEWLINE))

    def test_readline(self):
        with self.open(support.TESTFN, self.READMODE) as fp:
            data = []
            d = fp.readline()
            while d:
                data.append(d)
                d = fp.readline()
        self.assertEqual(data, DATA_SPLIT)
        self.assertEqual(repr(fp.newlines), repr(self.NEWLINE))

    def test_seek(self):
        with self.open(support.TESTFN, self.READMODE) as fp:
            fp.readline()
            pos = fp.tell()
            data = fp.readlines()
            self.assertEqual(data, DATA_SPLIT[1:])
            fp.seek(pos)
            data = fp.readlines()
        self.assertEqual(data, DATA_SPLIT[1:])


class TestCRNewlines(TestGenericUnivNewlines):
    NEWLINE = '\r'
    DATA = DATA_CR

class TestLFNewlines(TestGenericUnivNewlines):
    NEWLINE = '\n'
    DATA = DATA_LF

class TestCRLFNewlines(TestGenericUnivNewlines):
    NEWLINE = '\r\n'
    DATA = DATA_CRLF

    def test_tell(self):
        with self.open(support.TESTFN, self.READMODE) as fp:
            self.assertEqual(repr(fp.newlines), repr(None))
            data = fp.readline()
            pos = fp.tell()
        self.assertEqual(repr(fp.newlines), repr(self.NEWLINE))

class TestMixedNewlines(TestGenericUnivNewlines):
    NEWLINE = ('\r', '\n')
    DATA = DATA_MIXED


def test_main():
    base_tests = (TestCRNewlines,
                  TestLFNewlines,
                  TestCRLFNewlines,
                  TestMixedNewlines)
    tests = []
    # Test the C and Python implementations.
    for test in base_tests:
        class CTest(test):
            open = io.open
        CTest.__name__ = "C" + test.__name__
        class PyTest(test):
            open = staticmethod(pyio.open)
        PyTest.__name__ = "Py" + test.__name__
        tests.append(CTest)
        tests.append(PyTest)
    support.run_unittest(*tests)

if __name__ == '__main__':
    test_main()
