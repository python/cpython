#
# test_codecmaps_tw.py
#   Codec mapping tests for ROC encodings
#

from test import multibytecodec_support
from test import support
import unittest

class TestBIG5Map(multibytecodec_support.TestBase_Mapping,
                  unittest.TestCase):
    encoding = 'big5'
    mapfileurl = f'{support.TEST_HTTP_URL}/unicode/BIG5.TXT'

class TestCP950Map(multibytecodec_support.TestBase_Mapping,
                   unittest.TestCase):
    encoding = 'cp950'
    mapfileurl = f'{support.TEST_HTTP_URL}/unicode/CP950.TXT'
    pass_enctest = [
        (b'\xa2\xcc', '\u5341'),
        (b'\xa2\xce', '\u5345'),
    ]
    codectests = (
        (b"\xFFxy", "replace",  "\ufffdxy"),
    )

if __name__ == "__main__":
    unittest.main()
