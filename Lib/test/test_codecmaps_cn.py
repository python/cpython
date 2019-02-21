#
# test_codecmaps_cn.py
#   Codec mapping tests for PRC encodings
#

from test import multibytecodec_support
from test import support
import unittest

class TestGB2312Map(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gb2312'
    mapfileurl = f'{support.TEST_HTTP_URL}/unicode/EUC-CN.TXT'

class TestGBKMap(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gbk'
    mapfileurl = f'{support.TEST_HTTP_URL}/unicode/CP936.TXT'

class TestGB18030Map(multibytecodec_support.TestBase_Mapping,
                     unittest.TestCase):
    encoding = 'gb18030'
    mapfileurl = f'{support.TEST_HTTP_URL}/unicode/gb-18030-2000.xml'


if __name__ == "__main__":
    unittest.main()
