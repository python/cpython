import unittest
from test import test_support
import base64
import binascii



class LegacyBase64TestCase(unittest.TestCase):
    def test_encodestring(self):
        eq = self.assertEqual
        eq(base64.encodestring(b"www.python.org"), b"d3d3LnB5dGhvbi5vcmc=\n")
        eq(base64.encodestring(b"a"), b"YQ==\n")
        eq(base64.encodestring(b"ab"), b"YWI=\n")
        eq(base64.encodestring(b"abc"), b"YWJj\n")
        eq(base64.encodestring(b""), b"")
        eq(base64.encodestring(b"abcdefghijklmnopqrstuvwxyz"
                               b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               b"0123456789!@#0^&*();:<>,. []{}"),
           b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
           b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n")

    def test_decodestring(self):
        eq = self.assertEqual
        eq(base64.decodestring(b"d3d3LnB5dGhvbi5vcmc=\n"), b"www.python.org")
        eq(base64.decodestring(b"YQ==\n"), b"a")
        eq(base64.decodestring(b"YWI=\n"), b"ab")
        eq(base64.decodestring(b"YWJj\n"), b"abc")
        eq(base64.decodestring(b"YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                               b"RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                               b"Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n"),
           b"abcdefghijklmnopqrstuvwxyz"
           b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           b"0123456789!@#0^&*();:<>,. []{}")
        eq(base64.decodestring(b''), b'')

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
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars='*$'), b'01a*b$cd')
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
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64encode(b'\xd3V\xbeo\xf7\x1d'), b'01a-b_cd')

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
        eq(base64.b64decode(b'01a*b$cd', altchars='*$'), b'\xd3V\xbeo\xf7\x1d')
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
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64decode(b'01a-b_cd'), b'\xd3V\xbeo\xf7\x1d')

    def test_b64decode_error(self):
        self.assertRaises(binascii.Error, base64.b64decode, b'abc')

    def test_b32encode(self):
        eq = self.assertEqual
        eq(base64.b32encode(b''), b'')
        eq(base64.b32encode(b'\x00'), b'AA======')
        eq(base64.b32encode(b'a'), b'ME======')
        eq(base64.b32encode(b'ab'), b'MFRA====')
        eq(base64.b32encode(b'abc'), b'MFRGG===')
        eq(base64.b32encode(b'abcd'), b'MFRGGZA=')
        eq(base64.b32encode(b'abcde'), b'MFRGGZDF')

    def test_b32decode(self):
        eq = self.assertEqual
        eq(base64.b32decode(b''), b'')
        eq(base64.b32decode(b'AA======'), b'\x00')
        eq(base64.b32decode(b'ME======'), b'a')
        eq(base64.b32decode(b'MFRA===='), b'ab')
        eq(base64.b32decode(b'MFRGG==='), b'abc')
        eq(base64.b32decode(b'MFRGGZA='), b'abcd')
        eq(base64.b32decode(b'MFRGGZDF'), b'abcde')

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

    def test_b32decode_error(self):
        self.assertRaises(binascii.Error, base64.b32decode, b'abc')
        self.assertRaises(binascii.Error, base64.b32decode, b'ABCDEF==')

    def test_b16encode(self):
        eq = self.assertEqual
        eq(base64.b16encode(b'\x01\x02\xab\xcd\xef'), b'0102ABCDEF')
        eq(base64.b16encode(b'\x00'), b'00')

    def test_b16decode(self):
        eq = self.assertEqual
        eq(base64.b16decode(b'0102ABCDEF'), b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode(b'00'), b'\x00')
        # Lower case is not allowed without a flag
        self.assertRaises(binascii.Error, base64.b16decode, b'0102abcdef')
        # Case fold
        eq(base64.b16decode(b'0102abcdef', True), b'\x01\x02\xab\xcd\xef')

    def test_ErrorHeritage(self):
        self.assert_(issubclass(binascii.Error, ValueError))



def test_main():
    test_support.run_unittest(__name__)

if __name__ == '__main__':
    test_main()
