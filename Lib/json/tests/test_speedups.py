import decimal
from unittest import TestCase

from json import decoder
from json import encoder

class TestSpeedups(TestCase):
    def test_scanstring(self):
        self.assertEquals(decoder.scanstring.__module__, "_json")
        self.assert_(decoder.scanstring is decoder.c_scanstring)

    def test_encode_basestring_ascii(self):
        self.assertEquals(encoder.encode_basestring_ascii.__module__, "_json")
        self.assert_(encoder.encode_basestring_ascii is
                          encoder.c_encode_basestring_ascii)
