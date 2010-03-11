#!/usr/bin/env python3
#
# test_codecencodings_kr.py
#   Codec encoding tests for ROK encodings.
#

from test import support
from test import test_multibytecodec_support
import unittest

class Test_CP949(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'cp949'
    tstring = test_multibytecodec_support.load_teststring('cp949')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\uc894"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\uc894\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\uc894"),
    )

class Test_EUCKR(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'euc_kr'
    tstring = test_multibytecodec_support.load_teststring('euc_kr')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\uc894"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\uc894\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\uc894"),

        # composed make-up sequence errors
        (b"\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4", "strict", None),
        (b"\xa4\xd4\xa4\xb6", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa4\xd4", "strict", "\uc4d4"),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa4\xd4x", "strict", "\uc4d4x"),
        (b"a\xa4\xd4\xa4\xb6\xa4", "replace", "a\ufffd"),
        (b"\xa4\xd4\xa3\xb6\xa4\xd0\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa3\xd0\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa3\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xff\xa4\xd0\xa4\xd4", "replace", "\ufffd"),
        (b"\xa4\xd4\xa4\xb6\xa4\xff\xa4\xd4", "replace", "\ufffd"),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa4\xff", "replace", "\ufffd"),
        (b"\xc1\xc4", "strict", "\uc894"),
    )

class Test_JOHAB(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'johab'
    tstring = test_multibytecodec_support.load_teststring('johab')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ucd27"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ucd27\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\ucd27"),
    )

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
