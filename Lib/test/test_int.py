import sys

import unittest
from test import support

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
        (repr(sys.maxsize), sys.maxsize),
        ('  1x', ValueError),
        ('  1  ', 1),
        ('  1\02  ', ValueError),
        ('', ValueError),
        (' ', ValueError),
        ('  \t\t  ', ValueError),
        ("\u0200", ValueError)
]

class IntTestCases(unittest.TestCase):

    def test_basic(self):
        self.assertEqual(int(314), 314)
        self.assertEqual(int(3.14), 3)
        # Check that conversion from float truncates towards zero
        self.assertEqual(int(-3.14), -3)
        self.assertEqual(int(3.9), 3)
        self.assertEqual(int(-3.9), -3)
        self.assertEqual(int(3.5), 3)
        self.assertEqual(int(-3.5), -3)
        self.assertEqual(int("-3"), -3)
        self.assertEqual(int(" -3 "), -3)
        self.assertEqual(int("\N{EM SPACE}-3\N{EN SPACE}"), -3)
        # Different base:
        self.assertEqual(int("10",16), 16)
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
                    except ValueError:
                        pass

        s = repr(-1-sys.maxsize)
        x = int(s)
        self.assertEqual(x+1, -sys.maxsize)
        self.assertIsInstance(x, int)
        # should return int
        self.assertEqual(int(s[1:]), sys.maxsize+1)

        # should return int
        x = int(1e100)
        self.assertIsInstance(x, int)
        x = int(-1e100)
        self.assertIsInstance(x, int)


        # SF bug 434186:  0x80000000/2 != 0x80000000>>1.
        # Worked by accident in Windows release build, but failed in debug build.
        # Failed in all Linux builds.
        x = -1-sys.maxsize
        self.assertEqual(x >> 1, x//2)

        self.assertRaises(ValueError, int, '123\0')
        self.assertRaises(ValueError, int, '53', 40)

        # SF bug 1545497: embedded NULs were not detected with
        # explicit base
        self.assertRaises(ValueError, int, '123\0', 10)
        self.assertRaises(ValueError, int, '123\x00 245', 20)

        x = int('1' * 600)
        self.assertIsInstance(x, int)


        self.assertRaises(TypeError, int, 1, 12)

        self.assertEqual(int('0o123', 0), 83)
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

        self.assertEqual(int('100000000000000000000000000000000', 2), 4294967296)
        self.assertEqual(int('102002022201221111211', 3), 4294967296)
        self.assertEqual(int('10000000000000000', 4), 4294967296)
        self.assertEqual(int('32244002423141', 5), 4294967296)
        self.assertEqual(int('1550104015504', 6), 4294967296)
        self.assertEqual(int('211301422354', 7), 4294967296)
        self.assertEqual(int('40000000000', 8), 4294967296)
        self.assertEqual(int('12068657454', 9), 4294967296)
        self.assertEqual(int('4294967296', 10), 4294967296)
        self.assertEqual(int('1904440554', 11), 4294967296)
        self.assertEqual(int('9ba461594', 12), 4294967296)
        self.assertEqual(int('535a79889', 13), 4294967296)
        self.assertEqual(int('2ca5b7464', 14), 4294967296)
        self.assertEqual(int('1a20dcd81', 15), 4294967296)
        self.assertEqual(int('100000000', 16), 4294967296)
        self.assertEqual(int('a7ffda91', 17), 4294967296)
        self.assertEqual(int('704he7g4', 18), 4294967296)
        self.assertEqual(int('4f5aff66', 19), 4294967296)
        self.assertEqual(int('3723ai4g', 20), 4294967296)
        self.assertEqual(int('281d55i4', 21), 4294967296)
        self.assertEqual(int('1fj8b184', 22), 4294967296)
        self.assertEqual(int('1606k7ic', 23), 4294967296)
        self.assertEqual(int('mb994ag', 24), 4294967296)
        self.assertEqual(int('hek2mgl', 25), 4294967296)
        self.assertEqual(int('dnchbnm', 26), 4294967296)
        self.assertEqual(int('b28jpdm', 27), 4294967296)
        self.assertEqual(int('8pfgih4', 28), 4294967296)
        self.assertEqual(int('76beigg', 29), 4294967296)
        self.assertEqual(int('5qmcpqg', 30), 4294967296)
        self.assertEqual(int('4q0jto4', 31), 4294967296)
        self.assertEqual(int('4000000', 32), 4294967296)
        self.assertEqual(int('3aokq94', 33), 4294967296)
        self.assertEqual(int('2qhxjli', 34), 4294967296)
        self.assertEqual(int('2br45qb', 35), 4294967296)
        self.assertEqual(int('1z141z4', 36), 4294967296)

        # tests with base 0
        # this fails on 3.0, but in 2.x the old octal syntax is allowed
        self.assertEqual(int(' 0o123  ', 0), 83)
        self.assertEqual(int(' 0o123  ', 0), 83)
        self.assertEqual(int('000', 0), 0)
        self.assertEqual(int('0o123', 0), 83)
        self.assertEqual(int('0x123', 0), 291)
        self.assertEqual(int('0b100', 0), 4)
        self.assertEqual(int(' 0O123   ', 0), 83)
        self.assertEqual(int(' 0X123  ', 0), 291)
        self.assertEqual(int(' 0B100 ', 0), 4)

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
        self.assertEqual(int('100000000000000000000000000000001', 2), 4294967297)
        self.assertEqual(int('102002022201221111212', 3), 4294967297)
        self.assertEqual(int('10000000000000001', 4), 4294967297)
        self.assertEqual(int('32244002423142', 5), 4294967297)
        self.assertEqual(int('1550104015505', 6), 4294967297)
        self.assertEqual(int('211301422355', 7), 4294967297)
        self.assertEqual(int('40000000001', 8), 4294967297)
        self.assertEqual(int('12068657455', 9), 4294967297)
        self.assertEqual(int('4294967297', 10), 4294967297)
        self.assertEqual(int('1904440555', 11), 4294967297)
        self.assertEqual(int('9ba461595', 12), 4294967297)
        self.assertEqual(int('535a7988a', 13), 4294967297)
        self.assertEqual(int('2ca5b7465', 14), 4294967297)
        self.assertEqual(int('1a20dcd82', 15), 4294967297)
        self.assertEqual(int('100000001', 16), 4294967297)
        self.assertEqual(int('a7ffda92', 17), 4294967297)
        self.assertEqual(int('704he7g5', 18), 4294967297)
        self.assertEqual(int('4f5aff67', 19), 4294967297)
        self.assertEqual(int('3723ai4h', 20), 4294967297)
        self.assertEqual(int('281d55i5', 21), 4294967297)
        self.assertEqual(int('1fj8b185', 22), 4294967297)
        self.assertEqual(int('1606k7id', 23), 4294967297)
        self.assertEqual(int('mb994ah', 24), 4294967297)
        self.assertEqual(int('hek2mgm', 25), 4294967297)
        self.assertEqual(int('dnchbnn', 26), 4294967297)
        self.assertEqual(int('b28jpdn', 27), 4294967297)
        self.assertEqual(int('8pfgih5', 28), 4294967297)
        self.assertEqual(int('76beigh', 29), 4294967297)
        self.assertEqual(int('5qmcpqh', 30), 4294967297)
        self.assertEqual(int('4q0jto5', 31), 4294967297)
        self.assertEqual(int('4000001', 32), 4294967297)
        self.assertEqual(int('3aokq95', 33), 4294967297)
        self.assertEqual(int('2qhxjlj', 34), 4294967297)
        self.assertEqual(int('2br45qc', 35), 4294967297)
        self.assertEqual(int('1z141z5', 36), 4294967297)

    @support.cpython_only
    def test_small_ints(self):
        # Bug #3236: Return small longs from PyLong_FromString
        self.assertIs(int('10'), 10)
        self.assertIs(int('-1'), -1)
        self.assertIs(int(b'10'), 10)
        self.assertIs(int(b'-1'), -1)

    def test_no_args(self):
        self.assertEqual(int(), 0)

    def test_keyword_args(self):
        # Test invoking int() using keyword arguments.
        self.assertEqual(int(x=1.2), 1)
        self.assertEqual(int('100', base=2), 4)
        self.assertEqual(int(x='100', base=2), 4)
        self.assertRaises(TypeError, int, base=10)
        self.assertRaises(TypeError, int, base=0)

    def test_int_base_limits(self):
        """Testing the supported limits of the int() base parameter."""
        self.assertEqual(int('0', 5), 0)
        with self.assertRaises(ValueError):
            int('0', 1)
        with self.assertRaises(ValueError):
            int('0', 37)
        with self.assertRaises(ValueError):
            int('0', -909)  # An old magic value base from Python 2.
        with self.assertRaises(ValueError):
            int('0', base=0-(2**234))
        with self.assertRaises(ValueError):
            int('0', base=2**234)
        # Bases 2 through 36 are supported.
        for base in range(2,37):
            self.assertEqual(int('0', base=base), 0)

    def test_int_base_bad_types(self):
        """Not integer types are not valid bases; issue16772."""
        with self.assertRaises(TypeError):
            int('0', 5.5)
        with self.assertRaises(TypeError):
            int('0', 5.0)

    def test_int_base_indexable(self):
        class MyIndexable(object):
            def __init__(self, value):
                self.value = value
            def __index__(self):
                return self.value

        # Check out of range bases.
        for base in 2**100, -2**100, 1, 37:
            with self.assertRaises(ValueError):
                int('43', base)

        # Check in-range bases.
        self.assertEqual(int('101', base=MyIndexable(2)), 5)
        self.assertEqual(int('101', base=MyIndexable(10)), 101)
        self.assertEqual(int('101', base=MyIndexable(36)), 1 + 36**2)

    def test_non_numeric_input_types(self):
        # Test possible non-numeric types for the argument x, including
        # subclasses of the explicitly documented accepted types.
        class CustomStr(str): pass
        class CustomBytes(bytes): pass
        class CustomByteArray(bytearray): pass

        values = [b'100',
                  bytearray(b'100'),
                  CustomStr('100'),
                  CustomBytes(b'100'),
                  CustomByteArray(b'100')]

        for x in values:
            msg = 'x has type %s' % type(x).__name__
            self.assertEqual(int(x), 100, msg=msg)
            self.assertEqual(int(x, 2), 4, msg=msg)

    def test_string_float(self):
        self.assertRaises(ValueError, int, '1.2')

    def test_intconversion(self):
        # Test __int__()
        class ClassicMissingMethods:
            pass
        self.assertRaises(TypeError, int, ClassicMissingMethods())

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
                return 42

        class Foo5(int):
            def __int__(self):
                return 42.

        self.assertEqual(int(Foo0()), 42)
        self.assertEqual(int(Foo1()), 42)
        self.assertEqual(int(Foo2()), 42)
        self.assertEqual(int(Foo3()), 0)
        self.assertEqual(int(Foo4()), 42)
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

            class ExceptionalTrunc(base):
                def __trunc__(self):
                    1 / 0
            with self.assertRaises(ZeroDivisionError):
                int(ExceptionalTrunc())

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

                # Regression test for bugs.python.org/issue16060.
                class BadInt(trunc_result_base):
                    def __int__(self):
                        return 42.0

                class TruncReturnsBadInt(base):
                    def __trunc__(self):
                        return BadInt()

                with self.assertRaises(TypeError):
                    int(TruncReturnsBadInt())

    def test_error_message(self):
        testlist = ('\xbd', '123\xbd', '  123 456  ')
        for s in testlist:
            try:
                int(s)
            except ValueError as e:
                self.assertIn(s.strip(), e.args[0])
            else:
                self.fail("Expected int(%r) to raise a ValueError", s)

def test_main():
    support.run_unittest(IntTestCases)

if __name__ == "__main__":
    test_main()
