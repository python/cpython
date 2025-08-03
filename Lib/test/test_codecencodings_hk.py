#
# test_codecencodings_hk.py
#   Codec encoding tests for HongKong encodings.
#

import unittest

from test import multibytecodec_support


class Test_Big5HKSCS(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'big5hkscs'
    tstring = multibytecodec_support.load_teststring('big5hkscs')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\u8b10"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\u8b10\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u8b10"),
    )

if __name__ == "__main__":
    unittest.main()
