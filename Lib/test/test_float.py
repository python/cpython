
import unittest, struct
from test import test_support

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

def test_main():
    test_support.run_unittest(
        FormatFunctionsTestCase,
        UnknownFormatTestCase,
        IEEEFormatTestCase)

if __name__ == '__main__':
    test_main()
