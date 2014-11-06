#
# test_codecmaps_hk.py
#   Codec mapping tests for HongKong encodings
#

from test import support
from test import multibytecodec_support
import unittest

class TestBig5HKSCSMap(multibytecodec_support.TestBase_Mapping,
                       unittest.TestCase):
    encoding = 'big5hkscs'
    mapfileurl = 'http://www.pythontest.net/unicode/BIG5HKSCS-2004.TXT'

if __name__ == "__main__":
    unittest.main()
