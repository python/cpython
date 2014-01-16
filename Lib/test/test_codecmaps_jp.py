#
# test_codecmaps_jp.py
#   Codec mapping tests for Japanese encodings
#

from test import support
from test import multibytecodec_support
import unittest

class TestCP932Map(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'cp932'
    mapfileurl = 'http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT/' \
                 'WINDOWS/CP932.TXT'
    supmaps = [
        (b'\x80', '\u0080'),
        (b'\xa0', '\uf8f0'),
        (b'\xfd', '\uf8f1'),
        (b'\xfe', '\uf8f2'),
        (b'\xff', '\uf8f3'),
    ]
    for i in range(0xa1, 0xe0):
        supmaps.append((bytes([i]), chr(i+0xfec0)))


class TestEUCJPCOMPATMap(multibytecodec_support.TestBase_Mapping,
                         unittest.TestCase):
    encoding = 'euc_jp'
    mapfilename = 'EUC-JP.TXT'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/EUC-JP.TXT'


class TestSJISCOMPATMap(multibytecodec_support.TestBase_Mapping,
                        unittest.TestCase):
    encoding = 'shift_jis'
    mapfilename = 'SHIFTJIS.TXT'
    mapfileurl = 'http://www.unicode.org/Public/MAPPINGS/OBSOLETE' \
                 '/EASTASIA/JIS/SHIFTJIS.TXT'
    pass_enctest = [
        (b'\x81_', '\\'),
    ]
    pass_dectest = [
        (b'\\', '\xa5'),
        (b'~', '\u203e'),
        (b'\x81_', '\\'),
    ]

class TestEUCJISX0213Map(multibytecodec_support.TestBase_Mapping,
                         unittest.TestCase):
    encoding = 'euc_jisx0213'
    mapfilename = 'EUC-JISX0213.TXT'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/EUC-JISX0213.TXT'


class TestSJISX0213Map(multibytecodec_support.TestBase_Mapping,
                       unittest.TestCase):
    encoding = 'shift_jisx0213'
    mapfilename = 'SHIFT_JISX0213.TXT'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/SHIFT_JISX0213.TXT'


def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
