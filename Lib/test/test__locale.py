from test.test_support import verbose, TestSkipped
from _locale import setlocale, LC_NUMERIC, RADIXCHAR, THOUSEP, nl_langinfo
from _locale import localeconv, Error

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

oldlocale = setlocale(LC_NUMERIC)
try:
    saw_locale = 0
    for loc in candidate_locales:
        try:
            setlocale(LC_NUMERIC, loc)
        except Error:
            continue
        if verbose:
            print "locale %r" % loc
        saw_locale = 1
        nl_radixchar = nl_langinfo(RADIXCHAR)
        li_radixchar = localeconv()['decimal_point']
        if nl_radixchar != li_radixchar:
            print "%r != %r" % (nl_radixchar, li_radixchar)
        nl_radixchar = nl_langinfo(THOUSEP)
        li_radixchar = localeconv()['thousands_sep']
        if nl_radixchar != li_radixchar:
            print "%r != %r" % (nl_radixchar, li_radixchar)
    if not saw_locale:
            raise ImportError, "None of the listed locales found"
finally:
    setlocale(LC_NUMERIC, oldlocale)
