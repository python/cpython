
import unittest, struct
import os
from test import test_support
import math
from math import isinf, isnan, copysign, ldexp
import operator
import random, fractions

INF = float("inf")
NAN = float("nan")

class GeneralFloatCases(unittest.TestCase):

    def test_float(self):
        self.assertEqual(float(3.14), 3.14)
        self.assertEqual(float(314), 314.0)
        self.assertEqual(float(314L), 314.0)
        self.assertEqual(float("  3.14  "), 3.14)
        self.assertRaises(ValueError, float, "  0x3.1  ")
        self.assertRaises(ValueError, float, "  -0x3.p-1  ")
        self.assertRaises(ValueError, float, "  +0x3.p-1  ")
        self.assertRaises(ValueError, float, "++3.14")
        self.assertRaises(ValueError, float, "+-3.14")
        self.assertRaises(ValueError, float, "-+3.14")
        self.assertRaises(ValueError, float, "--3.14")
        if test_support.have_unicode:
            self.assertEqual(float(unicode("  3.14  ")), 3.14)
            self.assertEqual(float(unicode("  \u0663.\u0661\u0664  ",'raw-unicode-escape')), 3.14)
            # Implementation limitation in PyFloat_FromString()
            self.assertRaises(ValueError, float, unicode("1"*10000))

    @test_support.run_with_locale('LC_NUMERIC', 'fr_FR', 'de_DE')
    def test_float_with_comma(self):
        # set locale to something that doesn't use '.' for the decimal point
        # float must not accept the locale specific decimal point but
        # it still has to accept the normal python syntac
        import locale
        if not locale.localeconv()['decimal_point'] == ',':
            return

        self.assertEqual(float("  3.14  "), 3.14)
        self.assertEqual(float("+3.14  "), 3.14)
        self.assertEqual(float("-3.14  "), -3.14)
        self.assertEqual(float(".14  "), .14)
        self.assertEqual(float("3.  "), 3.0)
        self.assertEqual(float("3.e3  "), 3000.0)
        self.assertEqual(float("3.2e3  "), 3200.0)
        self.assertEqual(float("2.5e-1  "), 0.25)
        self.assertEqual(float("5e-1"), 0.5)
        self.assertRaises(ValueError, float, "  3,14  ")
        self.assertRaises(ValueError, float, "  +3,14  ")
        self.assertRaises(ValueError, float, "  -3,14  ")
        self.assertRaises(ValueError, float, "  0x3.1  ")
        self.assertRaises(ValueError, float, "  -0x3.p-1  ")
        self.assertRaises(ValueError, float, "  +0x3.p-1  ")
        self.assertEqual(float("  25.e-1  "), 2.5)
        self.assertEqual(test_support.fcmp(float("  .25e-1  "), .025), 0)

    def test_floatconversion(self):
        # Make sure that calls to __float__() work properly
        class Foo0:
            def __float__(self):
                return 42.

        class Foo1(object):
            def __float__(self):
                return 42.

        class Foo2(float):
            def __float__(self):
                return 42.

        class Foo3(float):
            def __new__(cls, value=0.):
                return float.__new__(cls, 2*value)

            def __float__(self):
                return self

        class Foo4(float):
            def __float__(self):
                return 42

        # Issue 5759: __float__ not called on str subclasses (though it is on
        # unicode subclasses).
        class FooStr(str):
            def __float__(self):
                return float(str(self)) + 1

        class FooUnicode(unicode):
            def __float__(self):
                return float(unicode(self)) + 1

        self.assertAlmostEqual(float(Foo0()), 42.)
        self.assertAlmostEqual(float(Foo1()), 42.)
        self.assertAlmostEqual(float(Foo2()), 42.)
        self.assertAlmostEqual(float(Foo3(21)), 42.)
        self.assertRaises(TypeError, float, Foo4(42))
        self.assertAlmostEqual(float(FooUnicode('8')), 9.)
        self.assertAlmostEqual(float(FooStr('8')), 9.)

    def test_floatasratio(self):
        for f, ratio in [
                (0.875, (7, 8)),
                (-0.875, (-7, 8)),
                (0.0, (0, 1)),
                (11.5, (23, 2)),
            ]:
            self.assertEqual(f.as_integer_ratio(), ratio)

        for i in range(10000):
            f = random.random()
            f *= 10 ** random.randint(-100, 100)
            n, d = f.as_integer_ratio()
            self.assertEqual(float(n).__truediv__(d), f)

        R = fractions.Fraction
        self.assertEqual(R(0, 1),
                         R(*float(0.0).as_integer_ratio()))
        self.assertEqual(R(5, 2),
                         R(*float(2.5).as_integer_ratio()))
        self.assertEqual(R(1, 2),
                         R(*float(0.5).as_integer_ratio()))
        self.assertEqual(R(4728779608739021, 2251799813685248),
                         R(*float(2.1).as_integer_ratio()))
        self.assertEqual(R(-4728779608739021, 2251799813685248),
                         R(*float(-2.1).as_integer_ratio()))
        self.assertEqual(R(-2100, 1),
                         R(*float(-2100.0).as_integer_ratio()))

        self.assertRaises(OverflowError, float('inf').as_integer_ratio)
        self.assertRaises(OverflowError, float('-inf').as_integer_ratio)
        self.assertRaises(ValueError, float('nan').as_integer_ratio)

class FormatFunctionsTestCase(unittest.TestCase):

    def setUp(self):
        self.save_formats = {'double':float.__getformat__('double'),
                             'float':float.__getformat__('float')}

    def tearDown(self):
        float.__setformat__('double', self.save_formats['double'])
        float.__setformat__('float', self.save_formats['float'])

    def test_getformat(self):
        self.assert_(float.__getformat__('double') in
                     ['unknown', 'IEEE, big-endian', 'IEEE, little-endian'])
        self.assert_(float.__getformat__('float') in
                     ['unknown', 'IEEE, big-endian', 'IEEE, little-endian'])
        self.assertRaises(ValueError, float.__getformat__, 'chicken')
        self.assertRaises(TypeError, float.__getformat__, 1)

    def test_setformat(self):
        for t in 'double', 'float':
            float.__setformat__(t, 'unknown')
            if self.save_formats[t] == 'IEEE, big-endian':
                self.assertRaises(ValueError, float.__setformat__,
                                  t, 'IEEE, little-endian')
            elif self.save_formats[t] == 'IEEE, little-endian':
                self.assertRaises(ValueError, float.__setformat__,
                                  t, 'IEEE, big-endian')
            else:
                self.assertRaises(ValueError, float.__setformat__,
                                  t, 'IEEE, big-endian')
                self.assertRaises(ValueError, float.__setformat__,
                                  t, 'IEEE, little-endian')
            self.assertRaises(ValueError, float.__setformat__,
                              t, 'chicken')
        self.assertRaises(ValueError, float.__setformat__,
                          'chicken', 'unknown')

BE_DOUBLE_INF = '\x7f\xf0\x00\x00\x00\x00\x00\x00'
LE_DOUBLE_INF = ''.join(reversed(BE_DOUBLE_INF))
BE_DOUBLE_NAN = '\x7f\xf8\x00\x00\x00\x00\x00\x00'
LE_DOUBLE_NAN = ''.join(reversed(BE_DOUBLE_NAN))

BE_FLOAT_INF = '\x7f\x80\x00\x00'
LE_FLOAT_INF = ''.join(reversed(BE_FLOAT_INF))
BE_FLOAT_NAN = '\x7f\xc0\x00\x00'
LE_FLOAT_NAN = ''.join(reversed(BE_FLOAT_NAN))

# on non-IEEE platforms, attempting to unpack a bit pattern
# representing an infinity or a NaN should raise an exception.

class UnknownFormatTestCase(unittest.TestCase):
    def setUp(self):
        self.save_formats = {'double':float.__getformat__('double'),
                             'float':float.__getformat__('float')}
        float.__setformat__('double', 'unknown')
        float.__setformat__('float', 'unknown')

    def tearDown(self):
        float.__setformat__('double', self.save_formats['double'])
        float.__setformat__('float', self.save_formats['float'])

    def test_double_specials_dont_unpack(self):
        for fmt, data in [('>d', BE_DOUBLE_INF),
                          ('>d', BE_DOUBLE_NAN),
                          ('<d', LE_DOUBLE_INF),
                          ('<d', LE_DOUBLE_NAN)]:
            self.assertRaises(ValueError, struct.unpack, fmt, data)

    def test_float_specials_dont_unpack(self):
        for fmt, data in [('>f', BE_FLOAT_INF),
                          ('>f', BE_FLOAT_NAN),
                          ('<f', LE_FLOAT_INF),
                          ('<f', LE_FLOAT_NAN)]:
            self.assertRaises(ValueError, struct.unpack, fmt, data)


# on an IEEE platform, all we guarantee is that bit patterns
# representing infinities or NaNs do not raise an exception; all else
# is accident (today).
# let's also try to guarantee that -0.0 and 0.0 don't get confused.

class IEEEFormatTestCase(unittest.TestCase):
    if float.__getformat__("double").startswith("IEEE"):
        def test_double_specials_do_unpack(self):
            for fmt, data in [('>d', BE_DOUBLE_INF),
                              ('>d', BE_DOUBLE_NAN),
                              ('<d', LE_DOUBLE_INF),
                              ('<d', LE_DOUBLE_NAN)]:
                struct.unpack(fmt, data)

    if float.__getformat__("float").startswith("IEEE"):
        def test_float_specials_do_unpack(self):
            for fmt, data in [('>f', BE_FLOAT_INF),
                              ('>f', BE_FLOAT_NAN),
                              ('<f', LE_FLOAT_INF),
                              ('<f', LE_FLOAT_NAN)]:
                struct.unpack(fmt, data)

    if float.__getformat__("double").startswith("IEEE"):
        def test_negative_zero(self):
            import math
            def pos_pos():
                return 0.0, math.atan2(0.0, -1)
            def pos_neg():
                return 0.0, math.atan2(-0.0, -1)
            def neg_pos():
                return -0.0, math.atan2(0.0, -1)
            def neg_neg():
                return -0.0, math.atan2(-0.0, -1)
            self.assertEquals(pos_pos(), neg_pos())
            self.assertEquals(pos_neg(), neg_neg())

    if float.__getformat__("double").startswith("IEEE"):
        def test_underflow_sign(self):
            import math
            # check that -1e-1000 gives -0.0, not 0.0
            self.assertEquals(math.atan2(-1e-1000, -1), math.atan2(-0.0, -1))
            self.assertEquals(math.atan2(float('-1e-1000'), -1),
                              math.atan2(-0.0, -1))

class ReprTestCase(unittest.TestCase):
    def test_repr(self):
        floats_file = open(os.path.join(os.path.split(__file__)[0],
                           'floating_points.txt'))
        for line in floats_file:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            v = eval(line)
            self.assertEqual(v, eval(repr(v)))
        floats_file.close()

# Beginning with Python 2.6 float has cross platform compatible
# ways to create and represent inf and nan
class InfNanTest(unittest.TestCase):
    def test_inf_from_str(self):
        self.assert_(isinf(float("inf")))
        self.assert_(isinf(float("+inf")))
        self.assert_(isinf(float("-inf")))
        self.assert_(isinf(float("infinity")))
        self.assert_(isinf(float("+infinity")))
        self.assert_(isinf(float("-infinity")))

        self.assertEqual(repr(float("inf")), "inf")
        self.assertEqual(repr(float("+inf")), "inf")
        self.assertEqual(repr(float("-inf")), "-inf")
        self.assertEqual(repr(float("infinity")), "inf")
        self.assertEqual(repr(float("+infinity")), "inf")
        self.assertEqual(repr(float("-infinity")), "-inf")

        self.assertEqual(repr(float("INF")), "inf")
        self.assertEqual(repr(float("+Inf")), "inf")
        self.assertEqual(repr(float("-iNF")), "-inf")
        self.assertEqual(repr(float("Infinity")), "inf")
        self.assertEqual(repr(float("+iNfInItY")), "inf")
        self.assertEqual(repr(float("-INFINITY")), "-inf")

        self.assertEqual(str(float("inf")), "inf")
        self.assertEqual(str(float("+inf")), "inf")
        self.assertEqual(str(float("-inf")), "-inf")
        self.assertEqual(str(float("infinity")), "inf")
        self.assertEqual(str(float("+infinity")), "inf")
        self.assertEqual(str(float("-infinity")), "-inf")

        self.assertRaises(ValueError, float, "info")
        self.assertRaises(ValueError, float, "+info")
        self.assertRaises(ValueError, float, "-info")
        self.assertRaises(ValueError, float, "in")
        self.assertRaises(ValueError, float, "+in")
        self.assertRaises(ValueError, float, "-in")
        self.assertRaises(ValueError, float, "infinit")
        self.assertRaises(ValueError, float, "+Infin")
        self.assertRaises(ValueError, float, "-INFI")
        self.assertRaises(ValueError, float, "infinitys")

    def test_inf_as_str(self):
        self.assertEqual(repr(1e300 * 1e300), "inf")
        self.assertEqual(repr(-1e300 * 1e300), "-inf")

        self.assertEqual(str(1e300 * 1e300), "inf")
        self.assertEqual(str(-1e300 * 1e300), "-inf")

    def test_nan_from_str(self):
        self.assert_(isnan(float("nan")))
        self.assert_(isnan(float("+nan")))
        self.assert_(isnan(float("-nan")))

        self.assertEqual(repr(float("nan")), "nan")
        self.assertEqual(repr(float("+nan")), "nan")
        self.assertEqual(repr(float("-nan")), "nan")

        self.assertEqual(repr(float("NAN")), "nan")
        self.assertEqual(repr(float("+NAn")), "nan")
        self.assertEqual(repr(float("-NaN")), "nan")

        self.assertEqual(str(float("nan")), "nan")
        self.assertEqual(str(float("+nan")), "nan")
        self.assertEqual(str(float("-nan")), "nan")

        self.assertRaises(ValueError, float, "nana")
        self.assertRaises(ValueError, float, "+nana")
        self.assertRaises(ValueError, float, "-nana")
        self.assertRaises(ValueError, float, "na")
        self.assertRaises(ValueError, float, "+na")
        self.assertRaises(ValueError, float, "-na")

    def test_nan_as_str(self):
        self.assertEqual(repr(1e300 * 1e300 * 0), "nan")
        self.assertEqual(repr(-1e300 * 1e300 * 0), "nan")

        self.assertEqual(str(1e300 * 1e300 * 0), "nan")
        self.assertEqual(str(-1e300 * 1e300 * 0), "nan")

    def notest_float_nan(self):
        self.assert_(NAN.is_nan())
        self.failIf(INF.is_nan())
        self.failIf((0.).is_nan())

    def notest_float_inf(self):
        self.assert_(INF.is_inf())
        self.failIf(NAN.is_inf())
        self.failIf((0.).is_inf())

fromHex = float.fromhex
toHex = float.hex
class HexFloatTestCase(unittest.TestCase):
    MAX = fromHex('0x.fffffffffffff8p+1024')  # max normal
    MIN = fromHex('0x1p-1022')                # min normal
    TINY = fromHex('0x0.0000000000001p-1022') # min subnormal
    EPS = fromHex('0x0.0000000000001p0') # diff between 1.0 and next float up

    def identical(self, x, y):
        # check that floats x and y are identical, or that both
        # are NaNs
        if isnan(x) or isnan(y):
            if isnan(x) == isnan(y):
                return
        elif x == y and (x != 0.0 or copysign(1.0, x) == copysign(1.0, y)):
            return
        self.fail('%r not identical to %r' % (x, y))

    def test_ends(self):
        self.identical(self.MIN, ldexp(1.0, -1022))
        self.identical(self.TINY, ldexp(1.0, -1074))
        self.identical(self.EPS, ldexp(1.0, -52))
        self.identical(self.MAX, 2.*(ldexp(1.0, 1023) - ldexp(1.0, 970)))

    def test_invalid_inputs(self):
        invalid_inputs = [
            'infi',   # misspelt infinities and nans
            '-Infinit',
            '++inf',
            '-+Inf',
            '--nan',
            '+-NaN',
            'snan',
            'NaNs',
            'nna',
            'an',
            'nf',
            'nfinity',
            'inity',
            'iinity',
            '0xnan',
            '',
            ' ',
            'x1.0p0',
            '0xX1.0p0',
            '+ 0x1.0p0', # internal whitespace
            '- 0x1.0p0',
            '0 x1.0p0',
            '0x 1.0p0',
            '0x1 2.0p0',
            '+0x1 .0p0',
            '0x1. 0p0',
            '-0x1.0 1p0',
            '-0x1.0 p0',
            '+0x1.0p +0',
            '0x1.0p -0',
            '0x1.0p 0',
            '+0x1.0p+ 0',
            '-0x1.0p- 0',
            '++0x1.0p-0', # double signs
            '--0x1.0p0',
            '+-0x1.0p+0',
            '-+0x1.0p0',
            '0x1.0p++0',
            '+0x1.0p+-0',
            '-0x1.0p-+0',
            '0x1.0p--0',
            '0x1.0.p0',
            '0x.p0', # no hex digits before or after point
            '0x1,p0', # wrong decimal point character
            '0x1pa',
            u'0x1p\uff10',  # fullwidth Unicode digits
            u'\uff10x1p0',
            u'0x\uff11p0',
            u'0x1.\uff10p0',
            '0x1p0 \n 0x2p0',
            '0x1p0\0 0x1p0',  # embedded null byte is not end of string
            ]
        for x in invalid_inputs:
            try:
                result = fromHex(x)
            except ValueError:
                pass
            else:
                self.fail('Expected float.fromhex(%r) to raise ValueError; '
                          'got %r instead' % (x, result))


    def test_whitespace(self):
        value_pairs = [
            ('inf', INF),
            ('-Infinity', -INF),
            ('nan', NAN),
            ('1.0', 1.0),
            ('-0x.2', -0.125),
            ('-0.0', -0.0)
            ]
        whitespace = [
            '',
            ' ',
            '\t',
            '\n',
            '\n \t',
            '\f',
            '\v',
            '\r'
            ]
        for inp, expected in value_pairs:
            for lead in whitespace:
                for trail in whitespace:
                    got = fromHex(lead + inp + trail)
                    self.identical(got, expected)


    def test_from_hex(self):
        MIN = self.MIN;
        MAX = self.MAX;
        TINY = self.TINY;
        EPS = self.EPS;

        # two spellings of infinity, with optional signs; case-insensitive
        self.identical(fromHex('inf'), INF)
        self.identical(fromHex('+Inf'), INF)
        self.identical(fromHex('-INF'), -INF)
        self.identical(fromHex('iNf'), INF)
        self.identical(fromHex('Infinity'), INF)
        self.identical(fromHex('+INFINITY'), INF)
        self.identical(fromHex('-infinity'), -INF)
        self.identical(fromHex('-iNFiNitY'), -INF)

        # nans with optional sign; case insensitive
        self.identical(fromHex('nan'), NAN)
        self.identical(fromHex('+NaN'), NAN)
        self.identical(fromHex('-NaN'), NAN)
        self.identical(fromHex('-nAN'), NAN)

        # variations in input format
        self.identical(fromHex('1'), 1.0)
        self.identical(fromHex('+1'), 1.0)
        self.identical(fromHex('1.'), 1.0)
        self.identical(fromHex('1.0'), 1.0)
        self.identical(fromHex('1.0p0'), 1.0)
        self.identical(fromHex('01'), 1.0)
        self.identical(fromHex('01.'), 1.0)
        self.identical(fromHex('0x1'), 1.0)
        self.identical(fromHex('0x1.'), 1.0)
        self.identical(fromHex('0x1.0'), 1.0)
        self.identical(fromHex('+0x1.0'), 1.0)
        self.identical(fromHex('0x1p0'), 1.0)
        self.identical(fromHex('0X1p0'), 1.0)
        self.identical(fromHex('0X1P0'), 1.0)
        self.identical(fromHex('0x1P0'), 1.0)
        self.identical(fromHex('0x1.p0'), 1.0)
        self.identical(fromHex('0x1.0p0'), 1.0)
        self.identical(fromHex('0x.1p4'), 1.0)
        self.identical(fromHex('0x.1p04'), 1.0)
        self.identical(fromHex('0x.1p004'), 1.0)
        self.identical(fromHex('0x1p+0'), 1.0)
        self.identical(fromHex('0x1P-0'), 1.0)
        self.identical(fromHex('+0x1p0'), 1.0)
        self.identical(fromHex('0x01p0'), 1.0)
        self.identical(fromHex('0x1p00'), 1.0)
        self.identical(fromHex(u'0x1p0'), 1.0)
        self.identical(fromHex(' 0x1p0 '), 1.0)
        self.identical(fromHex('\n 0x1p0'), 1.0)
        self.identical(fromHex('0x1p0 \t'), 1.0)
        self.identical(fromHex('0xap0'), 10.0)
        self.identical(fromHex('0xAp0'), 10.0)
        self.identical(fromHex('0xaP0'), 10.0)
        self.identical(fromHex('0xAP0'), 10.0)
        self.identical(fromHex('0xbep0'), 190.0)
        self.identical(fromHex('0xBep0'), 190.0)
        self.identical(fromHex('0xbEp0'), 190.0)
        self.identical(fromHex('0XBE0P-4'), 190.0)
        self.identical(fromHex('0xBEp0'), 190.0)
        self.identical(fromHex('0xB.Ep4'), 190.0)
        self.identical(fromHex('0x.BEp8'), 190.0)
        self.identical(fromHex('0x.0BEp12'), 190.0)

        # moving the point around
        pi = fromHex('0x1.921fb54442d18p1')
        self.identical(fromHex('0x.006487ed5110b46p11'), pi)
        self.identical(fromHex('0x.00c90fdaa22168cp10'), pi)
        self.identical(fromHex('0x.01921fb54442d18p9'), pi)
        self.identical(fromHex('0x.03243f6a8885a3p8'), pi)
        self.identical(fromHex('0x.06487ed5110b46p7'), pi)
        self.identical(fromHex('0x.0c90fdaa22168cp6'), pi)
        self.identical(fromHex('0x.1921fb54442d18p5'), pi)
        self.identical(fromHex('0x.3243f6a8885a3p4'), pi)
        self.identical(fromHex('0x.6487ed5110b46p3'), pi)
        self.identical(fromHex('0x.c90fdaa22168cp2'), pi)
        self.identical(fromHex('0x1.921fb54442d18p1'), pi)
        self.identical(fromHex('0x3.243f6a8885a3p0'), pi)
        self.identical(fromHex('0x6.487ed5110b46p-1'), pi)
        self.identical(fromHex('0xc.90fdaa22168cp-2'), pi)
        self.identical(fromHex('0x19.21fb54442d18p-3'), pi)
        self.identical(fromHex('0x32.43f6a8885a3p-4'), pi)
        self.identical(fromHex('0x64.87ed5110b46p-5'), pi)
        self.identical(fromHex('0xc9.0fdaa22168cp-6'), pi)
        self.identical(fromHex('0x192.1fb54442d18p-7'), pi)
        self.identical(fromHex('0x324.3f6a8885a3p-8'), pi)
        self.identical(fromHex('0x648.7ed5110b46p-9'), pi)
        self.identical(fromHex('0xc90.fdaa22168cp-10'), pi)
        self.identical(fromHex('0x1921.fb54442d18p-11'), pi)
        # ...
        self.identical(fromHex('0x1921fb54442d1.8p-47'), pi)
        self.identical(fromHex('0x3243f6a8885a3p-48'), pi)
        self.identical(fromHex('0x6487ed5110b46p-49'), pi)
        self.identical(fromHex('0xc90fdaa22168cp-50'), pi)
        self.identical(fromHex('0x1921fb54442d18p-51'), pi)
        self.identical(fromHex('0x3243f6a8885a30p-52'), pi)
        self.identical(fromHex('0x6487ed5110b460p-53'), pi)
        self.identical(fromHex('0xc90fdaa22168c0p-54'), pi)
        self.identical(fromHex('0x1921fb54442d180p-55'), pi)


        # results that should overflow...
        self.assertRaises(OverflowError, fromHex, '-0x1p1024')
        self.assertRaises(OverflowError, fromHex, '0x1p+1025')
        self.assertRaises(OverflowError, fromHex, '+0X1p1030')
        self.assertRaises(OverflowError, fromHex, '-0x1p+1100')
        self.assertRaises(OverflowError, fromHex, '0X1p123456789123456789')
        self.assertRaises(OverflowError, fromHex, '+0X.8p+1025')
        self.assertRaises(OverflowError, fromHex, '+0x0.8p1025')
        self.assertRaises(OverflowError, fromHex, '-0x0.4p1026')
        self.assertRaises(OverflowError, fromHex, '0X2p+1023')
        self.assertRaises(OverflowError, fromHex, '0x2.p1023')
        self.assertRaises(OverflowError, fromHex, '-0x2.0p+1023')
        self.assertRaises(OverflowError, fromHex, '+0X4p+1022')
        self.assertRaises(OverflowError, fromHex, '0x1.ffffffffffffffp+1023')
        self.assertRaises(OverflowError, fromHex, '-0X1.fffffffffffff9p1023')
        self.assertRaises(OverflowError, fromHex, '0X1.fffffffffffff8p1023')
        self.assertRaises(OverflowError, fromHex, '+0x3.fffffffffffffp1022')
        self.assertRaises(OverflowError, fromHex, '0x3fffffffffffffp+970')
        self.assertRaises(OverflowError, fromHex, '0x10000000000000000p960')
        self.assertRaises(OverflowError, fromHex, '-0Xffffffffffffffffp960')

        # ...and those that round to +-max float
        self.identical(fromHex('+0x1.fffffffffffffp+1023'), MAX)
        self.identical(fromHex('-0X1.fffffffffffff7p1023'), -MAX)
        self.identical(fromHex('0X1.fffffffffffff7fffffffffffffp1023'), MAX)

        # zeros
        self.identical(fromHex('0x0p0'), 0.0)
        self.identical(fromHex('0x0p1000'), 0.0)
        self.identical(fromHex('-0x0p1023'), -0.0)
        self.identical(fromHex('0X0p1024'), 0.0)
        self.identical(fromHex('-0x0p1025'), -0.0)
        self.identical(fromHex('0X0p2000'), 0.0)
        self.identical(fromHex('0x0p123456789123456789'), 0.0)
        self.identical(fromHex('-0X0p-0'), -0.0)
        self.identical(fromHex('-0X0p-1000'), -0.0)
        self.identical(fromHex('0x0p-1023'), 0.0)
        self.identical(fromHex('-0X0p-1024'), -0.0)
        self.identical(fromHex('-0x0p-1025'), -0.0)
        self.identical(fromHex('-0x0p-1072'), -0.0)
        self.identical(fromHex('0X0p-1073'), 0.0)
        self.identical(fromHex('-0x0p-1074'), -0.0)
        self.identical(fromHex('0x0p-1075'), 0.0)
        self.identical(fromHex('0X0p-1076'), 0.0)
        self.identical(fromHex('-0X0p-2000'), -0.0)
        self.identical(fromHex('-0x0p-123456789123456789'), -0.0)

        # values that should underflow to 0
        self.identical(fromHex('0X1p-1075'), 0.0)
        self.identical(fromHex('-0X1p-1075'), -0.0)
        self.identical(fromHex('-0x1p-123456789123456789'), -0.0)
        self.identical(fromHex('0x1.00000000000000001p-1075'), TINY)
        self.identical(fromHex('-0x1.1p-1075'), -TINY)
        self.identical(fromHex('0x1.fffffffffffffffffp-1075'), TINY)

        # check round-half-even is working correctly near 0 ...
        self.identical(fromHex('0x1p-1076'), 0.0)
        self.identical(fromHex('0X2p-1076'), 0.0)
        self.identical(fromHex('0X3p-1076'), TINY)
        self.identical(fromHex('0x4p-1076'), TINY)
        self.identical(fromHex('0X5p-1076'), TINY)
        self.identical(fromHex('0X6p-1076'), 2*TINY)
        self.identical(fromHex('0x7p-1076'), 2*TINY)
        self.identical(fromHex('0X8p-1076'), 2*TINY)
        self.identical(fromHex('0X9p-1076'), 2*TINY)
        self.identical(fromHex('0xap-1076'), 2*TINY)
        self.identical(fromHex('0Xbp-1076'), 3*TINY)
        self.identical(fromHex('0xcp-1076'), 3*TINY)
        self.identical(fromHex('0Xdp-1076'), 3*TINY)
        self.identical(fromHex('0Xep-1076'), 4*TINY)
        self.identical(fromHex('0xfp-1076'), 4*TINY)
        self.identical(fromHex('0x10p-1076'), 4*TINY)
        self.identical(fromHex('-0x1p-1076'), -0.0)
        self.identical(fromHex('-0X2p-1076'), -0.0)
        self.identical(fromHex('-0x3p-1076'), -TINY)
        self.identical(fromHex('-0X4p-1076'), -TINY)
        self.identical(fromHex('-0x5p-1076'), -TINY)
        self.identical(fromHex('-0x6p-1076'), -2*TINY)
        self.identical(fromHex('-0X7p-1076'), -2*TINY)
        self.identical(fromHex('-0X8p-1076'), -2*TINY)
        self.identical(fromHex('-0X9p-1076'), -2*TINY)
        self.identical(fromHex('-0Xap-1076'), -2*TINY)
        self.identical(fromHex('-0xbp-1076'), -3*TINY)
        self.identical(fromHex('-0xcp-1076'), -3*TINY)
        self.identical(fromHex('-0Xdp-1076'), -3*TINY)
        self.identical(fromHex('-0xep-1076'), -4*TINY)
        self.identical(fromHex('-0Xfp-1076'), -4*TINY)
        self.identical(fromHex('-0X10p-1076'), -4*TINY)

        # ... and near MIN ...
        self.identical(fromHex('0x0.ffffffffffffd6p-1022'), MIN-3*TINY)
        self.identical(fromHex('0x0.ffffffffffffd8p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffdap-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffdcp-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffdep-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffe0p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffe2p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffe4p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffe6p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffe8p-1022'), MIN-2*TINY)
        self.identical(fromHex('0x0.ffffffffffffeap-1022'), MIN-TINY)
        self.identical(fromHex('0x0.ffffffffffffecp-1022'), MIN-TINY)
        self.identical(fromHex('0x0.ffffffffffffeep-1022'), MIN-TINY)
        self.identical(fromHex('0x0.fffffffffffff0p-1022'), MIN-TINY)
        self.identical(fromHex('0x0.fffffffffffff2p-1022'), MIN-TINY)
        self.identical(fromHex('0x0.fffffffffffff4p-1022'), MIN-TINY)
        self.identical(fromHex('0x0.fffffffffffff6p-1022'), MIN-TINY)
        self.identical(fromHex('0x0.fffffffffffff8p-1022'), MIN)
        self.identical(fromHex('0x0.fffffffffffffap-1022'), MIN)
        self.identical(fromHex('0x0.fffffffffffffcp-1022'), MIN)
        self.identical(fromHex('0x0.fffffffffffffep-1022'), MIN)
        self.identical(fromHex('0x1.00000000000000p-1022'), MIN)
        self.identical(fromHex('0x1.00000000000002p-1022'), MIN)
        self.identical(fromHex('0x1.00000000000004p-1022'), MIN)
        self.identical(fromHex('0x1.00000000000006p-1022'), MIN)
        self.identical(fromHex('0x1.00000000000008p-1022'), MIN)
        self.identical(fromHex('0x1.0000000000000ap-1022'), MIN+TINY)
        self.identical(fromHex('0x1.0000000000000cp-1022'), MIN+TINY)
        self.identical(fromHex('0x1.0000000000000ep-1022'), MIN+TINY)
        self.identical(fromHex('0x1.00000000000010p-1022'), MIN+TINY)
        self.identical(fromHex('0x1.00000000000012p-1022'), MIN+TINY)
        self.identical(fromHex('0x1.00000000000014p-1022'), MIN+TINY)
        self.identical(fromHex('0x1.00000000000016p-1022'), MIN+TINY)
        self.identical(fromHex('0x1.00000000000018p-1022'), MIN+2*TINY)

        # ... and near 1.0.
        self.identical(fromHex('0x0.fffffffffffff0p0'), 1.0-EPS)
        self.identical(fromHex('0x0.fffffffffffff1p0'), 1.0-EPS)
        self.identical(fromHex('0X0.fffffffffffff2p0'), 1.0-EPS)
        self.identical(fromHex('0x0.fffffffffffff3p0'), 1.0-EPS)
        self.identical(fromHex('0X0.fffffffffffff4p0'), 1.0-EPS)
        self.identical(fromHex('0X0.fffffffffffff5p0'), 1.0-EPS/2)
        self.identical(fromHex('0X0.fffffffffffff6p0'), 1.0-EPS/2)
        self.identical(fromHex('0x0.fffffffffffff7p0'), 1.0-EPS/2)
        self.identical(fromHex('0x0.fffffffffffff8p0'), 1.0-EPS/2)
        self.identical(fromHex('0X0.fffffffffffff9p0'), 1.0-EPS/2)
        self.identical(fromHex('0X0.fffffffffffffap0'), 1.0-EPS/2)
        self.identical(fromHex('0x0.fffffffffffffbp0'), 1.0-EPS/2)
        self.identical(fromHex('0X0.fffffffffffffcp0'), 1.0)
        self.identical(fromHex('0x0.fffffffffffffdp0'), 1.0)
        self.identical(fromHex('0X0.fffffffffffffep0'), 1.0)
        self.identical(fromHex('0x0.ffffffffffffffp0'), 1.0)
        self.identical(fromHex('0X1.00000000000000p0'), 1.0)
        self.identical(fromHex('0X1.00000000000001p0'), 1.0)
        self.identical(fromHex('0x1.00000000000002p0'), 1.0)
        self.identical(fromHex('0X1.00000000000003p0'), 1.0)
        self.identical(fromHex('0x1.00000000000004p0'), 1.0)
        self.identical(fromHex('0X1.00000000000005p0'), 1.0)
        self.identical(fromHex('0X1.00000000000006p0'), 1.0)
        self.identical(fromHex('0X1.00000000000007p0'), 1.0)
        self.identical(fromHex('0x1.00000000000007ffffffffffffffffffffp0'),
                       1.0)
        self.identical(fromHex('0x1.00000000000008p0'), 1.0)
        self.identical(fromHex('0x1.00000000000008000000000000000001p0'),
                       1+EPS)
        self.identical(fromHex('0X1.00000000000009p0'), 1.0+EPS)
        self.identical(fromHex('0x1.0000000000000ap0'), 1.0+EPS)
        self.identical(fromHex('0x1.0000000000000bp0'), 1.0+EPS)
        self.identical(fromHex('0X1.0000000000000cp0'), 1.0+EPS)
        self.identical(fromHex('0x1.0000000000000dp0'), 1.0+EPS)
        self.identical(fromHex('0x1.0000000000000ep0'), 1.0+EPS)
        self.identical(fromHex('0X1.0000000000000fp0'), 1.0+EPS)
        self.identical(fromHex('0x1.00000000000010p0'), 1.0+EPS)
        self.identical(fromHex('0X1.00000000000011p0'), 1.0+EPS)
        self.identical(fromHex('0x1.00000000000012p0'), 1.0+EPS)
        self.identical(fromHex('0X1.00000000000013p0'), 1.0+EPS)
        self.identical(fromHex('0X1.00000000000014p0'), 1.0+EPS)
        self.identical(fromHex('0x1.00000000000015p0'), 1.0+EPS)
        self.identical(fromHex('0x1.00000000000016p0'), 1.0+EPS)
        self.identical(fromHex('0X1.00000000000017p0'), 1.0+EPS)
        self.identical(fromHex('0x1.00000000000017ffffffffffffffffffffp0'),
                       1.0+EPS)
        self.identical(fromHex('0x1.00000000000018p0'), 1.0+2*EPS)
        self.identical(fromHex('0X1.00000000000018000000000000000001p0'),
                       1.0+2*EPS)
        self.identical(fromHex('0x1.00000000000019p0'), 1.0+2*EPS)
        self.identical(fromHex('0X1.0000000000001ap0'), 1.0+2*EPS)
        self.identical(fromHex('0X1.0000000000001bp0'), 1.0+2*EPS)
        self.identical(fromHex('0x1.0000000000001cp0'), 1.0+2*EPS)
        self.identical(fromHex('0x1.0000000000001dp0'), 1.0+2*EPS)
        self.identical(fromHex('0x1.0000000000001ep0'), 1.0+2*EPS)
        self.identical(fromHex('0X1.0000000000001fp0'), 1.0+2*EPS)
        self.identical(fromHex('0x1.00000000000020p0'), 1.0+2*EPS)

    def test_roundtrip(self):
        def roundtrip(x):
            return fromHex(toHex(x))

        for x in [NAN, INF, self.MAX, self.MIN, self.MIN-self.TINY, self.TINY, 0.0]:
            self.identical(x, roundtrip(x))
            self.identical(-x, roundtrip(-x))

        # fromHex(toHex(x)) should exactly recover x, for any non-NaN float x.
        import random
        for i in xrange(10000):
            e = random.randrange(-1200, 1200)
            m = random.random()
            s = random.choice([1.0, -1.0])
            try:
                x = s*ldexp(m, e)
            except OverflowError:
                pass
            else:
                self.identical(x, fromHex(toHex(x)))


def test_main():
    test_support.run_unittest(
        GeneralFloatCases,
        FormatFunctionsTestCase,
        UnknownFormatTestCase,
        IEEEFormatTestCase,
        ReprTestCase,
        InfNanTest,
        HexFloatTestCase,
        )

if __name__ == '__main__':
    test_main()
