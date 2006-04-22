#!/usr/bin/env python
#
# test_codecmaps_hk.py
#   Codec mapping tests for HongKong encodings
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestBig5HKSCSMap(test_multibytecodec_support.TestBase_Mapping,
                       unittest.TestCase):
    encoding = 'big5hkscs'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/BIG5HKSCS.TXT'

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestBig5HKSCSMap))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
