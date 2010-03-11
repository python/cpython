#!/usr/bin/env python3
#
# test_codecencodings_cn.py
#   Codec encoding tests for PRC encodings.
#

from test import support
from test import test_multibytecodec_support
import unittest

class Test_GB2312(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb2312'
    tstring = test_multibytecodec_support.load_teststring('gb2312')
    codectests = (
        # invalid bytes
        (b"abc\x81\x81\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x81\x81\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x81\x81\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x81\x81\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\xc1\x64", "strict", None),
    )

class Test_GBK(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gbk'
    tstring = test_multibytecodec_support.load_teststring('gbk')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"\x83\x34\x83\x31", "strict", None),
        ("\u30fb", "strict", None),
    )

class Test_GB18030(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'gb18030'
    tstring = test_multibytecodec_support.load_teststring('gb18030')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u804a"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u804a\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u804a"),
        (b"abc\x84\x39\x84\x39\xc1\xc4", "replace", "abc\ufffd\u804a"),
        ("\u30fb", "strict", b"\x819\xa79"),
    )
    has_iso10646 = True

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
