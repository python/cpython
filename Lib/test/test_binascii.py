"""Test the binascii C module."""

import unittest
import binascii
import array
import re
import sys
from test.support import bigmemtest, _1G, _4G, check_impl_detail
from test.support.hypothesis_helper import hypothesis


# Note: "*_hex" functions are aliases for "(un)hexlify"
b2a_functions = ['b2a_ascii85', 'b2a_base64', 'b2a_base85', 'b2a_z85',
                 'b2a_hex', 'b2a_qp', 'b2a_uu',
                 'hexlify']
a2b_functions = ['a2b_ascii85', 'a2b_base64', 'a2b_base85', 'a2b_z85',
                 'a2b_hex', 'a2b_qp', 'a2b_uu',
                 'unhexlify']
all_functions = a2b_functions + b2a_functions + ['crc32', 'crc_hqx']


class BinASCIITest(unittest.TestCase):

    type2test = bytes
    # Create binary test data
    rawdata = b"The quick brown fox jumps over the lazy dog.\r\n"
    # Be slow so we don't depend on other modules
    rawdata += bytes(range(256))
    rawdata += b'\0'*32
    rawdata += b' '*32
    rawdata += b"\r\nHello world.\n"

    def setUp(self):
        self.data = self.type2test(self.rawdata)

    def assertConversion(self, original, converted, restored, **kwargs):
        self.assertIsInstance(original, bytes)
        self.assertIsInstance(converted, bytes)
        self.assertIsInstance(restored, bytes)
        if converted:
            self.assertLess(max(converted), 128)
        self.assertEqual(original, restored, msg=f'{self.type2test=} {kwargs=}')

    def test_exceptions(self):
        # Check module exceptions
        self.assertIsSubclass(binascii.Error, Exception)
        self.assertIsSubclass(binascii.Incomplete, Exception)

    def test_functions(self):
        # Check presence of all functions
        for name in all_functions:
            self.assertHasAttr(getattr(binascii, name), '__call__')
            self.assertRaises(TypeError, getattr(binascii, name))

    def test_returned_value(self):
        # Limit to the minimum of all limits (b2a_uu)
        MAX_ALL = 45
        raw = self.rawdata[:MAX_ALL]
        for fa, fb in zip(a2b_functions, b2a_functions):
            a2b = getattr(binascii, fa)
            b2a = getattr(binascii, fb)
            try:
                a = b2a(self.type2test(raw))
                res = a2b(self.type2test(a))
            except Exception as err:
                self.fail("{}/{} conversion raises {!r}".format(fb, fa, err))
            self.assertEqual(res, raw, "{}/{} conversion: "
                             "{!r} != {!r}".format(fb, fa, res, raw))
            self.assertConversion(raw, a, res)
        self.assertIsInstance(binascii.crc_hqx(raw, 0), int)
        self.assertIsInstance(binascii.crc32(raw), int)

    def test_base64valid(self):
        # Test base64 with valid data
        MAX_BASE64 = 57
        lines = []
        for i in range(0, len(self.rawdata), MAX_BASE64):
            b = self.type2test(self.rawdata[i:i+MAX_BASE64])
            a = binascii.b2a_base64(b)
            lines.append(a)
        res = bytes()
        for line in lines:
            a = self.type2test(line)
            b = binascii.a2b_base64(a)
            res += b
        self.assertEqual(res, self.rawdata)

    def test_base64invalid(self):
        # Test base64 with random invalid characters sprinkled throughout
        # (This requires a new version of binascii.)
        MAX_BASE64 = 57
        lines = []
        for i in range(0, len(self.data), MAX_BASE64):
            b = self.type2test(self.rawdata[i:i+MAX_BASE64])
            a = binascii.b2a_base64(b)
            lines.append(a)

        fillers = bytearray()
        valid = b"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/"
        for i in range(256):
            if i not in valid:
                fillers.append(i)
        def addnoise(line):
            noise = fillers
            ratio = len(line) // len(noise)
            res = bytearray()
            while line and noise:
                if len(line) // len(noise) > ratio:
                    c, line = line[0], line[1:]
                else:
                    c, noise = noise[0], noise[1:]
                res.append(c)
            return res + noise + line
        res = bytearray()
        for line in map(addnoise, lines):
            a = self.type2test(line)
            b = binascii.a2b_base64(a)
            res += b
        self.assertEqual(res, self.rawdata)

        # Test base64 with just invalid characters, which should return
        # empty strings. TBD: shouldn't it raise an exception instead ?
        self.assertEqual(binascii.a2b_base64(self.type2test(fillers)), b'')

    def test_base64_bad_padding(self):
        # Test malformed padding
        def _assertRegexTemplate(assert_regex, data,
                                 non_strict_mode_expected_result):
            data = self.type2test(data)
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_base64(data, strict_mode=True)
            self.assertEqual(binascii.a2b_base64(data, strict_mode=False),
                             non_strict_mode_expected_result)
            self.assertEqual(binascii.a2b_base64(data, strict_mode=True,
                                                 ignorechars=b'='),
                             non_strict_mode_expected_result)
            self.assertEqual(binascii.a2b_base64(data),
                             non_strict_mode_expected_result)

        def assertLeadingPadding(*args):
            _assertRegexTemplate(r'(?i)Leading padding', *args)

        def assertDiscontinuousPadding(*args):
            _assertRegexTemplate(r'(?i)Discontinuous padding', *args)

        def assertExcessPadding(*args):
            _assertRegexTemplate(r'(?i)Excess padding', *args)

        def assertInvalidLength(*args):
            _assertRegexTemplate(r'(?i)Invalid.+number of data characters', *args)

        assertExcessPadding(b'ab===', b'i')
        assertExcessPadding(b'ab====', b'i')
        assertExcessPadding(b'abc==', b'i\xb7')
        assertExcessPadding(b'abc===', b'i\xb7')
        assertExcessPadding(b'abc====', b'i\xb7')
        assertExcessPadding(b'abc=====', b'i\xb7')

        assertLeadingPadding(b'=', b'')
        assertLeadingPadding(b'==', b'')
        assertLeadingPadding(b'===', b'')
        assertLeadingPadding(b'====', b'')
        assertLeadingPadding(b'=====', b'')
        assertLeadingPadding(b'=abcd', b'i\xb7\x1d')
        assertLeadingPadding(b'==abcd', b'i\xb7\x1d')
        assertLeadingPadding(b'===abcd', b'i\xb7\x1d')
        assertLeadingPadding(b'====abcd', b'i\xb7\x1d')
        assertLeadingPadding(b'=====abcd', b'i\xb7\x1d')

        assertInvalidLength(b'a=b==', b'i')
        assertInvalidLength(b'a=bc=', b'i\xb7')
        assertInvalidLength(b'a=bc==', b'i\xb7')
        assertInvalidLength(b'a=bcd', b'i\xb7\x1d')
        assertInvalidLength(b'a=bcd=', b'i\xb7\x1d')

        assertDiscontinuousPadding(b'ab=c=', b'i\xb7')
        assertDiscontinuousPadding(b'ab=cd', b'i\xb7\x1d')
        assertDiscontinuousPadding(b'ab=cd==', b'i\xb7\x1d')

        assertExcessPadding(b'abcd=', b'i\xb7\x1d')
        assertExcessPadding(b'abcd==', b'i\xb7\x1d')
        assertExcessPadding(b'abcd===', b'i\xb7\x1d')
        assertExcessPadding(b'abcd====', b'i\xb7\x1d')
        assertExcessPadding(b'abcd=====', b'i\xb7\x1d')
        assertExcessPadding(b'abcd==', b'i\xb7\x1d')
        assertExcessPadding(b'abcd===', b'i\xb7\x1d')
        assertExcessPadding(b'abcd====', b'i\xb7\x1d')
        assertExcessPadding(b'abcd=====', b'i\xb7\x1d')
        assertExcessPadding(b'abcd=efgh', b'i\xb7\x1dy\xf8!')
        assertExcessPadding(b'abcd==efgh', b'i\xb7\x1dy\xf8!')
        assertExcessPadding(b'abcd===efgh', b'i\xb7\x1dy\xf8!')
        assertExcessPadding(b'abcd====efgh', b'i\xb7\x1dy\xf8!')
        assertExcessPadding(b'abcd=====efgh', b'i\xb7\x1dy\xf8!')

    def test_base64_invalidchars(self):
        # Test non-base64 data exceptions
        def assertNonBase64Data(data, expected, ignorechars):
            data = self.type2test(data)
            assert_regex = r'(?i)Only base64 data'
            self.assertEqual(binascii.a2b_base64(data), expected)
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_base64(data, strict_mode=True)
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_base64(data, ignorechars=b'')
            self.assertEqual(binascii.a2b_base64(data, ignorechars=ignorechars),
                             expected)
            self.assertEqual(binascii.a2b_base64(data, strict_mode=False, ignorechars=b''),
                             expected)

        assertNonBase64Data(b'\nab==', b'i', ignorechars=b'\n')
        assertNonBase64Data(b'ab:(){:|:&};:==', b'i', ignorechars=b':;(){}|&')
        assertNonBase64Data(b'a\nb==', b'i', ignorechars=b'\n')
        assertNonBase64Data(b'a\x00b==', b'i', ignorechars=b'\x00')
        assertNonBase64Data(b'ab:==', b'i', ignorechars=b':')
        assertNonBase64Data(b'ab=:=', b'i', ignorechars=b':')
        assertNonBase64Data(b'ab==:', b'i', ignorechars=b':')
        assertNonBase64Data(b'abc=:', b'i\xb7', ignorechars=b':')
        assertNonBase64Data(b'ab==\n', b'i', ignorechars=b'\n')
        assertNonBase64Data(b'a\nb==', b'i', ignorechars=bytearray(b'\n'))
        assertNonBase64Data(b'a\nb==', b'i', ignorechars=memoryview(b'\n'))

        # Same cell in the cache: '\r' >> 3 == '\n' >> 3.
        data = self.type2test(b'\r\n')
        with self.assertRaises(binascii.Error):
            binascii.a2b_base64(data, ignorechars=b'\r')
        self.assertEqual(binascii.a2b_base64(data, ignorechars=b'\r\n'), b'')
        # Same bit mask in the cache: '*' & 31 == '\n' & 31.
        data = self.type2test(b'*\n')
        with self.assertRaises(binascii.Error):
            binascii.a2b_base64(data, ignorechars=b'*')
        self.assertEqual(binascii.a2b_base64(data, ignorechars=b'*\n'), b'')

        data = self.type2test(b'a\nb==')
        with self.assertRaises(TypeError):
            binascii.a2b_base64(data, ignorechars='')
        with self.assertRaises(TypeError):
            binascii.a2b_base64(data, ignorechars=[])
        with self.assertRaises(TypeError):
            binascii.a2b_base64(data, ignorechars=None)

    def test_base64_excess_data(self):
        # Test excess data exceptions
        def assertExcessData(data, non_strict_expected,
                             ignore_padchar_expected=None):
            assert_regex = r'(?i)Excess data'
            data = self.type2test(data)
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_base64(data, strict_mode=True)
            self.assertEqual(binascii.a2b_base64(data, strict_mode=False),
                             non_strict_expected)
            if ignore_padchar_expected is not None:
                self.assertEqual(binascii.a2b_base64(data, strict_mode=True,
                                                     ignorechars=b'='),
                                 ignore_padchar_expected)
            self.assertEqual(binascii.a2b_base64(data), non_strict_expected)

        assertExcessData(b'ab==c', b'i')
        assertExcessData(b'ab==cd', b'i', b'i\xb7\x1d')
        assertExcessData(b'abc=d', b'i\xb7', b'i\xb7\x1d')

    def test_base64errors(self):
        # Test base64 with invalid padding
        def assertIncorrectPadding(data, strict_mode=True):
            data = self.type2test(data)
            with self.assertRaisesRegex(binascii.Error, r'(?i)Incorrect padding'):
                binascii.a2b_base64(data)
            with self.assertRaisesRegex(binascii.Error, r'(?i)Incorrect padding'):
                binascii.a2b_base64(data, strict_mode=False)
            if strict_mode:
                with self.assertRaisesRegex(binascii.Error, r'(?i)Incorrect padding'):
                    binascii.a2b_base64(data, strict_mode=True)

        assertIncorrectPadding(b'ab')
        assertIncorrectPadding(b'ab=')
        assertIncorrectPadding(b'abc')
        assertIncorrectPadding(b'abcdef')
        assertIncorrectPadding(b'abcdef=')
        assertIncorrectPadding(b'abcdefg')
        assertIncorrectPadding(b'a=b=', strict_mode=False)
        assertIncorrectPadding(b'a\nb=', strict_mode=False)

        # Test base64 with invalid number of valid characters (1 mod 4)
        def assertInvalidLength(data, strict_mode=True):
            n_data_chars = len(re.sub(br'[^A-Za-z0-9/+]', br'', data))
            data = self.type2test(data)
            expected_errmsg_re = \
                r'(?i)Invalid.+number of data characters.+' + str(n_data_chars)
            with self.assertRaisesRegex(binascii.Error, expected_errmsg_re):
                binascii.a2b_base64(data)
            with self.assertRaisesRegex(binascii.Error, expected_errmsg_re):
                binascii.a2b_base64(data, strict_mode=False)
            if strict_mode:
                with self.assertRaisesRegex(binascii.Error, expected_errmsg_re):
                    binascii.a2b_base64(data, strict_mode=True)

        assertInvalidLength(b'a')
        assertInvalidLength(b'a=')
        assertInvalidLength(b'a==')
        assertInvalidLength(b'a===')
        assertInvalidLength(b'a' * 5)
        assertInvalidLength(b'a' * (4 * 87 + 1))
        assertInvalidLength(b'A\tB\nC ??DE', # only 5 valid characters
                            strict_mode=False)

    def test_ascii85_valid(self):
        # Test Ascii85 with valid data
        ASCII85_PREFIX = b"<~"
        ASCII85_SUFFIX = b"~>"

        # Interleave blocks of 4 null bytes and 4 spaces into test data
        rawdata = bytearray()
        rawlines, i = [], 0
        for k in range(1, len(self.rawdata) + 1):
            b = b"\0\0\0\0" if k & 1 else b"    "
            b = b + self.rawdata[i:i + k]
            b = b"    " if k & 1 else b"\0\0\0\0"
            rawdata += b
            rawlines.append(b)
            i += k
            if i >= len(self.rawdata):
                break

        # Test core parameter combinations
        params = (False, False), (False, True), (True, False), (True, True)
        for foldspaces, adobe in params:
            lines = []
            for rawline in rawlines:
                b = self.type2test(rawline)
                a = binascii.b2a_ascii85(b, foldspaces=foldspaces, adobe=adobe)
                lines.append(a)
            res = bytearray()
            for line in lines:
                a = self.type2test(line)
                b = binascii.a2b_ascii85(a, foldspaces=foldspaces, adobe=adobe)
                res += b
            self.assertEqual(res, rawdata)

        # Test decoding inputs with length 1 mod 5
        params = [
            (b"a", False, False, b"", b""),
            (b"xbw", False, False, b"wx", b""),
            (b"<~c~>", False, True, b"", b""),
            (b"{d ~>", False, True, b" {", b""),
            (b"ye", True, False, b"", b"    "),
            (b"z\x01y\x00f", True, False, b"\x00\x01", b"\x00\x00\x00\x00    "),
            (b"<~FCfN8yg~>", True, True, b"", b"test    "),
            (b"FE;\x03#8zFCf\x02N8yh~>", True, True, b"\x02\x03", b"tset\x00\x00\x00\x00test    "),
        ]
        for a, foldspaces, adobe, ignorechars, b in params:
            kwargs = {"foldspaces": foldspaces, "adobe": adobe, "ignorechars": ignorechars}
            self.assertEqual(binascii.a2b_ascii85(self.type2test(a), **kwargs), b)

    def test_ascii85_invalid(self):
        # Test Ascii85 with invalid characters interleaved
        lines, i = [], 0
        for k in range(1, len(self.rawdata) + 1):
            b = self.type2test(self.rawdata[i:i + k])
            a = binascii.b2a_ascii85(b)
            lines.append(a)
            i += k
            if i >= len(self.rawdata):
                break

        fillers = bytearray()
        valid = b"!\"#$%&'()*+,-./0123456789:;<=>?@" \
                b"ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu" + b"z"
        for i in range(256):
            if i not in valid:
                fillers.append(i)
        def addnoise(line):
            res = bytearray()
            for i in range(len(line)):
                res.append(line[i])
                for j in range(i, len(fillers), len(line)):
                    res.append(fillers[j])
            return res
        res = bytearray()
        for line in map(addnoise, lines):
            a = self.type2test(line)
            b = binascii.a2b_ascii85(a, ignorechars=fillers)
            res += b
        self.assertEqual(res, self.rawdata)

        # Test Ascii85 with only invalid characters
        fillers = self.type2test(fillers)
        b = binascii.a2b_ascii85(fillers, ignorechars=fillers)
        self.assertEqual(b, b"")

    def test_ascii85_errors(self):
        def _assertRegexTemplate(assert_regex, data, **kwargs):
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_ascii85(self.type2test(data), **kwargs)

        def assertMissingDelimiter(data):
            _assertRegexTemplate(r"(?i)end with b'~>'", data, adobe=True)

        def assertOverflow(data):
            _assertRegexTemplate(r"(?i)Ascii85 overflow", data)

        def assertInvalidSpecial(data):
            _assertRegexTemplate(r"(?i)'[yz]'.+5-tuple", data, foldspaces=True)

        def assertInvalidChar(data, **kwargs):
            _assertRegexTemplate(r"(?i)Non-Ascii85 digit", data, **kwargs)

        # Test Ascii85 with missing delimiters
        assertMissingDelimiter(b"")
        assertMissingDelimiter(b"a")
        assertMissingDelimiter(b"<~")
        assertMissingDelimiter(b"<~!~")
        assertMissingDelimiter(b"<~abc>")
        assertMissingDelimiter(b"<~has delimiter but not terminal~>  !")

        # Test Ascii85 with out-of-range encoded value
        assertOverflow(b"t")
        assertOverflow(b"s9")
        assertOverflow(b"s8X")
        assertOverflow(b"s8W.")
        assertOverflow(b's8W-"')
        assertOverflow(b"s8W-!u")
        assertOverflow(b"s8W-!s8W-!zs8X")

        # Test Ascii85 with misplaced short form groups
        assertInvalidSpecial(b"ay")
        assertInvalidSpecial(b"az")
        assertInvalidSpecial(b"aby")
        assertInvalidSpecial(b"ayz")
        assertInvalidSpecial(b"abcz")
        assertInvalidSpecial(b"abcdy")
        assertInvalidSpecial(b"y!and!z!then!!y")

        # Test Ascii85 with non-ignored invalid characters
        assertInvalidChar(b"j\n")
        assertInvalidChar(b" ", ignorechars=b"")
        assertInvalidChar(b" valid\x02until\x03", ignorechars=b"\x00\x01\x02\x04")
        assertInvalidChar(b"\tFCb", ignorechars=b"\n")
        assertInvalidChar(b"xxxB\nP\thU'D v/F+", ignorechars=b" \n\tv")

    def test_ascii85_wrapcol(self):
        # Test Ascii85 splitting lines
        def assertEncode(a_expected, data, n, adobe=False):
            b = self.type2test(data)
            a = binascii.b2a_ascii85(b, adobe=adobe, wrapcol=n)
            self.assertEqual(a, a_expected)

        def assertDecode(data, b_expected, adobe=False):
            a = self.type2test(data)
            b = binascii.a2b_ascii85(a, adobe=adobe, ignorechars=b"\n")
            self.assertEqual(b, b_expected)

        tests = [
            (b"", 0, b"", b"<~~>"),
            (b"", 1, b"", b"<~\n~>"),
            (b"a", 0, b"@/", b"<~@/~>"),
            (b"a", 1, b"@\n/", b"<~\n@/\n~>"),
            (b"a", 2, b"@/", b"<~\n@/\n~>"),
            (b"a", 3, b"@/", b"<~@\n/~>"),
            (b"a", 4, b"@/", b"<~@/\n~>"),
            (b"a", 5, b"@/", b"<~@/\n~>"),
            (b"a", 6, b"@/", b"<~@/~>"),
            (b"a", 7, b"@/", b"<~@/~>"),
            (b"a", 123, b"@/", b"<~@/~>"),
            (b"this is a test", 7, b"FD,B0+D\nGm>@3BZ\n'F*%",
                                   b"<~FD,B0\n+DGm>@3\nBZ'F*%\n~>"),
            (b"a test!!!!!!!    ", 11, b"@3BZ'F*&QK+\nX&!P+WqmM+9",
                                       b"<~@3BZ'F*&Q\nK+X&!P+WqmM\n+9~>"),
            (b"\0" * 56, 7, b"zzzzzzz\nzzzzzzz", b"<~zzzzz\nzzzzzzz\nzz~>"),
        ]
        for b, n, a, a_wrap in tests:
            assertEncode(a, b, n)
            assertEncode(a_wrap, b, n, adobe=True)
            assertDecode(a, b)
            assertDecode(a_wrap, b, adobe=True)

    def test_ascii85_pad(self):
        # Test Ascii85 with encode padding
        rawdata = b"n1n3tee\n ch@rAcTer$"
        for i in range(1, len(rawdata) + 1):
            padding = -i % 4
            b = rawdata[:i]
            a_pad = binascii.b2a_ascii85(self.type2test(b), pad=True)
            b_pad = binascii.a2b_ascii85(self.type2test(a_pad))
            b_pad_expected = b + b"\0" * padding
            self.assertEqual(b_pad, b_pad_expected)

        # Test Ascii85 short form groups with encode padding
        def assertShortPad(data, expected, **kwargs):
            data = self.type2test(data)
            res = binascii.b2a_ascii85(data, **kwargs)
            self.assertEqual(res, expected)

        assertShortPad(b"\0", b"!!", pad=False)
        assertShortPad(b"\0", b"z", pad=True)
        assertShortPad(b"\0" * 2, b"z", pad=True)
        assertShortPad(b"\0" * 3, b"z", pad=True)
        assertShortPad(b"\0" * 4, b"z", pad=True)
        assertShortPad(b"\0" * 5, b"zz", pad=True)
        assertShortPad(b"\0" * 6, b"z!!!")
        assertShortPad(b" " * 7, b"y+<Vd,", foldspaces=True, pad=True)
        assertShortPad(b"\0" * 8, b"<~zz~>",
                       foldspaces=True, adobe=True, pad=True)
        assertShortPad(b"\0\0\0\0abcd    \0\0", b"<~z@:E_Wy\nz~>",
                       foldspaces=True, adobe=True, wrapcol=9, pad=True)

    def test_ascii85_ignorechars(self):
        # Test Ascii85 with ignored characters
        def assertIgnore(data, expected, ignorechars=b"", **kwargs):
            data = self.type2test(data)
            ignore = self.type2test(ignorechars)
            with self.assertRaisesRegex(binascii.Error, r"(?i)Non-Ascii85 digit"):
                binascii.a2b_ascii85(data, **kwargs)
            res = binascii.a2b_ascii85(data, ignorechars=ignorechars, **kwargs)
            self.assertEqual(res, expected)

        assertIgnore(b"\n", b"", ignorechars=b"\n")
        assertIgnore(b"<~ ~>", b"", ignorechars=b" ", adobe=True)
        assertIgnore(b"z|z", b"\0" * 8, ignorechars=b"|||")    # repeats don't matter
        assertIgnore(b"zz!!|", b"\0" * 9, ignorechars=b"|!z")  # ignore only if invalid
        assertIgnore(b"<~B P~@~>", b"hi", ignorechars=b" <~>", adobe=True)
        assertIgnore(b"zy}", b"\0\0\0\0", ignorechars=b"zy}")
        assertIgnore(b"zy}", b"\0\0\0\0    ", ignorechars=b"zy}", foldspaces=True)

    def test_base85_valid(self):
        # Test base85 with valid data
        lines, i = [], 0
        for k in range(1, len(self.rawdata) + 1):
            b = self.type2test(self.rawdata[i:i + k])
            a = binascii.b2a_base85(b)
            lines.append(a)
            i += k
            if i >= len(self.rawdata):
                break
        res = bytes()
        for line in lines:
            a = self.type2test(line)
            b = binascii.a2b_base85(a)
            res += b
        self.assertEqual(res, self.rawdata)

        # Test decoding inputs with different length
        self.assertEqual(binascii.a2b_base85(self.type2test(b'a')), b'')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'a')), b'')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'ab')), b'q')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'abc')), b'qa')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'abcd')),
                         b'qa\x9e')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'abcde')),
                         b'qa\x9e\xb6')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'abcdef')),
                         b'qa\x9e\xb6')
        self.assertEqual(binascii.a2b_base85(self.type2test(b'abcdefg')),
                         b'qa\x9e\xb6\x81')

    def test_base85_errors(self):
        def _assertRegexTemplate(assert_regex, data, **kwargs):
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_base85(self.type2test(data), **kwargs)

        def assertNonBase85Data(data):
            _assertRegexTemplate(r"(?i)bad base85 character", data)

        def assertOverflow(data):
            _assertRegexTemplate(r"(?i)base85 overflow", data)

        assertNonBase85Data(b"\xda")
        assertNonBase85Data(b"00\0\0")
        assertNonBase85Data(b"Z )*")
        assertNonBase85Data(b"bY*jNb0Hyq\n")

        # Test base85 with out-of-range encoded value
        assertOverflow(b"}")
        assertOverflow(b"|O")
        assertOverflow(b"|Nt")
        assertOverflow(b"|NsD")
        assertOverflow(b"|NsC1")
        assertOverflow(b"|NsC0~")
        assertOverflow(b"|NsC0|NsC0|NsD0")

    def test_base85_pad(self):
        # Test base85 with encode padding
        rawdata = b"n1n3Tee\n ch@rAc\te\r$"
        for i in range(1, len(rawdata) + 1):
            padding = -i % 4
            b = rawdata[:i]
            a_pad = binascii.b2a_base85(self.type2test(b), pad=True)
            b_pad = binascii.a2b_base85(self.type2test(a_pad))
            b_pad_expected = b + b"\0" * padding
            self.assertEqual(b_pad, b_pad_expected)

    def test_z85_valid(self):
        # Test Z85 with valid data
        lines, i = [], 0
        for k in range(1, len(self.rawdata) + 1):
            b = self.type2test(self.rawdata[i:i + k])
            a = binascii.b2a_z85(b)
            lines.append(a)
            i += k
            if i >= len(self.rawdata):
                break
        res = bytes()
        for line in lines:
            a = self.type2test(line)
            b = binascii.a2b_z85(a)
            res += b
        self.assertEqual(res, self.rawdata)

        # Test decoding inputs with different length
        self.assertEqual(binascii.a2b_z85(self.type2test(b'')), b'')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'a')), b'')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'ab')), b'\x1f')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'abc')),
                         b'\x1f\x85')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'abcd')),
                         b'\x1f\x85\x9a')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'abcde')),
                         b'\x1f\x85\x9a$')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'abcdef')),
                         b'\x1f\x85\x9a$')
        self.assertEqual(binascii.a2b_z85(self.type2test(b'abcdefg')),
                         b'\x1f\x85\x9a$/')

    def test_z85_errors(self):
        def _assertRegexTemplate(assert_regex, data, **kwargs):
            with self.assertRaisesRegex(binascii.Error, assert_regex):
                binascii.a2b_z85(self.type2test(data), **kwargs)

        def assertNonZ85Data(data):
            _assertRegexTemplate(r"(?i)bad z85 character", data)

        def assertOverflow(data):
            _assertRegexTemplate(r"(?i)z85 overflow", data)

        assertNonZ85Data(b"\xda")
        assertNonZ85Data(b"00\0\0")
        assertNonZ85Data(b"z !/")
        assertNonZ85Data(b"By/JnB0hYQ\n")

        # Test Z85 with out-of-range encoded value
        assertOverflow(b"%")
        assertOverflow(b"%n")
        assertOverflow(b"%nS")
        assertOverflow(b"%nSc")
        assertOverflow(b"%nSc1")
        assertOverflow(b"%nSc0$")
        assertOverflow(b"%nSc0%nSc0%nSD0")

    def test_z85_pad(self):
        # Test Z85 with encode padding
        rawdata = b"n1n3Tee\n ch@rAc\te\r$"
        for i in range(1, len(rawdata) + 1):
            padding = -i % 4
            b = rawdata[:i]
            a_pad = binascii.b2a_z85(self.type2test(b), pad=True)
            b_pad = binascii.a2b_z85(self.type2test(a_pad))
            b_pad_expected = b + b"\0" * padding
            self.assertEqual(b_pad, b_pad_expected)

    def test_uu(self):
        MAX_UU = 45
        for backtick in (True, False):
            lines = []
            for i in range(0, len(self.data), MAX_UU):
                b = self.type2test(self.rawdata[i:i+MAX_UU])
                a = binascii.b2a_uu(b, backtick=backtick)
                lines.append(a)
            res = bytes()
            for line in lines:
                a = self.type2test(line)
                b = binascii.a2b_uu(a)
                res += b
            self.assertEqual(res, self.rawdata)

        self.assertEqual(binascii.a2b_uu(b"\x7f"), b"\x00"*31)
        self.assertEqual(binascii.a2b_uu(b"\x80"), b"\x00"*32)
        self.assertEqual(binascii.a2b_uu(b"\xff"), b"\x00"*31)
        self.assertRaises(binascii.Error, binascii.a2b_uu, b"\xff\x00")
        self.assertRaises(binascii.Error, binascii.a2b_uu, b"!!!!")
        self.assertRaises(binascii.Error, binascii.b2a_uu, 46*b"!")

        # Issue #7701 (crash on a pydebug build)
        self.assertEqual(binascii.b2a_uu(b'x'), b'!>   \n')

        self.assertEqual(binascii.b2a_uu(b''), b' \n')
        self.assertEqual(binascii.b2a_uu(b'', backtick=True), b'`\n')
        self.assertEqual(binascii.a2b_uu(b' \n'), b'')
        self.assertEqual(binascii.a2b_uu(b'`\n'), b'')
        self.assertEqual(binascii.b2a_uu(b'\x00Cat'), b'$ $-A=   \n')
        self.assertEqual(binascii.b2a_uu(b'\x00Cat', backtick=True),
                         b'$`$-A=```\n')
        self.assertEqual(binascii.a2b_uu(b'$`$-A=```\n'),
                         binascii.a2b_uu(b'$ $-A=   \n'))
        with self.assertRaises(TypeError):
            binascii.b2a_uu(b"", True)

    @hypothesis.given(
        binary=hypothesis.strategies.binary(max_size=45),
        backtick=hypothesis.strategies.booleans(),
    )
    def test_b2a_roundtrip(self, binary, backtick):
        converted = binascii.b2a_uu(self.type2test(binary), backtick=backtick)
        restored = binascii.a2b_uu(self.type2test(converted))
        self.assertConversion(binary, converted, restored, backtick=backtick)

    def test_crc_hqx(self):
        crc = binascii.crc_hqx(self.type2test(b"Test the CRC-32 of"), 0)
        crc = binascii.crc_hqx(self.type2test(b" this string."), crc)
        self.assertEqual(crc, 14290)

        self.assertRaises(TypeError, binascii.crc_hqx)
        self.assertRaises(TypeError, binascii.crc_hqx, self.type2test(b''))

        for crc in 0, 1, 0x1234, 0x12345, 0x12345678, -1:
            self.assertEqual(binascii.crc_hqx(self.type2test(b''), crc),
                             crc & 0xffff)

    def test_crc32(self):
        crc = binascii.crc32(self.type2test(b"Test the CRC-32 of"))
        crc = binascii.crc32(self.type2test(b" this string."), crc)
        self.assertEqual(crc, 1571220330)

        self.assertRaises(TypeError, binascii.crc32)

    def test_hex(self):
        # test hexlification
        s = b'{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
        t = binascii.b2a_hex(self.type2test(s))
        u = binascii.a2b_hex(self.type2test(t))
        self.assertEqual(s, u)
        self.assertRaises(binascii.Error, binascii.a2b_hex, t[:-1])
        self.assertRaises(binascii.Error, binascii.a2b_hex, t[:-1] + b'q')
        self.assertRaises(binascii.Error, binascii.a2b_hex, bytes([255, 255]))
        self.assertRaises(binascii.Error, binascii.a2b_hex, b'0G')
        self.assertRaises(binascii.Error, binascii.a2b_hex, b'0g')
        self.assertRaises(binascii.Error, binascii.a2b_hex, b'G0')
        self.assertRaises(binascii.Error, binascii.a2b_hex, b'g0')

        # Confirm that b2a_hex == hexlify and a2b_hex == unhexlify
        self.assertEqual(binascii.hexlify(self.type2test(s)), t)
        self.assertEqual(binascii.unhexlify(self.type2test(t)), u)

    @hypothesis.given(binary=hypothesis.strategies.binary())
    def test_hex_roundtrip(self, binary):
        converted = binascii.hexlify(self.type2test(binary))
        restored = binascii.unhexlify(self.type2test(converted))
        self.assertConversion(binary, converted, restored)

    def test_hex_separator(self):
        """Test that hexlify and b2a_hex are binary versions of bytes.hex."""
        # Logic of separators is tested in test_bytes.py.  This checks that
        # arg parsing works and exercises the direct to bytes object code
        # path within pystrhex.c.
        s = b'{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
        self.assertEqual(binascii.hexlify(self.type2test(s)), s.hex().encode('ascii'))
        expected8 = s.hex('.', 8).encode('ascii')
        self.assertEqual(binascii.hexlify(self.type2test(s), '.', 8), expected8)
        expected1 = s.hex(':').encode('ascii')
        self.assertEqual(binascii.b2a_hex(self.type2test(s), ':'), expected1)

    def test_qp(self):
        type2test = self.type2test
        a2b_qp = binascii.a2b_qp
        b2a_qp = binascii.b2a_qp

        a2b_qp(data=b"", header=False)  # Keyword arguments allowed

        # A test for SF bug 534347 (segfaults without the proper fix)
        try:
            a2b_qp(b"", **{1:1})
        except TypeError:
            pass
        else:
            self.fail("binascii.a2b_qp(**{1:1}) didn't raise TypeError")

        self.assertEqual(a2b_qp(type2test(b"=")), b"")
        self.assertEqual(a2b_qp(type2test(b"= ")), b"= ")
        self.assertEqual(a2b_qp(type2test(b"==")), b"=")
        self.assertEqual(a2b_qp(type2test(b"=\nAB")), b"AB")
        self.assertEqual(a2b_qp(type2test(b"=\r\nAB")), b"AB")
        self.assertEqual(a2b_qp(type2test(b"=\rAB")), b"")  # ?
        self.assertEqual(a2b_qp(type2test(b"=\rAB\nCD")), b"CD")  # ?
        self.assertEqual(a2b_qp(type2test(b"=AB")), b"\xab")
        self.assertEqual(a2b_qp(type2test(b"=ab")), b"\xab")
        self.assertEqual(a2b_qp(type2test(b"=AX")), b"=AX")
        self.assertEqual(a2b_qp(type2test(b"=XA")), b"=XA")
        self.assertEqual(a2b_qp(type2test(b"=AB")[:-1]), b"=A")

        self.assertEqual(a2b_qp(type2test(b'_')), b'_')
        self.assertEqual(a2b_qp(type2test(b'_'), header=True), b' ')

        self.assertRaises(TypeError, b2a_qp, foo="bar")
        self.assertEqual(a2b_qp(type2test(b"=00\r\n=00")), b"\x00\r\n\x00")
        self.assertEqual(b2a_qp(type2test(b"\xff\r\n\xff\n\xff")),
                         b"=FF\r\n=FF\r\n=FF")
        self.assertEqual(b2a_qp(type2test(b"0"*75+b"\xff\r\n\xff\r\n\xff")),
                         b"0"*75+b"=\r\n=FF\r\n=FF\r\n=FF")

        self.assertEqual(b2a_qp(type2test(b'\x7f')), b'=7F')
        self.assertEqual(b2a_qp(type2test(b'=')), b'=3D')

        self.assertEqual(b2a_qp(type2test(b'_')), b'_')
        self.assertEqual(b2a_qp(type2test(b'_'), header=True), b'=5F')
        self.assertEqual(b2a_qp(type2test(b'x y'), header=True), b'x_y')
        self.assertEqual(b2a_qp(type2test(b'x '), header=True), b'x=20')
        self.assertEqual(b2a_qp(type2test(b'x y'), header=True, quotetabs=True),
                         b'x=20y')
        self.assertEqual(b2a_qp(type2test(b'x\ty'), header=True), b'x\ty')

        self.assertEqual(b2a_qp(type2test(b' ')), b'=20')
        self.assertEqual(b2a_qp(type2test(b'\t')), b'=09')
        self.assertEqual(b2a_qp(type2test(b' x')), b' x')
        self.assertEqual(b2a_qp(type2test(b'\tx')), b'\tx')
        self.assertEqual(b2a_qp(type2test(b' x')[:-1]), b'=20')
        self.assertEqual(b2a_qp(type2test(b'\tx')[:-1]), b'=09')
        self.assertEqual(b2a_qp(type2test(b'\0')), b'=00')

        self.assertEqual(b2a_qp(type2test(b'\0\n')), b'=00\n')
        self.assertEqual(b2a_qp(type2test(b'\0\n'), quotetabs=True), b'=00\n')

        self.assertEqual(b2a_qp(type2test(b'x y\tz')), b'x y\tz')
        self.assertEqual(b2a_qp(type2test(b'x y\tz'), quotetabs=True),
                         b'x=20y=09z')
        self.assertEqual(b2a_qp(type2test(b'x y\tz'), istext=False),
                         b'x y\tz')
        self.assertEqual(b2a_qp(type2test(b'x \ny\t\n')),
                         b'x=20\ny=09\n')
        self.assertEqual(b2a_qp(type2test(b'x \ny\t\n'), quotetabs=True),
                         b'x=20\ny=09\n')
        self.assertEqual(b2a_qp(type2test(b'x \ny\t\n'), istext=False),
                         b'x =0Ay\t=0A')
        self.assertEqual(b2a_qp(type2test(b'x \ry\t\r')),
                         b'x \ry\t\r')
        self.assertEqual(b2a_qp(type2test(b'x \ry\t\r'), quotetabs=True),
                         b'x=20\ry=09\r')
        self.assertEqual(b2a_qp(type2test(b'x \ry\t\r'), istext=False),
                         b'x =0Dy\t=0D')
        self.assertEqual(b2a_qp(type2test(b'x \r\ny\t\r\n')),
                         b'x=20\r\ny=09\r\n')
        self.assertEqual(b2a_qp(type2test(b'x \r\ny\t\r\n'), quotetabs=True),
                         b'x=20\r\ny=09\r\n')
        self.assertEqual(b2a_qp(type2test(b'x \r\ny\t\r\n'), istext=False),
                         b'x =0D=0Ay\t=0D=0A')

        self.assertEqual(b2a_qp(type2test(b'x \r\n')[:-1]), b'x \r')
        self.assertEqual(b2a_qp(type2test(b'x\t\r\n')[:-1]), b'x\t\r')
        self.assertEqual(b2a_qp(type2test(b'x \r\n')[:-1], quotetabs=True),
                         b'x=20\r')
        self.assertEqual(b2a_qp(type2test(b'x\t\r\n')[:-1], quotetabs=True),
                         b'x=09\r')
        self.assertEqual(b2a_qp(type2test(b'x \r\n')[:-1], istext=False),
                         b'x =0D')
        self.assertEqual(b2a_qp(type2test(b'x\t\r\n')[:-1], istext=False),
                         b'x\t=0D')

        self.assertEqual(b2a_qp(type2test(b'.')), b'=2E')
        self.assertEqual(b2a_qp(type2test(b'.\n')), b'=2E\n')
        self.assertEqual(b2a_qp(type2test(b'.\r')), b'=2E\r')
        self.assertEqual(b2a_qp(type2test(b'.\0')), b'=2E=00')
        self.assertEqual(b2a_qp(type2test(b'a.\n')), b'a.\n')
        self.assertEqual(b2a_qp(type2test(b'.a')[:-1]), b'=2E')

    @hypothesis.given(
        binary=hypothesis.strategies.binary(),
        quotetabs=hypothesis.strategies.booleans(),
        istext=hypothesis.strategies.booleans(),
        header=hypothesis.strategies.booleans(),
    )
    def test_b2a_qp_a2b_qp_round_trip(self, binary, quotetabs, istext, header):
        converted = binascii.b2a_qp(
            self.type2test(binary),
            quotetabs=quotetabs, istext=istext, header=header,
        )
        restored = binascii.a2b_qp(self.type2test(converted), header=header)
        self.assertConversion(binary, converted, restored,
                              quotetabs=quotetabs, istext=istext, header=header)

    def test_empty_string(self):
        # A test for SF bug #1022953.  Make sure SystemError is not raised.
        empty = self.type2test(b'')
        for func in all_functions:
            if func == 'crc_hqx':
                # crc_hqx needs 2 arguments
                binascii.crc_hqx(empty, 0)
                continue
            f = getattr(binascii, func)
            try:
                f(empty)
            except Exception as err:
                self.fail("{}({!r}) raises {!r}".format(func, empty, err))

    def test_unicode_b2a(self):
        # Unicode strings are not accepted by b2a_* functions.
        for func in set(all_functions) - set(a2b_functions):
            try:
                self.assertRaises(TypeError, getattr(binascii, func), "test")
            except Exception as err:
                self.fail('{}("test") raises {!r}'.format(func, err))
        # crc_hqx needs 2 arguments
        self.assertRaises(TypeError, binascii.crc_hqx, "test", 0)

    def test_unicode_a2b(self):
        # Unicode strings are accepted by a2b_* functions.
        MAX_ALL = 45
        raw = self.rawdata[:MAX_ALL]
        for fa, fb in zip(a2b_functions, b2a_functions):
            a2b = getattr(binascii, fa)
            b2a = getattr(binascii, fb)
            try:
                a = b2a(self.type2test(raw))
                binary_res = a2b(a)
                a = a.decode('ascii')
                res = a2b(a)
            except Exception as err:
                self.fail("{}/{} conversion raises {!r}".format(fb, fa, err))
            self.assertEqual(res, raw, "{}/{} conversion: "
                             "{!r} != {!r}".format(fb, fa, res, raw))
            self.assertEqual(res, binary_res)
            self.assertIsInstance(res, bytes)
            # non-ASCII string
            self.assertRaises(ValueError, a2b, "\x80")

    def test_b2a_base64_newline(self):
        # Issue #25357: test newline parameter
        b = self.type2test(b'hello')
        self.assertEqual(binascii.b2a_base64(b),
                         b'aGVsbG8=\n')
        self.assertEqual(binascii.b2a_base64(b, newline=True),
                         b'aGVsbG8=\n')
        self.assertEqual(binascii.b2a_base64(b, newline=False),
                         b'aGVsbG8=')
        b = self.type2test(b'')
        self.assertEqual(binascii.b2a_base64(b), b'\n')
        self.assertEqual(binascii.b2a_base64(b, newline=True), b'\n')
        self.assertEqual(binascii.b2a_base64(b, newline=False), b'')

    def test_b2a_base64_wrapcol(self):
        b = self.type2test(b'www.python.org')
        self.assertEqual(binascii.b2a_base64(b),
                         b'd3d3LnB5dGhvbi5vcmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=0),
                         b'd3d3LnB5dGhvbi5vcmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=8),
                         b'd3d3LnB5\ndGhvbi5v\ncmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=11),
                         b'd3d3LnB5\ndGhvbi5v\ncmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=76),
                         b'd3d3LnB5dGhvbi5vcmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=8, newline=False),
                         b'd3d3LnB5\ndGhvbi5v\ncmc=')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=1),
                         b'd3d3\nLnB5\ndGhv\nbi5v\ncmc=\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=sys.maxsize),
                         b'd3d3LnB5dGhvbi5vcmc=\n')
        if check_impl_detail():
            self.assertEqual(binascii.b2a_base64(b, wrapcol=sys.maxsize*2),
                             b'd3d3LnB5dGhvbi5vcmc=\n')
            with self.assertRaises(OverflowError):
                binascii.b2a_base64(b, wrapcol=2**1000)
        with self.assertRaises(ValueError):
            binascii.b2a_base64(b, wrapcol=-8)
        with self.assertRaises(TypeError):
            binascii.b2a_base64(b, wrapcol=8.0)
        with self.assertRaises(TypeError):
            binascii.b2a_base64(b, wrapcol='8')
        b = self.type2test(b'')
        self.assertEqual(binascii.b2a_base64(b), b'\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=0), b'\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=8), b'\n')
        self.assertEqual(binascii.b2a_base64(b, wrapcol=8, newline=False), b'')

    @hypothesis.given(
        binary=hypothesis.strategies.binary(),
        newline=hypothesis.strategies.booleans(),
    )
    def test_base64_roundtrip(self, binary, newline):
        converted = binascii.b2a_base64(self.type2test(binary), newline=newline)
        restored = binascii.a2b_base64(self.type2test(converted))
        self.assertConversion(binary, converted, restored, newline=newline)

    def test_c_contiguity(self):
        m = memoryview(bytearray(b'noncontig'))
        noncontig_writable = m[::-2]
        with self.assertRaises(BufferError):
            binascii.b2a_hex(noncontig_writable)


class ArrayBinASCIITest(BinASCIITest):
    def type2test(self, s):
        return array.array('B', list(s))


class BytearrayBinASCIITest(BinASCIITest):
    type2test = bytearray


class MemoryviewBinASCIITest(BinASCIITest):
    type2test = memoryview

class ChecksumBigBufferTestCase(unittest.TestCase):
    """bpo-38256 - check that inputs >=4 GiB are handled correctly."""

    @bigmemtest(size=_4G + 4, memuse=1, dry_run=False)
    def test_big_buffer(self, size):
        data = b"nyan" * (_1G + 1)
        self.assertEqual(binascii.crc32(data), 1044521549)


if __name__ == "__main__":
    unittest.main()
