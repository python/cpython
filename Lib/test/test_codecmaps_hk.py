#!/usr/bin/env python
#
# test_codecmaps_hk.py
#   Codec mapping tests for HongKong encodings
#
# $CJKCodecs: test_codecmaps_hk.py,v 1.1 2004/07/10 17:35:20 perky Exp $

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestBig5HKSCSMap(test_multibytecodec_support.TestBase_Mapping,
                       unittest.TestCase):
    encoding = 'big5hkscs'
    mapfilename = 'BIG5HKSCS.TXT'
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/BIG5HKSCS.TXT'

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestBig5HKSCSMap))
    test_support.run_suite(suite)

test_multibytecodec_support.register_skip_expected(TestBig5HKSCSMap)
if __name__ == "__main__":
    test_main()
