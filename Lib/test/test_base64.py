import unittest
from test import support
import base64
import binascii
import os
import sys
import subprocess



class LegacyBase64TestCase(unittest.TestCase):
    def test_encodebytes(self):
        eq = self.assertEqual
        eq(base64.encodebytes(b"www.python.org"), b"d3d3LnB5dGhvbi5vcmc=\n")
        eq(base64.encodebytes(b"a"), b"YQ==\n")
        eq(base64.encodebytes(b"ab"), b"YWI=\n")
        eq(base64.encodebytes(b"abc"), b"YWJj\n")
        eq(base64.encodebytes(b""), b"")
        eq(base64.encodebytes(b"abcdefghijklmnopqrstuvwxyz"
                               b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               b"0123456789!@#0^&*();:<>,. []{}"),
           b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
           b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n")
        # Non-bytes
        eq(base64.encodebytes(bytearray(b'abc')), b'YWJj\n')
        self.assertRaises(TypeError, base64.encodebytes, "")

    def test_decodebytes(self):
        eq = self.assertEqual
        eq(base64.decodebytes(b"d3d3LnB5dGhvbi5vcmc=\n"), b"www.python.org")
        eq(base64.decodebytes(b"YQ==\n"), b"a")
        eq(base64.decodebytes(b"YWI=\n"), b"ab")
        eq(base64.decodebytes(b"YWJj\n"), b"abc")
        eq(base64.decodebytes(b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                               b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                               b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n"),
           b"abcdefghijklmnopqrstuvwxyz"
           b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           b"0123456789!@#0^&*();:<>,. []{}")
        eq(base64.decodebytes(b''), b'')
        # Non-bytes
        eq(base64.decodebytes(bytearray(b'YWJj\n')), b'abc')
        self.assertRaises(TypeError, base64.decodebytes, "")

    def test_encode(self):
        eq = self.assertEqual
        from io import BytesIO, StringIO
        infp = BytesIO(b'abcdefghijklmnopqrstuvwxyz'
                       b'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
                       b'0123456789!@#0^&*();:<>,. []{}')
        outfp = BytesIO()
        base64.encode(infp, outfp)
        eq(outfp.getvalue(),
           b'YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE'
           b'RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT'
           b'Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n')
        # Non-binary files
        self.assertRaises(TypeError, base64.encode, StringIO('abc'), BytesIO())
        self.assertRaises(TypeError, base64.encode, BytesIO(b'abc'), StringIO())
        self.assertRaises(TypeError, base64.encode, StringIO('abc'), StringIO())

    def test_decode(self):
        from io import BytesIO, StringIO
        infp = BytesIO(b'd3d3LnB5dGhvbi5vcmc=')
        outfp = BytesIO()
        base64.decode(infp, outfp)
        self.assertEqual(outfp.getvalue(), b'www.python.org')
        # Non-binary files
        self.assertRaises(TypeError, base64.encode, StringIO('YWJj\n'), BytesIO())
        self.assertRaises(TypeError, base64.encode, BytesIO(b'YWJj\n'), StringIO())
        self.assertRaises(TypeError, base64.encode, StringIO('YWJj\n'), StringIO())


class BaseXYTestCase(unittest.TestCase):
    def test_b64encode(self):
        eq = self.assertEqual
        # Test default alphabet
        eq(base64.b64encode(b"www.python.org"), b"d3d3LnB5dGhvbi5vcmc=")
        eq(base64.b64encode(b'\x00'), b'AA==')
        eq(base64.b64encode(b"a"), b"YQ==")
        eq(base64.b64encode(b"ab"), b"YWI=")
        eq(base64.b64encode(b"abc"), b"YWJj")
        eq(base64.b64encode(b""), b"")
        eq(base64.b64encode(b"abcdefghijklmnopqrstuvwxyz"
                            b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            b"0123456789!@#0^&*();:<>,. []{}"),
           b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
           b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==")
        # Test with arbitrary alternative characters
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars=b'*$'), b'01a*b$cd')
        # Non-bytes
        eq(base64.b64encode(bytearray(b'abcd')), b'YWJjZA==')
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars=bytearray(b'*$')),
           b'01a*b$cd')
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.b64encode, "")
        self.assertRaises(TypeError, base64.b64encode, b"", altchars="")
        # Test standard alphabet
        eq(base64.standard_b64encode(b"www.python.org"), b"d3d3LnB5dGhvbi5vcmc=")
        eq(base64.standard_b64encode(b"a"), b"YQ==")
        eq(base64.standard_b64encode(b"ab"), b"YWI=")
        eq(base64.standard_b64encode(b"abc"), b"YWJj")
        eq(base64.standard_b64encode(b""), b"")
        eq(base64.standard_b64encode(b"abcdefghijklmnopqrstuvwxyz"
                                     b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     b"0123456789!@#0^&*();:<>,. []{}"),
           b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
           b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==")
        # Non-bytes
        eq(base64.standard_b64encode(bytearray(b'abcd')), b'YWJjZA==')
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.standard_b64encode, "")
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64encode(b'\xd3V\xbeo\xf7\x1d'), b'01a-b_cd')
        # Non-bytes
        eq(base64.urlsafe_b64encode(bytearray(b'\xd3V\xbeo\xf7\x1d')), b'01a-b_cd')
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.urlsafe_b64encode, "")

    def test_b64decode(self):
        eq = self.assertEqual

        tests = {b"d3d3LnB5dGhvbi5vcmc=": b"www.python.org",
                 b'AA==': b'\x00',
                 b"YQ==": b"a",
                 b"YWI=": b"ab",
                 b"YWJj": b"abc",
                 b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                 b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                 b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==":

                 b"abcdefghijklmnopqrstuvwxyz"
                 b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                 b"0123456789!@#0^&*();:<>,. []{}",
                 b'': b'',
                 }
        for data, res in tests.items():
            eq(base64.b64decode(data), res)
            eq(base64.b64decode(data.decode('ascii')), res)
        # Non-bytes
        eq(base64.b64decode(bytearray(b"YWJj")), b"abc")

        # Test with arbitrary alternative characters
        tests_altchars = {(b'01a*b$cd', b'*$'): b'\xd3V\xbeo\xf7\x1d',
                          }
        for (data, altchars), res in tests_altchars.items():
            data_str = data.decode('ascii')
            altchars_str = altchars.decode('ascii')

            eq(base64.b64decode(data, altchars=altchars), res)
            eq(base64.b64decode(data_str, altchars=altchars), res)
            eq(base64.b64decode(data, altchars=altchars_str), res)
            eq(base64.b64decode(data_str, altchars=altchars_str), res)

        # Test standard alphabet
        for data, res in tests.items():
            eq(base64.standard_b64decode(data), res)
            eq(base64.standard_b64decode(data.decode('ascii')), res)
        # Non-bytes
        eq(base64.standard_b64decode(bytearray(b"YWJj")), b"abc")

        # Test with 'URL safe' alternative characters
        tests_urlsafe = {b'01a-b_cd': b'\xd3V\xbeo\xf7\x1d',
                         b'': b'',
                         }
        for data, res in tests_urlsafe.items():
            eq(base64.urlsafe_b64decode(data), res)
            eq(base64.urlsafe_b64decode(data.decode('ascii')), res)
        # Non-bytes
        eq(base64.urlsafe_b64decode(bytearray(b'01a-b_cd')), b'\xd3V\xbeo\xf7\x1d')

    def test_b64decode_padding_error(self):
        self.assertRaises(binascii.Error, base64.b64decode, b'abc')
        self.assertRaises(binascii.Error, base64.b64decode, 'abc')

    def test_b64decode_invalid_chars(self):
        # issue 1466065: Test some invalid characters.
        tests = ((b'%3d==', b'\xdd'),
                 (b'$3d==', b'\xdd'),
                 (b'[==', b''),
                 (b'YW]3=', b'am'),
                 (b'3{d==', b'\xdd'),
                 (b'3d}==', b'\xdd'),
                 (b'@@', b''),
                 (b'!', b''),
                 (b'YWJj\nYWI=', b'abcab'))
        for bstr, res in tests:
            self.assertEqual(base64.b64decode(bstr), res)
            self.assertEqual(base64.b64decode(bstr.decode('ascii')), res)
            with self.assertRaises(binascii.Error):
                base64.b64decode(bstr, validate=True)
            with self.assertRaises(binascii.Error):
                base64.b64decode(bstr.decode('ascii'), validate=True)

    def test_b32encode(self):
        eq = self.assertEqual
        eq(base64.b32encode(b''), b'')
        eq(base64.b32encode(b'\x00'), b'AA======')
        eq(base64.b32encode(b'a'), b'ME======')
        eq(base64.b32encode(b'ab'), b'MFRA====')
        eq(base64.b32encode(b'abc'), b'MFRGG===')
        eq(base64.b32encode(b'abcd'), b'MFRGGZA=')
        eq(base64.b32encode(b'abcde'), b'MFRGGZDF')
        # Non-bytes
        eq(base64.b32encode(bytearray(b'abcd')), b'MFRGGZA=')
        self.assertRaises(TypeError, base64.b32encode, "")

    def test_b32decode(self):
        eq = self.assertEqual
        tests = {b'': b'',
                 b'AA======': b'\x00',
                 b'ME======': b'a',
                 b'MFRA====': b'ab',
                 b'MFRGG===': b'abc',
                 b'MFRGGZA=': b'abcd',
                 b'MFRGGZDF': b'abcde',
                 }
        for data, res in tests.items():
            eq(base64.b32decode(data), res)
            eq(base64.b32decode(data.decode('ascii')), res)
        # Non-bytes
        eq(base64.b32decode(bytearray(b'MFRGG===')), b'abc')

    def test_b32decode_casefold(self):
        eq = self.assertEqual
        tests = {b'': b'',
                 b'ME======': b'a',
                 b'MFRA====': b'ab',
                 b'MFRGG===': b'abc',
                 b'MFRGGZA=': b'abcd',
                 b'MFRGGZDF': b'abcde',
                 # Lower cases
                 b'me======': b'a',
                 b'mfra====': b'ab',
                 b'mfrgg===': b'abc',
                 b'mfrggza=': b'abcd',
                 b'mfrggzdf': b'abcde',
                 }

        for data, res in tests.items():
            eq(base64.b32decode(data, True), res)
            eq(base64.b32decode(data.decode('ascii'), True), res)

        self.assertRaises(TypeError, base64.b32decode, b'me======')
        self.assertRaises(TypeError, base64.b32decode, 'me======')

        # Mapping zero and one
        eq(base64.b32decode(b'MLO23456'), b'b\xdd\xad\xf3\xbe')
        eq(base64.b32decode('MLO23456'), b'b\xdd\xad\xf3\xbe')

        map_tests = {(b'M1023456', b'L'): b'b\xdd\xad\xf3\xbe',
                     (b'M1023456', b'I'): b'b\x1d\xad\xf3\xbe',
                     }
        for (data, map01), res in map_tests.items():
            data_str = data.decode('ascii')
            map01_str = map01.decode('ascii')

            eq(base64.b32decode(data, map01=map01), res)
            eq(base64.b32decode(data_str, map01=map01), res)
            eq(base64.b32decode(data, map01=map01_str), res)
            eq(base64.b32decode(data_str, map01=map01_str), res)

    def test_b32decode_error(self):
        for data in [b'abc', b'ABCDEF==']:
            with self.assertRaises(binascii.Error):
                base64.b32decode(data)
            with self.assertRaises(binascii.Error):
                base64.b32decode(data.decode('ascii'))

    def test_b16encode(self):
        eq = self.assertEqual
        eq(base64.b16encode(b'\x01\x02\xab\xcd\xef'), b'0102ABCDEF')
        eq(base64.b16encode(b'\x00'), b'00')
        # Non-bytes
        eq(base64.b16encode(bytearray(b'\x01\x02\xab\xcd\xef')), b'0102ABCDEF')
        self.assertRaises(TypeError, base64.b16encode, "")

    def test_b16decode(self):
        eq = self.assertEqual
        eq(base64.b16decode(b'0102ABCDEF'), b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode('0102ABCDEF'), b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode(b'00'), b'\x00')
        eq(base64.b16decode('00'), b'\x00')
        # Lower case is not allowed without a flag
        self.assertRaises(binascii.Error, base64.b16decode, b'0102abcdef')
        self.assertRaises(binascii.Error, base64.b16decode, '0102abcdef')
        # Case fold
        eq(base64.b16decode(b'0102abcdef', True), b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode('0102abcdef', True), b'\x01\x02\xab\xcd\xef')
        # Non-bytes
        eq(base64.b16decode(bytearray(b"0102ABCDEF")), b'\x01\x02\xab\xcd\xef')

    def test_decode_nonascii_str(self):
        decode_funcs = (base64.b64decode,
                        base64.standard_b64decode,
                        base64.urlsafe_b64decode,
                        base64.b32decode,
                        base64.b16decode)
        for f in decode_funcs:
            self.assertRaises(ValueError, f, 'with non-ascii \xcb')

    def test_ErrorHeritage(self):
        self.assertTrue(issubclass(binascii.Error, ValueError))



class TestMain(unittest.TestCase):
    def tearDown(self):
        if os.path.exists(support.TESTFN):
            os.unlink(support.TESTFN)

    def get_output(self, *args, **options):
        args = (sys.executable, '-m', 'base64') + args
        return subprocess.check_output(args, **options)

    def test_encode_decode(self):
        output = self.get_output('-t')
        self.assertSequenceEqual(output.splitlines(), (
            b"b'Aladdin:open sesame'",
            br"b'QWxhZGRpbjpvcGVuIHNlc2FtZQ==\n'",
            b"b'Aladdin:open sesame'",
        ))

    def test_encode_file(self):
        with open(support.TESTFN, 'wb') as fp:
            fp.write(b'a\xffb\n')

        output = self.get_output('-e', support.TESTFN)
        self.assertEqual(output.rstrip(), b'Yf9iCg==')

        with open(support.TESTFN, 'rb') as fp:
            output = self.get_output('-e', stdin=fp)
        self.assertEqual(output.rstrip(), b'Yf9iCg==')

    def test_decode(self):
        with open(support.TESTFN, 'wb') as fp:
            fp.write(b'Yf9iCg==')
        output = self.get_output('-d', support.TESTFN)
        self.assertEqual(output.rstrip(), b'a\xffb')



def test_main():
    support.run_unittest(__name__)

if __name__ == '__main__':
    test_main()
