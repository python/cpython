"""Test the binascii C module."""

from test import test_support
import unittest
import binascii

class BinASCIITest(unittest.TestCase):

    # Create binary test data
    data = "The quick brown fox jumps over the lazy dog.\r\n"
    # Be slow so we don't depend on other modules
    data += "".join(map(chr, xrange(256)))
    data += "\r\nHello world.\n"

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
                self.assert_(callable(getattr(binascii, name)))
                self.assertRaises(TypeError, getattr(binascii, name))
        for name in ("hexlify", "unhexlify"):
            self.assert_(callable(getattr(binascii, name)))
            self.assertRaises(TypeError, getattr(binascii, name))

    def test_base64valid(self):
        # Test base64 with valid data
        MAX_BASE64 = 57
        lines = []
        for i in range(0, len(self.data), MAX_BASE64):
            b = self.data[i:i+MAX_BASE64]
            a = binascii.b2a_base64(b)
            lines.append(a)
        res = ""
        for line in lines:
            b = binascii.a2b_base64(line)
            res = res + b
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
            b = binascii.a2b_base64(line)
            res += b
        self.assertEqual(res, self.data)

        # Test base64 with just invalid characters, which should return
        # empty strings. TBD: shouldn't it raise an exception instead ?
        self.assertEqual(binascii.a2b_base64(fillers), '')

    def test_uu(self):
        MAX_UU = 45
        lines = []
        for i in range(0, len(self.data), MAX_UU):
            b = self.data[i:i+MAX_UU]
            a = binascii.b2a_uu(b)
            lines.append(a)
        res = ""
        for line in lines:
            b = binascii.a2b_uu(line)
            res += b
        self.assertEqual(res, self.data)

        self.assertEqual(binascii.a2b_uu("\x7f"), "\x00"*31)
        self.assertEqual(binascii.a2b_uu("\x80"), "\x00"*32)
        self.assertEqual(binascii.a2b_uu("\xff"), "\x00"*31)
        self.assertRaises(binascii.Error, binascii.a2b_uu, "\xff\x00")
        self.assertRaises(binascii.Error, binascii.a2b_uu, "!!!!")

        self.assertRaises(binascii.Error, binascii.b2a_uu, 46*"!")

    def test_crc32(self):
        crc = binascii.crc32("Test the CRC-32 of")
        crc = binascii.crc32(" this string.", crc)
        self.assertEqual(crc, 1571220330)

        self.assertRaises(TypeError, binascii.crc32)

    # The hqx test is in test_binhex.py

    def test_hex(self):
        # test hexlification
        s = '{s\005\000\000\000worldi\002\000\000\000s\005\000\000\000helloi\001\000\000\0000'
        t = binascii.b2a_hex(s)
        u = binascii.a2b_hex(t)
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
        self.assertEqual(binascii.a2b_qp("= "), "")
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

    def test_empty_string(self):
        # A test for SF bug #1022953.  Make sure SystemError is not raised.
        for n in ['b2a_qp', 'a2b_hex', 'b2a_base64', 'a2b_uu', 'a2b_qp',
                  'b2a_hex', 'unhexlify', 'hexlify', 'crc32', 'b2a_hqx',
                  'a2b_hqx', 'a2b_base64', 'rlecode_hqx', 'b2a_uu',
                  'rledecode_hqx']:
            f = getattr(binascii, n)
            f('')
        binascii.crc_hqx('', 0)

def test_main():
    test_support.run_unittest(BinASCIITest)

if __name__ == "__main__":
    test_main()
