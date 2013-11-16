from ctypes import *
import unittest
import struct

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
        a = struct.unpack(fmt, ("\x00"*32)[:size])[0]
        b = struct.unpack(fmt, ("\xFF"*32)[:size])[0]
        c = struct.unpack(fmt, ("\x7F"+"\x00"*32)[:size])[0]
        d = struct.unpack(fmt, ("\x80"+"\xFF"*32)[:size])[0]
        result.append((min(a, b, c, d), max(a, b, c, d)))
    return result

ArgType = type(byref(c_int(0)))

unsigned_types = [c_ubyte, c_ushort, c_uint, c_ulong]
signed_types = [c_byte, c_short, c_int, c_long, c_longlong]

bool_types = []

float_types = [c_double, c_float]

try:
    c_ulonglong
    c_longlong
except NameError:
    pass
else:
    unsigned_types.append(c_ulonglong)
    signed_types.append(c_longlong)

try:
    c_bool
except NameError:
    pass
else:
    bool_types.append(c_bool)

unsigned_ranges = valid_ranges(*unsigned_types)
signed_ranges = valid_ranges(*signed_types)
bool_values = [True, False, 0, 1, -1, 5000, 'test', [], [1]]

################################################################

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
        from operator import truth
        for t, v in zip(bool_types, bool_values):
            self.assertEqual(t(v).value, truth(v))

    def test_typeerror(self):
        # Only numbers are allowed in the contructor,
        # otherwise TypeError is raised
        for t in signed_types + unsigned_types + float_types:
            self.assertRaises(TypeError, t, "")
            self.assertRaises(TypeError, t, None)

##    def test_valid_ranges(self):
##        # invalid values of the correct type
##        # raise ValueError (not OverflowError)
##        for t, (l, h) in zip(unsigned_types, unsigned_ranges):
##            self.assertRaises(ValueError, t, l-1)
##            self.assertRaises(ValueError, t, h+1)

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
        # Python int, long and float
        class FloatLike(object):
            def __float__(self):
                return 2.0
        f = FloatLike()
        for t in float_types:
            self.assertEqual(t(2.0).value, 2.0)
            self.assertEqual(t(2).value, 2.0)
            self.assertEqual(t(2L).value, 2.0)
            self.assertEqual(t(f).value, 2.0)

    def test_integers(self):
        class FloatLike(object):
            def __float__(self):
                return 2.0
        f = FloatLike()
        class IntLike(object):
            def __int__(self):
                return 2
        i = IntLike()
        # integers cannot be constructed from floats,
        # but from integer-like objects
        for t in signed_types + unsigned_types:
            self.assertRaises(TypeError, t, 3.14)
            self.assertRaises(TypeError, t, f)
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
        from array import array
        for t in signed_types + unsigned_types:
            # the array module doesn't support all format codes
            # (no 'q' or 'Q')
            try:
                array(t._type_)
            except ValueError:
                continue
            a = array(t._type_, [100])

            # v now is an integer at an 'external' memory location
            v = t.from_address(a.buffer_info()[0])
            self.assertEqual(v.value, a[0])
            self.assertEqual(type(v), t)

            # changing the value at the memory location changes v's value also
            a[0] = 42
            self.assertEqual(v.value, a[0])


    def test_float_from_address(self):
        from array import array
        for t in float_types:
            a = array(t._type_, [3.14])
            v = t.from_address(a.buffer_info()[0])
            self.assertEqual(v.value, a[0])
            self.assertIs(type(v), t)
            a[0] = 2.3456e17
            self.assertEqual(v.value, a[0])
            self.assertIs(type(v), t)

    def test_char_from_address(self):
        from ctypes import c_char
        from array import array

        a = array('c', 'x')
        v = c_char.from_address(a.buffer_info()[0])
        self.assertEqual(v.value, a[0])
        self.assertIs(type(v), c_char)

        a[0] = '?'
        self.assertEqual(v.value, a[0])

    # array does not support c_bool / 't'
    # def test_bool_from_address(self):
    #     from ctypes import c_bool
    #     from array import array
    #     a = array(c_bool._type_, [True])
    #     v = t.from_address(a.buffer_info()[0])
    #     self.assertEqual(v.value, a[0])
    #     self.assertEqual(type(v) is t)
    #     a[0] = False
    #     self.assertEqual(v.value, a[0])
    #     self.assertEqual(type(v) is t)

    def test_init(self):
        # c_int() can be initialized from Python's int, and c_int.
        # Not from c_long or so, which seems strange, abc should
        # probably be changed:
        self.assertRaises(TypeError, c_int, c_long(42))

    def test_float_overflow(self):
        import sys
        big_int = int(sys.float_info.max) * 2
        for t in float_types + [c_longdouble]:
            self.assertRaises(OverflowError, t, big_int)
            if (hasattr(t, "__ctype_be__")):
                self.assertRaises(OverflowError, t.__ctype_be__, big_int)
            if (hasattr(t, "__ctype_le__")):
                self.assertRaises(OverflowError, t.__ctype_le__, big_int)

##    def test_perf(self):
##        check_perf()

from ctypes import _SimpleCData
class c_int_S(_SimpleCData):
    _type_ = "i"
    __slots__ = []

def run_test(rep, msg, func, arg=None):
##    items = [None] * rep
    items = range(rep)
    from time import clock
    if arg is not None:
        start = clock()
        for i in items:
            func(arg); func(arg); func(arg); func(arg); func(arg)
        stop = clock()
    else:
        start = clock()
        for i in items:
            func(); func(); func(); func(); func()
        stop = clock()
    print "%15s: %.2f us" % (msg, ((stop-start)*1e6/5/rep))

def check_perf():
    # Construct 5 objects
    from ctypes import c_int

    REP = 200000

    run_test(REP, "int()", int)
    run_test(REP, "int(999)", int)
    run_test(REP, "c_int()", c_int)
    run_test(REP, "c_int(999)", c_int)
    run_test(REP, "c_int_S()", c_int_S)
    run_test(REP, "c_int_S(999)", c_int_S)

# Python 2.3 -OO, win2k, P4 700 MHz:
#
#          int(): 0.87 us
#       int(999): 0.87 us
#        c_int(): 3.35 us
#     c_int(999): 3.34 us
#      c_int_S(): 3.23 us
#   c_int_S(999): 3.24 us

# Python 2.2 -OO, win2k, P4 700 MHz:
#
#          int(): 0.89 us
#       int(999): 0.89 us
#        c_int(): 9.99 us
#     c_int(999): 10.02 us
#      c_int_S(): 9.87 us
#   c_int_S(999): 9.85 us

if __name__ == '__main__':
##    check_perf()
    unittest.main()
