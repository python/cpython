#!/usr/bin/env python
#
# test_codecencodings_cn.py
#   Codec encoding tests for PRC encodings.
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class Test_GB2312(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb2312'
    tstring = test_multibytecodec_support.load_teststring('gb2312')
    codectests = (
        # invalid bytes
        ("abc\x81\x81\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x81\x81\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x81\x81\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x81\x81\xc1\xc4", "ignore",  u"abc\u804a"),
        ("\xc1\x64", "strict", None),
    )

class Test_GBK(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gbk'
    tstring = test_multibytecodec_support.load_teststring('gbk')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u804a"),
        ("\x83\x34\x83\x31", "strict", None),
        (u"\u30fb", "strict", None),
    )

class Test_GB18030(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb18030'
    tstring = test_multibytecodec_support.load_teststring('gb18030')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u804a\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u804a"),
        ("abc\x84\x39\x84\x39\xc1\xc4", "replace", u"abc\ufffd\u804a"),
        (u"\u30fb", "strict", "\x819\xa79"),
    )
    has_iso10646 = True

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
