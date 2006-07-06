#!/usr/bin/env python
#
# test_multibytecodec.py
#   Unit test for multibytecodec itself
#

from test import test_support
from test import test_multibytecodec_support
from test.test_support import TESTFN
import unittest, StringIO, codecs, sys, os

class Test_MultibyteCodec(unittest.TestCase):

    def test_nullcoding(self):
        self.assertEqual(''.decode('gb18030'), u'')
        self.assertEqual(unicode('', 'gb18030'), u'')
        self.assertEqual(u''.encode('gb18030'), '')

    def test_str_decode(self):
        self.assertEqual('abcd'.encode('gb18030'), 'abcd')

    def test_errorcallback_longindex(self):
        dec = codecs.getdecoder('euc-kr')
        myreplace  = lambda exc: (u'', sys.maxint+1)
        codecs.register_error('test.cjktest', myreplace)
        self.assertRaises(IndexError, dec,
                          'apple\x92ham\x93spam', 'test.cjktest')

    def test_codingspec(self):
        print >> open(TESTFN, 'w'), '# coding: euc-kr'
        try:
            exec open(TESTFN)
        finally:
            os.unlink(TESTFN)

class Test_IncrementalEncoder(unittest.TestCase):

    def test_stateless(self):
        # cp949 encoder isn't stateful at all.
        encoder = codecs.getincrementalencoder('cp949')()
        self.assertEqual(encoder.encode(u'\ud30c\uc774\uc36c \ub9c8\uc744'),
                         '\xc6\xc4\xc0\xcc\xbd\xe3 \xb8\xb6\xc0\xbb')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode(u'\u2606\u223c\u2606', True),
                         '\xa1\xd9\xa1\xad\xa1\xd9')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode(u'', True), '')
        self.assertEqual(encoder.encode(u'', False), '')
        self.assertEqual(encoder.reset(), None)

    def test_stateful(self):
        # jisx0213 encoder is stateful for a few codepoints. eg)
        #   U+00E6 => A9DC
        #   U+00E6 U+0300 => ABC4
        #   U+0300 => ABDC

        encoder = codecs.getincrementalencoder('jisx0213')()
        self.assertEqual(encoder.encode(u'\u00e6\u0300'), '\xab\xc4')
        self.assertEqual(encoder.encode(u'\u00e6'), '')
        self.assertEqual(encoder.encode(u'\u0300'), '\xab\xc4')
        self.assertEqual(encoder.encode(u'\u00e6', True), '\xa9\xdc')

        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode(u'\u0300'), '\xab\xdc')

        self.assertEqual(encoder.encode(u'\u00e6'), '')
        self.assertEqual(encoder.encode('', True), '\xa9\xdc')
        self.assertEqual(encoder.encode('', True), '')

    def test_stateful_keep_buffer(self):
        encoder = codecs.getincrementalencoder('jisx0213')()
        self.assertEqual(encoder.encode(u'\u00e6'), '')
        self.assertRaises(UnicodeEncodeError, encoder.encode, u'\u0123')
        self.assertEqual(encoder.encode(u'\u0300\u00e6'), '\xab\xc4')
        self.assertRaises(UnicodeEncodeError, encoder.encode, u'\u0123')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode(u'\u0300'), '\xab\xdc')
        self.assertEqual(encoder.encode(u'\u00e6'), '')
        self.assertRaises(UnicodeEncodeError, encoder.encode, u'\u0123')
        self.assertEqual(encoder.encode(u'', True), '\xa9\xdc')


class Test_IncrementalDecoder(unittest.TestCase):

    def test_dbcs(self):
        # cp949 decoder is simple with only 1 or 2 bytes sequences.
        decoder = codecs.getincrementaldecoder('cp949')()
        self.assertEqual(decoder.decode('\xc6\xc4\xc0\xcc\xbd'),
                         u'\ud30c\uc774')
        self.assertEqual(decoder.decode('\xe3 \xb8\xb6\xc0\xbb'),
                         u'\uc36c \ub9c8\uc744')
        self.assertEqual(decoder.decode(''), u'')

    def test_dbcs_keep_buffer(self):
        decoder = codecs.getincrementaldecoder('cp949')()
        self.assertEqual(decoder.decode('\xc6\xc4\xc0'), u'\ud30c')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '', True)
        self.assertEqual(decoder.decode('\xcc'), u'\uc774')

        self.assertEqual(decoder.decode('\xc6\xc4\xc0'), u'\ud30c')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '\xcc\xbd', True)
        self.assertEqual(decoder.decode('\xcc'), u'\uc774')

    def test_iso2022(self):
        decoder = codecs.getincrementaldecoder('iso2022-jp')()
        ESC = '\x1b'
        self.assertEqual(decoder.decode(ESC + '('), u'')
        self.assertEqual(decoder.decode('B', True), u'')
        self.assertEqual(decoder.decode(ESC + '$'), u'')
        self.assertEqual(decoder.decode('B@$'), u'\u4e16')
        self.assertEqual(decoder.decode('@$@'), u'\u4e16')
        self.assertEqual(decoder.decode('$', True), u'\u4e16')
        self.assertEqual(decoder.reset(), None)
        self.assertEqual(decoder.decode('@$'), u'@$')
        self.assertEqual(decoder.decode(ESC + '$'), u'')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '', True)
        self.assertEqual(decoder.decode('B@$'), u'\u4e16')


class Test_StreamWriter(unittest.TestCase):
    if len(u'\U00012345') == 2: # UCS2
        def test_gb18030(self):
            s= StringIO.StringIO()
            c = codecs.getwriter('gb18030')(s)
            c.write(u'123')
            self.assertEqual(s.getvalue(), '123')
            c.write(u'\U00012345')
            self.assertEqual(s.getvalue(), '123\x907\x959')
            c.write(u'\U00012345'[0])
            self.assertEqual(s.getvalue(), '123\x907\x959')
            c.write(u'\U00012345'[1] + u'\U00012345' + u'\uac00\u00ac')
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            c.write(u'\U00012345'[0])
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            self.assertRaises(UnicodeError, c.reset)
            self.assertEqual(s.getvalue(),
                    '123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')

        def test_utf_8(self):
            s= StringIO.StringIO()
            c = codecs.getwriter('utf-8')(s)
            c.write(u'123')
            self.assertEqual(s.getvalue(), '123')
            c.write(u'\U00012345')
            self.assertEqual(s.getvalue(), '123\xf0\x92\x8d\x85')

            # Python utf-8 codec can't buffer surrogate pairs yet.
            if 0:
                c.write(u'\U00012345'[0])
                self.assertEqual(s.getvalue(), '123\xf0\x92\x8d\x85')
                c.write(u'\U00012345'[1] + u'\U00012345' + u'\uac00\u00ac')
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac')
                c.write(u'\U00012345'[0])
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac')
                c.reset()
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac\xed\xa0\x88')
                c.write(u'\U00012345'[1])
                self.assertEqual(s.getvalue(),
                    '123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    '\xea\xb0\x80\xc2\xac\xed\xa0\x88\xed\xbd\x85')

    else: # UCS4
        pass

    def test_streamwriter_strwrite(self):
        s = StringIO.StringIO()
        wr = codecs.getwriter('gb18030')(s)
        wr.write('abcd')
        self.assertEqual(s.getvalue(), 'abcd')

class Test_ISO2022(unittest.TestCase):
    def test_g2(self):
        iso2022jp2 = '\x1b(B:hu4:unit\x1b.A\x1bNi de famille'
        uni = u':hu4:unit\xe9 de famille'
        self.assertEqual(iso2022jp2.decode('iso2022-jp-2'), uni)

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(Test_MultibyteCodec))
    suite.addTest(unittest.makeSuite(Test_IncrementalEncoder))
    suite.addTest(unittest.makeSuite(Test_IncrementalDecoder))
    suite.addTest(unittest.makeSuite(Test_StreamWriter))
    suite.addTest(unittest.makeSuite(Test_ISO2022))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
