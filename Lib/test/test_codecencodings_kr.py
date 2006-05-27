#!/usr/bin/env python
#
# test_codecencodings_kr.py
#   Codec encoding tests for ROK encodings.
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class Test_CP949(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'cp949'
    tstring = test_multibytecodec_support.load_teststring('cp949')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\uc894"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\uc894\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\uc894"),
    )

class Test_EUCKR(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'euc_kr'
    tstring = test_multibytecodec_support.load_teststring('euc_kr')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\uc894"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\uc894\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\uc894"),
    )

class Test_JOHAB(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'johab'
    tstring = test_multibytecodec_support.load_teststring('johab')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\ucd27"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\ucd27\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\ucd27"),
    )

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Test_CP949))
    suite.addTest(unittest.makeSuite(Test_EUCKR))
    suite.addTest(unittest.makeSuite(Test_JOHAB))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
