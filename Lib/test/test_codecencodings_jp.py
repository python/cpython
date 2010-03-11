#!/usr/bin/env python3
#
# test_codecencodings_jp.py
#   Codec encoding tests for Japanese encodings.
#

from test import support
from test import test_multibytecodec_support
import unittest

class Test_CP932(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'cp932'
    tstring = test_multibytecodec_support.load_teststring('shift_jis')
    codectests = (
        # invalid bytes
        (b"abc\x81\x00\x81\x00\x82\x84", "strict",  None),
        (b"abc\xf8", "strict",  None),
        (b"abc\x81\x00\x82\x84", "replace", "abc\ufffd\uff44"),
        (b"abc\x81\x00\x82\x84\x88", "replace", "abc\ufffd\uff44\ufffd"),
        (b"abc\x81\x00\x82\x84", "ignore",  "abc\uff44"),
        # sjis vs cp932
        (b"\\\x7e", "replace", "\\\x7e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "replace", "\uff3c\u2225\uff0d"),
    )

class Test_EUC_JISX0213(test_multibytecodec_support.TestBase,
                        unittest.TestCase):
    encoding = 'euc_jisx0213'
    tstring = test_multibytecodec_support.load_teststring('euc_jisx0213')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u7956"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u7956\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u7956"),
        (b"abc\x8f\x83\x83", "replace", "abc\ufffd"),
        (b"\xc1\x64", "strict", None),
        (b"\xa1\xc0", "strict", "\uff3c"),
    )
    xmlcharnametest = (
        "\xab\u211c\xbb = \u2329\u1234\u232a",
        b"\xa9\xa8&real;\xa9\xb2 = &lang;&#4660;&rang;"
    )

eucjp_commontests = (
    (b"abc\x80\x80\xc1\xc4", "strict",  None),
    (b"abc\xc8", "strict",  None),
    (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\u7956"),
    (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\u7956\ufffd"),
    (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u7956"),
    (b"abc\x8f\x83\x83", "replace", "abc\ufffd"),
    (b"\xc1\x64", "strict", None),
)

class Test_EUC_JP_COMPAT(test_multibytecodec_support.TestBase,
                         unittest.TestCase):
    encoding = 'euc_jp'
    tstring = test_multibytecodec_support.load_teststring('euc_jp')
    codectests = eucjp_commontests + (
        (b"\xa1\xc0\\", "strict", "\uff3c\\"),
        ("\xa5", "strict", b"\x5c"),
        ("\u203e", "strict", b"\x7e"),
    )

shiftjis_commonenctests = (
    (b"abc\x80\x80\x82\x84", "strict",  None),
    (b"abc\xf8", "strict",  None),
    (b"abc\x80\x80\x82\x84", "replace", "abc\ufffd\uff44"),
    (b"abc\x80\x80\x82\x84\x88", "replace", "abc\ufffd\uff44\ufffd"),
    (b"abc\x80\x80\x82\x84def", "ignore",  "abc\uff44def"),
)

class Test_SJIS_COMPAT(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'shift_jis'
    tstring = test_multibytecodec_support.load_teststring('shift_jis')
    codectests = shiftjis_commonenctests + (
        (b"\\\x7e", "strict", "\\\x7e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "strict", "\uff3c\u2016\u2212"),
    )

class Test_SJISX0213(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'shift_jisx0213'
    tstring = test_multibytecodec_support.load_teststring('shift_jisx0213')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\x82\x84", "strict",  None),
        (b"abc\xf8", "strict",  None),
        (b"abc\x80\x80\x82\x84", "replace", "abc\ufffd\uff44"),
        (b"abc\x80\x80\x82\x84\x88", "replace", "abc\ufffd\uff44\ufffd"),
        (b"abc\x80\x80\x82\x84def", "ignore",  "abc\uff44def"),
        # sjis vs cp932
        (b"\\\x7e", "replace", "\xa5\u203e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "replace", "\x5c\u2016\u2212"),
    )
    xmlcharnametest = (
        "\xab\u211c\xbb = \u2329\u1234\u232a",
        b"\x85G&real;\x85Q = &lang;&#4660;&rang;"
    )

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
