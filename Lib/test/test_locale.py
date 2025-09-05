from decimal import Decimal
from test import support
from test.support import cpython_only, verbose, is_android, linked_to_musl, os_helper
from test.support.warnings_helper import check_warnings
from test.support.import_helper import ensure_lazy_imports, import_fresh_module
from unittest import mock
import unittest
import locale
import os
import sys
import codecs

class LazyImportTest(unittest.TestCase):
    @cpython_only
    def test_lazy_import(self):
        ensure_lazy_imports("locale", {"re", "warnings"})


class BaseLocalizedTest(unittest.TestCase):
    #
    # Base class for tests using a real locale
    #

    @classmethod
    def setUpClass(cls):
        if sys.platform == 'darwin':
            import os
            tlocs = ("en_US.UTF-8", "en_US.ISO8859-1", "en_US")
            if int(os.uname().release.split('.')[0]) < 10:
                # The locale test work fine on OSX 10.6, I (ronaldoussoren)
                # haven't had time yet to verify if tests work on OSX 10.5
                # (10.4 is known to be bad)
                raise unittest.SkipTest("Locale support on MacOSX is minimal")
        elif sys.platform.startswith("win"):
            tlocs = ("En", "English")
        else:
            tlocs = ("en_US.UTF-8", "en_US.ISO8859-1",
                     "en_US.US-ASCII", "en_US")
        try:
            oldlocale = locale.setlocale(locale.LC_NUMERIC)
            for tloc in tlocs:
                try:
                    locale.setlocale(locale.LC_NUMERIC, tloc)
                except locale.Error:
                    continue
                break
            else:
                raise unittest.SkipTest("Test locale not supported "
                                        "(tried %s)" % (', '.join(tlocs)))
            cls.enUS_locale = tloc
        finally:
            locale.setlocale(locale.LC_NUMERIC, oldlocale)

    def setUp(self):
        oldlocale = locale.setlocale(self.locale_type)
        self.addCleanup(locale.setlocale, self.locale_type, oldlocale)
        locale.setlocale(self.locale_type, self.enUS_locale)
        if verbose:
            print("testing with %r..." % self.enUS_locale, end=' ', flush=True)


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
        'currency_symbol': '\u20ac',
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

    def _test_format_string(self, format, value, out, **format_opts):
        self.assertEqual(
            locale.format_string(format, value, **format_opts), out)

    def _test_currency(self, value, out, **format_opts):
        self.assertEqual(locale.currency(value, **format_opts), out)


class EnUSNumberFormatting(BaseFormattingTest):
    # XXX there is a grouping + padding bug when the thousands separator
    # is empty but the grouping array contains values (e.g. Solaris 10)

    def setUp(self):
        self.sep = locale.localeconv()['thousands_sep']

    def test_grouping(self):
        self._test_format_string("%f", 1024, grouping=1, out='1%s024.000000' % self.sep)
        self._test_format_string("%f", 102, grouping=1, out='102.000000')
        self._test_format_string("%f", -42, grouping=1, out='-42.000000')
        self._test_format_string("%+f", -42, grouping=1, out='-42.000000')

    def test_grouping_and_padding(self):
        self._test_format_string("%20.f", -42, grouping=1, out='-42'.rjust(20))
        if self.sep:
            self._test_format_string("%+10.f", -4200, grouping=1,
                out=('-4%s200' % self.sep).rjust(10))
            self._test_format_string("%-10.f", -4200, grouping=1,
                out=('-4%s200' % self.sep).ljust(10))

    def test_integer_grouping(self):
        self._test_format_string("%d", 4200, grouping=True, out='4%s200' % self.sep)
        self._test_format_string("%+d", 4200, grouping=True, out='+4%s200' % self.sep)
        self._test_format_string("%+d", -4200, grouping=True, out='-4%s200' % self.sep)

    def test_integer_grouping_and_padding(self):
        self._test_format_string("%10d", 4200, grouping=True,
            out=('4%s200' % self.sep).rjust(10))
        self._test_format_string("%-10d", -4200, grouping=True,
            out=('-4%s200' % self.sep).ljust(10))

    def test_simple(self):
        self._test_format_string("%f", 1024, grouping=0, out='1024.000000')
        self._test_format_string("%f", 102, grouping=0, out='102.000000')
        self._test_format_string("%f", -42, grouping=0, out='-42.000000')
        self._test_format_string("%+f", -42, grouping=0, out='-42.000000')

    def test_padding(self):
        self._test_format_string("%20.f", -42, grouping=0, out='-42'.rjust(20))
        self._test_format_string("%+10.f", -4200, grouping=0, out='-4200'.rjust(10))
        self._test_format_string("%-10.f", 4200, grouping=0, out='4200'.ljust(10))

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

        self._test_format_string("total=%i%%", 100, out='total=100%')
        self._test_format_string("newline: %i\n", 3, out='newline: 3\n')
        self._test_format_string("extra: %ii", 3, out='extra: 3i')


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
        self._test_format_string("%.2f", 12345.67, grouping=True, out='12345.67')

    def test_grouping_and_padding(self):
        self._test_format_string("%9.2f", 12345.67, grouping=True, out=' 12345.67')


class TestFrFRNumberFormatting(FrFRCookedTest, BaseFormattingTest):
    # Test number formatting with a cooked "fr_FR" locale.

    def test_decimal_point(self):
        self._test_format_string("%.2f", 12345.67, out='12345,67')

    def test_grouping(self):
        self._test_format_string("%.2f", 345.67, grouping=True, out='345,67')
        self._test_format_string("%.2f", 12345.67, grouping=True, out='12 345,67')

    def test_grouping_and_padding(self):
        self._test_format_string("%6.2f", 345.67, grouping=True, out='345,67')
        self._test_format_string("%7.2f", 345.67, grouping=True, out=' 345,67')
        self._test_format_string("%8.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format_string("%9.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format_string("%10.2f", 12345.67, grouping=True, out=' 12 345,67')
        self._test_format_string("%-6.2f", 345.67, grouping=True, out='345,67')
        self._test_format_string("%-7.2f", 345.67, grouping=True, out='345,67 ')
        self._test_format_string("%-8.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format_string("%-9.2f", 12345.67, grouping=True, out='12 345,67')
        self._test_format_string("%-10.2f", 12345.67, grouping=True, out='12 345,67 ')

    def test_integer_grouping(self):
        self._test_format_string("%d", 200, grouping=True, out='200')
        self._test_format_string("%d", 4200, grouping=True, out='4 200')

    def test_integer_grouping_and_padding(self):
        self._test_format_string("%4d", 4200, grouping=True, out='4 200')
        self._test_format_string("%5d", 4200, grouping=True, out='4 200')
        self._test_format_string("%10d", 4200, grouping=True, out='4 200'.rjust(10))
        self._test_format_string("%-4d", 4200, grouping=True, out='4 200')
        self._test_format_string("%-5d", 4200, grouping=True, out='4 200')
        self._test_format_string("%-10d", 4200, grouping=True, out='4 200'.ljust(10))

    def test_currency(self):
        euro = '\u20ac'
        self._test_currency(50000, "50000,00 " + euro)
        self._test_currency(50000, "50 000,00 " + euro, grouping=True)
        self._test_currency(50000, "50 000,00 EUR",
            grouping=True, international=True)


class TestCollation(unittest.TestCase):
    # Test string collation functions

    def test_strcoll(self):
        self.assertLess(locale.strcoll('a', 'b'), 0)
        self.assertEqual(locale.strcoll('a', 'a'), 0)
        self.assertGreater(locale.strcoll('b', 'a'), 0)
        # embedded null character
        self.assertRaises(ValueError, locale.strcoll, 'a\0', 'a')
        self.assertRaises(ValueError, locale.strcoll, 'a', 'a\0')

    def test_strxfrm(self):
        self.assertLess(locale.strxfrm('a'), locale.strxfrm('b'))
        # embedded null character
        self.assertRaises(ValueError, locale.strxfrm, 'a\0')


class TestEnUSCollation(BaseLocalizedTest, TestCollation):
    # Test string collation functions with a real English locale

    locale_type = locale.LC_ALL

    def setUp(self):
        enc = codecs.lookup(locale.getencoding() or 'ascii').name
        if enc not in ('utf-8', 'iso8859-1', 'cp1252'):
            raise unittest.SkipTest('encoding not suitable')
        if enc != 'iso8859-1' and (sys.platform == 'darwin' or is_android or
                                   sys.platform.startswith('freebsd')):
            raise unittest.SkipTest('wcscoll/wcsxfrm have known bugs')
        BaseLocalizedTest.setUp(self)

    @unittest.skipIf(sys.platform.startswith('aix'),
                     'bpo-29972: broken test on AIX')
    @unittest.skipIf(linked_to_musl(), "musl libc issue, bpo-46390")
    @unittest.skipIf(sys.platform.startswith("netbsd"),
                     "gh-124108: NetBSD doesn't support UTF-8 for LC_COLLATE")
    def test_strcoll_with_diacritic(self):
        self.assertLess(locale.strcoll('à', 'b'), 0)

    @unittest.skipIf(sys.platform.startswith('aix'),
                     'bpo-29972: broken test on AIX')
    @unittest.skipIf(linked_to_musl(), "musl libc issue, bpo-46390")
    @unittest.skipIf(sys.platform.startswith("netbsd"),
                     "gh-124108: NetBSD doesn't support UTF-8 for LC_COLLATE")
    def test_strxfrm_with_diacritic(self):
        self.assertLess(locale.strxfrm('à'), locale.strxfrm('b'))


class NormalizeTest(unittest.TestCase):
    def check(self, localename, expected):
        self.assertEqual(locale.normalize(localename), expected, msg=localename)

    def test_locale_alias(self):
        for localename, alias in locale.locale_alias.items():
            with self.subTest(locale=(localename, alias)):
                self.check(localename, alias)

    def test_empty(self):
        self.check('', '')

    def test_c(self):
        self.check('c', 'C')
        self.check('posix', 'C')

    def test_c_utf8(self):
        self.check('c.utf8', 'C.UTF-8')
        self.check('C.UTF-8', 'C.UTF-8')

    def test_english(self):
        self.check('en', 'en_US.ISO8859-1')
        self.check('EN', 'en_US.ISO8859-1')
        self.check('en.iso88591', 'en_US.ISO8859-1')
        self.check('en_US', 'en_US.ISO8859-1')
        self.check('en_us', 'en_US.ISO8859-1')
        self.check('en_GB', 'en_GB.ISO8859-1')
        self.check('en_US.UTF-8', 'en_US.UTF-8')
        self.check('en_US.utf8', 'en_US.UTF-8')
        self.check('en_US:UTF-8', 'en_US.UTF-8')
        self.check('en_US.ISO8859-1', 'en_US.ISO8859-1')
        self.check('en_US.US-ASCII', 'en_US.ISO8859-1')
        self.check('en_US.88591', 'en_US.ISO8859-1')
        self.check('en_US.885915', 'en_US.ISO8859-15')
        self.check('english', 'en_EN.ISO8859-1')
        self.check('english_uk.ascii', 'en_GB.ISO8859-1')

    def test_hyphenated_encoding(self):
        self.check('az_AZ.iso88599e', 'az_AZ.ISO8859-9E')
        self.check('az_AZ.ISO8859-9E', 'az_AZ.ISO8859-9E')
        self.check('tt_RU.koi8c', 'tt_RU.KOI8-C')
        self.check('tt_RU.KOI8-C', 'tt_RU.KOI8-C')
        self.check('lo_LA.cp1133', 'lo_LA.IBM-CP1133')
        self.check('lo_LA.ibmcp1133', 'lo_LA.IBM-CP1133')
        self.check('lo_LA.IBM-CP1133', 'lo_LA.IBM-CP1133')
        self.check('uk_ua.microsoftcp1251', 'uk_UA.CP1251')
        self.check('uk_ua.microsoft-cp1251', 'uk_UA.CP1251')
        self.check('ka_ge.georgianacademy', 'ka_GE.GEORGIAN-ACADEMY')
        self.check('ka_GE.GEORGIAN-ACADEMY', 'ka_GE.GEORGIAN-ACADEMY')
        self.check('cs_CZ.iso88592', 'cs_CZ.ISO8859-2')
        self.check('cs_CZ.ISO8859-2', 'cs_CZ.ISO8859-2')

    def test_euro_modifier(self):
        self.check('de_DE@euro', 'de_DE.ISO8859-15@euro')
        self.check('en_US.ISO8859-15@euro', 'en_US.ISO8859-15@euro')
        self.check('de_DE.utf8@euro', 'de_DE.UTF-8')

    def test_latin_modifier(self):
        self.check('be_BY.UTF-8@latin', 'be_BY.UTF-8@latin')
        self.check('sr_RS.UTF-8@latin', 'sr_RS.UTF-8@latin')
        self.check('sr_RS.UTF-8@latn', 'sr_RS.UTF-8@latin')

    def test_valencia_modifier(self):
        self.check('ca_ES.UTF-8@valencia', 'ca_ES.UTF-8@valencia')
        self.check('ca_ES@valencia', 'ca_ES.UTF-8@valencia')
        self.check('ca@valencia', 'ca_ES.ISO8859-1@valencia')

    def test_devanagari_modifier(self):
        self.check('ks_IN.UTF-8@devanagari', 'ks_IN.UTF-8@devanagari')
        self.check('ks_IN@devanagari', 'ks_IN.UTF-8@devanagari')
        self.check('ks@devanagari', 'ks_IN.UTF-8@devanagari')
        self.check('ks_IN.UTF-8', 'ks_IN.UTF-8')
        self.check('ks_IN', 'ks_IN.UTF-8')
        self.check('ks', 'ks_IN.UTF-8')
        self.check('sd_IN.UTF-8@devanagari', 'sd_IN.UTF-8@devanagari')
        self.check('sd_IN@devanagari', 'sd_IN.UTF-8@devanagari')
        self.check('sd@devanagari', 'sd_IN.UTF-8@devanagari')
        self.check('sd_IN.UTF-8', 'sd_IN.UTF-8')
        self.check('sd_IN', 'sd_IN.UTF-8')
        self.check('sd', 'sd_IN.UTF-8')

    def test_euc_encoding(self):
        self.check('ja_jp.euc', 'ja_JP.eucJP')
        self.check('ja_jp.eucjp', 'ja_JP.eucJP')
        self.check('ko_kr.euc', 'ko_KR.eucKR')
        self.check('ko_kr.euckr', 'ko_KR.eucKR')
        self.check('zh_cn.euc', 'zh_CN.eucCN')
        self.check('zh_tw.euc', 'zh_TW.eucTW')
        self.check('zh_tw.euctw', 'zh_TW.eucTW')

    def test_japanese(self):
        self.check('ja', 'ja_JP.eucJP')
        self.check('ja.jis', 'ja_JP.JIS7')
        self.check('ja.sjis', 'ja_JP.SJIS')
        self.check('ja_jp', 'ja_JP.eucJP')
        self.check('ja_jp.ajec', 'ja_JP.eucJP')
        self.check('ja_jp.euc', 'ja_JP.eucJP')
        self.check('ja_jp.eucjp', 'ja_JP.eucJP')
        self.check('ja_jp.iso-2022-jp', 'ja_JP.JIS7')
        self.check('ja_jp.iso2022jp', 'ja_JP.JIS7')
        self.check('ja_jp.jis', 'ja_JP.JIS7')
        self.check('ja_jp.jis7', 'ja_JP.JIS7')
        self.check('ja_jp.mscode', 'ja_JP.SJIS')
        self.check('ja_jp.pck', 'ja_JP.SJIS')
        self.check('ja_jp.sjis', 'ja_JP.SJIS')
        self.check('ja_jp.ujis', 'ja_JP.eucJP')
        self.check('ja_jp.utf8', 'ja_JP.UTF-8')
        self.check('japan', 'ja_JP.eucJP')
        self.check('japanese', 'ja_JP.eucJP')
        self.check('japanese-euc', 'ja_JP.eucJP')
        self.check('japanese.euc', 'ja_JP.eucJP')
        self.check('japanese.sjis', 'ja_JP.SJIS')
        self.check('jp_jp', 'ja_JP.eucJP')


class TestRealLocales(unittest.TestCase):
    def setUp(self):
        oldlocale = locale.setlocale(locale.LC_CTYPE)
        self.addCleanup(locale.setlocale, locale.LC_CTYPE, oldlocale)

    def test_getsetlocale_issue1813(self):
        # Issue #1813: setting and getting the locale under a Turkish locale
        try:
            locale.setlocale(locale.LC_CTYPE, 'tr_TR')
        except locale.Error:
            # Unsupported locale on this system
            self.skipTest('test needs Turkish locale')
        loc = locale.getlocale(locale.LC_CTYPE)
        if verbose:
            print('testing with %a' % (loc,), end=' ', flush=True)
        try:
            locale.setlocale(locale.LC_CTYPE, loc)
        except locale.Error as exc:
            # bpo-37945: setlocale(LC_CTYPE) fails with getlocale(LC_CTYPE)
            # and the tr_TR locale on Windows. getlocale() builds a locale
            # which is not recognize by setlocale().
            self.skipTest(f"setlocale(LC_CTYPE, {loc!r}) failed: {exc!r}")
        self.assertEqual(loc, locale.getlocale(locale.LC_CTYPE))

    @unittest.skipUnless(os.name == 'nt', 'requires Windows')
    def test_setlocale_long_encoding(self):
        with self.assertRaises(locale.Error):
            locale.setlocale(locale.LC_CTYPE, 'English.%016d' % 1252)
        locale.setlocale(locale.LC_CTYPE, 'English.%015d' % 1252)
        loc = locale.setlocale(locale.LC_ALL)
        self.assertIn('.1252', loc)
        loc2 = loc.replace('.1252', '.%016d' % 1252, 1)
        with self.assertRaises(locale.Error):
            locale.setlocale(locale.LC_ALL, loc2)
        loc2 = loc.replace('.1252', '.%015d' % 1252, 1)
        locale.setlocale(locale.LC_ALL, loc2)

        # gh-137273: Debug assertion failure on Windows for long encoding.
        with self.assertRaises(locale.Error):
            locale.setlocale(locale.LC_CTYPE, 'en_US.' + 'x'*16)
        locale.setlocale(locale.LC_CTYPE, 'en_US.UTF-8')
        loc = locale.setlocale(locale.LC_ALL)
        self.assertIn('.UTF-8', loc)
        loc2 = loc.replace('.UTF-8', '.' + 'x'*16, 1)
        with self.assertRaises(locale.Error):
            locale.setlocale(locale.LC_ALL, loc2)

    @support.subTests('localename,localetuple', [
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', 'iso885915')),
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', 'iso88591')),
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', 'ISO8859-15')),
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', 'ISO8859-1')),
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', None)),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', 'iso885915')),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', 'iso88591')),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', 'ISO8859-15')),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', 'ISO8859-1')),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', None)),
        ('el_GR.ISO8859-7@euro', ('el_GR@euro', 'iso88597')),
        ('el_GR.ISO8859-7@euro', ('el_GR@euro', 'ISO8859-7')),
        ('el_GR.ISO8859-7@euro', ('el_GR@euro', None)),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', 'iso885915')),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', 'iso88591')),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', 'ISO8859-15')),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', 'ISO8859-1')),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', None)),
        ('ca_ES.UTF-8@valencia', ('ca_ES@valencia', 'utf8')),
        ('ca_ES.UTF-8@valencia', ('ca_ES@valencia', 'UTF-8')),
        ('ca_ES.UTF-8@valencia', ('ca_ES@valencia', None)),
        ('ks_IN.UTF-8@devanagari', ('ks_IN@devanagari', 'utf8')),
        ('ks_IN.UTF-8@devanagari', ('ks_IN@devanagari', 'UTF-8')),
        ('ks_IN.UTF-8@devanagari', ('ks_IN@devanagari', None)),
        ('sd_IN.UTF-8@devanagari', ('sd_IN@devanagari', 'utf8')),
        ('sd_IN.UTF-8@devanagari', ('sd_IN@devanagari', 'UTF-8')),
        ('sd_IN.UTF-8@devanagari', ('sd_IN@devanagari', None)),
        ('be_BY.UTF-8@latin', ('be_BY@latin', 'utf8')),
        ('be_BY.UTF-8@latin', ('be_BY@latin', 'UTF-8')),
        ('be_BY.UTF-8@latin', ('be_BY@latin', None)),
        ('sr_RS.UTF-8@latin', ('sr_RS@latin', 'utf8')),
        ('sr_RS.UTF-8@latin', ('sr_RS@latin', 'UTF-8')),
        ('sr_RS.UTF-8@latin', ('sr_RS@latin', None)),
        ('ug_CN.UTF-8@latin', ('ug_CN@latin', 'utf8')),
        ('ug_CN.UTF-8@latin', ('ug_CN@latin', 'UTF-8')),
        ('ug_CN.UTF-8@latin', ('ug_CN@latin', None)),
        ('uz_UZ.UTF-8@cyrillic', ('uz_UZ@cyrillic', 'utf8')),
        ('uz_UZ.UTF-8@cyrillic', ('uz_UZ@cyrillic', 'UTF-8')),
        ('uz_UZ.UTF-8@cyrillic', ('uz_UZ@cyrillic', None)),
    ])
    def test_setlocale_with_modifier(self, localename, localetuple):
        try:
            locale.setlocale(locale.LC_CTYPE, localename)
        except locale.Error as exc:
            self.skipTest(str(exc))
        loc = locale.setlocale(locale.LC_CTYPE, localetuple)
        self.assertEqual(loc, localename)

        loctuple = locale.getlocale(locale.LC_CTYPE)
        loc = locale.setlocale(locale.LC_CTYPE, loctuple)
        self.assertEqual(loc, localename)

    @support.subTests('localename,localetuple', [
        ('fr_FR.iso885915@euro', ('fr_FR@euro', 'ISO8859-15')),
        ('fr_FR.ISO8859-15@euro', ('fr_FR@euro', 'ISO8859-15')),
        ('fr_FR@euro', ('fr_FR@euro', 'ISO8859-15')),
        ('de_DE.iso885915@euro', ('de_DE@euro', 'ISO8859-15')),
        ('de_DE.ISO8859-15@euro', ('de_DE@euro', 'ISO8859-15')),
        ('de_DE@euro', ('de_DE@euro', 'ISO8859-15')),
        ('el_GR.iso88597@euro', ('el_GR@euro', 'ISO8859-7')),
        ('el_GR.ISO8859-7@euro', ('el_GR@euro', 'ISO8859-7')),
        ('el_GR@euro', ('el_GR@euro', 'ISO8859-7')),
        ('ca_ES.iso885915@euro', ('ca_ES@euro', 'ISO8859-15')),
        ('ca_ES.ISO8859-15@euro', ('ca_ES@euro', 'ISO8859-15')),
        ('ca_ES@euro', ('ca_ES@euro', 'ISO8859-15')),
        ('ca_ES.utf8@valencia', ('ca_ES@valencia', 'UTF-8')),
        ('ca_ES.UTF-8@valencia', ('ca_ES@valencia', 'UTF-8')),
        ('ca_ES@valencia', ('ca_ES@valencia', 'UTF-8')),
        ('ks_IN.utf8@devanagari', ('ks_IN@devanagari', 'UTF-8')),
        ('ks_IN.UTF-8@devanagari', ('ks_IN@devanagari', 'UTF-8')),
        ('ks_IN@devanagari', ('ks_IN@devanagari', 'UTF-8')),
        ('sd_IN.utf8@devanagari', ('sd_IN@devanagari', 'UTF-8')),
        ('sd_IN.UTF-8@devanagari', ('sd_IN@devanagari', 'UTF-8')),
        ('sd_IN@devanagari', ('sd_IN@devanagari', 'UTF-8')),
        ('be_BY.utf8@latin', ('be_BY@latin', 'UTF-8')),
        ('be_BY.UTF-8@latin', ('be_BY@latin', 'UTF-8')),
        ('be_BY@latin', ('be_BY@latin', 'UTF-8')),
        ('sr_RS.utf8@latin', ('sr_RS@latin', 'UTF-8')),
        ('sr_RS.UTF-8@latin', ('sr_RS@latin', 'UTF-8')),
        ('sr_RS@latin', ('sr_RS@latin', 'UTF-8')),
        ('ug_CN.utf8@latin', ('ug_CN@latin', 'UTF-8')),
        ('ug_CN.UTF-8@latin', ('ug_CN@latin', 'UTF-8')),
        ('ug_CN@latin', ('ug_CN@latin', 'UTF-8')),
        ('uz_UZ.utf8@cyrillic', ('uz_UZ@cyrillic', 'UTF-8')),
        ('uz_UZ.UTF-8@cyrillic', ('uz_UZ@cyrillic', 'UTF-8')),
        ('uz_UZ@cyrillic', ('uz_UZ@cyrillic', 'UTF-8')),
    ])
    def test_getlocale_with_modifier(self, localename, localetuple):
        try:
            locale.setlocale(locale.LC_CTYPE, localename)
        except locale.Error as exc:
            self.skipTest(str(exc))
        loctuple = locale.getlocale(locale.LC_CTYPE)
        self.assertEqual(loctuple, localetuple)

        locale.setlocale(locale.LC_CTYPE, loctuple)
        self.assertEqual(locale.getlocale(locale.LC_CTYPE), localetuple)


class TestMiscellaneous(unittest.TestCase):
    def test_defaults_UTF8(self):
        # Issue #18378: on (at least) macOS setting LC_CTYPE to "UTF-8" is
        # valid. Furthermore LC_CTYPE=UTF is used by the UTF-8 locale coercing
        # during interpreter startup (on macOS).
        import _locale

        self.assertEqual(locale._parse_localename('UTF-8'), (None, 'UTF-8'))

        if hasattr(_locale, '_getdefaultlocale'):
            orig_getlocale = _locale._getdefaultlocale
            del _locale._getdefaultlocale
        else:
            orig_getlocale = None

        try:
            with os_helper.EnvironmentVarGuard() as env:
                env.unset('LC_ALL', 'LC_CTYPE', 'LANG', 'LANGUAGE')
                env.set('LC_CTYPE', 'UTF-8')

                with check_warnings(('', DeprecationWarning)):
                    self.assertEqual(locale.getdefaultlocale(), (None, 'UTF-8'))
        finally:
            if orig_getlocale is not None:
                _locale._getdefaultlocale = orig_getlocale

    def test_getencoding(self):
        # Invoke getencoding to make sure it does not cause exceptions.
        enc = locale.getencoding()
        self.assertIsInstance(enc, str)
        self.assertNotEqual(enc, "")
        # make sure it is valid
        codecs.lookup(enc)

    def test_getencoding_fallback(self):
        # When _locale.getencoding() is missing, locale.getencoding() uses
        # the Python filesystem
        encoding = 'FALLBACK_ENCODING'
        with mock.patch.object(sys, 'getfilesystemencoding',
                               return_value=encoding):
            locale_fallback = import_fresh_module('locale', blocked=['_locale'])
            self.assertEqual(locale_fallback.getencoding(), encoding)

    def test_getpreferredencoding(self):
        # Invoke getpreferredencoding to make sure it does not cause exceptions.
        enc = locale.getpreferredencoding()
        if enc:
            # If encoding non-empty, make sure it is valid
            codecs.lookup(enc)

    def test_strcoll_3303(self):
        # test crasher from bug #3303
        self.assertRaises(TypeError, locale.strcoll, "a", None)
        self.assertRaises(TypeError, locale.strcoll, b"a", None)

    def test_setlocale_category(self):
        locale.setlocale(locale.LC_ALL)
        locale.setlocale(locale.LC_TIME)
        locale.setlocale(locale.LC_CTYPE)
        locale.setlocale(locale.LC_COLLATE)
        locale.setlocale(locale.LC_MONETARY)
        locale.setlocale(locale.LC_NUMERIC)

        # crasher from bug #7419
        self.assertRaises(locale.Error, locale.setlocale, 12345)

    def test_invalid_locale_format_in_localetuple(self):
        with self.assertRaises(TypeError):
            locale.setlocale(locale.LC_ALL, b'fi_FI')

    def test_invalid_iterable_in_localetuple(self):
        with self.assertRaises(TypeError):
            locale.setlocale(locale.LC_ALL, (b'not', b'valid'))


class BaseDelocalizeTest(BaseLocalizedTest):

    def _test_delocalize(self, value, out):
        self.assertEqual(locale.delocalize(value), out)

    def _test_atof(self, value, out):
        self.assertEqual(locale.atof(value), out)

    def _test_atoi(self, value, out):
        self.assertEqual(locale.atoi(value), out)


class TestEnUSDelocalize(EnUSCookedTest, BaseDelocalizeTest):

    def test_delocalize(self):
        self._test_delocalize('50000.00', '50000.00')
        self._test_delocalize('50,000.00', '50000.00')

    def test_atof(self):
        self._test_atof('50000.00', 50000.)
        self._test_atof('50,000.00', 50000.)

    def test_atoi(self):
        self._test_atoi('50000', 50000)
        self._test_atoi('50,000', 50000)


class TestCDelocalizeTest(CCookedTest, BaseDelocalizeTest):

    def test_delocalize(self):
        self._test_delocalize('50000.00', '50000.00')

    def test_atof(self):
        self._test_atof('50000.00', 50000.)

    def test_atoi(self):
        self._test_atoi('50000', 50000)


class TestfrFRDelocalizeTest(FrFRCookedTest, BaseDelocalizeTest):

    def test_delocalize(self):
        self._test_delocalize('50000,00', '50000.00')
        self._test_delocalize('50 000,00', '50000.00')

    def test_atof(self):
        self._test_atof('50000,00', 50000.)
        self._test_atof('50 000,00', 50000.)

    def test_atoi(self):
        self._test_atoi('50000', 50000)
        self._test_atoi('50 000', 50000)


class BaseLocalizeTest(BaseLocalizedTest):

    def _test_localize(self, value, out, grouping=False):
        self.assertEqual(locale.localize(value, grouping=grouping), out)


class TestEnUSLocalize(EnUSCookedTest, BaseLocalizeTest):

    def test_localize(self):
        self._test_localize('50000.00', '50000.00')
        self._test_localize(
            '{0:.16f}'.format(Decimal('1.15')), '1.1500000000000000')


class TestCLocalize(CCookedTest, BaseLocalizeTest):

    def test_localize(self):
        self._test_localize('50000.00', '50000.00')


class TestfrFRLocalize(FrFRCookedTest, BaseLocalizeTest):

    def test_localize(self):
        self._test_localize('50000.00', '50000,00')
        self._test_localize('50000.00', '50 000,00', grouping=True)


if __name__ == '__main__':
    unittest.main()
