""" Locale support.

    The module provides low-level access to the C lib's locale APIs
    and adds high level number formatting APIs as well as a locale
    aliasing engine to complement these.

    The aliasing engine includes support for many commonly used locale
    names and maps them to values suitable for passing to the C lib's
    setlocale() function. It also includes default encodings for all
    supported locale names.

"""

import string

### C lib locale APIs

from _locale import *

### Number formatting APIs

# Author: Martin von Loewis

#perform the grouping from right to left
def _group(s):
    conv=localeconv()
    grouping=conv['grouping']
    if not grouping:return s
    result=""
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
        else:
            result=s[-group:]
        s=s[:-group]
    if not result:
        return s
    if s:
        result=s+conv['thousands_sep']+result
    return result

def format(f,val,grouping=0):
    """Formats a value in the same way that the % formatting would use,
    but takes the current locale into account. 
    Grouping is applied if the third parameter is true."""
    result = f % val
    fields = string.split(result, ".")
    if grouping:
        fields[0]=_group(fields[0])
    if len(fields)==2:
        return fields[0]+localeconv()['decimal_point']+fields[1]
    elif len(fields)==1:
        return fields[0]
    else:
        raise Error,"Too many decimal points in result string"
    
def str(val):
    """Convert float to integer, taking the locale into account."""
    return format("%.12g",val)

def atof(str,func=string.atof):
    "Parses a string as a float according to the locale settings."
    #First, get rid of the grouping
    ts = localeconv()['thousands_sep']
    if ts:
        s=string.split(str,ts)
        str=string.join(s, "")
    #next, replace the decimal point with a dot
    dd = localeconv()['decimal_point']
    if dd:
        s=string.split(str,dd)
        str=string.join(s,'.')
    #finally, parse the string
    return func(str)

def atoi(str):
    "Converts a string to an integer according to the locale settings."
    return atof(str,string.atoi)

def _test():
    setlocale(LC_ALL,"")
    #do grouping
    s1=format("%d",123456789,1)
    print s1,"is",atoi(s1)
    #standard formatting
    s1=str(3.14)
    print s1,"is",atof(s1)

### Locale name aliasing engine

# Author: Marc-Andre Lemburg, mal@lemburg.com

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
    fullname = string.lower(localename)
    if ':' in fullname:
        # ':' is sometimes used as encoding delimiter.
        fullname = string.replace(fullname, ':', '.')
    if '.' in fullname:
        langname, encoding = string.split(fullname, '.')[:2]
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
            langname, defenc = string.split(code, '.')
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
        unkown to this implementation.

    """
    code = normalize(localename)
    if '.' in code:
        return string.split(code, '.')[:2]
    elif code == 'C':
        return None, None
    else:
        raise ValueError,'unkown locale: %s' % localename
    return l

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
    
def get_default(envvars=('LANGUAGE', 'LC_ALL', 'LC_CTYPE', 'LANG')):

    """ Tries to determine the default locale settings and returns
        them as tuple (language code, encoding).

        According to POSIX, a program which has not called
        setlocale(LC_ALL,"") runs using the portable 'C' locale.
        Calling setlocale(LC_ALL,"") lets it use the default locale as
        defined by the LANG variable. Since we don't want to interfere
        with the current locale setting we thus emulate the behaviour
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
    import os
    lookup = os.environ.get
    for variable in envvars:
        localename = lookup(variable,None)
        if localename is not None:
            break
    else:
        localename = 'C'
    return _parse_localename(localename)

def get_locale(category=LC_CTYPE):

    """ Returns the current setting for the given locale category as
        tuple (language code, encoding).

        category may be one of the LC_* value except LC_ALL. It
        defaults to LC_CTYPE.

        Except for the code 'C', the language code corresponds to RFC
        1766.  code and encoding can be None in case the values cannot
        be determined.

    """
    localename = setlocale(category)
    if category == LC_ALL and ';' in localename:
        raise TypeError,'category LC_ALL is not supported'
    return _parse_localename(localename)

def set_locale(localetuple, category=LC_ALL):

    """ Set the locale according to the localetuple (language code,
        encoding) as returned by get_locale() and get_default().

        The given codes are passed through the locale aliasing engine
        before being given to setlocale() for processing.

        category may be given as one of the LC_* values. It defaults
        to LC_ALL.

    """
    setlocale(category, normalize(_build_localename(localetuple)))

def set_to_default(category=LC_ALL):

    """ Sets the locale for category to the default setting.

        The default setting is determined by calling
        get_default(). category defaults to LC_ALL.
        
    """
    setlocale(category, _build_localename(get_default()))

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
        '437': 				'C',
        'c': 				'C',
        'iso8859': 			'ISO8859-1',
        '8859': 			'ISO8859-1',
        '88591': 			'ISO8859-1',
        'ascii': 			'ISO8859-1',
        'en': 				'ISO8859-1',
        'iso88591': 			'ISO8859-1',
        'iso_8859-1': 			'ISO8859-1',
        '885915': 			'ISO8859-15',
        'iso885915': 			'ISO8859-15',
        'iso_8859-15': 			'ISO8859-15',
        'iso8859-2': 			'ISO8859-2',
        'iso88592': 			'ISO8859-2',
        'iso_8859-2': 			'ISO8859-2',
        'iso88595': 			'ISO8859-5',
        'iso88596': 			'ISO8859-6',
        'iso88597': 			'ISO8859-7',
        'iso88598': 			'ISO8859-8',
        'iso88599': 			'ISO8859-9',
        'iso-2022-jp': 			'JIS7',
        'jis': 				'JIS7',
        'jis7': 			'JIS7',
        'sjis': 			'SJIS',
        'tis620': 			'TACTIS',
        'ajec': 			'eucJP',
        'eucjp': 			'eucJP',
        'ujis': 			'eucJP',
        'utf-8': 			'utf',
        'utf8': 			'utf',
        'utf8@ucs4': 			'utf',
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

    print 'Locale defaults as determined by get_default():'
    print '-'*72
    lang, enc = get_default()
    print 'Language: ', lang or '(undefined)'
    print 'Encoding: ', enc or '(undefined)'
    print

    print 'Locale settings on startup:'
    print '-'*72
    for name,category in categories.items():
        print name,'...'
        lang, enc = get_locale(category)
        print '   Language: ', lang or '(undefined)'
        print '   Encoding: ', enc or '(undefined)'
        print

    set_to_default()
    print
    print 'Locale settings after calling set_to_default():'
    print '-'*72
    for name,category in categories.items():
        print name,'...'
        lang, enc = get_locale(category)
        print '   Language: ', lang or '(undefined)'
        print '   Encoding: ', enc or '(undefined)'
        print
    
    try:
        setlocale(LC_ALL,"")
    except:
        print 'NOTE:'
        print 'setlocale(LC_ALL,"") does not support the default locale'
        print 'given in the OS environment variables.'
    else:
        print
        print 'Locale settings after calling setlocale(LC_ALL,""):'
        print '-'*72
        for name,category in categories.items():
            print name,'...'
            lang, enc = get_locale(category)
            print '   Language: ', lang or '(undefined)'
            print '   Encoding: ', enc or '(undefined)'
            print
    
###

if __name__=='__main__':
    print 'Locale aliasing:'
    print
    _print_locale()
    print
    print 'Number formatting:'
    print
    _test()
