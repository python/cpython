from test import test_support
import unittest, sys
import codecs, _iconv_codec
from encodings import iconv_codec
from StringIO import StringIO

class IconvCodecTest(unittest.TestCase):

    if sys.byteorder == 'big':
        spam = '\x00s\x00p\x00a\x00m' * 2
    else:
        spam = 's\x00p\x00a\x00m\x00' * 2

    def test_sane(self):
        # FIXME: Commented out, because it's not clear whether
        # the internal encoding choosen requires byte swapping
        # for this iconv() implementation.
        if False:
            self.encoder, self.decoder, self.reader, self.writer = \
                codecs.lookup(_iconv_codec.internal_encoding)
            self.assertEqual(self.decoder(self.spam), (u'spamspam', 16))
            self.assertEqual(self.encoder(u'spamspam'), (self.spam, 8))
            self.assertEqual(self.reader(StringIO(self.spam)).read(), u'spamspam')
            f = StringIO()
            self.writer(f).write(u'spamspam')
            self.assertEqual(f.getvalue(), self.spam)

    def test_basic_errors(self):
        self.encoder, self.decoder, self.reader, self.writer = \
            iconv_codec.lookup("ascii")
        def testencerror(errors):
            return self.encoder(u'sp\ufffdam', errors)
        def testdecerror(errors):
            return self.decoder('sp\xffam', errors)

        self.assertRaises(UnicodeEncodeError, testencerror, 'strict')
        self.assertRaises(UnicodeDecodeError, testdecerror, 'strict')
        self.assertEqual(testencerror('replace'), ('sp?am', 5))
        self.assertEqual(testdecerror('replace'), (u'sp\ufffdam', 5))
        self.assertEqual(testencerror('ignore'), ('spam', 5))
        self.assertEqual(testdecerror('ignore'), (u'spam', 5))

    def test_pep293_errors(self):
        self.encoder, self.decoder, self.reader, self.writer = \
            iconv_codec.lookup("ascii")
        def testencerror(errors):
            return self.encoder(u'sp\ufffdam', errors)
        def testdecerror(errors):
            return self.decoder('sp\xffam', errors)

        self.assertEqual(testencerror('xmlcharrefreplace'),
                         ('sp&#65533;am', 5))
        self.assertEqual(testencerror('backslashreplace'),
                         ('sp\\ufffdam', 5))

        def error_bomb(exc):
            return (u'*'*40, len(exc.object))
        def error_mock(exc):
            error_mock.lastexc = exc
            return (unicode(exc.object[exc.start - 1]), exc.end)

        codecs.register_error('test_iconv_codec.bomb', error_bomb)
        codecs.register_error('test_iconv_codec.mock', error_mock)

        self.assertEqual(testencerror('test_iconv_codec.bomb'),
                         ('sp' + ('*'*40), 5))
        self.assertEqual(testdecerror('test_iconv_codec.bomb'),
                         (u'sp' + (u'*'*40), 5))

        self.assertEqual(testencerror('test_iconv_codec.mock'), ('sppam', 5))
        exc = error_mock.lastexc
        self.assertEqual(exc.object, u'sp\ufffdam')
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)
        self.assert_(isinstance(exc, UnicodeEncodeError))

        self.assertEqual(testdecerror('test_iconv_codec.mock'), (u'sppam', 5))
        exc = error_mock.lastexc
        self.assertEqual(exc.object, 'sp\xffam')
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)
        self.assert_(isinstance(exc, UnicodeDecodeError))

    def test_empty_escape_decode(self):
        self.encoder, self.decoder, self.reader, self.writer = \
            iconv_codec.lookup("ascii")
        self.assertEquals(self.decoder(u""), ("", 0))
        self.assertEquals(self.encoder(""), (u"", 0))

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(IconvCodecTest))
    test_support.run_suite(suite)


if __name__ == "__main__":
    test_main()

# ex: ts=8 sts=4 et
