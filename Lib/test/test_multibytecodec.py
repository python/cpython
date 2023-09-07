#
# test_multibytecodec.py
#   Unit test for multibytecodec itself
#

import _multibytecodec
import codecs
import io
import sys
import textwrap
import unittest
from test import support
from test.support import os_helper
from test.support.os_helper import TESTFN

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
        myreplace  = lambda exc: ('', sys.maxsize+1)
        codecs.register_error('test.cjktest', myreplace)
        self.assertRaises(IndexError, dec,
                          b'apple\x92ham\x93spam', 'test.cjktest')

    def test_errorcallback_custom_ignore(self):
        # Issue #23215: MemoryError with custom error handlers and multibyte codecs
        data = 100 * "\udc00"
        codecs.register_error("test.ignore", codecs.ignore_errors)
        for enc in ALL_CJKENCODINGS:
            self.assertEqual(data.encode(enc, "test.ignore"), b'')

    def test_codingspec(self):
        try:
            for enc in ALL_CJKENCODINGS:
                code = '# coding: {}\n'.format(enc)
                exec(code)
        finally:
            os_helper.unlink(TESTFN)

    def test_init_segfault(self):
        # bug #3305: this used to segfault
        self.assertRaises(AttributeError,
                          _multibytecodec.MultibyteStreamReader, None)
        self.assertRaises(AttributeError,
                          _multibytecodec.MultibyteStreamWriter, None)

    def test_decode_unicode(self):
        # Trying to decode a unicode string should raise a TypeError
        for enc in ALL_CJKENCODINGS:
            self.assertRaises(TypeError, codecs.getdecoder(enc), "")

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
        # jisx0213 encoder is stateful for a few code points. eg)
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

    def test_state_methods_with_buffer_state(self):
        # euc_jis_2004 stores state as a buffer of pending bytes
        encoder = codecs.getincrementalencoder('euc_jis_2004')()

        initial_state = encoder.getstate()
        self.assertEqual(encoder.encode('\u00e6\u0300'), b'\xab\xc4')
        encoder.setstate(initial_state)
        self.assertEqual(encoder.encode('\u00e6\u0300'), b'\xab\xc4')

        self.assertEqual(encoder.encode('\u00e6'), b'')
        partial_state = encoder.getstate()
        self.assertEqual(encoder.encode('\u0300'), b'\xab\xc4')
        encoder.setstate(partial_state)
        self.assertEqual(encoder.encode('\u0300'), b'\xab\xc4')

    def test_state_methods_with_non_buffer_state(self):
        # iso2022_jp stores state without using a buffer
        encoder = codecs.getincrementalencoder('iso2022_jp')()

        self.assertEqual(encoder.encode('z'), b'z')
        en_state = encoder.getstate()

        self.assertEqual(encoder.encode('\u3042'), b'\x1b\x24\x42\x24\x22')
        jp_state = encoder.getstate()
        self.assertEqual(encoder.encode('z'), b'\x1b\x28\x42z')

        encoder.setstate(jp_state)
        self.assertEqual(encoder.encode('\u3042'), b'\x24\x22')

        encoder.setstate(en_state)
        self.assertEqual(encoder.encode('z'), b'z')

    def test_getstate_returns_expected_value(self):
        # Note: getstate is implemented such that these state values
        # are expected to be the same across all builds of Python,
        # regardless of x32/64 bit, endianness and compiler.

        # euc_jis_2004 stores state as a buffer of pending bytes
        buffer_state_encoder = codecs.getincrementalencoder('euc_jis_2004')()
        self.assertEqual(buffer_state_encoder.getstate(), 0)
        buffer_state_encoder.encode('\u00e6')
        self.assertEqual(buffer_state_encoder.getstate(),
                         int.from_bytes(
                             b"\x02"
                             b"\xc3\xa6"
                             b"\x00\x00\x00\x00\x00\x00\x00\x00",
                             'little'))
        buffer_state_encoder.encode('\u0300')
        self.assertEqual(buffer_state_encoder.getstate(), 0)

        # iso2022_jp stores state without using a buffer
        non_buffer_state_encoder = codecs.getincrementalencoder('iso2022_jp')()
        self.assertEqual(non_buffer_state_encoder.getstate(),
                         int.from_bytes(
                             b"\x00"
                             b"\x42\x42\x00\x00\x00\x00\x00\x00",
                             'little'))
        non_buffer_state_encoder.encode('\u3042')
        self.assertEqual(non_buffer_state_encoder.getstate(),
                         int.from_bytes(
                             b"\x00"
                             b"\xc2\x42\x00\x00\x00\x00\x00\x00",
                             'little'))

    def test_setstate_validates_input_size(self):
        encoder = codecs.getincrementalencoder('euc_jp')()
        pending_size_nine = int.from_bytes(
            b"\x09"
            b"\x00\x00\x00\x00\x00\x00\x00\x00"
            b"\x00\x00\x00\x00\x00\x00\x00\x00",
            'little')
        self.assertRaises(UnicodeError, encoder.setstate, pending_size_nine)

    def test_setstate_validates_input_bytes(self):
        encoder = codecs.getincrementalencoder('euc_jp')()
        invalid_utf8 = int.from_bytes(
            b"\x01"
            b"\xff"
            b"\x00\x00\x00\x00\x00\x00\x00\x00",
            'little')
        self.assertRaises(UnicodeDecodeError, encoder.setstate, invalid_utf8)

    def test_issue5640(self):
        encoder = codecs.getincrementalencoder('shift-jis')('backslashreplace')
        self.assertEqual(encoder.encode('\xff'), b'\\xff')
        self.assertEqual(encoder.encode('\n'), b'\n')

    @support.cpython_only
    def test_subinterp(self):
        # bpo-42846: Test a CJK codec in a subinterpreter
        import _testcapi
        encoding = 'cp932'
        text = "Python の開発は、1990 年ごろから開始されています。"
        code = textwrap.dedent("""
            import codecs
            encoding = %r
            text = %r
            encoder = codecs.getincrementalencoder(encoding)()
            text2 = encoder.encode(text).decode(encoding)
            if text2 != text:
                raise ValueError(f"encoding issue: {text2!a} != {text!a}")
        """) % (encoding, text)
        res = _testcapi.run_in_subinterp(code)
        self.assertEqual(res, 0)

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
        self.assertRaises(UnicodeDecodeError, decoder.decode, b'', True)
        self.assertEqual(decoder.decode(b'\xcc'), '\uc774')

        self.assertEqual(decoder.decode(b'\xc6\xc4\xc0'), '\ud30c')
        self.assertRaises(UnicodeDecodeError, decoder.decode,
                          b'\xcc\xbd', True)
        self.assertEqual(decoder.decode(b'\xcc'), '\uc774')

    def test_iso2022(self):
        decoder = codecs.getincrementaldecoder('iso2022-jp')()
        ESC = b'\x1b'
        self.assertEqual(decoder.decode(ESC + b'('), '')
        self.assertEqual(decoder.decode(b'B', True), '')
        self.assertEqual(decoder.decode(ESC + b'$'), '')
        self.assertEqual(decoder.decode(b'B@$'), '\u4e16')
        self.assertEqual(decoder.decode(b'@$@'), '\u4e16')
        self.assertEqual(decoder.decode(b'$', True), '\u4e16')
        self.assertEqual(decoder.reset(), None)
        self.assertEqual(decoder.decode(b'@$'), '@$')
        self.assertEqual(decoder.decode(ESC + b'$'), '')
        self.assertRaises(UnicodeDecodeError, decoder.decode, b'', True)
        self.assertEqual(decoder.decode(b'B@$'), '\u4e16')

    def test_decode_unicode(self):
        # Trying to decode a unicode string should raise a TypeError
        for enc in ALL_CJKENCODINGS:
            decoder = codecs.getincrementaldecoder(enc)()
            self.assertRaises(TypeError, decoder.decode, "")

    def test_state_methods(self):
        decoder = codecs.getincrementaldecoder('euc_jp')()

        # Decode a complete input sequence
        self.assertEqual(decoder.decode(b'\xa4\xa6'), '\u3046')
        pending1, _ = decoder.getstate()
        self.assertEqual(pending1, b'')

        # Decode first half of a partial input sequence
        self.assertEqual(decoder.decode(b'\xa4'), '')
        pending2, flags2 = decoder.getstate()
        self.assertEqual(pending2, b'\xa4')

        # Decode second half of a partial input sequence
        self.assertEqual(decoder.decode(b'\xa6'), '\u3046')
        pending3, _ = decoder.getstate()
        self.assertEqual(pending3, b'')

        # Jump back and decode second half of partial input sequence again
        decoder.setstate((pending2, flags2))
        self.assertEqual(decoder.decode(b'\xa6'), '\u3046')
        pending4, _ = decoder.getstate()
        self.assertEqual(pending4, b'')

        # Ensure state values are preserved correctly
        decoder.setstate((b'abc', 123456789))
        self.assertEqual(decoder.getstate(), (b'abc', 123456789))

    def test_setstate_validates_input(self):
        decoder = codecs.getincrementaldecoder('euc_jp')()
        self.assertRaises(TypeError, decoder.setstate, 123)
        self.assertRaises(TypeError, decoder.setstate, ("invalid", 0))
        self.assertRaises(TypeError, decoder.setstate, (b"1234", "invalid"))
        self.assertRaises(UnicodeError, decoder.setstate, (b"123456789", 0))

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
            os_helper.unlink(TESTFN)

class Test_StreamWriter(unittest.TestCase):
    def test_gb18030(self):
        s= io.BytesIO()
        c = codecs.getwriter('gb18030')(s)
        c.write('123')
        self.assertEqual(s.getvalue(), b'123')
        c.write('\U00012345')
        self.assertEqual(s.getvalue(), b'123\x907\x959')
        c.write('\uac00\u00ac')
        self.assertEqual(s.getvalue(),
                b'123\x907\x959\x827\xcf5\x810\x851')

    def test_utf_8(self):
        s= io.BytesIO()
        c = codecs.getwriter('utf-8')(s)
        c.write('123')
        self.assertEqual(s.getvalue(), b'123')
        c.write('\U00012345')
        self.assertEqual(s.getvalue(), b'123\xf0\x92\x8d\x85')
        c.write('\uac00\u00ac')
        self.assertEqual(s.getvalue(),
            b'123\xf0\x92\x8d\x85'
            b'\xea\xb0\x80\xc2\xac')

    def test_streamwriter_strwrite(self):
        s = io.BytesIO()
        wr = codecs.getwriter('gb18030')(s)
        wr.write('abcd')
        self.assertEqual(s.getvalue(), b'abcd')

class Test_ISO2022(unittest.TestCase):
    def test_g2(self):
        iso2022jp2 = b'\x1b(B:hu4:unit\x1b.A\x1bNi de famille'
        uni = ':hu4:unit\xe9 de famille'
        self.assertEqual(iso2022jp2.decode('iso2022-jp-2'), uni)

    def test_iso2022_jp_g0(self):
        self.assertNotIn(b'\x0e', '\N{SOFT HYPHEN}'.encode('iso-2022-jp-2'))
        for encoding in ('iso-2022-jp-2004', 'iso-2022-jp-3'):
            e = '\u3406'.encode(encoding)
            self.assertFalse(any(x > 0x80 for x in e))

    @support.requires_resource('cpu')
    def test_bug1572832(self):
        for x in range(0x10000, 0x110000):
            # Any ISO 2022 codec will cause the segfault
            chr(x).encode('iso_2022_jp', 'ignore')

class TestStateful(unittest.TestCase):
    text = '\u4E16\u4E16'
    encoding = 'iso-2022-jp'
    expected = b'\x1b$B@$@$'
    reset = b'\x1b(B'
    expected_reset = expected + reset

    def test_encode(self):
        self.assertEqual(self.text.encode(self.encoding), self.expected_reset)

    def test_incrementalencoder(self):
        encoder = codecs.getincrementalencoder(self.encoding)()
        output = b''.join(
            encoder.encode(char)
            for char in self.text)
        self.assertEqual(output, self.expected)
        self.assertEqual(encoder.encode('', final=True), self.reset)
        self.assertEqual(encoder.encode('', final=True), b'')

    def test_incrementalencoder_final(self):
        encoder = codecs.getincrementalencoder(self.encoding)()
        last_index = len(self.text) - 1
        output = b''.join(
            encoder.encode(char, index == last_index)
            for index, char in enumerate(self.text))
        self.assertEqual(output, self.expected_reset)
        self.assertEqual(encoder.encode('', final=True), b'')

class TestHZStateful(TestStateful):
    text = '\u804a\u804a'
    encoding = 'hz'
    expected = b'~{ADAD'
    reset = b'~}'
    expected_reset = expected + reset


if __name__ == "__main__":
    unittest.main()
