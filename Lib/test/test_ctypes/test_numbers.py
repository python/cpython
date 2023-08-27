import array
import struct
import sys
import unittest
from operator import truth
from ctypes import (byref, sizeof, alignment,
                    c_char, c_byte, c_ubyte, c_short, c_ushort, c_int, c_uint,
                    c_long, c_ulong, c_longlong, c_ulonglong,
                    c_float, c_double, c_longdouble, c_bool)


def valid_ranges(*types):
    # given a sequence of numeric types, collect their _type_
    # attribute, which is a single format character compatible with
    # the struct module, use the struct module to calculate the
    # minimum and maximum value allowed for this format.
    # Returns a list of (min, max) values.
    result = []
    for t in types:
        fmt = t._type_
        size = struct.calcsize(fmt)
        a = struct.unpack(fmt, (b"\x00"*32)[:size])[0]
        b = struct.unpack(fmt, (b"\xFF"*32)[:size])[0]
        c = struct.unpack(fmt, (b"\x7F"+b"\x00"*32)[:size])[0]
        d = struct.unpack(fmt, (b"\x80"+b"\xFF"*32)[:size])[0]
        result.append((min(a, b, c, d), max(a, b, c, d)))
    return result


ArgType = type(byref(c_int(0)))

unsigned_types = [c_ubyte, c_ushort, c_uint, c_ulong, c_ulonglong]
signed_types = [c_byte, c_short, c_int, c_long, c_longlong]
bool_types = [c_bool]
float_types = [c_double, c_float]

unsigned_ranges = valid_ranges(*unsigned_types)
signed_ranges = valid_ranges(*signed_types)
bool_values = [True, False, 0, 1, -1, 5000, 'test', [], [1]]


class NumberTestCase(unittest.TestCase):

    def test_default_init(self):
        # default values are set to zero
        for t in signed_types + unsigned_types + float_types:
            self.assertEqual(t().value, 0)

    def test_unsigned_values(self):
        # the value given to the constructor is available
        # as the 'value' attribute
        for t, (l, h) in zip(unsigned_types, unsigned_ranges):
            self.assertEqual(t(l).value, l)
            self.assertEqual(t(h).value, h)

    def test_signed_values(self):
        # see above
        for t, (l, h) in zip(signed_types, signed_ranges):
            self.assertEqual(t(l).value, l)
            self.assertEqual(t(h).value, h)

    def test_bool_values(self):
        for t, v in zip(bool_types, bool_values):
            self.assertEqual(t(v).value, truth(v))

    def test_typeerror(self):
        # Only numbers are allowed in the constructor,
        # otherwise TypeError is raised
        for t in signed_types + unsigned_types + float_types:
            self.assertRaises(TypeError, t, "")
            self.assertRaises(TypeError, t, None)

    def test_from_param(self):
        # the from_param class method attribute always
        # returns PyCArgObject instances
        for t in signed_types + unsigned_types + float_types:
            self.assertEqual(ArgType, type(t.from_param(0)))

    def test_byref(self):
        # calling byref returns also a PyCArgObject instance
        for t in signed_types + unsigned_types + float_types + bool_types:
            parm = byref(t())
            self.assertEqual(ArgType, type(parm))


    def test_floats(self):
        # c_float and c_double can be created from
        # Python int and float
        class FloatLike:
            def __float__(self):
                return 2.0
        f = FloatLike()
        for t in float_types:
            self.assertEqual(t(2.0).value, 2.0)
            self.assertEqual(t(2).value, 2.0)
            self.assertEqual(t(2).value, 2.0)
            self.assertEqual(t(f).value, 2.0)

    def test_integers(self):
        class FloatLike:
            def __float__(self):
                return 2.0
        f = FloatLike()
        class IntLike:
            def __int__(self):
                return 2
        d = IntLike()
        class IndexLike:
            def __index__(self):
                return 2
        i = IndexLike()
        # integers cannot be constructed from floats,
        # but from integer-like objects
        for t in signed_types + unsigned_types:
            self.assertRaises(TypeError, t, 3.14)
            self.assertRaises(TypeError, t, f)
            self.assertRaises(TypeError, t, d)
            self.assertEqual(t(i).value, 2)

    def test_sizes(self):
        for t in signed_types + unsigned_types + float_types + bool_types:
            try:
                size = struct.calcsize(t._type_)
            except struct.error:
                continue
            # sizeof of the type...
            self.assertEqual(sizeof(t), size)
            # and sizeof of an instance
            self.assertEqual(sizeof(t()), size)

    def test_alignments(self):
        for t in signed_types + unsigned_types + float_types:
            code = t._type_ # the typecode
            align = struct.calcsize("c%c" % code) - struct.calcsize(code)

            # alignment of the type...
            self.assertEqual((code, alignment(t)),
                                 (code, align))
            # and alignment of an instance
            self.assertEqual((code, alignment(t())),
                                 (code, align))

    def test_int_from_address(self):
        for t in signed_types + unsigned_types:
            # the array module doesn't support all format codes
            # (no 'q' or 'Q')
            try:
                array.array(t._type_)
            except ValueError:
                continue
            a = array.array(t._type_, [100])

            # v now is an integer at an 'external' memory location
            v = t.from_address(a.buffer_info()[0])
            self.assertEqual(v.value, a[0])
            self.assertEqual(type(v), t)

            # changing the value at the memory location changes v's value also
            a[0] = 42
            self.assertEqual(v.value, a[0])


    def test_float_from_address(self):
        for t in float_types:
            a = array.array(t._type_, [3.14])
            v = t.from_address(a.buffer_info()[0])
            self.assertEqual(v.value, a[0])
            self.assertIs(type(v), t)
            a[0] = 2.3456e17
            self.assertEqual(v.value, a[0])
            self.assertIs(type(v), t)

    def test_char_from_address(self):
        a = array.array('b', [0])
        a[0] = ord('x')
        v = c_char.from_address(a.buffer_info()[0])
        self.assertEqual(v.value, b'x')
        self.assertIs(type(v), c_char)

        a[0] = ord('?')
        self.assertEqual(v.value, b'?')

    def test_init(self):
        # c_int() can be initialized from Python's int, and c_int.
        # Not from c_long or so, which seems strange, abc should
        # probably be changed:
        self.assertRaises(TypeError, c_int, c_long(42))

    def test_float_overflow(self):
        big_int = int(sys.float_info.max) * 2
        for t in float_types + [c_longdouble]:
            self.assertRaises(OverflowError, t, big_int)
            if (hasattr(t, "__ctype_be__")):
                self.assertRaises(OverflowError, t.__ctype_be__, big_int)
            if (hasattr(t, "__ctype_le__")):
                self.assertRaises(OverflowError, t.__ctype_le__, big_int)


if __name__ == '__main__':
    unittest.main()
