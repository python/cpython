import os
import array
import unittest
import struct
import inspect
import warnings
from test.test_support import run_unittest, check_warnings, _check_py3k_warnings


from functools import wraps
from test.test_support import TestFailed, verbose, run_unittest

import sys
ISBIGENDIAN = sys.byteorder == "big"
IS32BIT = sys.maxsize == 0x7fffffff

testmod_filename = os.path.splitext(__file__)[0] + '.py'
try:
    import _struct
except ImportError:
    PY_STRUCT_RANGE_CHECKING = 0
    PY_STRUCT_OVERFLOW_MASKING = 1
    PY_STRUCT_FLOAT_COERCE = 2
else:
    PY_STRUCT_RANGE_CHECKING = getattr(_struct, '_PY_STRUCT_RANGE_CHECKING', 0)
    PY_STRUCT_OVERFLOW_MASKING = getattr(_struct, '_PY_STRUCT_OVERFLOW_MASKING', 0)
    PY_STRUCT_FLOAT_COERCE = getattr(_struct, '_PY_STRUCT_FLOAT_COERCE', 0)

def string_reverse(s):
    return "".join(reversed(s))

def bigendian_to_native(value):
    if ISBIGENDIAN:
        return value
    else:
        return string_reverse(value)

def with_warning_restore(func):
    @wraps(func)
    def decorator(*args, **kw):
        with warnings.catch_warnings():
            # We need this function to warn every time, so stick an
            # unqualifed 'always' at the head of the filter list
            warnings.simplefilter("always")
            warnings.filterwarnings("error", category=DeprecationWarning)
            return func(*args, **kw)
    return decorator

@with_warning_restore
def deprecated_err(func, *args):
    try:
        func(*args)
    except (struct.error, TypeError):
        pass
    except DeprecationWarning:
        if not PY_STRUCT_OVERFLOW_MASKING:
            raise TestFailed, "%s%s expected to raise DeprecationWarning" % (
                func.__name__, args)
    else:
        raise TestFailed, "%s%s did not raise error" % (
            func.__name__, args)


class StructTest(unittest.TestCase):

    @with_warning_restore
    def check_float_coerce(self, format, number):
        # SF bug 1530559. struct.pack raises TypeError where it used to convert.
        if PY_STRUCT_FLOAT_COERCE == 2:
            # Test for pre-2.5 struct module
            with check_warnings((".*integer argument expected, got float",
                                DeprecationWarning)) as w:
                packed = struct.pack(format, number)
                floored = struct.unpack(format, packed)[0]
            self.assertEqual(floored, int(number),
                             "did not correcly coerce float to int")
            return
        try:
            struct.pack(format, number)
        except (struct.error, TypeError):
            if PY_STRUCT_FLOAT_COERCE:
                self.fail("expected DeprecationWarning for float coerce")
        except DeprecationWarning:
            if not PY_STRUCT_FLOAT_COERCE:
                self.fail("expected to raise struct.error for float coerce")
        else:
            self.fail("did not raise error for float coerce")

    def test_isbigendian(self):
        self.assertEqual((struct.pack('=i', 1)[0] == chr(0)), ISBIGENDIAN)

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
        self.assertRaises(struct.error, struct.unpack, 'd', 'flap')
        s = struct.pack('ii', 1, 2)
        self.assertRaises(struct.error, struct.unpack, 'iii', s)
        self.assertRaises(struct.error, struct.unpack, 'i', s)

    def test_warnings_stacklevel(self):
        # Python versions between 2.6 and 2.6.5 were producing
        # warning messages at the wrong stacklevel.
        def inner(fn, *args):
            return inspect.currentframe().f_lineno, fn(*args)

        def check_warning_stacklevel(fn, *args):
            with warnings.catch_warnings(record=True) as w:
                # "always" to make sure __warningregistry__ isn't affected
                warnings.simplefilter("always")
                lineno, result = inner(fn, *args)
            for warn in w:
                self.assertEqual(warn.lineno, lineno)

        # out of range warnings
        check_warning_stacklevel(struct.pack, '<L', -1)
        check_warning_stacklevel(struct.pack, 'L', -1)
        check_warning_stacklevel(struct.pack, '<h', 65536)
        check_warning_stacklevel(struct.pack, '<l', 2**100)

        # float warnings
        check_warning_stacklevel(struct.pack, 'L', 3.1)

    def test_transitiveness(self):
        c = 'a'
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
            ('I', 70000000L, '\004,\035\200', '\200\035,\004', 0),
            ('I', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('l', 70000000, '\004,\035\200', '\200\035,\004', 0),
            ('l', -70000000, '\373\323\342\200', '\200\342\323\373', 0),
            ('L', 70000000L, '\004,\035\200', '\200\035,\004', 0),
            ('L', 0x100000000L-70000000, '\373\323\342\200', '\200\342\323\373', 0),
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
            for (xfmt, exp) in [('>'+fmt, big), ('!'+fmt, big), ('<'+fmt, lil),
                                ('='+fmt, ISBIGENDIAN and big or lil)]:
                res = struct.pack(xfmt, arg)
                self.assertEqual(res, exp)
                self.assertEqual(struct.calcsize(xfmt), len(res))
                rev = struct.unpack(xfmt, res)[0]
                if rev != arg:
                    self.assert_(asy)

    def test_native_qQ(self):
        # can't pack -1 as unsigned regardless
        self.assertRaises((struct.error, TypeError), struct.pack, "Q", -1)
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
            bytes = struct.calcsize('q')
            # The expected values here are in big-endian format, primarily
            # because I'm on a little-endian machine and so this is the
            # clearest way (for me) to force the code to get exercised.
            for format, input, expected in (
                    ('q', -1, '\xff' * bytes),
                    ('q', 0, '\x00' * bytes),
                    ('Q', 0, '\x00' * bytes),
                    ('q', 1L, '\x00' * (bytes-1) + '\x01'),
                    ('Q', (1L << (8*bytes))-1, '\xff' * bytes),
                    ('q', (1L << (8*bytes-1))-1, '\x7f' + '\xff' * (bytes - 1))):
                got = struct.pack(format, input)
                native_expected = bigendian_to_native(expected)
                self.assertEqual(got, native_expected)
                retrieved = struct.unpack(format, got)[0]
                self.assertEqual(retrieved, input)

    def test_standard_integers(self):
        # Standard integer tests (bBhHiIlLqQ).
        import binascii

        class IntTester(unittest.TestCase):

            # XXX Most std integer modes fail to test for out-of-range.
            # The "i" and "l" codes appear to range-check OK on 32-bit boxes, but
            # fail to check correctly on some 64-bit ones (Tru64 Unix + Compaq C
            # reported by Mark Favas).
            BUGGY_RANGE_CHECK = "bBhHiIlL"

            def __init__(self, formatpair, bytesize):
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
                self.unsigned_max = 2L**self.bitsize - 1
                self.signed_min = -(2L**(self.bitsize-1))
                self.signed_max = 2L**(self.bitsize-1) - 1

            def test_one(self, x, pack=struct.pack,
                                  unpack=struct.unpack,
                                  unhexlify=binascii.unhexlify):
                # Try signed.
                code = self.signed_code
                if self.signed_min <= x <= self.signed_max:
                    # Try big-endian.
                    expected = long(x)
                    if x < 0:
                        expected += 1L << self.bitsize
                        self.assert_(expected > 0)
                    expected = hex(expected)[2:-1] # chop "0x" and trailing 'L'
                    if len(expected) & 1:
                        expected = "0" + expected
                    expected = unhexlify(expected)
                    expected = "\x00" * (self.bytesize - len(expected)) + expected

                    # Pack work?
                    format = ">" + code
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError), unpack, format,
                                                                 '\x01' + got)
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
                    self.assertRaises((struct.error, TypeError), unpack, format,
                                                                 '\x01' + got)

                else:
                    # x is out of range -- verify pack realizes that.
                    if not PY_STRUCT_RANGE_CHECKING and code in self.BUGGY_RANGE_CHECK:
                        if verbose:
                            print "Skipping buggy range check for code", code
                    else:
                        deprecated_err(pack, ">" + code, x)
                        deprecated_err(pack, "<" + code, x)

                # Much the same for unsigned.
                code = self.unsigned_code
                if self.unsigned_min <= x <= self.unsigned_max:
                    # Try big-endian.
                    format = ">" + code
                    expected = long(x)
                    expected = hex(expected)[2:-1] # chop "0x" and trailing 'L'
                    if len(expected) & 1:
                        expected = "0" + expected
                    expected = unhexlify(expected)
                    expected = "\x00" * (self.bytesize - len(expected)) + expected

                    # Pack work?
                    got = pack(format, x)
                    self.assertEqual(got, expected)

                    # Unpack work?
                    retrieved = unpack(format, got)[0]
                    self.assertEqual(x, retrieved)

                    # Adding any byte should cause a "too big" error.
                    self.assertRaises((struct.error, TypeError), unpack, format,
                                                                 '\x01' + got)

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
                    self.assertRaises((struct.error, TypeError), unpack, format,
                                                                 '\x01' + got)

                else:
                    # x is out of range -- verify pack realizes that.
                    if not PY_STRUCT_RANGE_CHECKING and code in self.BUGGY_RANGE_CHECK:
                        if verbose:
                            print "Skipping buggy range check for code", code
                    else:
                        deprecated_err(pack, ">" + code, x)
                        deprecated_err(pack, "<" + code, x)

            def run(self):
                from random import randrange

                # Create all interesting powers of 2.
                values = []
                for exp in range(self.bitsize + 3):
                    values.append(1L << exp)

                # Add some random values.
                for i in range(self.bitsize):
                    val = 0L
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
                        for badobject in "a string", 3+42j, randrange:
                            self.assertRaises((struct.error, TypeError),
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
                ('p','abc', '\x00', ''),
                ('1p', 'abc', '\x00', ''),
                ('2p', 'abc', '\x01a', 'a'),
                ('3p', 'abc', '\x02ab', 'ab'),
                ('4p', 'abc', '\x03abc', 'abc'),
                ('5p', 'abc', '\x03abc\x00', 'abc'),
                ('6p', 'abc', '\x03abc\x00\x00', 'abc'),
                ('1000p', 'x'*1000, '\xff' + 'x'*999, 'x'*255)]:
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

    if PY_STRUCT_RANGE_CHECKING:
        def test_1229380(self):
            # SF bug 1229380. No struct.pack exception for some out of
            # range integers
            import sys
            for endian in ('', '>', '<'):
                for cls in (int, long):
                    for fmt in ('B', 'H', 'I', 'L'):
                        deprecated_err(struct.pack, endian + fmt, cls(-1))

                    deprecated_err(struct.pack, endian + 'B', cls(300))
                    deprecated_err(struct.pack, endian + 'H', cls(70000))

                deprecated_err(struct.pack, endian + 'I', sys.maxint * 4L)
                deprecated_err(struct.pack, endian + 'L', sys.maxint * 4L)

    def XXXtest_1530559(self):
        # XXX This is broken: see the bug report
        # SF bug 1530559. struct.pack raises TypeError where it used to convert.
        for endian in ('', '>', '<'):
            for fmt in ('B', 'H', 'I', 'L', 'b', 'h', 'i', 'l'):
                self.check_float_coerce(endian + fmt, 1.0)
                self.check_float_coerce(endian + fmt, 1.5)

    def test_issue4228(self):
        # Packing a long may yield either 32 or 64 bits
        with check_warnings(quiet=True):
            x = struct.pack('L', -1)[:4]
        self.assertEqual(x, '\xff'*4)

    def test_unpack_from(self, cls=str):
        data = cls('abcd01234')
        fmt = '4s'
        s = struct.Struct(fmt)

        self.assertEqual(s.unpack_from(data), ('abcd',))
        self.assertEqual(struct.unpack_from(fmt, data), ('abcd',))
        for i in xrange(6):
            self.assertEqual(s.unpack_from(data, i), (data[i:i+4],))
            self.assertEqual(struct.unpack_from(fmt, data, i), (data[i:i+4],))
        for i in xrange(6, len(data) + 1):
            self.assertRaises(struct.error, s.unpack_from, data, i)
            self.assertRaises(struct.error, struct.unpack_from, fmt, data, i)

    def test_pack_into(self):
        test_string = 'Reykjavik rocks, eow!'
        writable_buf = array.array('c', ' '*100)
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
        small_buf = array.array('c', ' '*10)
        self.assertRaises(struct.error, s.pack_into, small_buf, 0, test_string)
        self.assertRaises(struct.error, s.pack_into, small_buf, 2, test_string)

        # Test bogus offset (issue 3694)
        sb = small_buf
        self.assertRaises(TypeError, struct.pack_into, b'1', sb, None)

    def test_pack_into_fn(self):
        test_string = 'Reykjavik rocks, eow!'
        writable_buf = array.array('c', ' '*100)
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
        small_buf = array.array('c', ' '*10)
        self.assertRaises(struct.error, pack_into, small_buf, 0, test_string)
        self.assertRaises(struct.error, pack_into, small_buf, 2, test_string)

    def test_unpack_with_buffer(self):
        with _check_py3k_warnings(("buffer.. not supported in 3.x",
                                  DeprecationWarning)):
            # SF bug 1563759: struct.unpack doesn't support buffer protocol objects
            data1 = array.array('B', '\x12\x34\x56\x78')
            data2 = buffer('......\x12\x34\x56\x78......', 6, 4)
            for data in [data1, data2]:
                value, = struct.unpack('>I', data)
                self.assertEqual(value, 0x12345678)

            self.test_unpack_from(cls=buffer)

    def test_bool(self):
        for prefix in tuple("<>!=")+('',):
            false = (), [], [], '', 0
            true = [1], 'test', 5, -1, 0xffffffffL+1, 0xffffffff//2

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

            for c in '\x01\x7f\xff\x0f\xf0':
                self.assertTrue(struct.unpack('>?', c)[0])

    if IS32BIT:
        def test_crasher(self):
            self.assertRaises(MemoryError, struct.pack, "357913941c", "a")

    def test_count_overflow(self):
        hugecount = '{0}b'.format(sys.maxsize+1)
        self.assertRaises(struct.error, struct.calcsize, hugecount)

        hugecount2 = '{0}b{1}H'.format(sys.maxsize//2, sys.maxsize//2)
        self.assertRaises(struct.error, struct.calcsize, hugecount2)

def test_main():
    run_unittest(StructTest)

if __name__ == '__main__':
    test_main()
