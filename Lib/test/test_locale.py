from test.test_support import run_unittest, verbose
import unittest
import locale
import sys
import codecs


enUS_locale = None

def get_enUS_locale():
    global enUS_locale
    if sys.platform == 'darwin':
        import os
        tlocs = ("en_US.UTF-8", "en_US.ISO8859-1", "en_US")
        if int(os.uname()[2].split('.')[0]) < 10:
            # The locale test work fine on OSX 10.6, I (ronaldoussoren)
            # haven't had time yet to verify if tests work on OSX 10.5
            # (10.4 is known to be bad)
            raise unittest.SkipTest("Locale support on MacOSX is minimal")
    if sys.platform.startswith("win"):
        tlocs = ("En", "English")
    else:
        tlocs = ("en_US.UTF-8", "en_US.US-ASCII", "en_US")
    oldlocale = locale.setlocale(locale.LC_NUMERIC)
    for tloc in tlocs:
        try:
            locale.setlocale(locale.LC_NUMERIC, tloc)
        except locale.Error:
            continue
        break
    else:
        raise unittest.SkipTest(
            "Test locale not supported (tried %s)" % (', '.join(tlocs)))
    enUS_locale = tloc
    locale.setlocale(locale.LC_NUMERIC, oldlocale)


class BaseLocalizedTest(unittest.TestCase):
    #
    # Base class for tests using a real locale
    #

    def setUp(self):
        self.oldlocale = locale.setlocale(self.locale_type)
        locale.setlocale(self.locale_type, enUS_locale)
        if verbose:
            print "testing with \"%s\"..." % enUS_locale,

    def tearDown(self):
        locale.setlocale(self.locale_type, self.oldlocale)


class BaseCookedTest(unittest.TestCase):
    #
    # Base class for tests using cooked localeconv() values
    #

    def setUp(self):
        locale._override_localeconv = self.cooked_values

    def tearDown(self):
        locale._override_localeconv = {}


class CCookedTest(BaseCookedTest):
    # A cooked "C" locale

    cooked_values = {
        'currency_symbol': '',
        'decimal_point': '.',
        'frac_digits': 127,
        'grouping': [],
        'int_curr_symbol': '',
        'int_frac_digits': 127,
        'mon_decimal_point': '',
        'mon_grouping': [],
        'mon_thousands_sep': '',
        'n_cs_precedes': 127,
        'n_sep_by_space': 127,
        'n_sign_posn': 127,
        'negative_sign': '',
        'p_cs_precedes': 127,
        'p_sep_by_space': 127,
        'p_sign_posn': 127,
        'positive_sign': '',
        'thousands_sep': ''
    }

class EnUSCookedTest(BaseCookedTest):
    # A cooked "en_US" locale

    cooked_values = {
        'currency_symbol': '$',
        'decimal_point': '.',
        'frac_digits': 2,
        'grouping': [3, 3, 0],
        'int_curr_symbol': 'USD ',
        'int_frac_digits': 2,
        'mon_decimal_point': '.',
        'mon_grouping': [3, 3, 0],
        'mon_thousands_sep': ',',
        'n_cs_precedes': 1,
        'n_sep_by_space': 0,
        'n_sign_posn': 1,
        'negative_sign': '-',
        'p_cs_precedes': 1,
        'p_sep_by_space': 0,
        'p_sign_posn': 1,
        'positive_sign': '',
        'thousands_sep': ','
    }


class FrFRCookedTest(BaseCookedTest):
    # A cooked "fr_FR" locale with a space character as decimal separator
    # and a non-ASCII currency symbol.

    cooked_values = {
        'currency_symbol': '\xe2\x82\xac',
        'decimal_point': ',',
        'frac_digits': 2,
        'grouping': [3, 3, 0],
        'int_curr_symbol': 'EUR ',
        'int_frac_digits': 2,
        'mon_decimal_point': ',',
        'mon_grouping': [3, 3, 0],
        'mon_thousands_sep': ' ',
        'n_cs_precedes': 0,
        'n_sep_by_space': 1,
        'n_sign_posn': 1,
        'negative_sign': '-',
        'p_cs_precedes': 0,
        'p_sep_by_space': 1,
        'p_sign_posn': 1,
        'positive_sign': '',
        'thousands_sep': ' '
    }


class BaseFormattingTest(object):
    #
    # Utility functions for formatting tests
    #

    def _test_formatfunc(self, format, value, out, func, **format_opts):
        self.assertEqual(
            func(format, value, **format_opts), out)

    def _test_format(self, format, value, out, **format_opts):
        self._test_formatfunc(format, value, out,
            func=locale.format, **format_opts)

    def _test_format_string(self, format, value, out, **format_opts):
        self._test_formatfunc(format, value, out,
            func=locale.format_string, **format_opts)

    def _test_currency(self, value, out, **format_opts):
        self.assertEqual(locale.currency(value, **format_opts), out)


class EnUSNumberFormatting(BaseFormattingTest):
    # XXX there is a grouping + padding bug when the thousands separator
    # is empty but the grouping array contains values (e.g. Solaris 10)

    def setUp(self):
        self.sep = locale.localeconv()['thousands_sep']

    def test_grouping(self):
        self._test_format("%f", 1024, grouping=1, out='1%s024.000000' % self.sep)
        self._test_format("%f", 102, grouping=1, out='102.000000')
        self._test_format("%f", -42, grouping=1, out='-42.000000')
        self._test_format("%+f", -42, grouping=1, out='-42.000000')

    def test_grouping_and_padding(self):
        self._test_format("%20.f", -42, grouping=1, out='-42'.rjust(20))
        if self.sep:
            self._test_format("%+10.f", -4200, grouping=1,
                out=('-4%s200' % self.sep).rjust(10))
            self._test_format("%-10.f", -4200, grouping=1,
                out=('-4%s200' % self.sep).ljust(10))

    def test_integer_grouping(self):
        self._test_format("%d", 4200, grouping=True, out='4%s200' % self.sep)
        self._test_format("%+d", 4200, grouping=True, out='+4%s200' % self.sep)
        self._test_format("%+d", -4200, grouping=True, out='-4%s200' % self.sep)

    def test_integer_grouping_and_padding(self):
        self._test_format("%10d", 4200, grouping=True,
            out=('4%s200' % self.sep).rjust(10))
        self._test_format("%-10d", -4200, grouping=True,
            out=('-4%s200' % self.sep).ljust(10))

    def test_simple(self):
        self._test_format("%f", 1024, grouping=0, out='1024.000000')
        self._test_format("%f", 102, grouping=0, out='102.000000')
        self._test_format("%f", -42, grouping=0, out='-42.000000')
        self._test_format("%+f", -42, grouping=0, out='-42.000000')

    def test_padding(self):
        self._test_format("%20.f", -42, grouping=0, out='-42'.rjust(20))
        self._test_format("%+10.f", -4200, grouping=0, out='-4200'.rjust(10))
        self._test_format("%-10.f", 4200, grouping=0, out='4200'.ljust(10))

    def test_complex_formatting(self):
        # Spaces in formatting string
        self._test_format_string("One million is %i", 1000000, grouping=1,
            out='One million is 1%s000%s000' % (self.sep, self.sep))
        self._test_format_string("One  million is %i", 1000000, grouping=1,
            out='One  million is 1%s000%s000' % (self.sep, self.sep))
        # Dots in formatting string
        self._test_format_string(".%f.", 1000.0, out='.1000.000000.')
        # Padding
        if self.sep:
            self._test_format_string("-->  %10.2f", 4200, grouping=1,
                out='-->  ' + ('4%s200.00' % self.sep).rjust(10))
        # Asterisk formats
        self._test_format_string("%10.*f", (2, 1000), grouping=0,
            out='1000.00'.rjust(10))
        if self.sep:
            self._test_format_string("%*.*f", (10, 2, 1000), grouping=1,
                out=('1%s000.00' % self.sep).rjust(10))
        # Test more-in-one
        if self.sep:
            self._test_format_string("int %i float %.2f str %s",
                (1000, 1000.0, 'str'), grouping=1,
                out='int 1%s000 float 1%s000.00 str str' %
                (self.sep, self.sep))


class TestFormatPatternArg(unittest.TestCase):
    # Test handling of pattern argument of format

    def test_onlyOnePattern(self):
        # Issue 2522: accept exactly one % pattern, and no extra chars.
        self.assertRaises(ValueError, locale.format, "%f\n", 'foo')
        self.assertRaises(ValueError, locale.format, "%f\r", 'foo')
        self.assertRaises(ValueError, locale.format, "%f\r\n", 'foo')
        self.assertRaises(ValueError, locale.format, " %f", 'foo')
        self.assertRaises(ValueError, locale.format, "%fg", 'foo')
        self.assertRaises(ValueError, locale.format, "%^g", 'foo')
        self.assertRaises(ValueError, locale.format, "%f%%", 'foo')


class TestLocaleFormatString(unittest.TestCase):
    """General tests on locale.format_string"""

    def test_percent_escape(self):
        self.assertEqual(locale.format_string('%f%%', 1.0), '%f%%' % 1.0)
        self.assertEqual(locale.format_string('%d %f%%d', (1, 1.0)),
            '%d %f%%d' % (1, 1.0))
        self.assertEqual(locale.format_string('%(foo)s %%d', {'foo': 'bar'}),
            ('%(foo)s %%d' % {'foo': 'bar'}))

    def test_mapping(self):
        self.assertEqual(locale.format_string('%(foo)s bing.', {'foo': 'bar'}),
            ('%(foo)s bing.' % {'foo': 'bar'}))
        self.assertEqual(locale.format_string('%(foo)s', {'foo': 'bar'}),
            ('%(foo)s' % {'foo': 'bar'}))



class TestNumberFormatting(BaseLocalizedTest, EnUSNumberFormatting):
    # Test number formatting with a real English locale.

    locale_type = locale.LC_NUMERIC

    def setUp(self):
        BaseLocalizedTest.setUp(self)
        EnUSNumberFormatting.setUp(self)


class TestEnUSNumberFormatting(EnUSCookedTest, EnUSNumberFormatting):
    # Test number formatting with a cooked "en_US" locale.

    def setUp(self):
        EnUSCookedTest.setUp(self)
        EnUSNumberFormatting.setUp(self)

    def test_currency(self):
        self._test_currency(50000, "$50000.00")
        self._test_currency(50000, "$50,000.00", grouping=True)
        self._test_currency(50000, "USD 50,000.00",
            grouping=True, international=True)


class TestCNumberFormatting(CCookedTest, BaseFormattingTest):
    # Test number formatting with a cooked "C" locale.

    def test_grouping(self):
        self._test_format("%.2f", 12345.67, grouping=True, out='12345.67')

    def test_grouping_and_padding(self):
        self._test_format("%9.2f", 12345.67, grouping=True, out=' 12345.67')


class TestFrFRNumberFormatting(FrFRCookedTest, BaseFormattingTest):
    # Test number formatting with a cooked "fr_FR" locale.

    def test_decimal_point(self):
        self._test_format("%.2f", 12345.67, out='12345,67')

    def test_grouping(self):
        self._test_format("%.2f", 345.67, grouping=True, out='345,67')
        self._test_format("%.2f", 12345.67, grouping=True, out='12 345,67')

    def test_grouping_and_padding(self):
        self._test_format("%6.2f", 345.67, grouping=True, out='345,67')
        self._test_format("%7.2f", 345.67, grouping=True, out=' 345,67')
        self._test_format("%8.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format("%9.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format("%10.2f", 12345.67, grouping=True, out=' 12 345,67')
        self._test_format("%-6.2f", 345.67, grouping=True, out='345,67')
        self._test_format("%-7.2f", 345.67, grouping=True, out='345,67 ')
        self._test_format("%-8.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format("%-9.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format("%-10.2f", 12345.67, grouping=True, out='12 345,67 ')

    def test_integer_grouping(self):
        self._test_format("%d", 200, grouping=True, out='200')
        self._test_format("%d", 4200, grouping=True, out='4 200')

    def test_integer_grouping_and_padding(self):
        self._test_format("%4d", 4200, grouping=True, out='4 200')
        self._test_format("%5d", 4200, grouping=True, out='4 200')
        self._test_format("%10d", 4200, grouping=True, out='4 200'.rjust(10))
        self._test_format("%-4d", 4200, grouping=True, out='4 200')
        self._test_format("%-5d", 4200, grouping=True, out='4 200')
        self._test_format("%-10d", 4200, grouping=True, out='4 200'.ljust(10))

    def test_currency(self):
        euro = u'\u20ac'.encode('utf-8')
        self._test_currency(50000, "50000,00 " + euro)
        self._test_currency(50000, "50 000,00 " + euro, grouping=True)
        # XXX is the trailing space a bug?
        self._test_currency(50000, "50 000,00 EUR ",
            grouping=True, international=True)


class TestStringMethods(BaseLocalizedTest):
    locale_type = locale.LC_CTYPE

    if sys.platform != 'sunos5' and not sys.platform.startswith("win"):
        # Test BSD Rune locale's bug for isctype functions.

        def test_isspace(self):
            self.assertEqual('\x20'.isspace(), True)
            self.assertEqual('\xa0'.isspace(), False)
            self.assertEqual('\xa1'.isspace(), False)

        def test_isalpha(self):
            self.assertEqual('\xc0'.isalpha(), False)

        def test_isalnum(self):
            self.assertEqual('\xc0'.isalnum(), False)

        def test_isupper(self):
            self.assertEqual('\xc0'.isupper(), False)

        def test_islower(self):
            self.assertEqual('\xc0'.islower(), False)

        def test_lower(self):
            self.assertEqual('\xcc\x85'.lower(), '\xcc\x85')

        def test_upper(self):
            self.assertEqual('\xed\x95\xa0'.upper(), '\xed\x95\xa0')

        def test_strip(self):
            self.assertEqual('\xed\x95\xa0'.strip(), '\xed\x95\xa0')

        def test_split(self):
            self.assertEqual('\xec\xa0\xbc'.split(), ['\xec\xa0\xbc'])


class TestMiscellaneous(unittest.TestCase):
    def test_getpreferredencoding(self):
        # Invoke getpreferredencoding to make sure it does not cause exceptions.
        enc = locale.getpreferredencoding()
        if enc:
            # If encoding non-empty, make sure it is valid
            codecs.lookup(enc)

    if hasattr(locale, "strcoll"):
        def test_strcoll_3303(self):
            # test crasher from bug #3303
            self.assertRaises(TypeError, locale.strcoll, u"a", None)

    def test_setlocale_category(self):
        locale.setlocale(locale.LC_ALL)
        locale.setlocale(locale.LC_TIME)
        locale.setlocale(locale.LC_CTYPE)
        locale.setlocale(locale.LC_COLLATE)
        locale.setlocale(locale.LC_MONETARY)
        locale.setlocale(locale.LC_NUMERIC)

        # crasher from bug #7419
        self.assertRaises(locale.Error, locale.setlocale, 12345)


def test_main():
    tests = [
        TestMiscellaneous,
        TestFormatPatternArg,
        TestLocaleFormatString,
        TestEnUSNumberFormatting,
        TestCNumberFormatting,
        TestFrFRNumberFormatting,
    ]
    # SkipTest can't be raised inside unittests, handle it manually instead
    try:
        get_enUS_locale()
    except unittest.SkipTest as e:
        if verbose:
            print "Some tests will be disabled: %s" % e
    else:
        tests += [TestNumberFormatting, TestStringMethods]
    run_unittest(*tests)

if __name__ == '__main__':
    test_main()
