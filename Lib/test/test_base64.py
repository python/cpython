import unittest
from test import support
import base64
import binascii
import os
import sys
import subprocess
import struct
from array import array


class LegacyBase64TestCase(unittest.TestCase):

    # Legacy API is not as permissive as the modern API
    def check_type_errors(self, f):
        self.assertRaises(TypeError, f, "")
        self.assertRaises(TypeError, f, [])
        multidimensional = memoryview(b"1234").cast('B', (2, 2))
        self.assertRaises(TypeError, f, multidimensional)
        int_data = memoryview(b"1234").cast('I')
        self.assertRaises(TypeError, f, int_data)

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
        eq(base64.encodebytes(memoryview(b'abc')), b'YWJj\n')
        eq(base64.encodebytes(array('B', b'abc')), b'YWJj\n')
        self.check_type_errors(base64.encodebytes)

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
        eq(base64.decodebytes(memoryview(b'YWJj\n')), b'abc')
        eq(base64.decodebytes(array('B', b'YWJj\n')), b'abc')
        self.check_type_errors(base64.decodebytes)

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

    # Modern API completely ignores exported dimension and format data and
    # treats any buffer as a stream of bytes
    def check_encode_type_errors(self, f):
        self.assertRaises(TypeError, f, "")
        self.assertRaises(TypeError, f, [])

    def check_decode_type_errors(self, f):
        self.assertRaises(TypeError, f, [])

    def check_other_types(self, f, bytes_data, expected):
        eq = self.assertEqual
        b = bytearray(bytes_data)
        eq(f(b), expected)
        # The bytearray wasn't mutated
        eq(b, bytes_data)
        eq(f(memoryview(bytes_data)), expected)
        eq(f(array('B', bytes_data)), expected)
        # XXX why is b64encode hardcoded here?
        self.check_nonbyte_element_format(base64.b64encode, bytes_data)
        self.check_multidimensional(base64.b64encode, bytes_data)

    def check_multidimensional(self, f, data):
        padding = b"\x00" if len(data) % 2 else b""
        bytes_data = data + padding # Make sure cast works
        shape = (len(bytes_data) // 2, 2)
        multidimensional = memoryview(bytes_data).cast('B', shape)
        self.assertEqual(f(multidimensional), f(bytes_data))

    def check_nonbyte_element_format(self, f, data):
        padding = b"\x00" * ((4 - len(data)) % 4)
        bytes_data = data + padding # Make sure cast works
        int_data = memoryview(bytes_data).cast('I')
        self.assertEqual(f(int_data), f(bytes_data))


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
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars=bytearray(b'*$')),
           b'01a*b$cd')
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars=memoryview(b'*$')),
           b'01a*b$cd')
        eq(base64.b64encode(b'\xd3V\xbeo\xf7\x1d', altchars=array('B', b'*$')),
           b'01a*b$cd')
        # Non-bytes
        self.check_other_types(base64.b64encode, b'abcd', b'YWJjZA==')
        self.check_encode_type_errors(base64.b64encode)
        self.assertRaises(TypeError, base64.b64encode, b"", altchars="*$")
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
        self.check_other_types(base64.standard_b64encode,
                               b'abcd', b'YWJjZA==')
        self.check_encode_type_errors(base64.standard_b64encode)
        # Test with 'URL safe' alternative characters
        eq(base64.urlsafe_b64encode(b'\xd3V\xbeo\xf7\x1d'), b'01a-b_cd')
        # Non-bytes
        self.check_other_types(base64.urlsafe_b64encode,
                               b'\xd3V\xbeo\xf7\x1d', b'01a-b_cd')
        self.check_encode_type_errors(base64.urlsafe_b64encode)

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
        self.check_other_types(base64.b64decode, b"YWJj", b"abc")
        self.check_decode_type_errors(base64.b64decode)

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
        self.check_other_types(base64.standard_b64decode, b"YWJj", b"abc")
        self.check_decode_type_errors(base64.standard_b64decode)

        # Test with 'URL safe' alternative characters
        tests_urlsafe = {b'01a-b_cd': b'\xd3V\xbeo\xf7\x1d',
                         b'': b'',
                         }
        for data, res in tests_urlsafe.items():
            eq(base64.urlsafe_b64decode(data), res)
            eq(base64.urlsafe_b64decode(data.decode('ascii')), res)
        # Non-bytes
        self.check_other_types(base64.urlsafe_b64decode, b'01a-b_cd',
                               b'\xd3V\xbeo\xf7\x1d')
        self.check_decode_type_errors(base64.urlsafe_b64decode)

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
        self.check_other_types(base64.b32encode, b'abcd', b'MFRGGZA=')
        self.check_encode_type_errors(base64.b32encode)

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
        self.check_other_types(base64.b32decode, b'MFRGG===', b"abc")
        self.check_decode_type_errors(base64.b32decode)

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

        self.assertRaises(binascii.Error, base64.b32decode, b'me======')
        self.assertRaises(binascii.Error, base64.b32decode, 'me======')

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
            self.assertRaises(binascii.Error, base64.b32decode, data)
            self.assertRaises(binascii.Error, base64.b32decode, data_str)

    def test_b32decode_error(self):
        for data in [b'abc', b'ABCDEF==', b'==ABCDEF']:
            with self.assertRaises(binascii.Error):
                base64.b32decode(data)
            with self.assertRaises(binascii.Error):
                base64.b32decode(data.decode('ascii'))

    def test_b16encode(self):
        eq = self.assertEqual
        eq(base64.b16encode(b'\x01\x02\xab\xcd\xef'), b'0102ABCDEF')
        eq(base64.b16encode(b'\x00'), b'00')
        # Non-bytes
        self.check_other_types(base64.b16encode, b'\x01\x02\xab\xcd\xef',
                               b'0102ABCDEF')
        self.check_encode_type_errors(base64.b16encode)

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
        self.check_other_types(base64.b16decode, b"0102ABCDEF",
                               b'\x01\x02\xab\xcd\xef')
        self.check_decode_type_errors(base64.b16decode)
        eq(base64.b16decode(bytearray(b"0102abcdef"), True),
           b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode(memoryview(b"0102abcdef"), True),
           b'\x01\x02\xab\xcd\xef')
        eq(base64.b16decode(array('B', b"0102abcdef"), True),
           b'\x01\x02\xab\xcd\xef')

    def test_a85encode(self):
        eq = self.assertEqual

        tests = {
            b'': b'',
            b"www.python.org": b'GB\\6`E-ZP=Df.1GEb>',
            bytes(range(255)): b"""!!*-'"9eu7#RLhG$k3[W&.oNg'GVB"(`=52*$$"""
               b"""(B+<_pR,UFcb-n-Vr/1iJ-0JP==1c70M3&s#]4?Ykm5X@_(6q'R884cE"""
               b"""H9MJ8X:f1+h<)lt#=BSg3>[:ZC?t!MSA7]@cBPD3sCi+'.E,fo>FEMbN"""
               b"""G^4U^I!pHnJ:W<)KS>/9Ll%"IN/`jYOHG]iPa.Q$R$jD4S=Q7DTV8*TU"""
               b"""nsrdW2ZetXKAY/Yd(L?['d?O\\@K2_]Y2%o^qmn*`5Ta:aN;TJbg"GZd"""
               b"""*^:jeCE.%f\\,!5gtgiEi8N\\UjQ5OekiqBum-X60nF?)@o_%qPq"ad`"""
               b"""r;HT""",
            b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                b"0123456789!@#0^&*();:<>,. []{}":
                b'@:E_WAS,RgBkhF"D/O92EH6,BF`qtRH$VbC6UX@47n?3D92&&T'
                b":Jand;cHat='/U/0JP==1c70M3&r-I,;<FN.OZ`-3]oSW/g+A(H[P",
            b"no padding..": b'DJpY:@:Wn_DJ(RS',
            b"zero compression\0\0\0\0": b'H=_,8+Cf>,E,oN2F(oQ1z',
            b"zero compression\0\0\0": b'H=_,8+Cf>,E,oN2F(oQ1!!!!',
            b"Boundary:\0\0\0\0": b'6>q!aA79M(3WK-[!!',
            b"Space compr:    ": b';fH/TAKYK$D/aMV+<VdL',
            b'\xff': b'rr',
            b'\xff'*2: b's8N',
            b'\xff'*3: b's8W*',
            b'\xff'*4: b's8W-!',
            }

        for data, res in tests.items():
            eq(base64.a85encode(data), res, data)
            eq(base64.a85encode(data, adobe=False), res, data)
            eq(base64.a85encode(data, adobe=True), b'<~' + res + b'~>', data)

        self.check_other_types(base64.a85encode, b"www.python.org",
                               b'GB\\6`E-ZP=Df.1GEb>')

        self.assertRaises(TypeError, base64.a85encode, "")

        eq(base64.a85encode(b"www.python.org", wrapcol=7, adobe=False),
           b'GB\\6`E-\nZP=Df.1\nGEb>')
        eq(base64.a85encode(b"\0\0\0\0www.python.org", wrapcol=7, adobe=False),
           b'zGB\\6`E\n-ZP=Df.\n1GEb>')
        eq(base64.a85encode(b"www.python.org", wrapcol=7, adobe=True),
           b'<~GB\\6`\nE-ZP=Df\n.1GEb>\n~>')

        eq(base64.a85encode(b' '*8, foldspaces=True, adobe=False), b'yy')
        eq(base64.a85encode(b' '*7, foldspaces=True, adobe=False), b'y+<Vd')
        eq(base64.a85encode(b' '*6, foldspaces=True, adobe=False), b'y+<U')
        eq(base64.a85encode(b' '*5, foldspaces=True, adobe=False), b'y+9')

    def test_b85encode(self):
        eq = self.assertEqual

        tests = {
            b'': b'',
            b'www.python.org': b'cXxL#aCvlSZ*DGca%T',
            bytes(range(255)): b"""009C61O)~M2nh-c3=Iws5D^j+6crX17#SKH9337X"""
                b"""AR!_nBqb&%C@Cr{EG;fCFflSSG&MFiI5|2yJUu=?KtV!7L`6nNNJ&ad"""
                b"""OifNtP*GA-R8>}2SXo+ITwPvYU}0ioWMyV&XlZI|Y;A6DaB*^Tbai%j"""
                b"""czJqze0_d@fPsR8goTEOh>41ejE#<ukdcy;l$Dm3n3<ZJoSmMZprN9p"""
                b"""q@|{(sHv)}tgWuEu(7hUw6(UkxVgH!yuH4^z`?@9#Kp$P$jQpf%+1cv"""
                b"""(9zP<)YaD4*xB0K+}+;a;Njxq<mKk)=;`X~?CtLF@bU8V^!4`l`1$(#"""
                b"""{Qdp""",
            b"""abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"""
                b"""0123456789!@#0^&*();:<>,. []{}""":
                b"""VPa!sWoBn+X=-b1ZEkOHadLBXb#`}nd3r%YLqtVJM@UIZOH55pPf$@("""
                b"""Q&d$}S6EqEFflSSG&MFiI5{CeBQRbjDkv#CIy^osE+AW7dwl""",
            b'no padding..': b'Zf_uPVPs@!Zf7no',
            b'zero compression\x00\x00\x00\x00': b'dS!BNAY*TBaB^jHb7^mG00000',
            b'zero compression\x00\x00\x00': b'dS!BNAY*TBaB^jHb7^mG0000',
            b"""Boundary:\x00\x00\x00\x00""": b"""LT`0$WMOi7IsgCw00""",
            b'Space compr:    ': b'Q*dEpWgug3ZE$irARr(h',
            b'\xff': b'{{',
            b'\xff'*2: b'|Nj',
            b'\xff'*3: b'|Ns9',
            b'\xff'*4: b'|NsC0',
        }

        for data, res in tests.items():
            eq(base64.b85encode(data), res)

        self.check_other_types(base64.b85encode, b"www.python.org",
                               b'cXxL#aCvlSZ*DGca%T')

    def test_a85decode(self):
        eq = self.assertEqual

        tests = {
            b'': b'',
            b'GB\\6`E-ZP=Df.1GEb>': b'www.python.org',
            b"""! ! * -'"\n\t\t9eu\r\n7#  RL\vhG$k3[W&.oNg'GVB"(`=52*$$"""
               b"""(B+<_pR,UFcb-n-Vr/1iJ-0JP==1c70M3&s#]4?Ykm5X@_(6q'R884cE"""
               b"""H9MJ8X:f1+h<)lt#=BSg3>[:ZC?t!MSA7]@cBPD3sCi+'.E,fo>FEMbN"""
               b"""G^4U^I!pHnJ:W<)KS>/9Ll%"IN/`jYOHG]iPa.Q$R$jD4S=Q7DTV8*TU"""
               b"""nsrdW2ZetXKAY/Yd(L?['d?O\\@K2_]Y2%o^qmn*`5Ta:aN;TJbg"GZd"""
               b"""*^:jeCE.%f\\,!5gtgiEi8N\\UjQ5OekiqBum-X60nF?)@o_%qPq"ad`"""
               b"""r;HT""": bytes(range(255)),
            b"""@:E_WAS,RgBkhF"D/O92EH6,BF`qtRH$VbC6UX@47n?3D92&&T:Jand;c"""
                b"""Hat='/U/0JP==1c70M3&r-I,;<FN.OZ`-3]oSW/g+A(H[P""":
                b'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234'
                b'56789!@#0^&*();:<>,. []{}',
            b'DJpY:@:Wn_DJ(RS': b'no padding..',
            b'H=_,8+Cf>,E,oN2F(oQ1z': b'zero compression\x00\x00\x00\x00',
            b'H=_,8+Cf>,E,oN2F(oQ1!!!!': b'zero compression\x00\x00\x00',
            b'6>q!aA79M(3WK-[!!': b"Boundary:\x00\x00\x00\x00",
            b';fH/TAKYK$D/aMV+<VdL': b'Space compr:    ',
            b'rr': b'\xff',
            b's8N': b'\xff'*2,
            b's8W*': b'\xff'*3,
            b's8W-!': b'\xff'*4,
            }

        for data, res in tests.items():
            eq(base64.a85decode(data), res, data)
            eq(base64.a85decode(data, adobe=False), res, data)
            eq(base64.a85decode(data.decode("ascii"), adobe=False), res, data)
            eq(base64.a85decode(b'<~' + data + b'~>', adobe=True), res, data)
            eq(base64.a85decode('<~%s~>' % data.decode("ascii"), adobe=True),
               res, data)

        eq(base64.a85decode(b'yy', foldspaces=True, adobe=False), b' '*8)
        eq(base64.a85decode(b'y+<Vd', foldspaces=True, adobe=False), b' '*7)
        eq(base64.a85decode(b'y+<U', foldspaces=True, adobe=False), b' '*6)
        eq(base64.a85decode(b'y+9', foldspaces=True, adobe=False), b' '*5)

        self.check_other_types(base64.a85decode, b'GB\\6`E-ZP=Df.1GEb>',
                               b"www.python.org")

    def test_b85decode(self):
        eq = self.assertEqual

        tests = {
            b'': b'',
            b'cXxL#aCvlSZ*DGca%T': b'www.python.org',
            b"""009C61O)~M2nh-c3=Iws5D^j+6crX17#SKH9337X"""
                b"""AR!_nBqb&%C@Cr{EG;fCFflSSG&MFiI5|2yJUu=?KtV!7L`6nNNJ&ad"""
                b"""OifNtP*GA-R8>}2SXo+ITwPvYU}0ioWMyV&XlZI|Y;A6DaB*^Tbai%j"""
                b"""czJqze0_d@fPsR8goTEOh>41ejE#<ukdcy;l$Dm3n3<ZJoSmMZprN9p"""
                b"""q@|{(sHv)}tgWuEu(7hUw6(UkxVgH!yuH4^z`?@9#Kp$P$jQpf%+1cv"""
                b"""(9zP<)YaD4*xB0K+}+;a;Njxq<mKk)=;`X~?CtLF@bU8V^!4`l`1$(#"""
                b"""{Qdp""": bytes(range(255)),
            b"""VPa!sWoBn+X=-b1ZEkOHadLBXb#`}nd3r%YLqtVJM@UIZOH55pPf$@("""
                b"""Q&d$}S6EqEFflSSG&MFiI5{CeBQRbjDkv#CIy^osE+AW7dwl""":
                b"""abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"""
                b"""0123456789!@#0^&*();:<>,. []{}""",
            b'Zf_uPVPs@!Zf7no': b'no padding..',
            b'dS!BNAY*TBaB^jHb7^mG00000': b'zero compression\x00\x00\x00\x00',
            b'dS!BNAY*TBaB^jHb7^mG0000': b'zero compression\x00\x00\x00',
            b"""LT`0$WMOi7IsgCw00""": b"""Boundary:\x00\x00\x00\x00""",
            b'Q*dEpWgug3ZE$irARr(h': b'Space compr:    ',
            b'{{': b'\xff',
            b'|Nj': b'\xff'*2,
            b'|Ns9': b'\xff'*3,
            b'|NsC0': b'\xff'*4,
        }

        for data, res in tests.items():
            eq(base64.b85decode(data), res)
            eq(base64.b85decode(data.decode("ascii")), res)

        self.check_other_types(base64.b85decode, b'cXxL#aCvlSZ*DGca%T',
                               b"www.python.org")

    def test_a85_padding(self):
        eq = self.assertEqual

        eq(base64.a85encode(b"x", pad=True), b'GQ7^D')
        eq(base64.a85encode(b"xx", pad=True), b"G^'2g")
        eq(base64.a85encode(b"xxx", pad=True), b'G^+H5')
        eq(base64.a85encode(b"xxxx", pad=True), b'G^+IX')
        eq(base64.a85encode(b"xxxxx", pad=True), b'G^+IXGQ7^D')

        eq(base64.a85decode(b'GQ7^D'), b"x\x00\x00\x00")
        eq(base64.a85decode(b"G^'2g"), b"xx\x00\x00")
        eq(base64.a85decode(b'G^+H5'), b"xxx\x00")
        eq(base64.a85decode(b'G^+IX'), b"xxxx")
        eq(base64.a85decode(b'G^+IXGQ7^D'), b"xxxxx\x00\x00\x00")

    def test_b85_padding(self):
        eq = self.assertEqual

        eq(base64.b85encode(b"x", pad=True), b'cmMzZ')
        eq(base64.b85encode(b"xx", pad=True), b'cz6H+')
        eq(base64.b85encode(b"xxx", pad=True), b'czAdK')
        eq(base64.b85encode(b"xxxx", pad=True), b'czAet')
        eq(base64.b85encode(b"xxxxx", pad=True), b'czAetcmMzZ')

        eq(base64.b85decode(b'cmMzZ'), b"x\x00\x00\x00")
        eq(base64.b85decode(b'cz6H+'), b"xx\x00\x00")
        eq(base64.b85decode(b'czAdK'), b"xxx\x00")
        eq(base64.b85decode(b'czAet'), b"xxxx")
        eq(base64.b85decode(b'czAetcmMzZ'), b"xxxxx\x00\x00\x00")

    def test_a85decode_errors(self):
        illegal = (set(range(32)) | set(range(118, 256))) - set(b' \t\n\r\v')
        for c in illegal:
            with self.assertRaises(ValueError, msg=bytes([c])):
                base64.a85decode(b'!!!!' + bytes([c]))
            with self.assertRaises(ValueError, msg=bytes([c])):
                base64.a85decode(b'!!!!' + bytes([c]), adobe=False)
            with self.assertRaises(ValueError, msg=bytes([c])):
                base64.a85decode(b'<~!!!!' + bytes([c]) + b'~>', adobe=True)

        self.assertRaises(ValueError, base64.a85decode,
                                      b"malformed", adobe=True)
        self.assertRaises(ValueError, base64.a85decode,
                                      b"<~still malformed", adobe=True)
        self.assertRaises(ValueError, base64.a85decode,
                                      b"also malformed~>", adobe=True)

        # With adobe=False (the default), Adobe framing markers are disallowed
        self.assertRaises(ValueError, base64.a85decode,
                                      b"<~~>")
        self.assertRaises(ValueError, base64.a85decode,
                                      b"<~~>", adobe=False)
        base64.a85decode(b"<~~>", adobe=True)  # sanity check

        self.assertRaises(ValueError, base64.a85decode,
                                      b"abcx", adobe=False)
        self.assertRaises(ValueError, base64.a85decode,
                                      b"abcdey", adobe=False)
        self.assertRaises(ValueError, base64.a85decode,
                                      b"a b\nc", adobe=False, ignorechars=b"")

        self.assertRaises(ValueError, base64.a85decode, b's', adobe=False)
        self.assertRaises(ValueError, base64.a85decode, b's8', adobe=False)
        self.assertRaises(ValueError, base64.a85decode, b's8W', adobe=False)
        self.assertRaises(ValueError, base64.a85decode, b's8W-', adobe=False)
        self.assertRaises(ValueError, base64.a85decode, b's8W-"', adobe=False)

    def test_b85decode_errors(self):
        illegal = list(range(33)) + \
                  list(b'"\',./:[\\]') + \
                  list(range(128, 256))
        for c in illegal:
            with self.assertRaises(ValueError, msg=bytes([c])):
                base64.b85decode(b'0000' + bytes([c]))

        self.assertRaises(ValueError, base64.b85decode, b'|')
        self.assertRaises(ValueError, base64.b85decode, b'|N')
        self.assertRaises(ValueError, base64.b85decode, b'|Ns')
        self.assertRaises(ValueError, base64.b85decode, b'|NsC')
        self.assertRaises(ValueError, base64.b85decode, b'|NsC1')

    def test_decode_nonascii_str(self):
        decode_funcs = (base64.b64decode,
                        base64.standard_b64decode,
                        base64.urlsafe_b64decode,
                        base64.b32decode,
                        base64.b16decode,
                        base64.b85decode,
                        base64.a85decode)
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
