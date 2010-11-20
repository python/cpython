import unittest
from test import support
import base64
import binascii
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
        self.assertRaises(TypeError, base64.decodebytes, "")

    def test_encode(self):
        eq = self.assertEqual
        from io import BytesIO
        infp = BytesIO(b'abcdefghijklmnopqrstuvwxyz'
                       b'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
                       b'0123456789!@#0^&*();:<>,. []{}')
        outfp = BytesIO()
        base64.encode(infp, outfp)
        eq(outfp.getvalue(),
           b'YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE'
           b'RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT'
           b'Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n')

    def test_decode(self):
        from io import BytesIO
        infp = BytesIO(b'd3d3LnB5dGhvbi5vcmc=')
        outfp = BytesIO()
        base64.decode(infp, outfp)
        self.assertEqual(outfp.getvalue(), b'www.python.org')


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
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.standard_b64encode, "")
        self.assertRaises(TypeError, base64.standard_b64encode, b"", altchars="")
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64encode(b'\xd3V\xbeo\xf7\x1d'), b'01a-b_cd')
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.urlsafe_b64encode, "")

    def test_b64decode(self):
        eq = self.assertEqual
        eq(base64.b64decode(b"d3d3LnB5dGhvbi5vcmc="), b"www.python.org")
        eq(base64.b64decode(b'AA=='), b'\x00')
        eq(base64.b64decode(b"YQ=="), b"a")
        eq(base64.b64decode(b"YWI="), b"ab")
        eq(base64.b64decode(b"YWJj"), b"abc")
        eq(base64.b64decode(b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                            b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                            b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ=="),
           b"abcdefghijklmnopqrstuvwxyz"
           b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           b"0123456789!@#0^&*();:<>,. []{}")
        eq(base64.b64decode(b''), b'')
        # Test with arbitrary alternative characters
        eq(base64.b64decode(b'01a*b$cd', altchars=b'*$'), b'\xd3V\xbeo\xf7\x1d')
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.b64decode, "")
        self.assertRaises(TypeError, base64.b64decode, b"", altchars="")
        # Test standard alphabet
        eq(base64.standard_b64decode(b"d3d3LnB5dGhvbi5vcmc="), b"www.python.org")
        eq(base64.standard_b64decode(b"YQ=="), b"a")
        eq(base64.standard_b64decode(b"YWI="), b"ab")
        eq(base64.standard_b64decode(b"YWJj"), b"abc")
        eq(base64.standard_b64decode(b""), b"")
        eq(base64.standard_b64decode(b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                                     b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
                                     b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ=="),
           b"abcdefghijklmnopqrstuvwxyz"
           b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           b"0123456789!@#0^&*();:<>,. []{}")
        # Check if passing a str object raises an error
        self.assertRaises(TypeError, base64.standard_b64decode, "")
        self.assertRaises(TypeError, base64.standard_b64decode, b"", altchars="")
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64decode(b'01a-b_cd'), b'\xd3V\xbeo\xf7\x1d')
        self.assertRaises(TypeError, base64.urlsafe_b64decode, "")

    def test_b64decode_padding_error(self):
        self.assertRaises(binascii.Error, base64.b64decode, b'abc')

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
            with self.assertRaises(binascii.Error):
                base64.b64decode(bstr, validate=True)

    def test_b32encode(self):
        eq = self.assertEqual
        eq(base64.b32encode(b''), b'')
        eq(base64.b32encode(b'\x00'), b'AA======')
        eq(base64.b32encode(b'a'), b'ME======')
        eq(base64.b32encode(b'ab'), b'MFRA====')
        eq(base64.b32encode(b'abc'), b'MFRGG===')
        eq(base64.b32encode(b'abcd'), b'MFRGGZA=')
        eq(base64.b32encode(b'abcde'), b'MFRGGZDF')
        self.assertRaises(TypeError, base64.b32encode, "")

    def test_b32decode(self):
        eq = self.assertEqual
        eq(base64.b32decode(b''), b'')
        eq(base64.b32decode(b'AA======'), b'\x00')
        eq(base64.b32decode(b'ME======'), b'a')
        eq(base64.b32decode(b'MFRA===='), b'ab')
        eq(base64.b32decode(b'MFRGG==='), b'abc')
        eq(base64.b32decode(b'MFRGGZA='), b'abcd')
        eq(base64.b32decode(b'MFRGGZDF'), b'abcde')
        self.assertRaises(TypeError, base64.b32decode, "")

    def test_b32decode_casefold(self):
        eq = self.assertEqual
        eq(base64.b32decode(b'', True), b'')
        eq(base64.b32decode(b'ME======', True), b'a')
        eq(base64.b32decode(b'MFRA====', True), b'ab')
        eq(base64.b32decode(b'MFRGG===', True), b'abc')
        eq(base64.b32decode(b'MFRGGZA=', True), b'abcd')
        eq(base64.b32decode(b'MFRGGZDF', True), b'abcde')
        # Lower cases
        eq(base64.b32decode(b'me======', True), b'a')
        eq(base64.b32decode(b'mfra====', True), b'ab')
        eq(base64.b32decode(b'mfrgg===', True), b'abc')
        eq(base64.b32decode(b'mfrggza=', True), b'abcd')
        eq(base64.b32decode(b'mfrggzdf', True), b'abcde')
        # Expected exceptions
        self.assertRaises(TypeError, base64.b32decode, b'me======')
        # Mapping zero and one
        eq(base64.b32decode(b'MLO23456'), b'b\xdd\xad\xf3\xbe')
        eq(base64.b32decode(b'M1023456', map01=b'L'), b'b\xdd\xad\xf3\xbe')
        eq(base64.b32decode(b'M1023456', map01=b'I'), b'b\x1d\xad\xf3\xbe')
        self.assertRaises(TypeError, base64.b32decode, b"", map01="")

    def test_b32decode_error(self):
        self.assertRaises(binascii.Error, base64.b32decode, b'abc')
        self.assertRaises(binascii.Error, base64.b32decode, b'ABCDEF==')

    def test_b16encode(self):
        eq = self.assertEqual
        eq(base64.b16encode(b'\x01\x02\xab\xcd\xef'), b'0102ABCDEF')
        eq(base64.b16encode(b'\x00'), b'00')
        self.assertRaises(TypeError, base64.b16encode, "")

    def test_b16decode(self):
        eq = self.assertEqual
        eq(base64.b16decode(b'0102ABCDEF'), b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode(b'00'), b'\x00')
        # Lower case is not allowed without a flag
        self.assertRaises(binascii.Error, base64.b16decode, b'0102abcdef')
        # Case fold
        eq(base64.b16decode(b'0102abcdef', True), b'\x01\x02\xab\xcd\xef')
        self.assertRaises(TypeError, base64.b16decode, "")

    def test_ErrorHeritage(self):
        self.assertTrue(issubclass(binascii.Error, ValueError))



class TestMain(unittest.TestCase):
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
