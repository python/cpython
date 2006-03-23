""" Locale support.

    The module provides low-level access to the C lib's locale APIs
    and adds high level number formatting APIs as well as a locale
    aliasing engine to complement these.

    The aliasing engine includes support for many commonly used locale
    names and maps them to values suitable for passing to the C lib's
    setlocale() function. It also includes default encodings for all
    supported locale names.

"""

import sys

# Try importing the _locale module.
#
# If this fails, fall back on a basic 'C' locale emulation.

# Yuck:  LC_MESSAGES is non-standard:  can't tell whether it exists before
# trying the import.  So __all__ is also fiddled at the end of the file.
__all__ = ["setlocale","Error","localeconv","strcoll","strxfrm",
           "format","str","atof","atoi","LC_CTYPE","LC_COLLATE",
           "LC_TIME","LC_MONETARY","LC_NUMERIC", "LC_ALL","CHAR_MAX"]

try:

    from _locale import *

except ImportError:

    # Locale emulation

    CHAR_MAX = 127
    LC_ALL = 6
    LC_COLLATE = 3
    LC_CTYPE = 0
    LC_MESSAGES = 5
    LC_MONETARY = 4
    LC_NUMERIC = 1
    LC_TIME = 2
    Error = ValueError

    def localeconv():
        """ localeconv() -> dict.
            Returns numeric and monetary locale-specific parameters.
        """
        # 'C' locale default values
        return {'grouping': [127],
                'currency_symbol': '',
                'n_sign_posn': 127,
                'p_cs_precedes': 127,
                'n_cs_precedes': 127,
                'mon_grouping': [],
                'n_sep_by_space': 127,
                'decimal_point': '.',
                'negative_sign': '',
                'positive_sign': '',
                'p_sep_by_space': 127,
                'int_curr_symbol': '',
                'p_sign_posn': 127,
                'thousands_sep': '',
                'mon_thousands_sep': '',
                'frac_digits': 127,
                'mon_decimal_point': '',
                'int_frac_digits': 127}

    def setlocale(category, value=None):
        """ setlocale(integer,string=None) -> string.
            Activates/queries locale processing.
        """
        if value not in (None, '', 'C'):
            raise Error, '_locale emulation only supports "C" locale'
        return 'C'

    def strcoll(a,b):
        """ strcoll(string,string) -> int.
            Compares two strings according to the locale.
        """
        return cmp(a,b)

    def strxfrm(s):
        """ strxfrm(string) -> string.
            Returns a string that behaves for cmp locale-aware.
        """
        return s

### Number formatting APIs

# Author: Martin von Loewis

#perform the grouping from right to left
def _group(s):
    conv=localeconv()
    grouping=conv['grouping']
    if not grouping:return (s, 0)
    result=""
    seps = 0
    spaces = ""
    if s[-1] == ' ':
        sp = s.find(' ')
        spaces = s[sp:]
        s = s[:sp]
    while s and grouping:
        # if grouping is -1, we are done
        if grouping[0]==CHAR_MAX:
            break
        # 0: re-use last group ad infinitum
        elif grouping[0]!=0:
            #process last group
            group=grouping[0]
            grouping=grouping[1:]
        if result:
            result=s[-group:]+conv['thousands_sep']+result
            seps += 1
        else:
            result=s[-group:]
        s=s[:-group]
        if s and s[-1] not in "0123456789":
            # the leading string is only spaces and signs
            return s+result+spaces,seps
    if not result:
        return s+spaces,seps
    if s:
        result=s+conv['thousands_sep']+result
        seps += 1
    return result+spaces,seps

def format(f,val,grouping=0):
    """Formats a value in the same way that the % formatting would use,
    but takes the current locale into account.
    Grouping is applied if the third parameter is true."""
    result = f % val
    fields = result.split(".")
    seps = 0
    if grouping:
        fields[0],seps=_group(fields[0])
    if len(fields)==2:
        result = fields[0]+localeconv()['decimal_point']+fields[1]
    elif len(fields)==1:
        result = fields[0]
    else:
        raise Error, "Too many decimal points in result string"

    while seps:
        # If the number was formatted for a specific width, then it
        # might have been filled with spaces to the left or right. If
        # so, kill as much spaces as there where separators.
        # Leading zeroes as fillers are not yet dealt with, as it is
        # not clear how they should interact with grouping.
        sp = result.find(" ")
        if sp==-1:break
        result = result[:sp]+result[sp+1:]
        seps -= 1

    return result

def str(val):
    """Convert float to integer, taking the locale into account."""
    return format("%.12g",val)

def atof(string,func=float):
    "Parses a string as a float according to the locale settings."
    #First, get rid of the grouping
    ts = localeconv()['thousands_sep']
    if ts:
        string = string.replace(ts, '')
    #next, replace the decimal point with a dot
    dd = localeconv()['decimal_point']
    if dd:
        string = string.replace(dd, '.')
    #finally, parse the string
    return func(string)

def atoi(str):
    "Converts a string to an integer according to the locale settings."
    return atof(str, int)

def _test():
    setlocale(LC_ALL, "")
    #do grouping
    s1=format("%d", 123456789,1)
    print s1, "is", atoi(s1)
    #standard formatting
    s1=str(3.14)
    print s1, "is", atof(s1)

### Locale name aliasing engine

# Author: Marc-Andre Lemburg, mal@lemburg.com
# Various tweaks by Fredrik Lundh <fredrik@pythonware.com>

# store away the low-level version of setlocale (it's
# overridden below)
_setlocale = setlocale

def normalize(localename):

    """ Returns a normalized locale code for the given locale
        name.

        The returned locale code is formatted for use with
        setlocale().

        If normalization fails, the original name is returned
        unchanged.

        If the given encoding is not known, the function defaults to
        the default encoding for the locale code just like setlocale()
        does.

    """
    # Normalize the locale name and extract the encoding
    fullname = localename.lower()
    if ':' in fullname:
        # ':' is sometimes used as encoding delimiter.
        fullname = fullname.replace(':', '.')
    if '.' in fullname:
        langname, encoding = fullname.split('.')[:2]
        fullname = langname + '.' + encoding
    else:
        langname = fullname
        encoding = ''

    # First lookup: fullname (possibly with encoding)
    code = locale_alias.get(fullname, None)
    if code is not None:
        return code

    # Second try: langname (without encoding)
    code = locale_alias.get(langname, None)
    if code is not None:
        if '.' in code:
            langname, defenc = code.split('.')
        else:
            langname = code
            defenc = ''
        if encoding:
            encoding = encoding_alias.get(encoding, encoding)
        else:
            encoding = defenc
        if encoding:
            return langname + '.' + encoding
        else:
            return langname

    else:
        return localename

def _parse_localename(localename):

    """ Parses the locale code for localename and returns the
        result as tuple (language code, encoding).

        The localename is normalized and passed through the locale
        alias engine. A ValueError is raised in case the locale name
        cannot be parsed.

        The language code corresponds to RFC 1766.  code and encoding
        can be None in case the values cannot be determined or are
        unknown to this implementation.

    """
    code = normalize(localename)
    if '@' in code:
        # Deal with locale modifiers
        code, modifier = code.split('@')
        if modifier == 'euro' and '.' not in code:
            # Assume Latin-9 for @euro locales. This is bogus,
            # since some systems may use other encodings for these
            # locales. Also, we ignore other modifiers.
            return code, 'iso-8859-15'

    if '.' in code:
        return tuple(code.split('.')[:2])
    elif code == 'C':
        return None, None
    raise ValueError, 'unknown locale: %s' % localename

def _build_localename(localetuple):

    """ Builds a locale code from the given tuple (language code,
        encoding).

        No aliasing or normalizing takes place.

    """
    language, encoding = localetuple
    if language is None:
        language = 'C'
    if encoding is None:
        return language
    else:
        return language + '.' + encoding

def getdefaultlocale(envvars=('LC_ALL', 'LC_CTYPE', 'LANG', 'LANGUAGE')):

    """ Tries to determine the default locale settings and returns
        them as tuple (language code, encoding).

        According to POSIX, a program which has not called
        setlocale(LC_ALL, "") runs using the portable 'C' locale.
        Calling setlocale(LC_ALL, "") lets it use the default locale as
        defined by the LANG variable. Since we don't want to interfere
        with the current locale setting we thus emulate the behavior
        in the way described above.

        To maintain compatibility with other platforms, not only the
        LANG variable is tested, but a list of variables given as
        envvars parameter. The first found to be defined will be
        used. envvars defaults to the search path used in GNU gettext;
        it must always contain the variable name 'LANG'.

        Except for the code 'C', the language code corresponds to RFC
        1766.  code and encoding can be None in case the values cannot
        be determined.

    """

    try:
        # check if it's supported by the _locale module
        import _locale
        code, encoding = _locale._getdefaultlocale()
    except (ImportError, AttributeError):
        pass
    else:
        # make sure the code/encoding values are valid
        if sys.platform == "win32" and code and code[:2] == "0x":
            # map windows language identifier to language name
            code = windows_locale.get(int(code, 0))
        # ...add other platform-specific processing here, if
        # necessary...
        return code, encoding

    # fall back on POSIX behaviour
    import os
    lookup = os.environ.get
    for variable in envvars:
        localename = lookup(variable,None)
        if localename:
            if variable == 'LANGUAGE':
                localename = localename.split(':')[0]
            break
    else:
        localename = 'C'
    return _parse_localename(localename)


def getlocale(category=LC_CTYPE):

    """ Returns the current setting for the given locale category as
        tuple (language code, encoding).

        category may be one of the LC_* value except LC_ALL. It
        defaults to LC_CTYPE.

        Except for the code 'C', the language code corresponds to RFC
        1766.  code and encoding can be None in case the values cannot
        be determined.

    """
    localename = _setlocale(category)
    if category == LC_ALL and ';' in localename:
        raise TypeError, 'category LC_ALL is not supported'
    return _parse_localename(localename)

def setlocale(category, locale=None):

    """ Set the locale for the given category.  The locale can be
        a string, a locale tuple (language code, encoding), or None.

        Locale tuples are converted to strings the locale aliasing
        engine.  Locale strings are passed directly to the C lib.

        category may be given as one of the LC_* values.

    """
    if locale and type(locale) is not type(""):
        # convert to string
        locale = normalize(_build_localename(locale))
    return _setlocale(category, locale)

def resetlocale(category=LC_ALL):

    """ Sets the locale for category to the default setting.

        The default setting is determined by calling
        getdefaultlocale(). category defaults to LC_ALL.

    """
    _setlocale(category, _build_localename(getdefaultlocale()))

if sys.platform in ('win32', 'darwin', 'mac'):
    # On Win32, this will return the ANSI code page
    # On the Mac, it should return the system encoding;
    # it might return "ascii" instead
    def getpreferredencoding(do_setlocale = True):
        """Return the charset that the user is likely using."""
        import _locale
        return _locale._getdefaultlocale()[1]
else:
    # On Unix, if CODESET is available, use that.
    try:
        CODESET
    except NameError:
        # Fall back to parsing environment variables :-(
        def getpreferredencoding(do_setlocale = True):
            """Return the charset that the user is likely using,
            by looking at environment variables."""
            return getdefaultlocale()[1]
    else:
        def getpreferredencoding(do_setlocale = True):
            """Return the charset that the user is likely using,
            according to the system configuration."""
            if do_setlocale:
                oldloc = setlocale(LC_CTYPE)
                setlocale(LC_CTYPE, "")
                result = nl_langinfo(CODESET)
                setlocale(LC_CTYPE, oldloc)
                return result
            else:
                return nl_langinfo(CODESET)


### Database
#
# The following data was extracted from the locale.alias file which
# comes with X11 and then hand edited removing the explicit encoding
# definitions and adding some more aliases. The file is usually
# available as /usr/lib/X11/locale/locale.alias.
#

#
# The encoding_alias table maps lowercase encoding alias names to C
# locale encoding names (case-sensitive).
#
encoding_alias = {
        '437':                          'C',
        'c':                            'C',
        'iso8859':                      'ISO8859-1',
        '8859':                         'ISO8859-1',
        '88591':                        'ISO8859-1',
        'ascii':                        'ISO8859-1',
        'en':                           'ISO8859-1',
        'iso88591':                     'ISO8859-1',
        'iso_8859-1':                   'ISO8859-1',
        '885915':                       'ISO8859-15',
        'iso885915':                    'ISO8859-15',
        'iso_8859-15':                  'ISO8859-15',
        'iso8859-2':                    'ISO8859-2',
        'iso88592':                     'ISO8859-2',
        'iso_8859-2':                   'ISO8859-2',
        'iso88595':                     'ISO8859-5',
        'iso88596':                     'ISO8859-6',
        'iso88597':                     'ISO8859-7',
        'iso88598':                     'ISO8859-8',
        'iso88599':                     'ISO8859-9',
        'iso-2022-jp':                  'JIS7',
        'jis':                          'JIS7',
        'jis7':                         'JIS7',
        'sjis':                         'SJIS',
        'tis620':                       'TACTIS',
        'ajec':                         'eucJP',
        'eucjp':                        'eucJP',
        'ujis':                         'eucJP',
        'utf-8':                        'utf',
        'utf8':                         'utf',
        'utf8@ucs4':                    'utf',
}

#
# The locale_alias table maps lowercase alias names to C locale names
# (case-sensitive). Encodings are always separated from the locale
# name using a dot ('.'); they should only be given in case the
# language name is needed to interpret the given encoding alias
# correctly (CJK codes often have this need).
#
locale_alias = {
        'american':                      'en_US.ISO8859-1',
        'ar':                            'ar_AA.ISO8859-6',
        'ar_aa':                         'ar_AA.ISO8859-6',
        'ar_sa':                         'ar_SA.ISO8859-6',
        'arabic':                        'ar_AA.ISO8859-6',
        'bg':                            'bg_BG.ISO8859-5',
        'bg_bg':                         'bg_BG.ISO8859-5',
        'bulgarian':                     'bg_BG.ISO8859-5',
        'c-french':                      'fr_CA.ISO8859-1',
        'c':                             'C',
        'c_c':                           'C',
        'cextend':                       'en_US.ISO8859-1',
        'chinese-s':                     'zh_CN.eucCN',
        'chinese-t':                     'zh_TW.eucTW',
        'croatian':                      'hr_HR.ISO8859-2',
        'cs':                            'cs_CZ.ISO8859-2',
        'cs_cs':                         'cs_CZ.ISO8859-2',
        'cs_cz':                         'cs_CZ.ISO8859-2',
        'cz':                            'cz_CZ.ISO8859-2',
        'cz_cz':                         'cz_CZ.ISO8859-2',
        'czech':                         'cs_CS.ISO8859-2',
        'da':                            'da_DK.ISO8859-1',
        'da_dk':                         'da_DK.ISO8859-1',
        'danish':                        'da_DK.ISO8859-1',
        'de':                            'de_DE.ISO8859-1',
        'de_at':                         'de_AT.ISO8859-1',
        'de_ch':                         'de_CH.ISO8859-1',
        'de_de':                         'de_DE.ISO8859-1',
        'dutch':                         'nl_BE.ISO8859-1',
        'ee':                            'ee_EE.ISO8859-4',
        'el':                            'el_GR.ISO8859-7',
        'el_gr':                         'el_GR.ISO8859-7',
        'en':                            'en_US.ISO8859-1',
        'en_au':                         'en_AU.ISO8859-1',
        'en_ca':                         'en_CA.ISO8859-1',
        'en_gb':                         'en_GB.ISO8859-1',
        'en_ie':                         'en_IE.ISO8859-1',
        'en_nz':                         'en_NZ.ISO8859-1',
        'en_uk':                         'en_GB.ISO8859-1',
        'en_us':                         'en_US.ISO8859-1',
        'eng_gb':                        'en_GB.ISO8859-1',
        'english':                       'en_EN.ISO8859-1',
        'english_uk':                    'en_GB.ISO8859-1',
        'english_united-states':         'en_US.ISO8859-1',
        'english_us':                    'en_US.ISO8859-1',
        'es':                            'es_ES.ISO8859-1',
        'es_ar':                         'es_AR.ISO8859-1',
        'es_bo':                         'es_BO.ISO8859-1',
        'es_cl':                         'es_CL.ISO8859-1',
        'es_co':                         'es_CO.ISO8859-1',
        'es_cr':                         'es_CR.ISO8859-1',
        'es_ec':                         'es_EC.ISO8859-1',
        'es_es':                         'es_ES.ISO8859-1',
        'es_gt':                         'es_GT.ISO8859-1',
        'es_mx':                         'es_MX.ISO8859-1',
        'es_ni':                         'es_NI.ISO8859-1',
        'es_pa':                         'es_PA.ISO8859-1',
        'es_pe':                         'es_PE.ISO8859-1',
        'es_py':                         'es_PY.ISO8859-1',
        'es_sv':                         'es_SV.ISO8859-1',
        'es_uy':                         'es_UY.ISO8859-1',
        'es_ve':                         'es_VE.ISO8859-1',
        'et':                            'et_EE.ISO8859-4',
        'et_ee':                         'et_EE.ISO8859-4',
        'fi':                            'fi_FI.ISO8859-1',
        'fi_fi':                         'fi_FI.ISO8859-1',
        'finnish':                       'fi_FI.ISO8859-1',
        'fr':                            'fr_FR.ISO8859-1',
        'fr_be':                         'fr_BE.ISO8859-1',
        'fr_ca':                         'fr_CA.ISO8859-1',
        'fr_ch':                         'fr_CH.ISO8859-1',
        'fr_fr':                         'fr_FR.ISO8859-1',
        'fre_fr':                        'fr_FR.ISO8859-1',
        'french':                        'fr_FR.ISO8859-1',
        'french_france':                 'fr_FR.ISO8859-1',
        'ger_de':                        'de_DE.ISO8859-1',
        'german':                        'de_DE.ISO8859-1',
        'german_germany':                'de_DE.ISO8859-1',
        'greek':                         'el_GR.ISO8859-7',
        'hebrew':                        'iw_IL.ISO8859-8',
        'hr':                            'hr_HR.ISO8859-2',
        'hr_hr':                         'hr_HR.ISO8859-2',
        'hu':                            'hu_HU.ISO8859-2',
        'hu_hu':                         'hu_HU.ISO8859-2',
        'hungarian':                     'hu_HU.ISO8859-2',
        'icelandic':                     'is_IS.ISO8859-1',
        'id':                            'id_ID.ISO8859-1',
        'id_id':                         'id_ID.ISO8859-1',
        'is':                            'is_IS.ISO8859-1',
        'is_is':                         'is_IS.ISO8859-1',
        'iso-8859-1':                    'en_US.ISO8859-1',
        'iso-8859-15':                   'en_US.ISO8859-15',
        'iso8859-1':                     'en_US.ISO8859-1',
        'iso8859-15':                    'en_US.ISO8859-15',
        'iso_8859_1':                    'en_US.ISO8859-1',
        'iso_8859_15':                   'en_US.ISO8859-15',
        'it':                            'it_IT.ISO8859-1',
        'it_ch':                         'it_CH.ISO8859-1',
        'it_it':                         'it_IT.ISO8859-1',
        'italian':                       'it_IT.ISO8859-1',
        'iw':                            'iw_IL.ISO8859-8',
        'iw_il':                         'iw_IL.ISO8859-8',
        'ja':                            'ja_JP.eucJP',
        'ja.jis':                        'ja_JP.JIS7',
        'ja.sjis':                       'ja_JP.SJIS',
        'ja_jp':                         'ja_JP.eucJP',
        'ja_jp.ajec':                    'ja_JP.eucJP',
        'ja_jp.euc':                     'ja_JP.eucJP',
        'ja_jp.eucjp':                   'ja_JP.eucJP',
        'ja_jp.iso-2022-jp':             'ja_JP.JIS7',
        'ja_jp.jis':                     'ja_JP.JIS7',
        'ja_jp.jis7':                    'ja_JP.JIS7',
        'ja_jp.mscode':                  'ja_JP.SJIS',
        'ja_jp.sjis':                    'ja_JP.SJIS',
        'ja_jp.ujis':                    'ja_JP.eucJP',
        'japan':                         'ja_JP.eucJP',
        'japanese':                      'ja_JP.SJIS',
        'japanese-euc':                  'ja_JP.eucJP',
        'japanese.euc':                  'ja_JP.eucJP',
        'jp_jp':                         'ja_JP.eucJP',
        'ko':                            'ko_KR.eucKR',
        'ko_kr':                         'ko_KR.eucKR',
        'ko_kr.euc':                     'ko_KR.eucKR',
        'korean':                        'ko_KR.eucKR',
        'lt':                            'lt_LT.ISO8859-4',
        'lv':                            'lv_LV.ISO8859-4',
        'mk':                            'mk_MK.ISO8859-5',
        'mk_mk':                         'mk_MK.ISO8859-5',
        'nl':                            'nl_NL.ISO8859-1',
        'nl_be':                         'nl_BE.ISO8859-1',
        'nl_nl':                         'nl_NL.ISO8859-1',
        'no':                            'no_NO.ISO8859-1',
        'no_no':                         'no_NO.ISO8859-1',
        'norwegian':                     'no_NO.ISO8859-1',
        'pl':                            'pl_PL.ISO8859-2',
        'pl_pl':                         'pl_PL.ISO8859-2',
        'polish':                        'pl_PL.ISO8859-2',
        'portuguese':                    'pt_PT.ISO8859-1',
        'portuguese_brazil':             'pt_BR.ISO8859-1',
        'posix':                         'C',
        'posix-utf2':                    'C',
        'pt':                            'pt_PT.ISO8859-1',
        'pt_br':                         'pt_BR.ISO8859-1',
        'pt_pt':                         'pt_PT.ISO8859-1',
        'ro':                            'ro_RO.ISO8859-2',
        'ro_ro':                         'ro_RO.ISO8859-2',
        'ru':                            'ru_RU.ISO8859-5',
        'ru_ru':                         'ru_RU.ISO8859-5',
        'rumanian':                      'ro_RO.ISO8859-2',
        'russian':                       'ru_RU.ISO8859-5',
        'serbocroatian':                 'sh_YU.ISO8859-2',
        'sh':                            'sh_YU.ISO8859-2',
        'sh_hr':                         'sh_HR.ISO8859-2',
        'sh_sp':                         'sh_YU.ISO8859-2',
        'sh_yu':                         'sh_YU.ISO8859-2',
        'sk':                            'sk_SK.ISO8859-2',
        'sk_sk':                         'sk_SK.ISO8859-2',
        'sl':                            'sl_CS.ISO8859-2',
        'sl_cs':                         'sl_CS.ISO8859-2',
        'sl_si':                         'sl_SI.ISO8859-2',
        'slovak':                        'sk_SK.ISO8859-2',
        'slovene':                       'sl_CS.ISO8859-2',
        'sp':                            'sp_YU.ISO8859-5',
        'sp_yu':                         'sp_YU.ISO8859-5',
        'spanish':                       'es_ES.ISO8859-1',
        'spanish_spain':                 'es_ES.ISO8859-1',
        'sr_sp':                         'sr_SP.ISO8859-2',
        'sv':                            'sv_SE.ISO8859-1',
        'sv_se':                         'sv_SE.ISO8859-1',
        'swedish':                       'sv_SE.ISO8859-1',
        'th_th':                         'th_TH.TACTIS',
        'tr':                            'tr_TR.ISO8859-9',
        'tr_tr':                         'tr_TR.ISO8859-9',
        'turkish':                       'tr_TR.ISO8859-9',
        'univ':                          'en_US.utf',
        'universal':                     'en_US.utf',
        'zh':                            'zh_CN.eucCN',
        'zh_cn':                         'zh_CN.eucCN',
        'zh_cn.big5':                    'zh_TW.eucTW',
        'zh_cn.euc':                     'zh_CN.eucCN',
        'zh_tw':                         'zh_TW.eucTW',
        'zh_tw.euc':                     'zh_TW.eucTW',
}

#
# This maps Windows language identifiers to locale strings.
#
# This list has been updated from
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/intl/nls_238z.asp
# to include every locale up to Windows XP.
#
# NOTE: this mapping is incomplete.  If your language is missing, please
# submit a bug report to Python bug manager, which you can find via:
#     http://www.python.org/dev/
# Make sure you include the missing language identifier and the suggested
# locale code.
#

windows_locale = {
    0x0436: "af_ZA", # Afrikaans
    0x041c: "sq_AL", # Albanian
    0x0401: "ar_SA", # Arabic - Saudi Arabia
    0x0801: "ar_IQ", # Arabic - Iraq
    0x0c01: "ar_EG", # Arabic - Egypt
    0x1001: "ar_LY", # Arabic - Libya
    0x1401: "ar_DZ", # Arabic - Algeria
    0x1801: "ar_MA", # Arabic - Morocco
    0x1c01: "ar_TN", # Arabic - Tunisia
    0x2001: "ar_OM", # Arabic - Oman
    0x2401: "ar_YE", # Arabic - Yemen
    0x2801: "ar_SY", # Arabic - Syria
    0x2c01: "ar_JO", # Arabic - Jordan
    0x3001: "ar_LB", # Arabic - Lebanon
    0x3401: "ar_KW", # Arabic - Kuwait
    0x3801: "ar_AE", # Arabic - United Arab Emirates
    0x3c01: "ar_BH", # Arabic - Bahrain
    0x4001: "ar_QA", # Arabic - Qatar
    0x042b: "hy_AM", # Armenian
    0x042c: "az_AZ", # Azeri Latin
    0x082c: "az_AZ", # Azeri - Cyrillic
    0x042d: "eu_ES", # Basque
    0x0423: "be_BY", # Belarusian
    0x0445: "bn_IN", # Begali
    0x201a: "bs_BA", # Bosnian
    0x141a: "bs_BA", # Bosnian - Cyrillic
    0x047e: "br_FR", # Breton - France
    0x0402: "bg_BG", # Bulgarian
    0x0403: "ca_ES", # Catalan
    0x0004: "zh_CHS",# Chinese - Simplified
    0x0404: "zh_TW", # Chinese - Taiwan
    0x0804: "zh_CN", # Chinese - PRC
    0x0c04: "zh_HK", # Chinese - Hong Kong S.A.R.
    0x1004: "zh_SG", # Chinese - Singapore
    0x1404: "zh_MO", # Chinese - Macao S.A.R.
    0x7c04: "zh_CHT",# Chinese - Traditional
    0x041a: "hr_HR", # Croatian
    0x101a: "hr_BA", # Croatian - Bosnia
    0x0405: "cs_CZ", # Czech
    0x0406: "da_DK", # Danish
    0x048c: "gbz_AF",# Dari - Afghanistan
    0x0465: "div_MV",# Divehi - Maldives
    0x0413: "nl_NL", # Dutch - The Netherlands
    0x0813: "nl_BE", # Dutch - Belgium
    0x0409: "en_US", # English - United States
    0x0809: "en_GB", # English - United Kingdom
    0x0c09: "en_AU", # English - Australia
    0x1009: "en_CA", # English - Canada
    0x1409: "en_NZ", # English - New Zealand
    0x1809: "en_IE", # English - Ireland
    0x1c09: "en_ZA", # English - South Africa
    0x2009: "en_JA", # English - Jamaica
    0x2409: "en_CB", # English - Carribbean
    0x2809: "en_BZ", # English - Belize
    0x2c09: "en_TT", # English - Trinidad
    0x3009: "en_ZW", # English - Zimbabwe
    0x3409: "en_PH", # English - Phillippines
    0x0425: "et_EE", # Estonian
    0x0438: "fo_FO", # Faroese
    0x0464: "fil_PH",# Filipino
    0x040b: "fi_FI", # Finnish
    0x040c: "fr_FR", # French - France
    0x080c: "fr_BE", # French - Belgium
    0x0c0c: "fr_CA", # French - Canada
    0x100c: "fr_CH", # French - Switzerland
    0x140c: "fr_LU", # French - Luxembourg
    0x180c: "fr_MC", # French - Monaco
    0x0462: "fy_NL", # Frisian - Netherlands
    0x0456: "gl_ES", # Galician
    0x0437: "ka_GE", # Georgian
    0x0407: "de_DE", # German - Germany
    0x0807: "de_CH", # German - Switzerland
    0x0c07: "de_AT", # German - Austria
    0x1007: "de_LU", # German - Luxembourg
    0x1407: "de_LI", # German - Liechtenstein
    0x0408: "el_GR", # Greek
    0x0447: "gu_IN", # Gujarati
    0x040d: "he_IL", # Hebrew
    0x0439: "hi_IN", # Hindi
    0x040e: "hu_HU", # Hungarian
    0x040f: "is_IS", # Icelandic
    0x0421: "id_ID", # Indonesian
    0x045d: "iu_CA", # Inuktitut
    0x085d: "iu_CA", # Inuktitut - Latin
    0x083c: "ga_IE", # Irish - Ireland
    0x0434: "xh_ZA", # Xhosa - South Africa
    0x0435: "zu_ZA", # Zulu
    0x0410: "it_IT", # Italian - Italy
    0x0810: "it_CH", # Italian - Switzerland
    0x0411: "ja_JP", # Japanese
    0x044b: "kn_IN", # Kannada - India
    0x043f: "kk_KZ", # Kazakh
    0x0457: "kok_IN",# Konkani
    0x0412: "ko_KR", # Korean
    0x0440: "ky_KG", # Kyrgyz
    0x0426: "lv_LV", # Latvian
    0x0427: "lt_LT", # Lithuanian
    0x046e: "lb_LU", # Luxembourgish
    0x042f: "mk_MK", # FYRO Macedonian
    0x043e: "ms_MY", # Malay - Malaysia
    0x083e: "ms_BN", # Malay - Brunei
    0x044c: "ml_IN", # Malayalam - India
    0x043a: "mt_MT", # Maltese
    0x0481: "mi_NZ", # Maori
    0x047a: "arn_CL",# Mapudungun
    0x044e: "mr_IN", # Marathi
    0x047c: "moh_CA",# Mohawk - Canada
    0x0450: "mn_MN", # Mongolian
    0x0461: "ne_NP", # Nepali
    0x0414: "nb_NO", # Norwegian - Bokmal
    0x0814: "nn_NO", # Norwegian - Nynorsk
    0x0482: "oc_FR", # Occitan - France
    0x0448: "or_IN", # Oriya - India
    0x0463: "ps_AF", # Pashto - Afghanistan
    0x0429: "fa_IR", # Persian
    0x0415: "pl_PL", # Polish
    0x0416: "pt_BR", # Portuguese - Brazil
    0x0816: "pt_PT", # Portuguese - Portugal
    0x0446: "pa_IN", # Punjabi
    0x046b: "quz_BO",# Quechua (Bolivia)
    0x086b: "quz_EC",# Quechua (Ecuador)
    0x0c6b: "quz_PE",# Quechua (Peru)
    0x0418: "ro_RO", # Romanian - Romania
    0x0417: "rm_CH", # Raeto-Romanese
    0x0419: "ru_RU", # Russian
    0x243b: "smn_FI",# Sami Finland
    0x103b: "smj_NO",# Sami Norway
    0x143b: "smj_SE",# Sami Sweden
    0x043b: "se_NO", # Sami Northern Norway
    0x083b: "se_SE", # Sami Northern Sweden
    0x0c3b: "se_FI", # Sami Northern Finland
    0x203b: "sms_FI",# Sami Skolt
    0x183b: "sma_NO",# Sami Southern Norway
    0x1c3b: "sma_SE",# Sami Southern Sweden
    0x044f: "sa_IN", # Sanskrit
    0x0c1a: "sr_SP", # Serbian - Cyrillic
    0x1c1a: "sr_BA", # Serbian - Bosnia Cyrillic
    0x081a: "sr_SP", # Serbian - Latin
    0x181a: "sr_BA", # Serbian - Bosnia Latin
    0x046c: "ns_ZA", # Northern Sotho
    0x0432: "tn_ZA", # Setswana - Southern Africa
    0x041b: "sk_SK", # Slovak
    0x0424: "sl_SI", # Slovenian
    0x040a: "es_ES", # Spanish - Spain
    0x080a: "es_MX", # Spanish - Mexico
    0x0c0a: "es_ES", # Spanish - Spain (Modern)
    0x100a: "es_GT", # Spanish - Guatemala
    0x140a: "es_CR", # Spanish - Costa Rica
    0x180a: "es_PA", # Spanish - Panama
    0x1c0a: "es_DO", # Spanish - Dominican Republic
    0x200a: "es_VE", # Spanish - Venezuela
    0x240a: "es_CO", # Spanish - Colombia
    0x280a: "es_PE", # Spanish - Peru
    0x2c0a: "es_AR", # Spanish - Argentina
    0x300a: "es_EC", # Spanish - Ecuador
    0x340a: "es_CL", # Spanish - Chile
    0x380a: "es_UR", # Spanish - Uruguay
    0x3c0a: "es_PY", # Spanish - Paraguay
    0x400a: "es_BO", # Spanish - Bolivia
    0x440a: "es_SV", # Spanish - El Salvador
    0x480a: "es_HN", # Spanish - Honduras
    0x4c0a: "es_NI", # Spanish - Nicaragua
    0x500a: "es_PR", # Spanish - Puerto Rico
    0x0441: "sw_KE", # Swahili
    0x041d: "sv_SE", # Swedish - Sweden
    0x081d: "sv_FI", # Swedish - Finland
    0x045a: "syr_SY",# Syriac
    0x0449: "ta_IN", # Tamil
    0x0444: "tt_RU", # Tatar
    0x044a: "te_IN", # Telugu
    0x041e: "th_TH", # Thai
    0x041f: "tr_TR", # Turkish
    0x0422: "uk_UA", # Ukrainian
    0x0420: "ur_PK", # Urdu
    0x0820: "ur_IN", # Urdu - India
    0x0443: "uz_UZ", # Uzbek - Latin
    0x0843: "uz_UZ", # Uzbek - Cyrillic
    0x042a: "vi_VN", # Vietnamese
    0x0452: "cy_GB", # Welsh
}

def _print_locale():

    """ Test function.
    """
    categories = {}
    def _init_categories(categories=categories):
        for k,v in globals().items():
            if k[:3] == 'LC_':
                categories[k] = v
    _init_categories()
    del categories['LC_ALL']

    print 'Locale defaults as determined by getdefaultlocale():'
    print '-'*72
    lang, enc = getdefaultlocale()
    print 'Language: ', lang or '(undefined)'
    print 'Encoding: ', enc or '(undefined)'
    print

    print 'Locale settings on startup:'
    print '-'*72
    for name,category in categories.items():
        print name, '...'
        lang, enc = getlocale(category)
        print '   Language: ', lang or '(undefined)'
        print '   Encoding: ', enc or '(undefined)'
        print

    print
    print 'Locale settings after calling resetlocale():'
    print '-'*72
    resetlocale()
    for name,category in categories.items():
        print name, '...'
        lang, enc = getlocale(category)
        print '   Language: ', lang or '(undefined)'
        print '   Encoding: ', enc or '(undefined)'
        print

    try:
        setlocale(LC_ALL, "")
    except:
        print 'NOTE:'
        print 'setlocale(LC_ALL, "") does not support the default locale'
        print 'given in the OS environment variables.'
    else:
        print
        print 'Locale settings after calling setlocale(LC_ALL, ""):'
        print '-'*72
        for name,category in categories.items():
            print name, '...'
            lang, enc = getlocale(category)
            print '   Language: ', lang or '(undefined)'
            print '   Encoding: ', enc or '(undefined)'
            print

###

try:
    LC_MESSAGES
except NameError:
    pass
else:
    __all__.append("LC_MESSAGES")

if __name__=='__main__':
    print 'Locale aliasing:'
    print
    _print_locale()
    print
    print 'Number formatting:'
    print
    _test()
