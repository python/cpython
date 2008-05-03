
import unittest, struct
import os
from test import test_support
import math
from math import isinf, isnan
import operator

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
        if have_unicode:
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
        self.assertEqual(fcmp(float("  .25e-1  "), .025), 0)

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

        self.assertAlmostEqual(float(Foo0()), 42.)
        self.assertAlmostEqual(float(Foo1()), 42.)
        self.assertAlmostEqual(float(Foo2()), 42.)
        self.assertAlmostEqual(float(Foo3(21)), 42.)
        self.assertRaises(TypeError, float, Foo4(42))

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
# ways to create and representate inf and nan
class InfNanTest(unittest.TestCase):
    def test_inf_from_str(self):
        self.assert_(isinf(float("inf")))
        self.assert_(isinf(float("+inf")))
        self.assert_(isinf(float("-inf")))

        self.assertEqual(repr(float("inf")), "inf")
        self.assertEqual(repr(float("+inf")), "inf")
        self.assertEqual(repr(float("-inf")), "-inf")

        self.assertEqual(repr(float("INF")), "inf")
        self.assertEqual(repr(float("+Inf")), "inf")
        self.assertEqual(repr(float("-iNF")), "-inf")

        self.assertEqual(str(float("inf")), "inf")
        self.assertEqual(str(float("+inf")), "inf")
        self.assertEqual(str(float("-inf")), "-inf")

        self.assertRaises(ValueError, float, "info")
        self.assertRaises(ValueError, float, "+info")
        self.assertRaises(ValueError, float, "-info")
        self.assertRaises(ValueError, float, "in")
        self.assertRaises(ValueError, float, "+in")
        self.assertRaises(ValueError, float, "-in")

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


def test_main():
    test_support.run_unittest(
        FormatFunctionsTestCase,
        UnknownFormatTestCase,
        IEEEFormatTestCase,
        ReprTestCase,
        InfNanTest,
        )

if __name__ == '__main__':
    test_main()
