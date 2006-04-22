#!/usr/bin/env python
#
# test_codecencodings_tw.py
#   Codec encoding tests for ROC encodings.
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class Test_Big5(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'big5'
    tstring = test_multibytecodec_support.load_teststring('big5')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u8b10"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u8b10\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u8b10"),
    )

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Test_Big5))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
