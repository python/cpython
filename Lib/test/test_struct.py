import array
import unittest
import struct
import sys

from test.support import run_unittest

ISBIGENDIAN = sys.byteorder == "big"
IS32BIT = sys.maxsize == 0x7fffffff

def string_reverse(s):
    return s[::-1]

def bigendian_to_native(value):
    if ISBIGENDIAN:
        return value
    else:
        return string_reverse(value)

class StructTest(unittest.TestCase):
    def test_isbigendian(self):
        self.assertEqual((struct.pack('=i', 1)[0] == 0), ISBIGENDIAN)

    def test_consistence(self):
        self.assertRaises(struct.error, struct.calcsize, 'Z')

        sz = struct.calcsize('i')
        self.assertEqual(sz * 3, struct.calcsize('iii'))

        fmt = 'cbxxxxxxhhhhiillffd?'
        fmt3 = '3c3b18x12h6i6l6f3d3?'
        sz = struct.calcsize(fmt)
        sz3 = struct.calcsize(fmt3)
        self.assertEqual(sz * 3, sz3)

        self.assertRaises(struct.error, struct.pack, 'iii', 3)
        self.assertRaises(struct.error, struct.pack, 'i', 3, 3, 3)
        self.assertRaises(struct.error, struct.pack, 'i', 'foo')
        self.assertRaises(struct.error, struct.pack, 'P', 'foo')
        self.assertRaises(struct.error, struct.unpack, 'd', b'flap')
        s = struct.pack('ii', 1, 2)
        self.assertRaises(struct.error, struct.unpack, 'iii', s)
        self.assertRaises(struct.error, struct.unpack, 'i', s)

    def test_transitiveness(self):
        c = b'a'
        b = 1
        h = 255
        i = 65535
        l = 65536
        f = 3.1415
        d = 3.1415
        t = True

        for prefix in ('', '@', '<', '>', '=', '!'):
            for format in ('xcbhilfd?', 'xcBHILfd?'):
                format = prefix + format
                s = struct.pack(format, c, b, h, i, l, f, d, t)
                cp, bp, hp, ip, lp, fp, dp, tp = struct.unpack(format, s)
                self.assertEqual(cp, c)
                self.assertEqual(bp, b)
                self.assertEqual(hp, h)
                self.assertEqual(ip, i)
                self.assertEqual(lp, l)
                self.assertEqual(int(100 * fp), int(100 * f))
                self.assertEqual(int(100 * dp), int(100 * d))
                self.assertEqual(tp, t)

    def test_new_features(self):
        # Test some of the new features in detail
        # (format, argument, big-endian result, little-endian result, asymmetric)
        tests = [
            ('c', 'a', 'a', 'a', 0),
            ('xc', 'a', '\0a', '\0a', 0),
            ('cx', 'a', 'a\0', 'a\0', 0),
            ('s', 'a', 'a', 'a', 0),
            ('0s', 'helloworld', '', '', 1),
            ('1s', 'helloworld', 'h', 'h', 1),
            ('9s', 'helloworld', 'helloworl', 'helloworl', 1),
            ('10s', 'helloworld', 'helloworld', 'helloworld', 0),
            ('11s', 'helloworld', 'helloworld\0', 'helloworld\0', 1),
            ('20s', 'helloworld', 'helloworld'+10*'\0', 'helloworld'+10*'\0', 1),
            ('b', 7, '\7', '\7', 0),
            ('b', -7, '\371', '\371', 0),
            ('B', 7, '\7', '\7', 0),
            ('B', 249, '\371', '\371', 0),
            ('h', 700, '\002\274', '\274\002', 0),
            ('h', -700, '\375D', 'D\375', 0),
            ('H', 700, '\002\274', '\274\002', 0),
            ('H', 0x10000-700, '\375D', 'D\375', 0),
            ('i', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('i', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('I', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('I', 0x100000000-70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('l', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('l', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('L', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('L', 0x100000000-70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('f', 2.0, '@\000\000\000', '\000\000\000@', 0),
            ('d', 2.0, '@\000\000\000\000\000\000\000',
                       '\000\000\000\000\000\000\000@', 0),
            ('f', -2.0, '\300\000\000\000', '\000\000\000\300', 0),
            ('d', -2.0, '\300\000\000\000\000\000\000\000',
                       '\000\000\000\000\000\000\000\300', 0),
                ('?', 0, '\0', '\0', 0),
                ('?', 3, '\1', '\1', 1),
                ('?', True, '\1', '\1', 0),
                ('?', [], '\0', '\0', 1),
                ('?', (1,), '\1', '\1', 1),
        ]

        for fmt, arg, big, lil, asy in tests:
            big = bytes(big, "latin-1")
            lil = bytes(lil, "latin-1")
            for (xfmt, exp) in [('>'+fmt, big), ('!'+fmt, big), ('<'+fmt, lil),
                                ('='+fmt, ISBIGENDIAN and big or lil)]:
                res = struct.pack(xfmt, arg)
                self.assertEqual(res, exp)
                self.assertEqual(struct.calcsize(xfmt), len(res))
                rev = struct.unpack(xfmt, res)[0]
                if isinstance(arg, str):
                    # Strings are returned as bytes since you can't know the
                    # encoding of the string when packed.
                    arg = bytes(arg, 'latin1')
                if rev != arg:
                    self.assertTrue(asy)

    def test_native_qQ(self):
        # can't pack -1 as unsigned regardless
        self.assertRaises((struct.error, OverflowError), struct.pack, "Q", -1)
        # can't pack string as 'q' regardless
        self.assertRaises(struct.error, struct.pack, "q", "a")
        # ditto, but 'Q'
        self.assertRaises(struct.error, struct.pack, "Q", "a")

        try:
            struct.pack("q", 5)
        except struct.error:
            # does not have native q/Q
            pass
        else:
            nbytes = struct.calcsize('q')
            # The expected values here are in big-endian format, primarily
            # because I'm on a little-endian machine and so this is the
            # clearest way (for me) to force the code to get exercised.
            for format, input, expected in (
                    ('q', -1, '\xff' * nbytes),
                    ('q', 0, '\x00' * nbytes),
                    ('Q', 0, '\x00' * nbytes),
                    ('q', 1, '\x00' * (nbytes-1) + '\x01'),
                    ('Q', (1 << (8*nbytes))-1, '\xff' * nbytes),
                    ('q', (1 << (8*nbytes-1))-1, '\x7f' + '\xff' * (nbytes - 1))):
                expected = bytes(expected, "latin-1")
                got = struct.pack(format, input)
                native_expected = bigendian_to_native(expected)
                self.assertEqual(got, native_expected)
                retrieved = struct.unpack(format, got)[0]
                self.assertEqual(retrieved, input)

    def test_standard_integers(self):
        # Standard integer tests (bBhHiIlLqQ).
        import binascii

        class IntTester(unittest.TestCase):

            def __init__(self, formatpair, bytesize):
                super(IntTester, self).__init__(methodName='test_one')
                self.assertEqual(len(formatpair), 2)
                self.formatpair = formatpair
                for direction in "<>!=":
                    for code in formatpair:
                        format = direction + code
                        self.assertEqual(struct.calcsize(format), bytesize)
                self.bytesize = bytesize
                self.bitsize = bytesize * 8
                self.signed_code, self.unsigned_code = formatpair
                self.unsigned_min = 0
                self.unsigned_max = 2**self.bitsize - 1
                self.signed_min = -(2**(self.bitsize-1))
                self.signed_max = 2**(self.bitsize-1) - 1

            def test_one(self, x, pack=struct.pack,
                                  unpack=struct.unpack,
                                  unhexlify=binascii.unhexlify):
                # Try signed.
                code = self.signed_code
                if self.signed_min <= x <= self.signed_max:
                    # Try big-endian.
                    expected = x
                    if x < 0:
                        expected += 1 << self.bitsize
                        self.assertTrue(expected > 0)
                    expected = hex(expected)[2:] # chop "0x"
                    if len(expected) & 1:
                        expected = "0" + expected
                    expected = unhexlify(expected)
                    expected = b"\x00" * (self.bytesize - len(expected)) + expected

                    # Pack work?
                    format = ">" + code
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError),
                                      unpack, format, b'\x01' + got)

                    # Try little-endian.
                    format = "<" + code
                    expected = string_reverse(expected)

                    # Pack work?
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError),
                                      unpack, format, b'\x01' + got)

                else:
                    # x is out of range -- verify pack realizes that.
                    self.assertRaises(struct.error, pack, ">" + code, x)
                    self.assertRaises(struct.error, pack, "<" + code, x)

                # Much the same for unsigned.
                code = self.unsigned_code
                if self.unsigned_min <= x <= self.unsigned_max:
                    # Try big-endian.
                    format = ">" + code
                    expected = x
                    expected = hex(expected)[2:] # chop "0x"
                    if len(expected) & 1:
                        expected = "0" + expected
                    expected = unhexlify(expected)
                    expected = b"\x00" * (self.bytesize - len(expected)) + expected

                    # Pack work?
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError),
                                      unpack, format, b'\x01' + got)

                    # Try little-endian.
                    format = "<" + code
                    expected = string_reverse(expected)

                    # Pack work?
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError),
                                      unpack, format, b'\x01' + got)

                else:
                    # x is out of range -- verify pack realizes that.
                    self.assertRaises(struct.error, pack, ">" + code, x)
                    self.assertRaises(struct.error, pack, "<" + code, x)

            def run(self):
                from random import randrange

                # Create all interesting powers of 2.
                values = []
                for exp in range(self.bitsize + 3):
                    values.append(1 << exp)

                # Add some random values.
                for i in range(self.bitsize):
                    val = 0
                    for j in range(self.bytesize):
                        val = (val << 8) | randrange(256)
                    values.append(val)

                # Try all those, and their negations, and +-1 from them.  Note
                # that this tests all power-of-2 boundaries in range, and a few out
                # of range, plus +-(2**n +- 1).
                for base in values:
                    for val in -base, base:
                        for incr in -1, 0, 1:
                            x = val + incr
                            try:
                                x = int(x)
                            except OverflowError:
                                pass
                            self.test_one(x)

                # Some error cases.
                for direction in "<>":
                    for code in self.formatpair:
                        for badobject in "a string", 3+42j, randrange, -1729.0:
                            self.assertRaises(struct.error,
                                              struct.pack, direction + code,
                                              badobject)

        for args in [("bB", 1),
                     ("hH", 2),
                     ("iI", 4),
                     ("lL", 4),
                     ("qQ", 8)]:
            t = IntTester(*args)
            t.run()

    def test_p_code(self):
        # Test p ("Pascal string") code.
        for code, input, expected, expectedback in [
                ('p','abc', '\x00', b''),
                ('1p', 'abc', '\x00', b''),
                ('2p', 'abc', '\x01a', b'a'),
                ('3p', 'abc', '\x02ab', b'ab'),
                ('4p', 'abc', '\x03abc', b'abc'),
                ('5p', 'abc', '\x03abc\x00', b'abc'),
                ('6p', 'abc', '\x03abc\x00\x00', b'abc'),
                ('1000p', 'x'*1000, '\xff' + 'x'*999, b'x'*255)]:
            expected = bytes(expected, "latin-1")
            got = struct.pack(code, input)
            self.assertEqual(got, expected)
            (got,) = struct.unpack(code, got)
            self.assertEqual(got, expectedback)

    def test_705836(self):
        # SF bug 705836.  "<f" and ">f" had a severe rounding bug, where a carry
        # from the low-order discarded bits could propagate into the exponent
        # field, causing the result to be wrong by a factor of 2.
        import math

        for base in range(1, 33):
            # smaller <- largest representable float less than base.
            delta = 0.5
            while base - delta / 2.0 != base:
                delta /= 2.0
            smaller = base - delta
            # Packing this rounds away a solid string of trailing 1 bits.
            packed = struct.pack("<f", smaller)
            unpacked = struct.unpack("<f", packed)[0]
            # This failed at base = 2, 4, and 32, with unpacked = 1, 2, and
            # 16, respectively.
            self.assertEqual(base, unpacked)
            bigpacked = struct.pack(">f", smaller)
            self.assertEqual(bigpacked, string_reverse(packed))
            unpacked = struct.unpack(">f", bigpacked)[0]
            self.assertEqual(base, unpacked)

        # Largest finite IEEE single.
        big = (1 << 24) - 1
        big = math.ldexp(big, 127 - 23)
        packed = struct.pack(">f", big)
        unpacked = struct.unpack(">f", packed)[0]
        self.assertEqual(big, unpacked)

        # The same, but tack on a 1 bit so it rounds up to infinity.
        big = (1 << 25) - 1
        big = math.ldexp(big, 127 - 24)
        self.assertRaises(OverflowError, struct.pack, ">f", big)

    def test_1229380(self):
        # SF bug 1229380. No struct.pack exception for some out of
        # range integers
        for endian in ('', '>', '<'):
            for fmt in ('B', 'H', 'I', 'L'):
                self.assertRaises((struct.error, OverflowError), struct.pack,
                                  endian + fmt, -1)

            self.assertRaises((struct.error, OverflowError), struct.pack,
                              endian + 'B', 300)
            self.assertRaises((struct.error, OverflowError), struct.pack,
                              endian + 'H', 70000)

            self.assertRaises((struct.error, OverflowError), struct.pack,
                              endian + 'I', sys.maxsize * 4)
            self.assertRaises((struct.error, OverflowError), struct.pack,
                              endian + 'L', sys.maxsize * 4)

    def test_1530559(self):
        for endian in ('', '>', '<'):
            for fmt in ('B', 'H', 'I', 'L', 'Q', 'b', 'h', 'i', 'l', 'q'):
                self.assertRaises(struct.error, struct.pack, endian + fmt, 1.0)
                self.assertRaises(struct.error, struct.pack, endian + fmt, 1.5)
        self.assertRaises(struct.error, struct.pack, 'P', 1.0)
        self.assertRaises(struct.error, struct.pack, 'P', 1.5)


    def test_unpack_from(self):
        test_string = b'abcd01234'
        fmt = '4s'
        s = struct.Struct(fmt)
        for cls in (bytes, bytearray):
            data = cls(test_string)
            if not isinstance(data, (bytes, bytearray)):
                bytes_data = bytes(data, 'latin1')
            else:
                bytes_data = data
            self.assertEqual(s.unpack_from(data), (b'abcd',))
            self.assertEqual(s.unpack_from(data, 2), (b'cd01',))
            self.assertEqual(s.unpack_from(data, 4), (b'0123',))
            for i in range(6):
                self.assertEqual(s.unpack_from(data, i), (bytes_data[i:i+4],))
            for i in range(6, len(test_string) + 1):
                self.assertRaises(struct.error, s.unpack_from, data, i)
        for cls in (bytes, bytearray):
            data = cls(test_string)
            self.assertEqual(struct.unpack_from(fmt, data), (b'abcd',))
            self.assertEqual(struct.unpack_from(fmt, data, 2), (b'cd01',))
            self.assertEqual(struct.unpack_from(fmt, data, 4), (b'0123',))
            for i in range(6):
                self.assertEqual(struct.unpack_from(fmt, data, i), (data[i:i+4],))
            for i in range(6, len(test_string) + 1):
                self.assertRaises(struct.error, struct.unpack_from, fmt, data, i)

    def test_pack_into(self):
        test_string = b'Reykjavik rocks, eow!'
        writable_buf = array.array('b', b' '*100)
        fmt = '21s'
        s = struct.Struct(fmt)

        # Test without offset
        s.pack_into(writable_buf, 0, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)]
        self.assertEqual(from_buf, test_string)

        # Test with offset.
        s.pack_into(writable_buf, 10, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)+10]
        self.assertEqual(from_buf, test_string[:10] + test_string)

        # Go beyond boundaries.
        small_buf = array.array('b', b' '*10)
        self.assertRaises((ValueError, struct.error), s.pack_into, small_buf, 0,
                          test_string)
        self.assertRaises((ValueError, struct.error), s.pack_into, small_buf, 2,
                          test_string)

        # Test bogus offset (issue 3694)
        sb = small_buf
        self.assertRaises(TypeError, struct.pack_into, b'1', sb, None)

    def test_pack_into_fn(self):
        test_string = b'Reykjavik rocks, eow!'
        writable_buf = array.array('b', b' '*100)
        fmt = '21s'
        pack_into = lambda *args: struct.pack_into(fmt, *args)

        # Test without offset.
        pack_into(writable_buf, 0, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)]
        self.assertEqual(from_buf, test_string)

        # Test with offset.
        pack_into(writable_buf, 10, test_string)
        from_buf = writable_buf.tostring()[:len(test_string)+10]
        self.assertEqual(from_buf, test_string[:10] + test_string)

        # Go beyond boundaries.
        small_buf = array.array('b', b' '*10)
        self.assertRaises((ValueError, struct.error), pack_into, small_buf, 0,
                          test_string)
        self.assertRaises((ValueError, struct.error), pack_into, small_buf, 2,
                          test_string)

    def test_unpack_with_buffer(self):
        # SF bug 1563759: struct.unpack doesn't support buffer protocol objects
        data1 = array.array('B', b'\x12\x34\x56\x78')
        data2 = memoryview(b'\x12\x34\x56\x78') # XXX b'......XXXX......', 6, 4
        for data in [data1, data2]:
            value, = struct.unpack('>I', data)
            self.assertEqual(value, 0x12345678)

    def test_bool(self):
        class ExplodingBool(object):
            def __bool__(self):
                raise IOError
        for prefix in tuple("<>!=")+('',):
            false = (), [], [], '', 0
            true = [1], 'test', 5, -1, 0xffffffff+1, 0xffffffff/2

            falseFormat = prefix + '?' * len(false)
            packedFalse = struct.pack(falseFormat, *false)
            unpackedFalse = struct.unpack(falseFormat, packedFalse)

            trueFormat = prefix + '?' * len(true)
            packedTrue = struct.pack(trueFormat, *true)
            unpackedTrue = struct.unpack(trueFormat, packedTrue)

            self.assertEqual(len(true), len(unpackedTrue))
            self.assertEqual(len(false), len(unpackedFalse))

            for t in unpackedFalse:
                self.assertFalse(t)
            for t in unpackedTrue:
                self.assertTrue(t)

            packed = struct.pack(prefix+'?', 1)

            self.assertEqual(len(packed), struct.calcsize(prefix+'?'))

            if len(packed) != 1:
                self.assertFalse(prefix, msg='encoded bool is not one byte: %r'
                                             %packed)

            self.assertRaises(IOError, struct.pack, prefix + '?',
                              ExplodingBool())

        for c in [b'\x01', b'\x7f', b'\xff', b'\x0f', b'\xf0']:
            self.assertTrue(struct.unpack('>?', c)[0])

    def test_count_overflow(self):
        hugecount = '{}b'.format(sys.maxsize+1)
        self.assertRaises(struct.error, struct.calcsize, hugecount)

        hugecount2 = '{}b{}H'.format(sys.maxsize//2, sys.maxsize//2)
        self.assertRaises(struct.error, struct.calcsize, hugecount2)

    if IS32BIT:
        def test_crasher(self):
            self.assertRaises(MemoryError, struct.pack, "357913941b", "a")


def test_main():
    run_unittest(StructTest)

if __name__ == '__main__':
    test_main()
