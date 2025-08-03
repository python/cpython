#
# test_codecmaps_cn.py
#   Codec mapping tests for PRC encodings
#

import unittest

from test import multibytecodec_support


class TestGB2312Map(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gb2312'
    mapfileurl = 'http://www.pythontest.net/unicode/EUC-CN.TXT'

class TestGBKMap(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gbk'
    mapfileurl = 'http://www.pythontest.net/unicode/CP936.TXT'

class TestGB18030Map(multibytecodec_support.TestBase_Mapping,
                     unittest.TestCase):
    encoding = 'gb18030'
    mapfileurl = 'http://www.pythontest.net/unicode/gb-18030-2000.xml'


if __name__ == "__main__":
    unittest.main()
