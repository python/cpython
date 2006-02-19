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
        self.assertEquals(unicode('abc', codecname), u'abc')
        self.assertEquals(unicode('xdef', codecname), u'abcdef')
        self.assertEquals(unicode('defx', codecname), u'defabc')
        self.assertEquals(unicode('dxf', codecname), u'dabcf')
        self.assertEquals(unicode('dxfx', codecname), u'dabcfabc')

    def test_encodex(self):
        self.assertEquals(u'abc'.encode(codecname), 'abc')
        self.assertEquals(u'xdef'.encode(codecname), 'abcdef')
        self.assertEquals(u'defx'.encode(codecname), 'defabc')
        self.assertEquals(u'dxf'.encode(codecname), 'dabcf')
        self.assertEquals(u'dxfx'.encode(codecname), 'dabcfabc')

    def test_constructory(self):
        self.assertEquals(unicode('ydef', codecname), u'def')
        self.assertEquals(unicode('defy', codecname), u'def')
        self.assertEquals(unicode('dyf', codecname), u'df')
        self.assertEquals(unicode('dyfy', codecname), u'df')

    def test_maptoundefined(self):
        self.assertRaises(UnicodeError, unicode, 'abc\001', codecname)

def test_main():
    test.test_support.run_unittest(CharmapCodecTest)

if __name__ == "__main__":
    test_main()
