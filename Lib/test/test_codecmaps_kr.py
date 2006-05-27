#!/usr/bin/env python
#
# test_codecmaps_kr.py
#   Codec mapping tests for ROK encodings
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestCP949Map(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'cp949'
    mapfileurl = 'http://www.unicode.org/Public/MAPPINGS/VENDORS/MICSFT' \
                 '/WINDOWS/CP949.TXT'


class TestEUCKRMap(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'euc_kr'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/EUC-KR.TXT'


class TestJOHABMap(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'johab'
    mapfileurl = 'http://www.unicode.org/Public/MAPPINGS/OBSOLETE/EASTASIA/' \
                 'KSC/JOHAB.TXT'
    # KS X 1001 standard assigned 0x5c as WON SIGN.
    # but, in early 90s that is the only era used johab widely,
    # the most softwares implements it as REVERSE SOLIDUS.
    # So, we ignore the standard here.
    pass_enctest = [('\\', u'\u20a9')]
    pass_dectest = [('\\', u'\u20a9')]

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestCP949Map))
    suite.addTest(unittest.makeSuite(TestEUCKRMap))
    suite.addTest(unittest.makeSuite(TestJOHABMap))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
