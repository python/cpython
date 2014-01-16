#
# test_codecencodings_jp.py
#   Codec encoding tests for Japanese encodings.
#

from test import support
from test import multibytecodec_support
import unittest

class Test_CP932(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'cp932'
    tstring = multibytecodec_support.load_teststring('shift_jis')
    codectests = (
        # invalid bytes
        (b"abc\x81\x00\x81\x00\x82\x84", "strict",  None),
        (b"abc\xf8", "strict",  None),
        (b"abc\x81\x00\x82\x84", "replace", "abc\ufffd\x00\uff44"),
        (b"abc\x81\x00\x82\x84\x88", "replace", "abc\ufffd\x00\uff44\ufffd"),
        (b"abc\x81\x00\x82\x84", "ignore",  "abc\x00\uff44"),
        (b"ab\xEBxy", "replace", "ab\uFFFDxy"),
        (b"ab\xF0\x39xy", "replace", "ab\uFFFD9xy"),
        (b"ab\xEA\xF0xy", "replace", 'ab\ufffd\ue038y'),
        # sjis vs cp932
        (b"\\\x7e", "replace", "\\\x7e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "replace", "\uff3c\u2225\uff0d"),
    )

euc_commontests = (
    # invalid bytes
    (b"abc\x80\x80\xc1\xc4", "strict",  None),
    (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\u7956"),
    (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\u7956\ufffd"),
    (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\u7956"),
    (b"abc\xc8", "strict",  None),
    (b"abc\x8f\x83\x83", "replace", "abc\ufffd\ufffd\ufffd"),
    (b"\x82\xFCxy", "replace", "\ufffd\ufffdxy"),
    (b"\xc1\x64", "strict", None),
    (b"\xa1\xc0", "strict", "\uff3c"),
    (b"\xa1\xc0\\", "strict", "\uff3c\\"),
    (b"\x8eXY", "replace", "\ufffdXY"),
)

class Test_EUC_JIS_2004(multibytecodec_support.TestBase,
                        unittest.TestCase):
    encoding = 'euc_jis_2004'
    tstring = multibytecodec_support.load_teststring('euc_jisx0213')
    codectests = euc_commontests
    xmlcharnametest = (
        "\xab\u211c\xbb = \u2329\u1234\u232a",
        b"\xa9\xa8&real;\xa9\xb2 = &lang;&#4660;&rang;"
    )

class Test_EUC_JISX0213(multibytecodec_support.TestBase,
                        unittest.TestCase):
    encoding = 'euc_jisx0213'
    tstring = multibytecodec_support.load_teststring('euc_jisx0213')
    codectests = euc_commontests
    xmlcharnametest = (
        "\xab\u211c\xbb = \u2329\u1234\u232a",
        b"\xa9\xa8&real;\xa9\xb2 = &lang;&#4660;&rang;"
    )

class Test_EUC_JP_COMPAT(multibytecodec_support.TestBase,
                         unittest.TestCase):
    encoding = 'euc_jp'
    tstring = multibytecodec_support.load_teststring('euc_jp')
    codectests = euc_commontests + (
        ("\xa5", "strict", b"\x5c"),
        ("\u203e", "strict", b"\x7e"),
    )

shiftjis_commonenctests = (
    (b"abc\x80\x80\x82\x84", "strict",  None),
    (b"abc\xf8", "strict",  None),
    (b"abc\x80\x80\x82\x84def", "ignore",  "abc\uff44def"),
)

class Test_SJIS_COMPAT(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'shift_jis'
    tstring = multibytecodec_support.load_teststring('shift_jis')
    codectests = shiftjis_commonenctests + (
        (b"abc\x80\x80\x82\x84", "replace", "abc\ufffd\ufffd\uff44"),
        (b"abc\x80\x80\x82\x84\x88", "replace", "abc\ufffd\ufffd\uff44\ufffd"),

        (b"\\\x7e", "strict", "\\\x7e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "strict", "\uff3c\u2016\u2212"),
        (b"abc\x81\x39", "replace",  "abc\ufffd9"),
        (b"abc\xEA\xFC", "replace",  "abc\ufffd\ufffd"),
        (b"abc\xFF\x58", "replace",  "abc\ufffdX"),
    )

class Test_SJIS_2004(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'shift_jis_2004'
    tstring = multibytecodec_support.load_teststring('shift_jis')
    codectests = shiftjis_commonenctests + (
        (b"\\\x7e", "strict", "\xa5\u203e"),
        (b"\x81\x5f\x81\x61\x81\x7c", "strict", "\\\u2016\u2212"),
        (b"abc\xEA\xFC", "strict",  "abc\u64bf"),
        (b"\x81\x39xy", "replace",  "\ufffd9xy"),
        (b"\xFF\x58xy", "replace",  "\ufffdXxy"),
        (b"\x80\x80\x82\x84xy", "replace", "\ufffd\ufffd\uff44xy"),
        (b"\x80\x80\x82\x84\x88xy", "replace", "\ufffd\ufffd\uff44\u5864y"),
        (b"\xFC\xFBxy", "replace", '\ufffd\u95b4y'),
    )
    xmlcharnametest = (
        "\xab\u211c\xbb = \u2329\u1234\u232a",
        b"\x85G&real;\x85Q = &lang;&#4660;&rang;"
    )

class Test_SJISX0213(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'shift_jisx0213'
    tstring = multibytecodec_support.load_teststring('shift_jisx0213')
    codectests = shiftjis_commonenctests + (
        (b"abc\x80\x80\x82\x84", "replace", "abc\ufffd\ufffd\uff44"),
        (b"abc\x80\x80\x82\x84\x88", "replace", "abc\ufffd\ufffd\uff44\ufffd"),

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
