import unittest
from test import test_support
import base64



class LegacyBase64TestCase(unittest.TestCase):
    def test_encodestring(self):
        eq = self.assertEqual
        eq(base64.encodestring("www.python.org"), "d3d3LnB5dGhvbi5vcmc=\n")
        eq(base64.encodestring("a"), "YQ==\n")
        eq(base64.encodestring("ab"), "YWI=\n")
        eq(base64.encodestring("abc"), "YWJj\n")
        eq(base64.encodestring(""), "")
        eq(base64.encodestring("abcdefghijklmnopqrstuvwxyz"
                               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                               "0123456789!@#0^&*();:<>,. []{}"),
           "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
           "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n")
        # Non-bytes
        eq(base64.encodestring(bytearray('abc')), 'YWJj\n')

    def test_decodestring(self):
        eq = self.assertEqual
        eq(base64.decodestring("d3d3LnB5dGhvbi5vcmc=\n"), "www.python.org")
        eq(base64.decodestring("YQ==\n"), "a")
        eq(base64.decodestring("YWI=\n"), "ab")
        eq(base64.decodestring("YWJj\n"), "abc")
        eq(base64.decodestring("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                               "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                               "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n"),
           "abcdefghijklmnopqrstuvwxyz"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           "0123456789!@#0^&*();:<>,. []{}")
        eq(base64.decodestring(''), '')
        # Non-bytes
        eq(base64.decodestring(bytearray("YWJj\n")), "abc")

    def test_encode(self):
        eq = self.assertEqual
        from cStringIO import StringIO
        infp = StringIO('abcdefghijklmnopqrstuvwxyz'
                        'ABCDEFGHIJKLMNOPQRSTUVWXYZ'
                        '0123456789!@#0^&*();:<>,. []{}')
        outfp = StringIO()
        base64.encode(infp, outfp)
        eq(outfp.getvalue(),
           'YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE'
           'RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT'
           'Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==\n')

    def test_decode(self):
        from cStringIO import StringIO
        infp = StringIO('d3d3LnB5dGhvbi5vcmc=')
        outfp = StringIO()
        base64.decode(infp, outfp)
        self.assertEqual(outfp.getvalue(), 'www.python.org')



class BaseXYTestCase(unittest.TestCase):
    def test_b64encode(self):
        eq = self.assertEqual
        # Test default alphabet
        eq(base64.b64encode("www.python.org"), "d3d3LnB5dGhvbi5vcmc=")
        eq(base64.b64encode('\x00'), 'AA==')
        eq(base64.b64encode("a"), "YQ==")
        eq(base64.b64encode("ab"), "YWI=")
        eq(base64.b64encode("abc"), "YWJj")
        eq(base64.b64encode(""), "")
        eq(base64.b64encode("abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "0123456789!@#0^&*();:<>,. []{}"),
           "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
           "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==")
        # Test with arbitrary alternative characters
        eq(base64.b64encode('\xd3V\xbeo\xf7\x1d', altchars='*$'), '01a*b$cd')
        # Non-bytes
        eq(base64.b64encode(bytearray('abcd')), 'YWJjZA==')
        self.assertRaises(TypeError, base64.b64encode,
                          '\xd3V\xbeo\xf7\x1d', altchars=bytearray('*$'))
        # Test standard alphabet
        eq(base64.standard_b64encode("www.python.org"), "d3d3LnB5dGhvbi5vcmc=")
        eq(base64.standard_b64encode("a"), "YQ==")
        eq(base64.standard_b64encode("ab"), "YWI=")
        eq(base64.standard_b64encode("abc"), "YWJj")
        eq(base64.standard_b64encode(""), "")
        eq(base64.standard_b64encode("abcdefghijklmnopqrstuvwxyz"
                                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                     "0123456789!@#0^&*();:<>,. []{}"),
           "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
           "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
           "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ==")
        # Non-bytes
        eq(base64.standard_b64encode(bytearray('abcd')), 'YWJjZA==')
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64encode('\xd3V\xbeo\xf7\x1d'), '01a-b_cd')
        # Non-bytes
        eq(base64.urlsafe_b64encode(bytearray('\xd3V\xbeo\xf7\x1d')), '01a-b_cd')

    def test_b64decode(self):
        eq = self.assertEqual
        eq(base64.b64decode("d3d3LnB5dGhvbi5vcmc="), "www.python.org")
        eq(base64.b64decode('AA=='), '\x00')
        eq(base64.b64decode("YQ=="), "a")
        eq(base64.b64decode("YWI="), "ab")
        eq(base64.b64decode("YWJj"), "abc")
        eq(base64.b64decode("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                            "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0\nNT"
                            "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ=="),
           "abcdefghijklmnopqrstuvwxyz"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           "0123456789!@#0^&*();:<>,. []{}")
        eq(base64.b64decode(''), '')
        # Test with arbitrary alternative characters
        eq(base64.b64decode('01a*b$cd', altchars='*$'), '\xd3V\xbeo\xf7\x1d')
        # Non-bytes
        eq(base64.b64decode(bytearray("YWJj")), "abc")
        # Test standard alphabet
        eq(base64.standard_b64decode("d3d3LnB5dGhvbi5vcmc="), "www.python.org")
        eq(base64.standard_b64decode("YQ=="), "a")
        eq(base64.standard_b64decode("YWI="), "ab")
        eq(base64.standard_b64decode("YWJj"), "abc")
        eq(base64.standard_b64decode(""), "")
        eq(base64.standard_b64decode("YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXpBQkNE"
                                     "RUZHSElKS0xNTk9QUVJTVFVWV1hZWjAxMjM0NT"
                                     "Y3ODkhQCMwXiYqKCk7Ojw+LC4gW117fQ=="),
           "abcdefghijklmnopqrstuvwxyz"
           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
           "0123456789!@#0^&*();:<>,. []{}")
        # Non-bytes
        eq(base64.standard_b64decode(bytearray("YWJj")), "abc")
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64decode('01a-b_cd'), '\xd3V\xbeo\xf7\x1d')
        # Non-bytes
        eq(base64.urlsafe_b64decode(bytearray('01a-b_cd')), '\xd3V\xbeo\xf7\x1d')

    def test_b64decode_error(self):
        self.assertRaises(TypeError, base64.b64decode, 'abc')

    def test_b32encode(self):
        eq = self.assertEqual
        eq(base64.b32encode(''), '')
        eq(base64.b32encode('\x00'), 'AA======')
        eq(base64.b32encode('a'), 'ME======')
        eq(base64.b32encode('ab'), 'MFRA====')
        eq(base64.b32encode('abc'), 'MFRGG===')
        eq(base64.b32encode('abcd'), 'MFRGGZA=')
        eq(base64.b32encode('abcde'), 'MFRGGZDF')
        # Non-bytes
        eq(base64.b32encode(bytearray('abcd')), 'MFRGGZA=')

    def test_b32decode(self):
        eq = self.assertEqual
        eq(base64.b32decode(''), '')
        eq(base64.b32decode('AA======'), '\x00')
        eq(base64.b32decode('ME======'), 'a')
        eq(base64.b32decode('MFRA===='), 'ab')
        eq(base64.b32decode('MFRGG==='), 'abc')
        eq(base64.b32decode('MFRGGZA='), 'abcd')
        eq(base64.b32decode('MFRGGZDF'), 'abcde')
        # Non-bytes
        self.assertRaises(TypeError, base64.b32decode, bytearray('MFRGG==='))

    def test_b32decode_casefold(self):
        eq = self.assertEqual
        eq(base64.b32decode('', True), '')
        eq(base64.b32decode('ME======', True), 'a')
        eq(base64.b32decode('MFRA====', True), 'ab')
        eq(base64.b32decode('MFRGG===', True), 'abc')
        eq(base64.b32decode('MFRGGZA=', True), 'abcd')
        eq(base64.b32decode('MFRGGZDF', True), 'abcde')
        # Lower cases
        eq(base64.b32decode('me======', True), 'a')
        eq(base64.b32decode('mfra====', True), 'ab')
        eq(base64.b32decode('mfrgg===', True), 'abc')
        eq(base64.b32decode('mfrggza=', True), 'abcd')
        eq(base64.b32decode('mfrggzdf', True), 'abcde')
        # Expected exceptions
        self.assertRaises(TypeError, base64.b32decode, 'me======')
        # Mapping zero and one
        eq(base64.b32decode('MLO23456'), 'b\xdd\xad\xf3\xbe')
        eq(base64.b32decode('M1023456', map01='L'), 'b\xdd\xad\xf3\xbe')
        eq(base64.b32decode('M1023456', map01='I'), 'b\x1d\xad\xf3\xbe')

    def test_b32decode_error(self):
        self.assertRaises(TypeError, base64.b32decode, 'abc')
        self.assertRaises(TypeError, base64.b32decode, 'ABCDEF==')

    def test_b16encode(self):
        eq = self.assertEqual
        eq(base64.b16encode('\x01\x02\xab\xcd\xef'), '0102ABCDEF')
        eq(base64.b16encode('\x00'), '00')
        # Non-bytes
        eq(base64.b16encode(bytearray('\x01\x02\xab\xcd\xef')), '0102ABCDEF')

    def test_b16decode(self):
        eq = self.assertEqual
        eq(base64.b16decode('0102ABCDEF'), '\x01\x02\xab\xcd\xef')
        eq(base64.b16decode('00'), '\x00')
        # Lower case is not allowed without a flag
        self.assertRaises(TypeError, base64.b16decode, '0102abcdef')
        # Case fold
        eq(base64.b16decode('0102abcdef', True), '\x01\x02\xab\xcd\xef')
        # Non-bytes
        eq(base64.b16decode(bytearray("0102ABCDEF")), '\x01\x02\xab\xcd\xef')



def test_main():
    test_support.run_unittest(__name__)

if __name__ == '__main__':
    test_main()
