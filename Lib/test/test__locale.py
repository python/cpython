from _locale import (setlocale, LC_ALL, LC_CTYPE, LC_NUMERIC, LC_TIME, localeconv, Error)
try:
    from _locale import (RADIXCHAR, THOUSEP, nl_langinfo)
except ImportError:
    nl_langinfo = None

import locale
import sys
import unittest
from platform import uname

from test import support

if uname().system == "Darwin":
    maj, min, mic = [int(part) for part in uname().release.split(".")]
    if (maj, min, mic) < (8, 0, 0):
        raise unittest.SkipTest("locale support broken for OS X < 10.4")

candidate_locales = ['es_UY', 'fr_FR', 'fi_FI', 'es_CO', 'pt_PT', 'it_IT',
    'et_EE', 'es_PY', 'no_NO', 'nl_NL', 'lv_LV', 'el_GR', 'be_BY', 'fr_BE',
    'ro_RO', 'ru_UA', 'ru_RU', 'es_VE', 'ca_ES', 'se_NO', 'es_EC', 'id_ID',
    'ka_GE', 'es_CL', 'wa_BE', 'hu_HU', 'lt_LT', 'sl_SI', 'hr_HR', 'es_AR',
    'es_ES', 'oc_FR', 'gl_ES', 'bg_BG', 'is_IS', 'mk_MK', 'de_AT', 'pt_BR',
    'da_DK', 'nn_NO', 'cs_CZ', 'de_LU', 'es_BO', 'sq_AL', 'sk_SK', 'fr_CH',
    'de_DE', 'sr_YU', 'br_FR', 'nl_BE', 'sv_FI', 'pl_PL', 'fr_CA', 'fo_FO',
    'bs_BA', 'fr_LU', 'kl_GL', 'fa_IR', 'de_BE', 'sv_SE', 'it_CH', 'uk_UA',
    'eu_ES', 'vi_VN', 'af_ZA', 'nb_NO', 'en_DK', 'tg_TJ', 'ps_AF', 'en_US',
    'fr_FR.ISO8859-1', 'fr_FR.UTF-8', 'fr_FR.ISO8859-15@euro',
    'ru_RU.KOI8-R', 'ko_KR.eucKR',
    'ja_JP.UTF-8', 'lzh_TW.UTF-8', 'my_MM.UTF-8', 'or_IN.UTF-8', 'shn_MM.UTF-8',
    'ar_AE.UTF-8', 'bn_IN.UTF-8', 'mr_IN.UTF-8', 'th_TH.TIS620',
]

def setUpModule():
    global candidate_locales
    # Issue #13441: Skip some locales (e.g. cs_CZ and hu_HU) on Solaris to
    # workaround a mbstowcs() bug. For example, on Solaris, the hu_HU locale uses
    # the locale encoding ISO-8859-2, the thousands separator is b'\xA0' and it is
    # decoded as U+30000020 (an invalid character) by mbstowcs().
    if sys.platform == 'sunos5':
        old_locale = locale.setlocale(locale.LC_ALL)
        try:
            locales = []
            for loc in candidate_locales:
                try:
                    locale.setlocale(locale.LC_ALL, loc)
                except Error:
                    continue
                encoding = locale.getencoding()
                try:
                    localeconv()
                except Exception as err:
                    print("WARNING: Skip locale %s (encoding %s): [%s] %s"
                        % (loc, encoding, type(err), err))
                else:
                    locales.append(loc)
            candidate_locales = locales
        finally:
            locale.setlocale(locale.LC_ALL, old_locale)

    # Workaround for MSVC6(debug) crash bug
    if "MSC v.1200" in sys.version:
        def accept(loc):
            a = loc.split(".")
            return not(len(a) == 2 and len(a[-1]) >= 9)
        candidate_locales = [loc for loc in candidate_locales if accept(loc)]

# List known locale values to test against when available.
# Dict formatted as ``<locale> : (<decimal_point>, <thousands_sep>)``.  If a
# value is not known, use '' .
known_numerics = {
    'en_US': ('.', ','),
    'de_DE' : (',', '.'),
    # The French thousands separator may be a breaking or non-breaking space
    # depending on the platform, so do not test it
    'fr_FR' : (',', ''),
    'ps_AF': ('\u066b', '\u066c'),
}

known_alt_digits = {
    'C': (0, {}),
    'en_US': (0, {}),
    'fa_IR': (100, {0: '\u06f0\u06f0', 10: '\u06f1\u06f0', 99: '\u06f9\u06f9'}),
    'ja_JP': (100, {1: '\u4e00', 10: '\u5341', 99: '\u4e5d\u5341\u4e5d'}),
    'lzh_TW': (32, {0: '\u3007', 10: '\u5341', 31: '\u5345\u4e00'}),
    'my_MM': (100, {0: '\u1040\u1040', 10: '\u1041\u1040', 99: '\u1049\u1049'}),
    'or_IN': (100, {0: '\u0b66', 10: '\u0b67\u0b66', 99: '\u0b6f\u0b6f'}),
    'shn_MM': (100, {0: '\u1090\u1090', 10: '\u1091\u1090', 99: '\u1099\u1099'}),
    'ar_AE': (100, {0: '\u0660', 10: '\u0661\u0660', 99: '\u0669\u0669'}),
    'bn_IN': (100, {0: '\u09e6', 10: '\u09e7\u09e6', 99: '\u09ef\u09ef'}),
}

known_era = {
    'C': (0, ''),
    'en_US': (0, ''),
    'ja_JP': (11, '+:1:2019/05/01:2019/12/31:令和:%EC元年'),
    'zh_TW': (3, '+:1:1912/01/01:1912/12/31:民國:%EC元年'),
    'th_TW': (1, '+:1:-543/01/01:+*:พ.ศ.:%EC %Ey'),
}

if sys.platform == 'win32':
    # ps_AF doesn't work on Windows: see bpo-38324 (msg361830)
    del known_numerics['ps_AF']

if sys.platform == 'sunos5':
    # On Solaris, Japanese ERAs start with the year 1927,
    # and thus there's less of them.
    known_era['ja_JP'] = (5, '+:1:2019/05/01:2019/12/31:令和:%EC元年')

class _LocaleTests(unittest.TestCase):

    def setUp(self):
        self.oldlocale = setlocale(LC_ALL)

    def tearDown(self):
        setlocale(LC_ALL, self.oldlocale)

    # Want to know what value was calculated, what it was compared against,
    # what function was used for the calculation, what type of data was used,
    # the locale that was supposedly set, and the actual locale that is set.
    lc_numeric_err_msg = "%s != %s (%s for %s; set to %s, using %s)"

    def numeric_tester(self, calc_type, calc_value, data_type, used_locale):
        """Compare calculation against known value, if available"""
        try:
            set_locale = setlocale(LC_NUMERIC)
        except Error:
            set_locale = "<not able to determine>"
        known_value = known_numerics.get(used_locale,
                                    ('', ''))[data_type == 'thousands_sep']
        if known_value and calc_value:
            self.assertEqual(calc_value, known_value,
                                self.lc_numeric_err_msg % (
                                    calc_value, known_value,
                                    calc_type, data_type, set_locale,
                                    used_locale))
            return True

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    @unittest.skipIf(
        support.is_emscripten or support.is_wasi,
        "musl libc issue on Emscripten, bpo-46390"
    )
    def test_lc_numeric_nl_langinfo(self):
        # Test nl_langinfo against known values
        tested = False
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue
            for li, lc in ((RADIXCHAR, "decimal_point"),
                            (THOUSEP, "thousands_sep")):
                if self.numeric_tester('nl_langinfo', nl_langinfo(li), lc, loc):
                    tested = True
        if not tested:
            self.skipTest('no suitable locales')

    @unittest.skipIf(
        support.is_emscripten or support.is_wasi,
        "musl libc issue on Emscripten, bpo-46390"
    )
    def test_lc_numeric_localeconv(self):
        # Test localeconv against known values
        tested = False
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue
            formatting = localeconv()
            for lc in ("decimal_point",
                        "thousands_sep"):
                if self.numeric_tester('localeconv', formatting[lc], lc, loc):
                    tested = True
        if not tested:
            self.skipTest('no suitable locales')

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    def test_lc_numeric_basic(self):
        # Test nl_langinfo against localeconv
        tested = False
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue
            for li, lc in ((RADIXCHAR, "decimal_point"),
                            (THOUSEP, "thousands_sep")):
                nl_radixchar = nl_langinfo(li)
                li_radixchar = localeconv()[lc]
                try:
                    set_locale = setlocale(LC_NUMERIC)
                except Error:
                    set_locale = "<not able to determine>"
                self.assertEqual(nl_radixchar, li_radixchar,
                                "%s (nl_langinfo) != %s (localeconv) "
                                "(set to %s, using %s)" % (
                                                nl_radixchar, li_radixchar,
                                                loc, set_locale))
                tested = True
        if not tested:
            self.skipTest('no suitable locales')

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    @unittest.skipUnless(hasattr(locale, 'ALT_DIGITS'), "requires locale.ALT_DIGITS")
    @unittest.skipIf(
        support.is_emscripten or support.is_wasi,
        "musl libc issue on Emscripten, bpo-46390"
    )
    def test_alt_digits_nl_langinfo(self):
        # Test nl_langinfo(ALT_DIGITS)
        tested = False
        for loc in candidate_locales:
            with self.subTest(locale=loc):
                try:
                    setlocale(LC_TIME, loc)
                    setlocale(LC_CTYPE, loc)
                except Error:
                    self.skipTest(f'no locale {loc!r}')
                    continue

                with self.subTest(locale=loc):
                    alt_digits = nl_langinfo(locale.ALT_DIGITS)
                    self.assertIsInstance(alt_digits, str)
                    alt_digits = alt_digits.split(';') if alt_digits else []
                    if alt_digits:
                        self.assertGreaterEqual(len(alt_digits), 10, alt_digits)
                    loc1 = loc.split('.', 1)[0]
                    if loc1 in known_alt_digits:
                        count, samples = known_alt_digits[loc1]
                        if count and not alt_digits:
                            self.skipTest(f'ALT_DIGITS is not set for locale {loc!r} on this platform')
                        self.assertEqual(len(alt_digits), count, alt_digits)
                        for i in samples:
                            self.assertEqual(alt_digits[i], samples[i])
                    tested = True
        if not tested:
            self.skipTest('no suitable locales')

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    @unittest.skipUnless(hasattr(locale, 'ERA'), "requires locale.ERA")
    @unittest.skipIf(
        support.is_emscripten or support.is_wasi,
        "musl libc issue on Emscripten, bpo-46390"
    )
    def test_era_nl_langinfo(self):
        # Test nl_langinfo(ERA)
        tested = False
        for loc in candidate_locales:
            with self.subTest(locale=loc):
                try:
                    setlocale(LC_TIME, loc)
                    setlocale(LC_CTYPE, loc)
                except Error:
                    self.skipTest(f'no locale {loc!r}')
                    continue

                with self.subTest(locale=loc):
                    era = nl_langinfo(locale.ERA)
                    self.assertIsInstance(era, str)
                    if era:
                        self.assertEqual(era.count(':'), (era.count(';') + 1) * 5, era)

                    loc1 = loc.split('.', 1)[0]
                    if loc1 in known_era:
                        count, sample = known_era[loc1]
                        if count:
                            if not era:
                                self.skipTest(f'ERA is not set for locale {loc!r} on this platform')
                            self.assertGreaterEqual(era.count(';') + 1, count)
                            self.assertIn(sample, era)
                        else:
                            self.assertEqual(era, '')
                    tested = True
        if not tested:
            self.skipTest('no suitable locales')

    def test_float_parsing(self):
        # Bug #1391872: Test whether float parsing is okay on European
        # locales.
        tested = False
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue

            # Ignore buggy locale databases. (Mac OS 10.4 and some other BSDs)
            if loc == 'eu_ES' and localeconv()['decimal_point'] == "' ":
                continue

            self.assertEqual(int(eval('3.14') * 100), 314,
                                "using eval('3.14') failed for %s" % loc)
            self.assertEqual(int(float('3.14') * 100), 314,
                                "using float('3.14') failed for %s" % loc)
            if localeconv()['decimal_point'] != '.':
                self.assertRaises(ValueError, float,
                                  localeconv()['decimal_point'].join(['1', '23']))
            tested = True
        if not tested:
            self.skipTest('no suitable locales')


if __name__ == '__main__':
    unittest.main()
