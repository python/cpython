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
    mapfileurl = 'http://www.pythontest.net/unicode/EUC-CN.TXT'

class TestGBKMap(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'gbk'
    mapfileurl = 'http://www.pythontest.net/unicode/CP936.TXT'

class TestGB18030Map(test_multibytecodec_support.TestBase_Mapping,
                     unittest.TestCase):
    encoding = 'gb18030'
    mapfileurl = 'http://www.pythontest.net/unicode/gb-18030-2000.xml'


def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
