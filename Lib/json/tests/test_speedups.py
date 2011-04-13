from unittest import TestCase, skipUnless

from json import decoder, encoder, scanner

try:
    import _json
except ImportError:
    _json = None

@skipUnless(_json, 'test requires the _json module')
class TestSpeedups(TestCase):
    def test_scanstring(self):
        self.assertEqual(decoder.scanstring.__module__, "_json")
        self.assertIs(decoder.scanstring, decoder.c_scanstring)

    def test_encode_basestring_ascii(self):
        self.assertEqual(encoder.encode_basestring_ascii.__module__, "_json")
        self.assertIs(encoder.encode_basestring_ascii,
                      encoder.c_encode_basestring_ascii)

class TestDecode(TestCase):
    def test_make_scanner(self):
        self.assertRaises(AttributeError, scanner.c_make_scanner, 1)

    def test_make_encoder(self):
        self.assertRaises(TypeError, encoder.c_make_encoder,
            (True, False),
            b"\xCD\x7D\x3D\x4E\x12\x4C\xF9\x79\xD7\x52\xBA\x82\xF2\x27\x4A\x7D\xA0\xCA\x75",
            None)
