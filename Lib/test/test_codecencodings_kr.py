#
# test_codecencodings_kr.py
#   Codec encoding tests for ROK encodings.
#

from test import multibytecodec_support
import unittest

class Test_CP949(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'cp949'
    tstring = multibytecodec_support.load_teststring('cp949')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\uc894"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\uc894\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\uc894"),
    )

class Test_EUCKR(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'euc_kr'
    tstring = multibytecodec_support.load_teststring('euc_kr')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", 'abc\ufffd\ufffd\uc894'),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\uc894\ufffd"),
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
        (b"a\xa4\xd4\xa4\xb6\xa4", "replace", 'a\ufffd'),
        (b"\xa4\xd4\xa3\xb6\xa4\xd0\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa3\xd0\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa3\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xff\xa4\xd0\xa4\xd4", "replace", '\ufffd\u6e21\ufffd\u3160\ufffd'),
        (b"\xa4\xd4\xa4\xb6\xa4\xff\xa4\xd4", "replace", '\ufffd\u6e21\ub544\ufffd\ufffd'),
        (b"\xa4\xd4\xa4\xb6\xa4\xd0\xa4\xff", "replace", '\ufffd\u6e21\ub544\u572d\ufffd'),
        (b"\xa4\xd4\xff\xa4\xd4\xa4\xb6\xa4\xd0\xa4\xd4", "replace", '\ufffd\ufffd\ufffd\uc4d4'),
        (b"\xc1\xc4", "strict", "\uc894"),

        # make-up sequences for syllables that KS X 1001 encodes directly
        (b"\xa4\xd4\xa4\xa1\xa4\xbf\xa4\xd4", "strict", None),
        (b"\xa4\xd4\xa4\xa1\xa4\xbf\xa4\xd4", "replace", "\ufffd"),
        (b"\xa4\xd4\xa4\xa1\xa4\xbf\xa4\xd4x", "ignore", "x"),
        (b"\xa4\xd4\xa4\xa1\xa4\xbf\xa4\xd4x", "replace", "\ufffdx"),
        (b"\xa4\xd4\xa4\xbe\xa4\xd3\xa4\xbe", "strict", "\ud7a3"),
    )

    def test_makeup_sequence_roundtrip(self):
        accepted = 0
        for cho in range(0xa1, 0xd5):
            for jung in range(0xa1, 0xd5):
                for jong in range(0xa1, 0xd5):
                    b = bytes([0xa4, 0xd4, 0xa4, cho, 0xa4, jung, 0xa4, jong])
                    with self.subTest(b=b):
                        try:
                            s = b.decode('euc_kr')
                        except UnicodeDecodeError:
                            s = b.decode('euc_kr', 'surrogateescape')
                            self.assertEqual(s.encode('euc_kr', 'surrogateescape'), b)
                            continue
                        accepted += 1
                        self.assertEqual(s.encode('euc_kr'), b)
        # Of the 11172 Hangul syllables, KS X 1001 encodes 2350 in 2 bytes;
        # only the rest may be written as make-up sequences.
        self.assertEqual(accepted, 11172 - 2350)

class Test_JOHAB(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'johab'
    tstring = multibytecodec_support.load_teststring('johab')
    codectests = (
        # invalid bytes
        (b"abc\x80\x80\xc1\xc4", "strict",  None),
        (b"abc\xc8", "strict",  None),
        (b"abc\x80\x80\xc1\xc4", "replace", "abc\ufffd\ufffd\ucd27"),
        (b"abc\x80\x80\xc1\xc4\xc8", "replace", "abc\ufffd\ufffd\ucd27\ufffd"),
        (b"abc\x80\x80\xc1\xc4", "ignore",  "abc\ucd27"),
        (b"\xD8abc", "replace",  "\uFFFDabc"),
        (b"\xD8\xFFabc", "replace",  "\uFFFD\uFFFDabc"),
        (b"\x84bxy", "replace",  "\uFFFDbxy"),
        (b"\x8CBxy", "replace",  "\uFFFDBxy"),
    )

if __name__ == "__main__":
    unittest.main()
