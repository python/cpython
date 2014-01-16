# Codec encoding tests for ISO 2022 encodings.

from test import test_support
from test import test_multibytecodec_support
import unittest

COMMON_CODEC_TESTS = (
        # invalid bytes
        (b'ab\xFFcd', 'replace', u'ab\uFFFDcd'),
        (b'ab\x1Bdef', 'replace', u'ab\x1Bdef'),
        (b'ab\x1B$def', 'replace', u'ab\uFFFD'),
    )

class Test_ISO2022_JP(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp'
    tstring = test_multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', u'ab\x1BNdef'),
    )

class Test_ISO2022_JP2(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_jp_2'
    tstring = test_multibytecodec_support.load_teststring('iso2022_jp')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', u'abdef'),
    )

class Test_ISO2022_KR(test_multibytecodec_support.TestBase, unittest.TestCase):
    encoding = 'iso2022_kr'
    tstring = test_multibytecodec_support.load_teststring('iso2022_kr')
    codectests = COMMON_CODEC_TESTS + (
        (b'ab\x1BNdef', 'replace', u'ab\x1BNdef'),
    )

    # iso2022_kr.txt cannot be used to test "chunk coding": the escape
    # sequence is only written on the first line
    @unittest.skip('iso2022_kr.txt cannot be used to test "chunk coding"')
    def test_chunkcoding(self):
        pass

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
