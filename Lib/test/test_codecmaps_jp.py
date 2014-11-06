#
# test_codecmaps_jp.py
#   Codec mapping tests for Japanese encodings
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestCP932Map(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'cp932'
    mapfileurl = 'http://www.pythontest.net/unicode/CP932.TXT'
    supmaps = [
        ('\x80', u'\u0080'),
        ('\xa0', u'\uf8f0'),
        ('\xfd', u'\uf8f1'),
        ('\xfe', u'\uf8f2'),
        ('\xff', u'\uf8f3'),
    ]
    for i in range(0xa1, 0xe0):
        supmaps.append((chr(i), unichr(i+0xfec0)))


class TestEUCJPCOMPATMap(test_multibytecodec_support.TestBase_Mapping,
                         unittest.TestCase):
    encoding = 'euc_jp'
    mapfilename = 'EUC-JP.TXT'
    mapfileurl = 'http://www.pythontest.net/unicode/EUC-JP.TXT'


class TestSJISCOMPATMap(test_multibytecodec_support.TestBase_Mapping,
                        unittest.TestCase):
    encoding = 'shift_jis'
    mapfilename = 'SHIFTJIS.TXT'
    mapfileurl = 'http://www.pythontest.net/unicode/SHIFTJIS.TXT'
    pass_enctest = [
        ('\x81_', u'\\'),
    ]
    pass_dectest = [
        ('\\', u'\xa5'),
        ('~', u'\u203e'),
        ('\x81_', u'\\'),
    ]

class TestEUCJISX0213Map(test_multibytecodec_support.TestBase_Mapping,
                         unittest.TestCase):
    encoding = 'euc_jisx0213'
    mapfilename = 'EUC-JISX0213.TXT'
    mapfileurl = 'http://www.pythontest.net/unicode/EUC-JISX0213.TXT'


class TestSJISX0213Map(test_multibytecodec_support.TestBase_Mapping,
                       unittest.TestCase):
    encoding = 'shift_jisx0213'
    mapfilename = 'SHIFT_JISX0213.TXT'
    mapfileurl = 'http://www.pythontest.net/unicode/SHIFT_JISX0213.TXT'


def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
