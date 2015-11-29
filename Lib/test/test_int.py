import sys

import unittest
from test import test_support
from test.test_support import run_unittest, have_unicode
import math

L = [
        ('0', 0),
        ('1', 1),
        ('9', 9),
        ('10', 10),
        ('99', 99),
        ('100', 100),
        ('314', 314),
        (' 314', 314),
        ('314 ', 314),
        ('  \t\t  314  \t\t  ', 314),
        (repr(sys.maxint), sys.maxint),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError)
]
if have_unicode:
    L += [
        (unicode('0'), 0),
        (unicode('1'), 1),
        (unicode('9'), 9),
        (unicode('10'), 10),
        (unicode('99'), 99),
        (unicode('100'), 100),
        (unicode('314'), 314),
        (unicode(' 314'), 314),
        (unicode('\u0663\u0661\u0664 ','raw-unicode-escape'), 314),
        (unicode('  \t\t  314  \t\t  '), 314),
        (unicode('  1x'), ValueError),
        (unicode('  1  '), 1),
        (unicode('  1\02  '), ValueError),
        (unicode(''), ValueError),
        (unicode(' '), ValueError),
        (unicode('  \t\t  '), ValueError),
        (unichr(0x200), ValueError),
]

class IntSubclass(int):
    pass

class IntLongCommonTests(object):

    """Mixin of test cases to share between both test_int and test_long."""

    # Change to int or long in the TestCase subclass.
    ntype = None

    def test_no_args(self):
        self.assertEqual(self.ntype(), 0)

    def test_keyword_args(self):
        # Test invoking constructor using keyword arguments.
        self.assertEqual(self.ntype(x=1.2), 1)
        self.assertEqual(self.ntype('100', base=2), 4)
        self.assertEqual(self.ntype(x='100', base=2), 4)
        self.assertRaises(TypeError, self.ntype, base=10)
        self.assertRaises(TypeError, self.ntype, base=0)

class IntTestCases(IntLongCommonTests, unittest.TestCase):

    ntype = int

    def test_basic(self):
        self.assertEqual(int(314), 314)
        self.assertEqual(int(3.14), 3)
        self.assertEqual(int(314L), 314)
        # Check that conversion from float truncates towards zero
        self.assertEqual(int(-3.14), -3)
        self.assertEqual(int(3.9), 3)
        self.assertEqual(int(-3.9), -3)
        self.assertEqual(int(3.5), 3)
        self.assertEqual(int(-3.5), -3)
        # Different base:
        self.assertEqual(int("10",16), 16L)
        if have_unicode:
            self.assertEqual(int(unicode("10"),16), 16L)
        # Test conversion from strings and various anomalies
        for s, v in L:
            for sign in "", "+", "-":
                for prefix in "", " ", "\t", "  \t\t  ":
                    ss = prefix + sign + s
                    vv = v
                    if sign == "-" and v is not ValueError:
                        vv = -v
                    try:
                        self.assertEqual(int(ss), vv)
                    except v:
                        pass

        s = repr(-1-sys.maxint)
        x = int(s)
        self.assertEqual(x+1, -sys.maxint)
        self.assertIsInstance(x, int)
        # should return long
        self.assertEqual(int(s[1:]), sys.maxint+1)

        # should return long
        x = int(1e100)
        self.assertIsInstance(x, long)
        x = int(-1e100)
        self.assertIsInstance(x, long)


        # SF bug 434186:  0x80000000/2 != 0x80000000>>1.
        # Worked by accident in Windows release build, but failed in debug build.
        # Failed in all Linux builds.
        x = -1-sys.maxint
        self.assertEqual(x >> 1, x//2)

        self.assertRaises(ValueError, int, '123\0')
        self.assertRaises(ValueError, int, '53', 40)

        # SF bug 1545497: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, int, '123\0', 10)
        self.assertRaises(ValueError, int, '123\x00 245', 20)

        x = int('1' * 600)
        self.assertIsInstance(x, long)

        if have_unicode:
            x = int(unichr(0x661) * 600)
            self.assertIsInstance(x, long)

        self.assertRaises(TypeError, int, 1, 12)

        self.assertEqual(int('0123', 0), 83)
        self.assertEqual(int('0x123', 16), 291)

        # Bug 1679: "0x" is not a valid hex literal
        self.assertRaises(ValueError, int, "0x", 16)
        self.assertRaises(ValueError, int, "0x", 0)

        self.assertRaises(ValueError, int, "0o", 8)
        self.assertRaises(ValueError, int, "0o", 0)

        self.assertRaises(ValueError, int, "0b", 2)
        self.assertRaises(ValueError, int, "0b", 0)


        # SF bug 1334662: int(string, base) wrong answers
        # Various representations of 2**32 evaluated to 0
        # rather than 2**32 in previous versions

        self.assertEqual(int('100000000000000000000000000000000', 2), 4294967296L)
        self.assertEqual(int('102002022201221111211', 3), 4294967296L)
        self.assertEqual(int('10000000000000000', 4), 4294967296L)
        self.assertEqual(int('32244002423141', 5), 4294967296L)
        self.assertEqual(int('1550104015504', 6), 4294967296L)
        self.assertEqual(int('211301422354', 7), 4294967296L)
        self.assertEqual(int('40000000000', 8), 4294967296L)
        self.assertEqual(int('12068657454', 9), 4294967296L)
        self.assertEqual(int('4294967296', 10), 4294967296L)
        self.assertEqual(int('1904440554', 11), 4294967296L)
        self.assertEqual(int('9ba461594', 12), 4294967296L)
        self.assertEqual(int('535a79889', 13), 4294967296L)
        self.assertEqual(int('2ca5b7464', 14), 4294967296L)
        self.assertEqual(int('1a20dcd81', 15), 4294967296L)
        self.assertEqual(int('100000000', 16), 4294967296L)
        self.assertEqual(int('a7ffda91', 17), 4294967296L)
        self.assertEqual(int('704he7g4', 18), 4294967296L)
        self.assertEqual(int('4f5aff66', 19), 4294967296L)
        self.assertEqual(int('3723ai4g', 20), 4294967296L)
        self.assertEqual(int('281d55i4', 21), 4294967296L)
        self.assertEqual(int('1fj8b184', 22), 4294967296L)
        self.assertEqual(int('1606k7ic', 23), 4294967296L)
        self.assertEqual(int('mb994ag', 24), 4294967296L)
        self.assertEqual(int('hek2mgl', 25), 4294967296L)
        self.assertEqual(int('dnchbnm', 26), 4294967296L)
        self.assertEqual(int('b28jpdm', 27), 4294967296L)
        self.assertEqual(int('8pfgih4', 28), 4294967296L)
        self.assertEqual(int('76beigg', 29), 4294967296L)
        self.assertEqual(int('5qmcpqg', 30), 4294967296L)
        self.assertEqual(int('4q0jto4', 31), 4294967296L)
        self.assertEqual(int('4000000', 32), 4294967296L)
        self.assertEqual(int('3aokq94', 33), 4294967296L)
        self.assertEqual(int('2qhxjli', 34), 4294967296L)
        self.assertEqual(int('2br45qb', 35), 4294967296L)
        self.assertEqual(int('1z141z4', 36), 4294967296L)

        # tests with base 0
        # this fails on 3.0, but in 2.x the old octal syntax is allowed
        self.assertEqual(int(' 0123  ', 0), 83)
        self.assertEqual(int(' 0123  ', 0), 83)
        self.assertEqual(int('000', 0), 0)
        self.assertEqual(int('0o123', 0), 83)
        self.assertEqual(int('0x123', 0), 291)
        self.assertEqual(int('0b100', 0), 4)
        self.assertEqual(int(' 0O123   ', 0), 83)
        self.assertEqual(int(' 0X123  ', 0), 291)
        self.assertEqual(int(' 0B100 ', 0), 4)
        self.assertEqual(int('0', 0), 0)
        self.assertEqual(int('+0', 0), 0)
        self.assertEqual(int('-0', 0), 0)
        self.assertEqual(int('00', 0), 0)
        self.assertRaises(ValueError, int, '08', 0)
        self.assertRaises(ValueError, int, '-012395', 0)

        # without base still base 10
        self.assertEqual(int('0123'), 123)
        self.assertEqual(int('0123', 10), 123)

        # tests with prefix and base != 0
        self.assertEqual(int('0x123', 16), 291)
        self.assertEqual(int('0o123', 8), 83)
        self.assertEqual(int('0b100', 2), 4)
        self.assertEqual(int('0X123', 16), 291)
        self.assertEqual(int('0O123', 8), 83)
        self.assertEqual(int('0B100', 2), 4)

        # the code has special checks for the first character after the
        #  type prefix
        self.assertRaises(ValueError, int, '0b2', 2)
        self.assertRaises(ValueError, int, '0b02', 2)
        self.assertRaises(ValueError, int, '0B2', 2)
        self.assertRaises(ValueError, int, '0B02', 2)
        self.assertRaises(ValueError, int, '0o8', 8)
        self.assertRaises(ValueError, int, '0o08', 8)
        self.assertRaises(ValueError, int, '0O8', 8)
        self.assertRaises(ValueError, int, '0O08', 8)
        self.assertRaises(ValueError, int, '0xg', 16)
        self.assertRaises(ValueError, int, '0x0g', 16)
        self.assertRaises(ValueError, int, '0Xg', 16)
        self.assertRaises(ValueError, int, '0X0g', 16)

        # SF bug 1334662: int(string, base) wrong answers
        # Checks for proper evaluation of 2**32 + 1
        self.assertEqual(int('100000000000000000000000000000001', 2), 4294967297L)
        self.assertEqual(int('102002022201221111212', 3), 4294967297L)
        self.assertEqual(int('10000000000000001', 4), 4294967297L)
        self.assertEqual(int('32244002423142', 5), 4294967297L)
        self.assertEqual(int('1550104015505', 6), 4294967297L)
        self.assertEqual(int('211301422355', 7), 4294967297L)
        self.assertEqual(int('40000000001', 8), 4294967297L)
        self.assertEqual(int('12068657455', 9), 4294967297L)
        self.assertEqual(int('4294967297', 10), 4294967297L)
        self.assertEqual(int('1904440555', 11), 4294967297L)
        self.assertEqual(int('9ba461595', 12), 4294967297L)
        self.assertEqual(int('535a7988a', 13), 4294967297L)
        self.assertEqual(int('2ca5b7465', 14), 4294967297L)
        self.assertEqual(int('1a20dcd82', 15), 4294967297L)
        self.assertEqual(int('100000001', 16), 4294967297L)
        self.assertEqual(int('a7ffda92', 17), 4294967297L)
        self.assertEqual(int('704he7g5', 18), 4294967297L)
        self.assertEqual(int('4f5aff67', 19), 4294967297L)
        self.assertEqual(int('3723ai4h', 20), 4294967297L)
        self.assertEqual(int('281d55i5', 21), 4294967297L)
        self.assertEqual(int('1fj8b185', 22), 4294967297L)
        self.assertEqual(int('1606k7id', 23), 4294967297L)
        self.assertEqual(int('mb994ah', 24), 4294967297L)
        self.assertEqual(int('hek2mgm', 25), 4294967297L)
        self.assertEqual(int('dnchbnn', 26), 4294967297L)
        self.assertEqual(int('b28jpdn', 27), 4294967297L)
        self.assertEqual(int('8pfgih5', 28), 4294967297L)
        self.assertEqual(int('76beigh', 29), 4294967297L)
        self.assertEqual(int('5qmcpqh', 30), 4294967297L)
        self.assertEqual(int('4q0jto5', 31), 4294967297L)
        self.assertEqual(int('4000001', 32), 4294967297L)
        self.assertEqual(int('3aokq95', 33), 4294967297L)
        self.assertEqual(int('2qhxjlj', 34), 4294967297L)
        self.assertEqual(int('2br45qc', 35), 4294967297L)
        self.assertEqual(int('1z141z5', 36), 4294967297L)

    def test_bit_length(self):
        tiny = 1e-10
        for x in xrange(-65000, 65000):
            k = x.bit_length()
            # Check equivalence with Python version
            self.assertEqual(k, len(bin(x).lstrip('-0b')))
            # Behaviour as specified in the docs
            if x != 0:
                self.assertTrue(2**(k-1) <= abs(x) < 2**k)
            else:
                self.assertEqual(k, 0)
            # Alternative definition: x.bit_length() == 1 + floor(log_2(x))
            if x != 0:
                # When x is an exact power of 2, numeric errors can
                # cause floor(log(x)/log(2)) to be one too small; for
                # small x this can be fixed by adding a small quantity
                # to the quotient before taking the floor.
                self.assertEqual(k, 1 + math.floor(
                        math.log(abs(x))/math.log(2) + tiny))

        self.assertEqual((0).bit_length(), 0)
        self.assertEqual((1).bit_length(), 1)
        self.assertEqual((-1).bit_length(), 1)
        self.assertEqual((2).bit_length(), 2)
        self.assertEqual((-2).bit_length(), 2)
        for i in [2, 3, 15, 16, 17, 31, 32, 33, 63, 64]:
            a = 2**i
            self.assertEqual((a-1).bit_length(), i)
            self.assertEqual((1-a).bit_length(), i)
            self.assertEqual((a).bit_length(), i+1)
            self.assertEqual((-a).bit_length(), i+1)
            self.assertEqual((a+1).bit_length(), i+1)
            self.assertEqual((-a-1).bit_length(), i+1)

    @unittest.skipUnless(float.__getformat__("double").startswith("IEEE"),
                         "test requires IEEE 754 doubles")
    def test_float_conversion(self):
        # values exactly representable as floats
        exact_values = [-2, -1, 0, 1, 2, 2**52, 2**53-1, 2**53, 2**53+2,
                         2**53+4, 2**54-4, 2**54-2, 2**63, -2**63, 2**64,
                         -2**64, 10**20, 10**21, 10**22]
        for value in exact_values:
            self.assertEqual(int(float(int(value))), value)

        # test round-half-to-even
        self.assertEqual(int(float(2**53+1)), 2**53)
        self.assertEqual(int(float(2**53+2)), 2**53+2)
        self.assertEqual(int(float(2**53+3)), 2**53+4)
        self.assertEqual(int(float(2**53+5)), 2**53+4)
        self.assertEqual(int(float(2**53+6)), 2**53+6)
        self.assertEqual(int(float(2**53+7)), 2**53+8)

        self.assertEqual(int(float(-2**53-1)), -2**53)
        self.assertEqual(int(float(-2**53-2)), -2**53-2)
        self.assertEqual(int(float(-2**53-3)), -2**53-4)
        self.assertEqual(int(float(-2**53-5)), -2**53-4)
        self.assertEqual(int(float(-2**53-6)), -2**53-6)
        self.assertEqual(int(float(-2**53-7)), -2**53-8)

        self.assertEqual(int(float(2**54-2)), 2**54-2)
        self.assertEqual(int(float(2**54-1)), 2**54)
        self.assertEqual(int(float(2**54+2)), 2**54)
        self.assertEqual(int(float(2**54+3)), 2**54+4)
        self.assertEqual(int(float(2**54+5)), 2**54+4)
        self.assertEqual(int(float(2**54+6)), 2**54+8)
        self.assertEqual(int(float(2**54+10)), 2**54+8)
        self.assertEqual(int(float(2**54+11)), 2**54+12)

    def test_valid_non_numeric_input_types_for_x(self):
        # Test possible valid non-numeric types for x, including subclasses
        # of the allowed built-in types.
        class CustomStr(str): pass
        class CustomByteArray(bytearray): pass
        factories = [str, bytearray, CustomStr, CustomByteArray, buffer]

        if have_unicode:
            class CustomUnicode(unicode): pass
            factories += [unicode, CustomUnicode]

        for f in factories:
            with test_support.check_py3k_warnings(quiet=True):
                x = f('100')
            msg = 'x has value %s and type %s' % (x, type(x).__name__)
            try:
                self.assertEqual(int(x), 100, msg=msg)
                if isinstance(x, basestring):
                    self.assertEqual(int(x, 2), 4, msg=msg)
            except TypeError, err:
                raise AssertionError('For %s got TypeError: %s' %
                                     (type(x).__name__, err))
            if not isinstance(x, basestring):
                errmsg = "can't convert non-string"
                with self.assertRaisesRegexp(TypeError, errmsg, msg=msg):
                    int(x, 2)
            errmsg = 'invalid literal'
            with self.assertRaisesRegexp(ValueError, errmsg, msg=msg), \
                 test_support.check_py3k_warnings(quiet=True):
                int(f('A' * 0x10))

    def test_int_buffer(self):
        with test_support.check_py3k_warnings():
            self.assertEqual(int(buffer('123', 1, 2)), 23)
            self.assertEqual(int(buffer('123\x00', 1, 2)), 23)
            self.assertEqual(int(buffer('123 ', 1, 2)), 23)
            self.assertEqual(int(buffer('123A', 1, 2)), 23)
            self.assertEqual(int(buffer('1234', 1, 2)), 23)

    def test_error_on_string_float_for_x(self):
        self.assertRaises(ValueError, int, '1.2')

    def test_error_on_bytearray_for_x(self):
        self.assertRaises(TypeError, int, bytearray('100'), 2)

    def test_error_on_invalid_int_bases(self):
        for base in [-1, 1, 1000]:
            self.assertRaises(ValueError, int, '100', base)

    def test_error_on_string_base(self):
        self.assertRaises(TypeError, int, 100, base='foo')

    @test_support.cpython_only
    def test_small_ints(self):
        self.assertIs(int('10'), 10)
        self.assertIs(int('-1'), -1)
        if have_unicode:
            self.assertIs(int(u'10'), 10)
            self.assertIs(int(u'-1'), -1)

    def test_intconversion(self):
        # Test __int__()
        class ClassicMissingMethods:
            pass
        self.assertRaises(AttributeError, int, ClassicMissingMethods())

        class MissingMethods(object):
            pass
        self.assertRaises(TypeError, int, MissingMethods())

        class Foo0:
            def __int__(self):
                return 42

        class Foo1(object):
            def __int__(self):
                return 42

        class Foo2(int):
            def __int__(self):
                return 42

        class Foo3(int):
            def __int__(self):
                return self

        class Foo4(int):
            def __int__(self):
                return 42L

        class Foo5(int):
            def __int__(self):
                return 42.

        self.assertEqual(int(Foo0()), 42)
        self.assertEqual(int(Foo1()), 42)
        self.assertEqual(int(Foo2()), 42)
        self.assertEqual(int(Foo3()), 0)
        self.assertEqual(int(Foo4()), 42L)
        self.assertRaises(TypeError, int, Foo5())

        class Classic:
            pass
        for base in (object, Classic):
            class IntOverridesTrunc(base):
                def __int__(self):
                    return 42
                def __trunc__(self):
                    return -12
            self.assertEqual(int(IntOverridesTrunc()), 42)

            class JustTrunc(base):
                def __trunc__(self):
                    return 42
            self.assertEqual(int(JustTrunc()), 42)

            for trunc_result_base in (object, Classic):
                class Integral(trunc_result_base):
                    def __int__(self):
                        return 42

                class TruncReturnsNonInt(base):
                    def __trunc__(self):
                        return Integral()
                self.assertEqual(int(TruncReturnsNonInt()), 42)

                class NonIntegral(trunc_result_base):
                    def __trunc__(self):
                        # Check that we avoid infinite recursion.
                        return NonIntegral()

                class TruncReturnsNonIntegral(base):
                    def __trunc__(self):
                        return NonIntegral()
                try:
                    int(TruncReturnsNonIntegral())
                except TypeError as e:
                    self.assertEqual(str(e),
                                      "__trunc__ returned non-Integral"
                                      " (type NonIntegral)")
                else:
                    self.fail("Failed to raise TypeError with %s" %
                              ((base, trunc_result_base),))

                class TruncReturnsIntSubclass(base):
                    def __trunc__(self):
                        return True
                good_int = TruncReturnsIntSubclass()
                n = int(good_int)
                self.assertEqual(n, 1)
                self.assertIs(type(n), bool)
                n = IntSubclass(good_int)
                self.assertEqual(n, 1)
                self.assertIs(type(n), IntSubclass)


def test_main():
    run_unittest(IntTestCases)

if __name__ == "__main__":
    test_main()
