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
        self.assertEquals(str('abc', codecname), 'abc')
        self.assertEquals(str('xdef', codecname), 'abcdef')
        self.assertEquals(str('defx', codecname), 'defabc')
        self.assertEquals(str('dxf', codecname), 'dabcf')
        self.assertEquals(str('dxfx', codecname), 'dabcfabc')

    def test_encodex(self):
        self.assertEquals('abc'.encode(codecname), 'abc')
        self.assertEquals('xdef'.encode(codecname), 'abcdef')
        self.assertEquals('defx'.encode(codecname), 'defabc')
        self.assertEquals('dxf'.encode(codecname), 'dabcf')
        self.assertEquals('dxfx'.encode(codecname), 'dabcfabc')

    def test_constructory(self):
        self.assertEquals(str('ydef', codecname), 'def')
        self.assertEquals(str('defy', codecname), 'def')
        self.assertEquals(str('dyf', codecname), 'df')
        self.assertEquals(str('dyfy', codecname), 'df')

    def test_maptoundefined(self):
        self.assertRaises(UnicodeError, str, 'abc\001', codecname)

def test_main():
    test.test_support.run_unittest(CharmapCodecTest)

if __name__ == "__main__":
    test_main()
