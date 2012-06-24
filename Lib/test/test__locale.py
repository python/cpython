from _locale import (setlocale, LC_ALL, LC_CTYPE, LC_NUMERIC, localeconv, Error)
try:
    from _locale import (RADIXCHAR, THOUSEP, nl_langinfo)
except ImportError:
    nl_langinfo = None

import codecs
import locale
import sys
import unittest
from platform import uname
from test.support import run_unittest

if uname()[0] == "Darwin":
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
    'eu_ES', 'vi_VN', 'af_ZA', 'nb_NO', 'en_DK', 'tg_TJ', 'en_US',
    'es_ES.ISO8859-1', 'fr_FR.ISO8859-15', 'ru_RU.KOI8-R', 'ko_KR.eucKR']

# Issue #13441: Skip some locales (e.g. cs_CZ and hu_HU) on Solaris to
# workaround a mbstowcs() bug. For example, on Solaris, the hu_HU locale uses
# the locale encoding ISO-8859-2, the thousauds separator is b'\xA0' and it is
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
            encoding = locale.getpreferredencoding(False)
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
known_numerics = {'fr_FR' : (',', ''), 'en_US':('.', ',')}

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

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    def test_lc_numeric_nl_langinfo(self):
        # Test nl_langinfo against known values
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue
            for li, lc in ((RADIXCHAR, "decimal_point"),
                            (THOUSEP, "thousands_sep")):
                self.numeric_tester('nl_langinfo', nl_langinfo(li), lc, loc)

    def test_lc_numeric_localeconv(self):
        # Test localeconv against known values
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
                setlocale(LC_CTYPE, loc)
            except Error:
                continue
            formatting = localeconv()
            for lc in ("decimal_point",
                        "thousands_sep"):
                self.numeric_tester('localeconv', formatting[lc], lc, loc)

    @unittest.skipUnless(nl_langinfo, "nl_langinfo is not available")
    def test_lc_numeric_basic(self):
        # Test nl_langinfo against localeconv
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

    def test_float_parsing(self):
        # Bug #1391872: Test whether float parsing is okay on European
        # locales.
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

def test_main():
    run_unittest(_LocaleTests)

if __name__ == '__main__':
    test_main()
