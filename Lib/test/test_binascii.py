"""Test the binascii C module."""

from test import test_support
import unittest
import binascii
import array

# Note: "*_hex" functions are aliases for "(un)hexlify"
b2a_functions = ['b2a_base64', 'b2a_hex', 'b2a_hqx', 'b2a_qp', 'b2a_uu',
                 'hexlify', 'rlecode_hqx']
a2b_functions = ['a2b_base64', 'a2b_hex', 'a2b_hqx', 'a2b_qp', 'a2b_uu',
                 'unhexlify', 'rledecode_hqx']
all_functions = a2b_functions + b2a_functions + ['crc32', 'crc_hqx']


class BinASCIITest(unittest.TestCase):

    type2test = str
    # Create binary test data
    rawdata = "The quick brown fox jumps over the lazy dog.\r\n"
    # Be slow so we don't depend on other modules
    rawdata += "".join(map(chr, xrange(256)))
    rawdata += "\r\nHello world.\n"

    def setUp(self):
        self.data = self.type2test(self.rawdata)

    def test_exceptions(self):
        # Check module exceptions
        self.assertTrue(issubclass(binascii.Error, Exception))
        self.assertTrue(issubclass(binascii.Incomplete, Exception))

    def test_functions(self):
        # Check presence of all functions
        for name in all_functions:
            self.assertTrue(hasattr(getattr(binascii, name), '__call__'))
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
            except Exception, err:
                self.fail("{}/{} conversion raises {!r}".format(fb, fa, err))
            if fb == 'b2a_hqx':
                # b2a_hqx returns a tuple
                res, _ = res
            self.assertEqual(res, raw, "{}/{} conversion: "
                             "{!r} != {!r}".format(fb, fa, res, raw))
            self.assertIsInstance(res, str)
            self.assertIsInstance(a, str)
            self.assertLess(max(ord(c) for c in a), 128)
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
        res = ""
        for line in lines:
            a = self.type2test(line)
            b = binascii.a2b_base64(a)
            res = res + b
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

        fillers = ""
        valid = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+/"
        for i in xrange(256):
            c = chr(i)
            if c not in valid:
                fillers += c
        def addnoise(line):
            noise = fillers
            ratio = len(line) // len(noise)
            res = ""
            while line and noise:
                if len(line) // len(noise) > ratio:
                    c, line = line[0], line[1:]
                else:
                    c, noise = noise[0], noise[1:]
                res += c
            return res + noise + line
        res = ""
        for line in map(addnoise, lines):
            a = self.type2test(line)
            b = binascii.a2b_base64(a)
            res += b
        self.assertEqual(res, self.rawdata)

        # Test base64 with just invalid characters, which should return
        # empty strings. TBD: shouldn't it raise an exception instead ?
        self.assertEqual(binascii.a2b_base64(self.type2test(fillers)), '')

    def test_uu(self):
        MAX_UU = 45
        lines = []
        for i in range(0, len(self.data), MAX_UU):
            b = self.type2test(self.rawdata[i:i+MAX_UU])
            a = binascii.b2a_uu(b)
            lines.append(a)
        res = ""
        for line in lines:
            a = self.type2test(line)
            b = binascii.a2b_uu(a)
            res += b
        self.assertEqual(res, self.rawdata)

        self.assertEqual(binascii.a2b_uu("\x7f"), "\x00"*31)
        self.assertEqual(binascii.a2b_uu("\x80"), "\x00"*32)
        self.assertEqual(binascii.a2b_uu("\xff"), "\x00"*31)
        self.assertRaises(binascii.Error, binascii.a2b_uu, "\xff\x00")
        self.assertRaises(binascii.Error, binascii.a2b_uu, "!!!!")

        self.assertRaises(binascii.Error, binascii.b2a_uu, 46*"!")

        # Issue #7701 (crash on a pydebug build)
        self.assertEqual(binascii.b2a_uu('x'), '!>   \n')

    def test_crc32(self):
        crc = binascii.crc32(self.type2test("Test the CRC-32 of"))
        crc = binascii.crc32(self.type2test(" this string."), crc)
        self.assertEqual(crc, 1571220330)

        self.assertRaises(TypeError, binascii.crc32)

    def test_hqx(self):
        # Perform binhex4 style RLE-compression
        # Then calculate the hexbin4 binary-to-ASCII translation
        rle = binascii.rlecode_hqx(self.data)
        a = binascii.b2a_hqx(self.type2test(rle))
        b, _ = binascii.a2b_hqx(self.type2test(a))
        res = binascii.rledecode_hqx(b)

        self.assertEqual(res, self.rawdata)

    def test_hex(self):
        # test hexlification
        s = '{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
        t = binascii.b2a_hex(self.type2test(s))
        u = binascii.a2b_hex(self.type2test(t))
        self.assertEqual(s, u)
        self.assertRaises(TypeError, binascii.a2b_hex, t[:-1])
        self.assertRaises(TypeError, binascii.a2b_hex, t[:-1] + 'q')

        # Verify the treatment of Unicode strings
        if test_support.have_unicode:
            self.assertEqual(binascii.hexlify(unicode('a', 'ascii')), '61')

    def test_qp(self):
        # A test for SF bug 534347 (segfaults without the proper fix)
        try:
            binascii.a2b_qp("", **{1:1})
        except TypeError:
            pass
        else:
            self.fail("binascii.a2b_qp(**{1:1}) didn't raise TypeError")
        self.assertEqual(binascii.a2b_qp("= "), "= ")
        self.assertEqual(binascii.a2b_qp("=="), "=")
        self.assertEqual(binascii.a2b_qp("=AX"), "=AX")
        self.assertRaises(TypeError, binascii.b2a_qp, foo="bar")
        self.assertEqual(binascii.a2b_qp("=00\r\n=00"), "\x00\r\n\x00")
        self.assertEqual(
            binascii.b2a_qp("\xff\r\n\xff\n\xff"),
            "=FF\r\n=FF\r\n=FF"
        )
        self.assertEqual(
            binascii.b2a_qp("0"*75+"\xff\r\n\xff\r\n\xff"),
            "0"*75+"=\r\n=FF\r\n=FF\r\n=FF"
        )

        self.assertEqual(binascii.b2a_qp('\0\n'), '=00\n')
        self.assertEqual(binascii.b2a_qp('\0\n', quotetabs=True), '=00\n')
        self.assertEqual(binascii.b2a_qp('foo\tbar\t\n'), 'foo\tbar=09\n')
        self.assertEqual(binascii.b2a_qp('foo\tbar\t\n', quotetabs=True), 'foo=09bar=09\n')

        self.assertEqual(binascii.b2a_qp('.'), '=2E')
        self.assertEqual(binascii.b2a_qp('.\n'), '=2E\n')
        self.assertEqual(binascii.b2a_qp('a.\n'), 'a.\n')

    def test_empty_string(self):
        # A test for SF bug #1022953.  Make sure SystemError is not raised.
        empty = self.type2test('')
        for func in all_functions:
            if func == 'crc_hqx':
                # crc_hqx needs 2 arguments
                binascii.crc_hqx(empty, 0)
                continue
            f = getattr(binascii, func)
            try:
                f(empty)
            except Exception, err:
                self.fail("{}({!r}) raises {!r}".format(func, empty, err))


class ArrayBinASCIITest(BinASCIITest):
    def type2test(self, s):
        return array.array('c', s)


class BytearrayBinASCIITest(BinASCIITest):
    type2test = bytearray


class MemoryviewBinASCIITest(BinASCIITest):
    type2test = memoryview


def test_main():
    test_support.run_unittest(BinASCIITest,
                              ArrayBinASCIITest,
                              BytearrayBinASCIITest,
                              MemoryviewBinASCIITest)

if __name__ == "__main__":
    test_main()
