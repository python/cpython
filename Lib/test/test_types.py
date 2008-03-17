# Python test set -- part 6, built-in types

from test.test_support import run_unittest, have_unicode
import unittest
import sys

class TypesTests(unittest.TestCase):

    def test_truth_values(self):
        if None: self.fail('None is true instead of false')
        if 0: self.fail('0 is true instead of false')
        if 0L: self.fail('0L is true instead of false')
        if 0.0: self.fail('0.0 is true instead of false')
        if '': self.fail('\'\' is true instead of false')
        if not 1: self.fail('1 is false instead of true')
        if not 1L: self.fail('1L is false instead of true')
        if not 1.0: self.fail('1.0 is false instead of true')
        if not 'x': self.fail('\'x\' is false instead of true')
        if not {'x': 1}: self.fail('{\'x\': 1} is false instead of true')
        def f(): pass
        class C: pass
        import sys
        x = C()
        if not f: self.fail('f is false instead of true')
        if not C: self.fail('C is false instead of true')
        if not sys: self.fail('sys is false instead of true')
        if not x: self.fail('x is false instead of true')

    def test_boolean_ops(self):
        if 0 or 0: self.fail('0 or 0 is true instead of false')
        if 1 and 1: pass
        else: self.fail('1 and 1 is false instead of true')
        if not 1: self.fail('not 1 is true instead of false')

    def test_comparisons(self):
        if 0 < 1 <= 1 == 1 >= 1 > 0 != 1: pass
        else: self.fail('int comparisons failed')
        if 0L < 1L <= 1L == 1L >= 1L > 0L != 1L: pass
        else: self.fail('long int comparisons failed')
        if 0.0 < 1.0 <= 1.0 == 1.0 >= 1.0 > 0.0 != 1.0: pass
        else: self.fail('float comparisons failed')
        if '' < 'a' <= 'a' == 'a' < 'abc' < 'abd' < 'b': pass
        else: self.fail('string comparisons failed')
        if None is None: pass
        else: self.fail('identity test failed')

    def test_float_constructor(self):
        self.assertRaises(ValueError, float, '')
        self.assertRaises(ValueError, float, '5\0')

    def test_zero_division(self):
        try: 5.0 / 0.0
        except ZeroDivisionError: pass
        else: self.fail("5.0 / 0.0 didn't raise ZeroDivisionError")

        try: 5.0 // 0.0
        except ZeroDivisionError: pass
        else: self.fail("5.0 // 0.0 didn't raise ZeroDivisionError")

        try: 5.0 % 0.0
        except ZeroDivisionError: pass
        else: self.fail("5.0 % 0.0 didn't raise ZeroDivisionError")

        try: 5 / 0L
        except ZeroDivisionError: pass
        else: self.fail("5 / 0L didn't raise ZeroDivisionError")

        try: 5 // 0L
        except ZeroDivisionError: pass
        else: self.fail("5 // 0L didn't raise ZeroDivisionError")

        try: 5 % 0L
        except ZeroDivisionError: pass
        else: self.fail("5 % 0L didn't raise ZeroDivisionError")

    def test_numeric_types(self):
        if 0 != 0L or 0 != 0.0 or 0L != 0.0: self.fail('mixed comparisons')
        if 1 != 1L or 1 != 1.0 or 1L != 1.0: self.fail('mixed comparisons')
        if -1 != -1L or -1 != -1.0 or -1L != -1.0:
            self.fail('int/long/float value not equal')
        # calling built-in types without argument must return 0
        if int() != 0: self.fail('int() does not return 0')
        if long() != 0L: self.fail('long() does not return 0L')
        if float() != 0.0: self.fail('float() does not return 0.0')
        if int(1.9) == 1 == int(1.1) and int(-1.1) == -1 == int(-1.9): pass
        else: self.fail('int() does not round properly')
        if long(1.9) == 1L == long(1.1) and long(-1.1) == -1L == long(-1.9): pass
        else: self.fail('long() does not round properly')
        if float(1) == 1.0 and float(-1) == -1.0 and float(0) == 0.0: pass
        else: self.fail('float() does not work properly')

    def test_float_to_string(self):
        def test(f, result):
            self.assertEqual(f.__format__('e'), result)
            self.assertEqual('%e' % f, result)

        # test all 2 digit exponents, both with __format__ and with
        #  '%' formatting
        for i in range(-99, 100):
            test(float('1.5e'+str(i)), '1.500000e{0:+03d}'.format(i))

        # test some 3 digit exponents
        self.assertEqual(1.5e100.__format__('e'), '1.500000e+100')
        self.assertEqual('%e' % 1.5e100, '1.500000e+100')

        self.assertEqual(1.5e101.__format__('e'), '1.500000e+101')
        self.assertEqual('%e' % 1.5e101, '1.500000e+101')

        self.assertEqual(1.5e-100.__format__('e'), '1.500000e-100')
        self.assertEqual('%e' % 1.5e-100, '1.500000e-100')

        self.assertEqual(1.5e-101.__format__('e'), '1.500000e-101')
        self.assertEqual('%e' % 1.5e-101, '1.500000e-101')

    def test_normal_integers(self):
        # Ensure the first 256 integers are shared
        a = 256
        b = 128*2
        if a is not b: self.fail('256 is not shared')
        if 12 + 24 != 36: self.fail('int op')
        if 12 + (-24) != -12: self.fail('int op')
        if (-12) + 24 != 12: self.fail('int op')
        if (-12) + (-24) != -36: self.fail('int op')
        if not 12 < 24: self.fail('int op')
        if not -24 < -12: self.fail('int op')
        # Test for a particular bug in integer multiply
        xsize, ysize, zsize = 238, 356, 4
        if not (xsize*ysize*zsize == zsize*xsize*ysize == 338912):
            self.fail('int mul commutativity')
        # And another.
        m = -sys.maxint - 1
        for divisor in 1, 2, 4, 8, 16, 32:
            j = m // divisor
            prod = divisor * j
            if prod != m:
                self.fail("%r * %r == %r != %r" % (divisor, j, prod, m))
            if type(prod) is not int:
                self.fail("expected type(prod) to be int, not %r" %
                                   type(prod))
        # Check for expected * overflow to long.
        for divisor in 1, 2, 4, 8, 16, 32:
            j = m // divisor - 1
            prod = divisor * j
            if type(prod) is not long:
                self.fail("expected type(%r) to be long, not %r" %
                                   (prod, type(prod)))
        # Check for expected * overflow to long.
        m = sys.maxint
        for divisor in 1, 2, 4, 8, 16, 32:
            j = m // divisor + 1
            prod = divisor * j
            if type(prod) is not long:
                self.fail("expected type(%r) to be long, not %r" %
                                   (prod, type(prod)))

    def test_long_integers(self):
        if 12L + 24L != 36L: self.fail('long op')
        if 12L + (-24L) != -12L: self.fail('long op')
        if (-12L) + 24L != 12L: self.fail('long op')
        if (-12L) + (-24L) != -36L: self.fail('long op')
        if not 12L < 24L: self.fail('long op')
        if not -24L < -12L: self.fail('long op')
        x = sys.maxint
        if int(long(x)) != x: self.fail('long op')
        try: y = int(long(x)+1L)
        except OverflowError: self.fail('long op')
        if not isinstance(y, long): self.fail('long op')
        x = -x
        if int(long(x)) != x: self.fail('long op')
        x = x-1
        if int(long(x)) != x: self.fail('long op')
        try: y = int(long(x)-1L)
        except OverflowError: self.fail('long op')
        if not isinstance(y, long): self.fail('long op')

        try: 5 << -5
        except ValueError: pass
        else: self.fail('int negative shift <<')

        try: 5L << -5L
        except ValueError: pass
        else: self.fail('long negative shift <<')

        try: 5 >> -5
        except ValueError: pass
        else: self.fail('int negative shift >>')

        try: 5L >> -5L
        except ValueError: pass
        else: self.fail('long negative shift >>')

    def test_floats(self):
        if 12.0 + 24.0 != 36.0: self.fail('float op')
        if 12.0 + (-24.0) != -12.0: self.fail('float op')
        if (-12.0) + 24.0 != 12.0: self.fail('float op')
        if (-12.0) + (-24.0) != -36.0: self.fail('float op')
        if not 12.0 < 24.0: self.fail('float op')
        if not -24.0 < -12.0: self.fail('float op')

    def test_strings(self):
        if len('') != 0: self.fail('len(\'\')')
        if len('a') != 1: self.fail('len(\'a\')')
        if len('abcdef') != 6: self.fail('len(\'abcdef\')')
        if 'xyz' + 'abcde' != 'xyzabcde': self.fail('string concatenation')
        if 'xyz'*3 != 'xyzxyzxyz': self.fail('string repetition *3')
        if 0*'abcde' != '': self.fail('string repetition 0*')
        if min('abc') != 'a' or max('abc') != 'c': self.fail('min/max string')
        if 'a' in 'abc' and 'b' in 'abc' and 'c' in 'abc' and 'd' not in 'abc': pass
        else: self.fail('in/not in string')
        x = 'x'*103
        if '%s!'%x != x+'!': self.fail('nasty string formatting bug')

        #extended slices for strings
        a = '0123456789'
        self.assertEqual(a[::], a)
        self.assertEqual(a[::2], '02468')
        self.assertEqual(a[1::2], '13579')
        self.assertEqual(a[::-1],'9876543210')
        self.assertEqual(a[::-2], '97531')
        self.assertEqual(a[3::-2], '31')
        self.assertEqual(a[-100:100:], a)
        self.assertEqual(a[100:-100:-1], a[::-1])
        self.assertEqual(a[-100L:100L:2L], '02468')

        if have_unicode:
            a = unicode('0123456789', 'ascii')
            self.assertEqual(a[::], a)
            self.assertEqual(a[::2], unicode('02468', 'ascii'))
            self.assertEqual(a[1::2], unicode('13579', 'ascii'))
            self.assertEqual(a[::-1], unicode('9876543210', 'ascii'))
            self.assertEqual(a[::-2], unicode('97531', 'ascii'))
            self.assertEqual(a[3::-2], unicode('31', 'ascii'))
            self.assertEqual(a[-100:100:], a)
            self.assertEqual(a[100:-100:-1], a[::-1])
            self.assertEqual(a[-100L:100L:2L], unicode('02468', 'ascii'))


    def test_type_function(self):
        self.assertRaises(TypeError, type, 1, 2)
        self.assertRaises(TypeError, type, 1, 2, 3, 4)

    def test_buffers(self):
        self.assertRaises(ValueError, buffer, 'asdf', -1)
        cmp(buffer("abc"), buffer("def")) # used to raise a warning: tp_compare didn't return -1, 0, or 1

        self.assertRaises(TypeError, buffer, None)

        a = buffer('asdf')
        hash(a)
        b = a * 5
        if a == b:
            self.fail('buffers should not be equal')
        if str(b) != ('asdf' * 5):
            self.fail('repeated buffer has wrong content')
        if str(a * 0) != '':
            self.fail('repeated buffer zero times has wrong content')
        if str(a + buffer('def')) != 'asdfdef':
            self.fail('concatenation of buffers yields wrong content')
        if str(buffer(a)) != 'asdf':
            self.fail('composing buffers failed')
        if str(buffer(a, 2)) != 'df':
            self.fail('specifying buffer offset failed')
        if str(buffer(a, 0, 2)) != 'as':
            self.fail('specifying buffer size failed')
        if str(buffer(a, 1, 2)) != 'sd':
            self.fail('specifying buffer offset and size failed')
        self.assertRaises(ValueError, buffer, buffer('asdf', 1), -1)
        if str(buffer(buffer('asdf', 0, 2), 0)) != 'as':
            self.fail('composing length-specified buffer failed')
        if str(buffer(buffer('asdf', 0, 2), 0, 5000)) != 'as':
            self.fail('composing length-specified buffer failed')
        if str(buffer(buffer('asdf', 0, 2), 0, -1)) != 'as':
            self.fail('composing length-specified buffer failed')
        if str(buffer(buffer('asdf', 0, 2), 1, 2)) != 's':
            self.fail('composing length-specified buffer failed')

        try: a[1] = 'g'
        except TypeError: pass
        else: self.fail("buffer assignment should raise TypeError")

        try: a[0:1] = 'g'
        except TypeError: pass
        else: self.fail("buffer slice assignment should raise TypeError")

        # array.array() returns an object that does not implement a char buffer,
        # something which int() uses for conversion.
        import array
        try: int(buffer(array.array('c')))
        except TypeError: pass
        else: self.fail("char buffer (at C level) not working")

    def test_int__format__(self):
        def test(i, format_spec, result):
            # just make sure I'm not accidentally checking longs
            assert type(i) == int
            assert type(format_spec) == str
            self.assertEqual(i.__format__(format_spec), result)
            self.assertEqual(i.__format__(unicode(format_spec)), result)

        test(123456789, 'd', '123456789')
        test(123456789, 'd', '123456789')

        test(1, 'c', '\01')

        # sign and aligning are interdependent
        test(1, "-", '1')
        test(-1, "-", '-1')
        test(1, "-3", '  1')
        test(-1, "-3", ' -1')
        test(1, "+3", ' +1')
        test(-1, "+3", ' -1')
        test(1, " 3", '  1')
        test(-1, " 3", ' -1')
        test(1, " ", ' 1')
        test(-1, " ", '-1')

        # hex
        test(3, "x", "3")
        test(3, "X", "3")
        test(1234, "x", "4d2")
        test(-1234, "x", "-4d2")
        test(1234, "8x", "     4d2")
        test(-1234, "8x", "    -4d2")
        test(1234, "x", "4d2")
        test(-1234, "x", "-4d2")
        test(-3, "x", "-3")
        test(-3, "X", "-3")
        test(int('be', 16), "x", "be")
        test(int('be', 16), "X", "BE")
        test(-int('be', 16), "x", "-be")
        test(-int('be', 16), "X", "-BE")

        # octal
        test(3, "o", "3")
        test(-3, "o", "-3")
        test(65, "o", "101")
        test(-65, "o", "-101")
        test(1234, "o", "2322")
        test(-1234, "o", "-2322")
        test(1234, "-o", "2322")
        test(-1234, "-o", "-2322")
        test(1234, " o", " 2322")
        test(-1234, " o", "-2322")
        test(1234, "+o", "+2322")
        test(-1234, "+o", "-2322")

        # binary
        test(3, "b", "11")
        test(-3, "b", "-11")
        test(1234, "b", "10011010010")
        test(-1234, "b", "-10011010010")
        test(1234, "-b", "10011010010")
        test(-1234, "-b", "-10011010010")
        test(1234, " b", " 10011010010")
        test(-1234, " b", "-10011010010")
        test(1234, "+b", "+10011010010")
        test(-1234, "+b", "-10011010010")

        # make sure these are errors

        # precision disallowed
        self.assertRaises(ValueError, 3 .__format__, "1.3")
        # sign not allowed with 'c'
        self.assertRaises(ValueError, 3 .__format__, "+c")
        # format spec must be string
        self.assertRaises(TypeError, 3 .__format__, None)
        self.assertRaises(TypeError, 3 .__format__, 0)

        # ensure that only int and float type specifiers work
        for format_spec in ([chr(x) for x in range(ord('a'), ord('z')+1)] +
                            [chr(x) for x in range(ord('A'), ord('Z')+1)]):
            if not format_spec in 'bcdoxXeEfFgGn%':
                self.assertRaises(ValueError, 0 .__format__, format_spec)
                self.assertRaises(ValueError, 1 .__format__, format_spec)
                self.assertRaises(ValueError, (-1) .__format__, format_spec)

        # ensure that float type specifiers work; format converts
        #  the int to a float
        for format_spec in 'eEfFgGn%':
            for value in [0, 1, -1, 100, -100, 1234567890, -1234567890]:
                self.assertEqual(value.__format__(format_spec),
                                 float(value).__format__(format_spec))

    def test_long__format__(self):
        def test(i, format_spec, result):
            # make sure we're not accidentally checking ints
            assert type(i) == long
            assert type(format_spec) == str
            self.assertEqual(i.__format__(format_spec), result)
            self.assertEqual(i.__format__(unicode(format_spec)), result)

        test(10**100, 'd', '1' + '0' * 100)
        test(10**100+100, 'd', '1' + '0' * 97 + '100')

        test(123456789L, 'd', '123456789')
        test(123456789L, 'd', '123456789')

        # sign and aligning are interdependent
        test(1L, "-", '1')
        test(-1L, "-", '-1')
        test(1L, "-3", '  1')
        test(-1L, "-3", ' -1')
        test(1L, "+3", ' +1')
        test(-1L, "+3", ' -1')
        test(1L, " 3", '  1')
        test(-1L, " 3", ' -1')
        test(1L, " ", ' 1')
        test(-1L, " ", '-1')

        test(1L, 'c', '\01')

        # hex
        test(3L, "x", "3")
        test(3L, "X", "3")
        test(1234L, "x", "4d2")
        test(-1234L, "x", "-4d2")
        test(1234L, "8x", "     4d2")
        test(-1234L, "8x", "    -4d2")
        test(1234L, "x", "4d2")
        test(-1234L, "x", "-4d2")
        test(-3L, "x", "-3")
        test(-3L, "X", "-3")
        test(long('be', 16), "x", "be")
        test(long('be', 16), "X", "BE")
        test(-long('be', 16), "x", "-be")
        test(-long('be', 16), "X", "-BE")

        # octal
        test(3L, "o", "3")
        test(-3L, "o", "-3")
        test(65L, "o", "101")
        test(-65L, "o", "-101")
        test(1234L, "o", "2322")
        test(-1234L, "o", "-2322")
        test(1234L, "-o", "2322")
        test(-1234L, "-o", "-2322")
        test(1234L, " o", " 2322")
        test(-1234L, " o", "-2322")
        test(1234L, "+o", "+2322")
        test(-1234L, "+o", "-2322")

        # binary
        test(3L, "b", "11")
        test(-3L, "b", "-11")
        test(1234L, "b", "10011010010")
        test(-1234L, "b", "-10011010010")
        test(1234L, "-b", "10011010010")
        test(-1234L, "-b", "-10011010010")
        test(1234L, " b", " 10011010010")
        test(-1234L, " b", "-10011010010")
        test(1234L, "+b", "+10011010010")
        test(-1234L, "+b", "-10011010010")

        # make sure these are errors

        # precision disallowed
        self.assertRaises(ValueError, 3L .__format__, "1.3")
        # sign not allowed with 'c'
        self.assertRaises(ValueError, 3L .__format__, "+c")
        # format spec must be string
        self.assertRaises(TypeError, 3L .__format__, None)
        self.assertRaises(TypeError, 3L .__format__, 0)

        # ensure that only int and float type specifiers work
        for format_spec in ([chr(x) for x in range(ord('a'), ord('z')+1)] +
                            [chr(x) for x in range(ord('A'), ord('Z')+1)]):
            if not format_spec in 'bcdoxXeEfFgGn%':
                self.assertRaises(ValueError, 0L .__format__, format_spec)
                self.assertRaises(ValueError, 1L .__format__, format_spec)
                self.assertRaises(ValueError, (-1L) .__format__, format_spec)

        # ensure that float type specifiers work; format converts
        #  the long to a float
        for format_spec in 'eEfFgGn%':
            for value in [0L, 1L, -1L, 100L, -100L, 1234567890L, -1234567890L]:
                self.assertEqual(value.__format__(format_spec),
                                 float(value).__format__(format_spec))

    def test_float__format__(self):
        # these should be rewritten to use both format(x, spec) and
        # x.__format__(spec)

        def test(f, format_spec, result):
            assert type(f) == float
            assert type(format_spec) == str
            self.assertEqual(f.__format__(format_spec), result)
            self.assertEqual(f.__format__(unicode(format_spec)), result)

        test(0.0, 'f', '0.000000')

        # the default is 'g', except for empty format spec
        test(0.0, '', '0.0')
        test(0.01, '', '0.01')
        test(0.01, 'g', '0.01')

        test( 1.0, ' g', ' 1')
        test(-1.0, ' g', '-1')
        test( 1.0, '+g', '+1')
        test(-1.0, '+g', '-1')
        test(1.1234e200, 'g', '1.1234e+200')
        test(1.1234e200, 'G', '1.1234E+200')


        test(1.0, 'f', '1.000000')

        test(-1.0, 'f', '-1.000000')

        test( 1.0, ' f', ' 1.000000')
        test(-1.0, ' f', '-1.000000')
        test( 1.0, '+f', '+1.000000')
        test(-1.0, '+f', '-1.000000')
        test(1.1234e90, 'f', '1.1234e+90')
        test(1.1234e90, 'F', '1.1234e+90')
        test(1.1234e200, 'f', '1.1234e+200')
        test(1.1234e200, 'F', '1.1234e+200')

        test( 1.0, 'e', '1.000000e+00')
        test(-1.0, 'e', '-1.000000e+00')
        test( 1.0, 'E', '1.000000E+00')
        test(-1.0, 'E', '-1.000000E+00')
        test(1.1234e20, 'e', '1.123400e+20')
        test(1.1234e20, 'E', '1.123400E+20')

        # No format code means use g, but must have a decimal
        # and a number after the decimal.  This is tricky, because
        # a totaly empty format specifier means something else.
        # So, just use a sign flag
        test(1e200, '+g', '+1e+200')
        test(1e200, '+', '+1.0e+200')
        test(1.1e200, '+g', '+1.1e+200')
        test(1.1e200, '+', '+1.1e+200')

        # % formatting
        test(-1.0, '%', '-100.000000%')

        # format spec must be string
        self.assertRaises(TypeError, 3.0.__format__, None)
        self.assertRaises(TypeError, 3.0.__format__, 0)

        # other format specifiers shouldn't work on floats,
        #  in particular int specifiers
        for format_spec in ([chr(x) for x in range(ord('a'), ord('z')+1)] +
                            [chr(x) for x in range(ord('A'), ord('Z')+1)]):
            if not format_spec in 'eEfFgGn%':
                self.assertRaises(ValueError, format, 0.0, format_spec)
                self.assertRaises(ValueError, format, 1.0, format_spec)
                self.assertRaises(ValueError, format, -1.0, format_spec)
                self.assertRaises(ValueError, format, 1e100, format_spec)
                self.assertRaises(ValueError, format, -1e100, format_spec)
                self.assertRaises(ValueError, format, 1e-100, format_spec)
                self.assertRaises(ValueError, format, -1e-100, format_spec)


def test_main():
    run_unittest(TypesTests)

if __name__ == '__main__':
    test_main()
