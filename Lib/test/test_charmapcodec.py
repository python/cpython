""" Python character mapping codec test

This uses the test codec in testcodec.py and thus also tests the
encodings package lookup scheme.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright 2000 Guido van Rossum.

"""#"

import test.test_support, unittest

import codecs

# Register a search function which knows about our codec
def codec_search_function(encoding):
    if encoding == 'testcodec':
        from test import testcodec
        return tuple(testcodec.getregentry())
    return None

codecs.register(codec_search_function)

# test codec's name (see test/testcodec.py)
codecname = 'testcodec'

class CharmapCodecTest(unittest.TestCase):
    def test_constructorx(self):
        self.assertEquals(str(b'abc', codecname), 'abc')
        self.assertEquals(str(b'xdef', codecname), 'abcdef')
        self.assertEquals(str(b'defx', codecname), 'defabc')
        self.assertEquals(str(b'dxf', codecname), 'dabcf')
        self.assertEquals(str(b'dxfx', codecname), 'dabcfabc')

    def test_encodex(self):
        self.assertEquals('abc'.encode(codecname), b'abc')
        self.assertEquals('xdef'.encode(codecname), b'abcdef')
        self.assertEquals('defx'.encode(codecname), b'defabc')
        self.assertEquals('dxf'.encode(codecname), b'dabcf')
        self.assertEquals('dxfx'.encode(codecname), b'dabcfabc')

    def test_constructory(self):
        self.assertEquals(str(b'ydef', codecname), 'def')
        self.assertEquals(str(b'defy', codecname), 'def')
        self.assertEquals(str(b'dyf', codecname), 'df')
        self.assertEquals(str(b'dyfy', codecname), 'df')

    def test_maptoundefined(self):
        self.assertRaises(UnicodeError, str, b'abc\001', codecname)

def test_main():
    test.test_support.run_unittest(CharmapCodecTest)

if __name__ == "__main__":
    test_main()
