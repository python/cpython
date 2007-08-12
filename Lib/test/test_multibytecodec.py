#!/usr/bin/env python
#
# test_multibytecodec.py
#   Unit test for multibytecodec itself
#

from test import test_support
from test import test_multibytecodec_support
from test.test_support import TESTFN
import unittest, io, codecs, sys, os

ALL_CJKENCODINGS = [
# _codecs_cn
    'gb2312', 'gbk', 'gb18030', 'hz',
# _codecs_hk
    'big5hkscs',
# _codecs_jp
    'cp932', 'shift_jis', 'euc_jp', 'euc_jisx0213', 'shift_jisx0213',
    'euc_jis_2004', 'shift_jis_2004',
# _codecs_kr
    'cp949', 'euc_kr', 'johab',
# _codecs_tw
    'big5', 'cp950',
# _codecs_iso2022
    'iso2022_jp', 'iso2022_jp_1', 'iso2022_jp_2', 'iso2022_jp_2004',
    'iso2022_jp_3', 'iso2022_jp_ext', 'iso2022_kr',
]

class Test_MultibyteCodec(unittest.TestCase):

    def test_nullcoding(self):
        for enc in ALL_CJKENCODINGS:
            self.assertEqual(b''.decode(enc), '')
            self.assertEqual(str(b'', enc), '')
            self.assertEqual(''.encode(enc), b'')

    def test_str_decode(self):
        for enc in ALL_CJKENCODINGS:
            self.assertEqual('abcd'.encode(enc), b'abcd')

    def test_errorcallback_longindex(self):
        dec = codecs.getdecoder('euc-kr')
        myreplace  = lambda exc: ('', sys.maxint+1)
        codecs.register_error('test.cjktest', myreplace)
        self.assertRaises(IndexError, dec,
                          'apple\x92ham\x93spam', 'test.cjktest')

    def test_codingspec(self):
        try:
            for enc in ALL_CJKENCODINGS:
                print('# coding:', enc, file=io.open(TESTFN, 'w'))
                exec(open(TESTFN).read())
        finally:
            test_support.unlink(TESTFN)

class Test_IncrementalEncoder(unittest.TestCase):

    def test_stateless(self):
        # cp949 encoder isn't stateful at all.
        encoder = codecs.getincrementalencoder('cp949')()
        self.assertEqual(encoder.encode('\ud30c\uc774\uc36c \ub9c8\uc744'),
                         b'\xc6\xc4\xc0\xcc\xbd\xe3 \xb8\xb6\xc0\xbb')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode('\u2606\u223c\u2606', True),
                         b'\xa1\xd9\xa1\xad\xa1\xd9')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode('', True), b'')
        self.assertEqual(encoder.encode('', False), b'')
        self.assertEqual(encoder.reset(), None)

    def test_stateful(self):
        # jisx0213 encoder is stateful for a few codepoints. eg)
        #   U+00E6 => A9DC
        #   U+00E6 U+0300 => ABC4
        #   U+0300 => ABDC

        encoder = codecs.getincrementalencoder('jisx0213')()
        self.assertEqual(encoder.encode('\u00e6\u0300'), b'\xab\xc4')
        self.assertEqual(encoder.encode('\u00e6'), b'')
        self.assertEqual(encoder.encode('\u0300'), b'\xab\xc4')
        self.assertEqual(encoder.encode('\u00e6', True), b'\xa9\xdc')

        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode('\u0300'), b'\xab\xdc')

        self.assertEqual(encoder.encode('\u00e6'), b'')
        self.assertEqual(encoder.encode('', True), b'\xa9\xdc')
        self.assertEqual(encoder.encode('', True), b'')

    def test_stateful_keep_buffer(self):
        encoder = codecs.getincrementalencoder('jisx0213')()
        self.assertEqual(encoder.encode('\u00e6'), b'')
        self.assertRaises(UnicodeEncodeError, encoder.encode, '\u0123')
        self.assertEqual(encoder.encode('\u0300\u00e6'), b'\xab\xc4')
        self.assertRaises(UnicodeEncodeError, encoder.encode, '\u0123')
        self.assertEqual(encoder.reset(), None)
        self.assertEqual(encoder.encode('\u0300'), b'\xab\xdc')
        self.assertEqual(encoder.encode('\u00e6'), b'')
        self.assertRaises(UnicodeEncodeError, encoder.encode, '\u0123')
        self.assertEqual(encoder.encode('', True), b'\xa9\xdc')


class Test_IncrementalDecoder(unittest.TestCase):

    def test_dbcs(self):
        # cp949 decoder is simple with only 1 or 2 bytes sequences.
        decoder = codecs.getincrementaldecoder('cp949')()
        self.assertEqual(decoder.decode(b'\xc6\xc4\xc0\xcc\xbd'),
                         '\ud30c\uc774')
        self.assertEqual(decoder.decode(b'\xe3 \xb8\xb6\xc0\xbb'),
                         '\uc36c \ub9c8\uc744')
        self.assertEqual(decoder.decode(b''), '')

    def test_dbcs_keep_buffer(self):
        decoder = codecs.getincrementaldecoder('cp949')()
        self.assertEqual(decoder.decode(b'\xc6\xc4\xc0'), '\ud30c')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '', True)
        self.assertEqual(decoder.decode(b'\xcc'), '\uc774')

        self.assertEqual(decoder.decode(b'\xc6\xc4\xc0'), '\ud30c')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '\xcc\xbd', True)
        self.assertEqual(decoder.decode(b'\xcc'), '\uc774')

    def test_iso2022(self):
        decoder = codecs.getincrementaldecoder('iso2022-jp')()
        ESC = '\x1b'
        self.assertEqual(decoder.decode(ESC + '('), '')
        self.assertEqual(decoder.decode('B', True), '')
        self.assertEqual(decoder.decode(ESC + '$'), '')
        self.assertEqual(decoder.decode('B@$'), '\u4e16')
        self.assertEqual(decoder.decode('@$@'), '\u4e16')
        self.assertEqual(decoder.decode('$', True), '\u4e16')
        self.assertEqual(decoder.reset(), None)
        self.assertEqual(decoder.decode('@$'), '@$')
        self.assertEqual(decoder.decode(ESC + '$'), '')
        self.assertRaises(UnicodeDecodeError, decoder.decode, '', True)
        self.assertEqual(decoder.decode('B@$'), '\u4e16')

class Test_StreamReader(unittest.TestCase):
    def test_bug1728403(self):
        try:
            f = open(TESTFN, 'wb')
            try:
                f.write(b'\xa1')
            finally:
                f.close()
            f = codecs.open(TESTFN, encoding='cp949')
            try:
                self.assertRaises(UnicodeDecodeError, f.read, 2)
            finally:
                f.close()
        finally:
            test_support.unlink(TESTFN)

class Test_StreamWriter(unittest.TestCase):
    if len('\U00012345') == 2: # UCS2
        def test_gb18030(self):
            s= io.BytesIO()
            c = codecs.getwriter('gb18030')(s)
            c.write('123')
            self.assertEqual(s.getvalue(), b'123')
            c.write('\U00012345')
            self.assertEqual(s.getvalue(), b'123\x907\x959')
            c.write('\U00012345'[0])
            self.assertEqual(s.getvalue(), b'123\x907\x959')
            c.write('\U00012345'[1] + '\U00012345' + '\uac00\u00ac')
            self.assertEqual(s.getvalue(),
                    b'123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            c.write('\U00012345'[0])
            self.assertEqual(s.getvalue(),
                    b'123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')
            self.assertRaises(UnicodeError, c.reset)
            self.assertEqual(s.getvalue(),
                    b'123\x907\x959\x907\x959\x907\x959\x827\xcf5\x810\x851')

        def test_utf_8(self):
            s= io.BytesIO()
            c = codecs.getwriter('utf-8')(s)
            c.write('123')
            self.assertEqual(s.getvalue(), b'123')
            c.write('\U00012345')
            self.assertEqual(s.getvalue(), b'123\xf0\x92\x8d\x85')

            # Python utf-8 codec can't buffer surrogate pairs yet.
            if 0:
                c.write('\U00012345'[0])
                self.assertEqual(s.getvalue(), b'123\xf0\x92\x8d\x85')
                c.write('\U00012345'[1] + '\U00012345' + '\uac00\u00ac')
                self.assertEqual(s.getvalue(),
                    b'123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    b'\xea\xb0\x80\xc2\xac')
                c.write('\U00012345'[0])
                self.assertEqual(s.getvalue(),
                    b'123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    b'\xea\xb0\x80\xc2\xac')
                c.reset()
                self.assertEqual(s.getvalue(),
                    b'123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    b'\xea\xb0\x80\xc2\xac\xed\xa0\x88')
                c.write('\U00012345'[1])
                self.assertEqual(s.getvalue(),
                    b'123\xf0\x92\x8d\x85\xf0\x92\x8d\x85\xf0\x92\x8d\x85'
                    b'\xea\xb0\x80\xc2\xac\xed\xa0\x88\xed\xbd\x85')

    else: # UCS4
        pass

    def test_streamwriter_strwrite(self):
        s = io.BytesIO()
        wr = codecs.getwriter('gb18030')(s)
        wr.write('abcd')
        self.assertEqual(s.getvalue(), b'abcd')

class Test_ISO2022(unittest.TestCase):
    def test_g2(self):
        iso2022jp2 = '\x1b(B:hu4:unit\x1b.A\x1bNi de famille'
        uni = ':hu4:unit\xe9 de famille'
        self.assertEqual(iso2022jp2.decode('iso2022-jp-2'), uni)

    def test_iso2022_jp_g0(self):
        self.failIf(b'\x0e' in '\N{SOFT HYPHEN}'.encode('iso-2022-jp-2'))
        for encoding in ('iso-2022-jp-2004', 'iso-2022-jp-3'):
            e = '\u3406'.encode(encoding)
            self.failIf(any(x > 0x80 for x in e))

    def test_bug1572832(self):
        if sys.maxunicode >= 0x10000:
            myunichr = chr
        else:
            myunichr = lambda x: chr(0xD7C0+(x>>10)) + chr(0xDC00+(x&0x3FF))

        for x in range(0x10000, 0x110000):
            # Any ISO 2022 codec will cause the segfault
            myunichr(x).encode('iso_2022_jp', 'ignore')

def test_main():
    test_support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
