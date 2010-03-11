#!/usr/bin/env python3
#
# test_codecencodings_tw.py
#   Codec encoding tests for ROC encodings.
#

from test import support
from test import test_multibytecodec_support
import unittest

class Test_Big5(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'big5'
    tstring = test_multibytecodec_support.load_teststring('big5')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u8b10"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u8b10\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u8b10"),
    )

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
