"""Test the binascii C module."""

from test import test_support
import unittest
import binascii

class BinASCIITest(unittest.TestCase):

    # Create binary test data
    data = b"The quick brown fox jumps over the lazy dog.\r\n"
    # Be slow so we don't depend on other modules
    data += bytes(range(256))
    data += b"\r\nHello world.\n"

    def test_exceptions(self):
        # Check module exceptions
        self.assert_(issubclass(binascii.Error, Exception))
        self.assert_(issubclass(binascii.Incomplete, Exception))

    def test_functions(self):
        # Check presence of all functions
        funcs = []
        for suffix in "base64", "hqx", "uu", "hex":
            prefixes = ["a2b_", "b2a_"]
            if suffix == "hqx":
                prefixes.extend(["crc_", "rlecode_", "rledecode_"])
            for prefix in prefixes:
                name = prefix + suffix
                self.assert_(hasattr(getattr(binascii, name), '__call__'))
                self.assertRaises(TypeError, getattr(binascii, name))
        for name in ("hexlify", "unhexlify"):
            self.assert_(hasattr(getattr(binascii, name), '__call__'))
            self.assertRaises(TypeError, getattr(binascii, name))

    def test_base64valid(self):
        # Test base64 with valid data
        MAX_BASE64 = 57
        lines = []
        for i in range(0, len(self.data), MAX_BASE64):
            b = self.data[i:i+MAX_BASE64]
            a = binascii.b2a_base64(b)
            lines.append(a)
        res = bytes()
        for line in lines:
            b = binascii.a2b_base64(line)
            res += b
        self.assertEqual(res, self.data)

    def test_base64invalid(self):
        # Test base64 with random invalid characters sprinkled throughout
        # (This requires a new version of binascii.)
        MAX_BASE64 = 57
        lines = []
        for i in range(0, len(self.data), MAX_BASE64):
            b = self.data[i:i+MAX_BASE64]
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
            b = binascii.a2b_base64(line)
            res += b
        self.assertEqual(res, self.data)

        # Test base64 with just invalid characters, which should return
        # empty strings. TBD: shouldn't it raise an exception instead ?
        self.assertEqual(binascii.a2b_base64(fillers), b'')

    def test_uu(self):
        MAX_UU = 45
        lines = []
        for i in range(0, len(self.data), MAX_UU):
            b = self.data[i:i+MAX_UU]
            a = binascii.b2a_uu(b)
            lines.append(a)
        res = bytes()
        for line in lines:
            b = binascii.a2b_uu(line)
            res += b
        self.assertEqual(res, self.data)

        self.assertEqual(binascii.a2b_uu(b"\x7f"), b"\x00"*31)
        self.assertEqual(binascii.a2b_uu(b"\x80"), b"\x00"*32)
        self.assertEqual(binascii.a2b_uu(b"\xff"), b"\x00"*31)
        self.assertRaises(binascii.Error, binascii.a2b_uu, b"\xff\x00")
        self.assertRaises(binascii.Error, binascii.a2b_uu, b"!!!!")

        self.assertRaises(binascii.Error, binascii.b2a_uu, 46*b"!")

    def test_crc32(self):
        crc = binascii.crc32(b"Test the CRC-32 of")
        crc = binascii.crc32(b" this string.", crc)
        self.assertEqual(crc, 1571220330)

        self.assertRaises(TypeError, binascii.crc32)

    # The hqx test is in test_binhex.py

    def test_hex(self):
        # test hexlification
        s = b'{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
        t = binascii.b2a_hex(s)
        u = binascii.a2b_hex(t)
        self.assertEqual(s, u)
        self.assertRaises(binascii.Error, binascii.a2b_hex, t[:-1])
        self.assertRaises(binascii.Error, binascii.a2b_hex, t[:-1] + b'q')

        self.assertEqual(binascii.hexlify('a'), b'61')

    def test_qp(self):
        # A test for SF bug 534347 (segfaults without the proper fix)
        try:
            binascii.a2b_qp("", **{1:1})
        except TypeError:
            pass
        else:
            self.fail("binascii.a2b_qp(**{1:1}) didn't raise TypeError")
        self.assertEqual(binascii.a2b_qp(b"= "), b"= ")
        self.assertEqual(binascii.a2b_qp(b"=="), b"=")
        self.assertEqual(binascii.a2b_qp(b"=AX"), b"=AX")
        self.assertRaises(TypeError, binascii.b2a_qp, foo="bar")
        self.assertEqual(binascii.a2b_qp(b"=00\r\n=00"), b"\x00\r\n\x00")
        self.assertEqual(
            binascii.b2a_qp(b"\xff\r\n\xff\n\xff"),
            b"=FF\r\n=FF\r\n=FF"
        )
        self.assertEqual(
            binascii.b2a_qp(b"0"*75+b"\xff\r\n\xff\r\n\xff"),
            b"0"*75+b"=\r\n=FF\r\n=FF\r\n=FF"
        )

        self.assertEqual(binascii.b2a_qp(b'\0\n'), b'=00\n')
        self.assertEqual(binascii.b2a_qp(b'\0\n', quotetabs=True), b'=00\n')
        self.assertEqual(binascii.b2a_qp(b'foo\tbar\t\n'), b'foo\tbar=09\n')
        self.assertEqual(binascii.b2a_qp(b'foo\tbar\t\n', quotetabs=True),
                         b'foo=09bar=09\n')

        self.assertEqual(binascii.b2a_qp(b'.'), b'=2E')
        self.assertEqual(binascii.b2a_qp(b'.\n'), b'=2E\n')
        self.assertEqual(binascii.b2a_qp(b'a.\n'), b'a.\n')

    def test_empty_string(self):
        # A test for SF bug #1022953.  Make sure SystemError is not raised.
        for n in ['b2a_qp', 'a2b_hex', 'b2a_base64', 'a2b_uu', 'a2b_qp',
                  'b2a_hex', 'unhexlify', 'hexlify', 'crc32', 'b2a_hqx',
                  'a2b_hqx', 'a2b_base64', 'rlecode_hqx', 'b2a_uu',
                  'rledecode_hqx']:
            f = getattr(binascii, n)
            try:
                f(b'')
            except SystemError as err:
                self.fail("%s(b'') raises SystemError: %s" % (n, err))
        binascii.crc_hqx('', 0)

def test_main():
    test_support.run_unittest(BinASCIITest)

if __name__ == "__main__":
    test_main()
