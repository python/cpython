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
    mapfileurl = 'http://people.freebsd.org/~perky/i18n/BIG5HKSCS-2004.TXT'

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    support.use_resources = ['urlfetch']
    test_main()
