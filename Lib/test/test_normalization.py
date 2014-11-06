from test.test_support import run_unittest, open_urlresource
import unittest

from httplib import HTTPException
import sys
import os
from unicodedata import normalize, unidata_version

TESTDATAFILE = "NormalizationTest.txt"
TESTDATAURL = "http://www.pythontest.net/unicode/" + unidata_version + "/" + TESTDATAFILE

def check_version(testfile):
    hdr = testfile.readline()
    return unidata_version in hdr

class RangeError(Exception):
    pass

def NFC(str):
    return normalize("NFC", str)

def NFKC(str):
    return normalize("NFKC", str)

def NFD(str):
    return normalize("NFD", str)

def NFKD(str):
    return normalize("NFKD", str)

def unistr(data):
    data = [int(x, 16) for x in data.split(" ")]
    for x in data:
        if x > sys.maxunicode:
            raise RangeError
    return u"".join([unichr(x) for x in data])

class NormalizationTest(unittest.TestCase):
    def test_main(self):
        part = None
        part1_data = {}
        # Hit the exception early
        try:
            testdata = open_urlresource(TESTDATAURL, check_version)
        except (IOError, HTTPException):
            self.skipTest("Could not retrieve " + TESTDATAURL)
        for line in testdata:
            if '#' in line:
                line = line.split('#')[0]
            line = line.strip()
            if not line:
                continue
            if line.startswith("@Part"):
                part = line.split()[0]
                continue
            try:
                c1,c2,c3,c4,c5 = [unistr(x) for x in line.split(';')[:-1]]
            except RangeError:
                # Skip unsupported characters;
                # try at least adding c1 if we are in part1
                if part == "@Part1":
                    try:
                        c1 = unistr(line.split(';')[0])
                    except RangeError:
                        pass
                    else:
                        part1_data[c1] = 1
                continue

            # Perform tests
            self.assertTrue(c2 ==  NFC(c1) ==  NFC(c2) ==  NFC(c3), line)
            self.assertTrue(c4 ==  NFC(c4) ==  NFC(c5), line)
            self.assertTrue(c3 ==  NFD(c1) ==  NFD(c2) ==  NFD(c3), line)
            self.assertTrue(c5 ==  NFD(c4) ==  NFD(c5), line)
            self.assertTrue(c4 == NFKC(c1) == NFKC(c2) == \
                            NFKC(c3) == NFKC(c4) == NFKC(c5),
                            line)
            self.assertTrue(c5 == NFKD(c1) == NFKD(c2) == \
                            NFKD(c3) == NFKD(c4) == NFKD(c5),
                            line)

            # Record part 1 data
            if part == "@Part1":
                part1_data[c1] = 1

        # Perform tests for all other data
        for c in range(sys.maxunicode+1):
            X = unichr(c)
            if X in part1_data:
                continue
            self.assertTrue(X == NFC(X) == NFD(X) == NFKC(X) == NFKD(X), c)

    def test_bug_834676(self):
        # Check for bug 834676
        normalize('NFC', u'\ud55c\uae00')


def test_main():
    run_unittest(NormalizationTest)

if __name__ == "__main__":
    test_main()
