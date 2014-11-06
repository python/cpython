#
# test_codecmaps_tw.py
#   Codec mapping tests for ROC encodings
#

from test import test_support
from test import test_multibytecodec_support
import unittest

class TestBIG5Map(test_multibytecodec_support.TestBase_Mapping,
                  unittest.TestCase):
    encoding = 'big5'
    mapfileurl = 'http://www.pythontest.net/unicode/BIG5.TXT'

class TestCP950Map(test_multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'cp950'
    mapfileurl = 'http://www.pythontest.net/unicode/CP950.TXT'
    pass_enctest = [
        ('\xa2\xcc', u'\u5341'),
        ('\xa2\xce', u'\u5345'),
    ]

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
