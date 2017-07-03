#
# test_codecencodings_hk.py
#   Codec encoding tests for HongKong encodings.
#

from test import test_support
from test import multibytecodec_support
import unittest

class Test_Big5HKSCS(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'big5hkscs'
    tstring = multibytecodec_support.load_teststring('big5hkscs')
    codectests = (
        # invalid bytes
        ("abc\x80\x80\xc1\xc4", "strict",  None),
        ("abc\xc8", "strict",  None),
        ("abc\x80\x80\xc1\xc4", "replace", u"abc\ufffd\u8b10"),
        ("abc\x80\x80\xc1\xc4\xc8", "replace", u"abc\ufffd\u8b10\ufffd"),
        ("abc\x80\x80\xc1\xc4", "ignore",  u"abc\u8b10"),
    )

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
