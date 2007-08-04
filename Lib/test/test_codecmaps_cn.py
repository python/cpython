#!/usr/bin/env python
#
# test_codecmaps_cn.py
#   Codec mapping tests for PRC encodings
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestGB2312Map(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gb2312'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/EUC-CN.TXT'

class TestGBKMap(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gbk'
    mapfileurl = 'http://www.unicode.org/Public/MAPPINGS/VENDORS/' \
                 'MICSFT/WINDOWS/CP936.TXT'

class TestGB18030Map(test_multibytecodec_support.TestBase_Mapping,
                     unittest.TestCase):
    encoding = 'gb18030'
    mapfileurl = 'http://source.icu-project.org/repos/icu/data/' \
                 'trunk/charset/data/xml/gb-18030-2000.xml'


def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestGB2312Map))
    suite.addTest(unittest.makeSuite(TestGBKMap))
    suite.addTest(unittest.makeSuite(TestGB18030Map))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
