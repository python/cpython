from test.test_support import verbose, TestSkipped, run_unittest
from _locale import (setlocale, LC_NUMERIC, RADIXCHAR, THOUSEP, nl_langinfo,
                    localeconv, Error)
import unittest

candidate_locales = ['es_UY', 'fr_FR', 'fi_FI', 'es_CO', 'pt_PT', 'it_IT',
    'et_EE', 'es_PY', 'no_NO', 'nl_NL', 'lv_LV', 'el_GR', 'be_BY', 'fr_BE',
    'ro_RO', 'ru_UA', 'ru_RU', 'es_VE', 'ca_ES', 'se_NO', 'es_EC', 'id_ID',
    'ka_GE', 'es_CL', 'hu_HU', 'wa_BE', 'lt_LT', 'sl_SI', 'hr_HR', 'es_AR',
    'es_ES', 'oc_FR', 'gl_ES', 'bg_BG', 'is_IS', 'mk_MK', 'de_AT', 'pt_BR',
    'da_DK', 'nn_NO', 'cs_CZ', 'de_LU', 'es_BO', 'sq_AL', 'sk_SK', 'fr_CH',
    'de_DE', 'sr_YU', 'br_FR', 'nl_BE', 'sv_FI', 'pl_PL', 'fr_CA', 'fo_FO',
    'bs_BA', 'fr_LU', 'kl_GL', 'fa_IR', 'de_BE', 'sv_SE', 'it_CH', 'uk_UA',
    'eu_ES', 'vi_VN', 'af_ZA', 'nb_NO', 'en_DK', 'tg_TJ',
    'es_ES.ISO8859-1', 'fr_FR.ISO8859-15', 'ru_RU.KOI8-R', 'ko_KR.eucKR']

class _LocaleTests(unittest.TestCase):

    def setUp(self):
        self.oldlocale = setlocale(LC_NUMERIC)

    def tearDown(self):
        setlocale(LC_NUMERIC, self.oldlocale)

    def test_lc_numeric(self):
        for loc in candidate_locales:
            try:
                setlocale(LC_NUMERIC, loc)
            except Error:
                continue
            for li, lc in ((RADIXCHAR, "decimal_point"),
                            (THOUSEP, "thousands_sep")):
                nl_radixchar = nl_langinfo(li)
                li_radixchar = localeconv()[lc]
                # Both with seeing what the locale is set to in order to detect
                # when setlocale lies and says it accepted the locale setting
                # but in actuality didn't use it (as seen in OS X 10.3)
                try:
                    set_locale = setlocale(LC_NUMERIC)
                except Error:
                    set_locale = "<not able to determine>"
                self.assertEquals(nl_radixchar, li_radixchar,
                                    "%s != %s (%s); "
                                    "supposed to be %s, set to %s" %
                                        (nl_radixchar, li_radixchar, lc,
                                         loc, set_locale))

def test_main():
    run_unittest(_LocaleTests)

if __name__ == '__main__':
    test_main()
