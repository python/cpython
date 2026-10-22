# Codec encoding tests for ISO 2022 encodings.

from test import multibytecodec_support
import unittest

COMMON_CODEC_TESTS = (
        # invalid bytes
        (b'ab\xFFcd', 'replace', 'ab\uFFFDcd'),
        (b'ab\x1Bdef', 'replace', 'ab\x1Bdef'),
        (b'ab\x1B$def', 'replace', 'ab\uFFFD'),
    )

class Test_ISO2022_JP(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp'
    tstring = multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', 'ab\x1BNdef'),
        # there is no G1 in this encoding
        (b'ab\x1B$)Bdef', 'strict', None),
        (b'ab\x1B)Jdef', 'strict', None),
        (b'ab\x1B)Bdef', 'strict', None),
    )

class Test_ISO2022_JP2(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp_2'
    tstring = multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', 'abdef'),
        # a charset can only be designated to its own plane
        (b'\x1B.A\x1BNi', 'strict', '\xe9'),
        (b'\x1B$(C\x30\x21\x1B(B', 'strict', '\uac00'),
        (b'ab\x1B(Adef', 'strict', None),
        (b'ab\x1B(Fdef', 'strict', None),
        (b'ab\x1B.J\x1BNdef', 'strict', None),
        (b'ab\x1B.Bdef', 'strict', None),
        (b'ab\x1B$)Cdef', 'strict', None),
    )

class Test_ISO2022_JP3(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp_3'
    tstring = multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', 'ab\x1BNdef'),
        (b'\x1B$(O\x2E\x23\x1B(B', 'strict', '\u3402'      ),
        (b'\x1B$(O\x2E\x22\x1B(B', 'strict', '\U0002000B'  ),
        (b'\x1B$(O\x24\x77\x1B(B', 'strict', '\u304B\u309A'),
        (b'\x1B$(P\x21\x22\x1B(B', 'strict', '\u4E02'      ),
        (b'\x1B$(P\x7E\x76\x1B(B', 'strict', '\U0002A6B2'  ),
        ('\u3402',       'strict', b'\x1B$(O\x2E\x23\x1B(B'),
        ('\U0002000B',   'strict', b'\x1B$(O\x2E\x22\x1B(B'),
        ('\u304B\u309A', 'strict', b'\x1B$(O\x24\x77\x1B(B'),
        ('\u4E02',       'strict', b'\x1B$(P\x21\x22\x1B(B'),
        ('\U0002A6B2',   'strict', b'\x1B$(P\x7E\x76\x1B(B'),
        (b'ab\x1B$(O\x2E\x21\x1B(Bdef', 'replace', 'ab\uFFFDdef'),
        ('ab\u4FF1def', 'replace', b'ab?def'),
    )
    xmlcharnametest = (
        '\xAB\u211C\xBB = \u2329\u1234\u232A',
        b'\x1B$(O\x29\x28\x1B(B&real;\x1B$(O\x29\x32\x1B(B = &lang;&#4660;&rang;'
    )

class Test_ISO2022_JP2004(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp_2004'
    tstring = multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', 'ab\x1BNdef'),
        (b'\x1B$(Q\x2E\x23\x1B(B', 'strict', '\u3402'      ),
        (b'\x1B$(Q\x2E\x22\x1B(B', 'strict', '\U0002000B'  ),
        (b'\x1B$(Q\x24\x77\x1B(B', 'strict', '\u304B\u309A'),
        (b'\x1B$(P\x21\x22\x1B(B', 'strict', '\u4E02'      ),
        (b'\x1B$(P\x7E\x76\x1B(B', 'strict', '\U0002A6B2'  ),
        ('\u3402',       'strict', b'\x1B$(Q\x2E\x23\x1B(B'),
        ('\U0002000B',   'strict', b'\x1B$(Q\x2E\x22\x1B(B'),
        ('\u304B\u309A', 'strict', b'\x1B$(Q\x24\x77\x1B(B'),
        ('\u4E02',       'strict', b'\x1B$(P\x21\x22\x1B(B'),
        ('\U0002A6B2',   'strict', b'\x1B$(P\x7E\x76\x1B(B'),
        (b'ab\x1B$(Q\x2E\x21\x1B(Bdef', 'replace', 'ab\u4FF1def'),
        ('ab\u4FF1def', 'replace', b'ab\x1B$(Q\x2E\x21\x1B(Bdef'),
    )
    xmlcharnametest = (
        '\xAB\u211C\xBB = \u2329\u1234\u232A',
        b'\x1B$(Q\x29\x28\x1B(B&real;\x1B$(Q\x29\x32\x1B(B = &lang;&#4660;&rang;'
    )

class Test_ISO2022_KR(multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_kr'
    tstring = multibytecodec_support.load_teststring('iso2022_kr')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', 'ab\x1BNdef'),
        # KS X 1001 can only be designated to G1
        (b'\x1B$)C\x0E\x30\x21\x0Fdef', 'strict', '\uac00def'),
        (b'ab\x1B$(Cdefg', 'strict', None),
        (b'ab\x1B$Cdefg', 'strict', None),
        (b'ab\x1B)Bdefg', 'strict', None),
        (b'ab\x1B$(Cdefg', 'replace', 'ab\ufffddefg'),
    )

    # iso2022_kr.txt cannot be used to test "chunk coding": the escape
    # sequence is only written on the first line
    @unittest.skip('iso2022_kr.txt cannot be used to test "chunk coding"')
    def test_chunkcoding(self):
        pass

if __name__ == "__main__":
    unittest.main()
