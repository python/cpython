""" Encoding Aliases Support

    This module is used by the encodings package search function to
    map encodings names to module names.

    Note that the search function converts the encoding names to lower
    case and replaces hyphens with underscores *before* performing the
    lookup.

    Contents:

        The following aliases dictionary contains mappings of all IANA
        character set names for which the Python core library provides
        codecs. In addition to these, a few Python specific codec
        aliases have also been added.

    About the CJK codec aliases:

        The codecs for these encodings are not distributed with the
        Python core, but are included here for reference, since the
        locale module relies on having these aliases available.

"""
aliases = {

    # ascii codec
    '646'                : 'ascii',
    'ansi_x3.4_1968'     : 'ascii',
    'ansi_x3.4_1986'     : 'ascii',
    'cp367'              : 'ascii',
    'csascii'            : 'ascii',
    'ibm367'             : 'ascii',
    'iso646_us'          : 'ascii',
    'iso_646.irv:1991'   : 'ascii',
    'iso_ir_6'           : 'ascii',
    'us'                 : 'ascii',
    'us_ascii'           : 'ascii',

    # base64_codec codec
    'base64'             : 'base64_codec',
    'base_64'            : 'base64_codec',

    # cp037 codec
    'csibm037'           : 'cp037',
    'ebcdic_cp_ca'       : 'cp037',
    'ebcdic_cp_nl'       : 'cp037',
    'ebcdic_cp_us'       : 'cp037',
    'ebcdic_cp_wt'       : 'cp037',
    'ibm037'             : 'cp037',
    'ibm039'             : 'cp037',

    # cp1026 codec
    'csibm1026'          : 'cp1026',
    'ibm1026'            : 'cp1026',

    # cp1140 codec
    'ibm1140'            : 'cp1140',

    # cp1250 codec
    'windows_1250'       : 'cp1250',

    # cp1251 codec
    'windows_1251'       : 'cp1251',

    # cp1252 codec
    'windows_1252'       : 'cp1252',

    # cp1253 codec
    'windows_1253'       : 'cp1253',

    # cp1254 codec
    'windows_1254'       : 'cp1254',

    # cp1255 codec
    'windows_1255'       : 'cp1255',

    # cp1256 codec
    'windows_1256'       : 'cp1256',

    # cp1257 codec
    'windows_1257'       : 'cp1257',

    # cp1258 codec
    'windows_1258'       : 'cp1258',

    # cp424 codec
    'csibm424'           : 'cp424',
    'ebcdic_cp_he'       : 'cp424',
    'ibm424'             : 'cp424',

    # cp437 codec
    '437'                : 'cp437',
    'cspc8codepage437'   : 'cp437',
    'ibm437'             : 'cp437',

    # cp500 codec
    'csibm500'           : 'cp500',
    'ebcdic_cp_be'       : 'cp500',
    'ebcdic_cp_ch'       : 'cp500',
    'ibm500'             : 'cp500',

    # cp775 codec
    'cspc775baltic'      : 'cp775',
    'ibm775'             : 'cp775',

    # cp850 codec
    '850'                : 'cp850',
    'cspc850multilingual' : 'cp850',
    'ibm850'             : 'cp850',

    # cp852 codec
    '852'                : 'cp852',
    'cspcp852'           : 'cp852',
    'ibm852'             : 'cp852',

    # cp855 codec
    '855'                : 'cp855',
    'csibm855'           : 'cp855',
    'ibm855'             : 'cp855',

    # cp857 codec
    '857'                : 'cp857',
    'csibm857'           : 'cp857',
    'ibm857'             : 'cp857',

    # cp860 codec
    '860'                : 'cp860',
    'csibm860'           : 'cp860',
    'ibm860'             : 'cp860',

    # cp861 codec
    '861'                : 'cp861',
    'cp_is'              : 'cp861',
    'csibm861'           : 'cp861',
    'ibm861'             : 'cp861',

    # cp862 codec
    '862'                : 'cp862',
    'cspc862latinhebrew' : 'cp862',
    'ibm862'             : 'cp862',

    # cp863 codec
    '863'                : 'cp863',
    'csibm863'           : 'cp863',
    'ibm863'             : 'cp863',

    # cp864 codec
    'csibm864'           : 'cp864',
    'ibm864'             : 'cp864',

    # cp865 codec
    '865'                : 'cp865',
    'csibm865'           : 'cp865',
    'ibm865'             : 'cp865',

    # cp866 codec
    '866'                : 'cp866',
    'csibm866'           : 'cp866',
    'ibm866'             : 'cp866',

    # cp869 codec
    '869'                : 'cp869',
    'cp_gr'              : 'cp869',
    'csibm869'           : 'cp869',
    'ibm869'             : 'cp869',

    # hex_codec codec
    'hex'                : 'hex_codec',

    # iso8859_10 codec
    'csisolatin6'        : 'iso8859_10',
    'iso_8859_10'        : 'iso8859_10',
    'iso_8859_10:1992'   : 'iso8859_10',
    'iso_ir_157'         : 'iso8859_10',
    'l6'                 : 'iso8859_10',
    'latin6'             : 'iso8859_10',

    # iso8859_13 codec
    'iso_8859_13'        : 'iso8859_13',

    # iso8859_14 codec
    'iso_8859_14'        : 'iso8859_14',
    'iso_8859_14:1998'   : 'iso8859_14',
    'iso_celtic'         : 'iso8859_14',
    'iso_ir_199'         : 'iso8859_14',
    'l8'                 : 'iso8859_14',
    'latin8'             : 'iso8859_14',

    # iso8859_15 codec
    'iso_8859_15'        : 'iso8859_15',

    # iso8859_2 codec
    'csisolatin2'        : 'iso8859_2',
    'iso_8859_2'         : 'iso8859_2',
    'iso_8859_2:1987'    : 'iso8859_2',
    'iso_ir_101'         : 'iso8859_2',
    'l2'                 : 'iso8859_2',
    'latin2'             : 'iso8859_2',

    # iso8859_3 codec
    'csisolatin3'        : 'iso8859_3',
    'iso_8859_3'         : 'iso8859_3',
    'iso_8859_3:1988'    : 'iso8859_3',
    'iso_ir_109'         : 'iso8859_3',
    'l3'                 : 'iso8859_3',
    'latin3'             : 'iso8859_3',

    # iso8859_4 codec
    'csisolatin4'        : 'iso8859_4',
    'iso_8859_4'         : 'iso8859_4',
    'iso_8859_4:1988'    : 'iso8859_4',
    'iso_ir_110'         : 'iso8859_4',
    'l4'                 : 'iso8859_4',
    'latin4'             : 'iso8859_4',

    # iso8859_5 codec
    'csisolatincyrillic' : 'iso8859_5',
    'cyrillic'           : 'iso8859_5',
    'iso_8859_5'         : 'iso8859_5',
    'iso_8859_5:1988'    : 'iso8859_5',
    'iso_ir_144'         : 'iso8859_5',

    # iso8859_6 codec
    'arabic'             : 'iso8859_6',
    'asmo_708'           : 'iso8859_6',
    'csisolatinarabic'   : 'iso8859_6',
    'ecma_114'           : 'iso8859_6',
    'iso_8859_6'         : 'iso8859_6',
    'iso_8859_6:1987'    : 'iso8859_6',
    'iso_ir_127'         : 'iso8859_6',

    # iso8859_7 codec
    'csisolatingreek'    : 'iso8859_7',
    'ecma_118'           : 'iso8859_7',
    'elot_928'           : 'iso8859_7',
    'greek'              : 'iso8859_7',
    'greek8'             : 'iso8859_7',
    'iso_8859_7'         : 'iso8859_7',
    'iso_8859_7:1987'    : 'iso8859_7',
    'iso_ir_126'         : 'iso8859_7',

    # iso8859_8 codec
    'csisolatinhebrew'   : 'iso8859_8',
    'hebrew'             : 'iso8859_8',
    'iso_8859_8'         : 'iso8859_8',
    'iso_8859_8:1988'    : 'iso8859_8',
    'iso_ir_138'         : 'iso8859_8',

    # iso8859_9 codec
    'csisolatin5'        : 'iso8859_9',
    'iso_8859_9'         : 'iso8859_9',
    'iso_8859_9:1989'    : 'iso8859_9',
    'iso_ir_148'         : 'iso8859_9',
    'l5'                 : 'iso8859_9',
    'latin5'             : 'iso8859_9',

    # jis_7 codec
    'csiso2022jp'        : 'jis_7',
    'iso_2022_jp'        : 'jis_7',

    # koi8_r codec
    'cskoi8r'            : 'koi8_r',

    # latin_1 codec
    '8859'               : 'latin_1',
    'cp819'              : 'latin_1',
    'csisolatin1'        : 'latin_1',
    'ibm819'             : 'latin_1',
    'iso8859'            : 'latin_1',
    'iso_8859_1'         : 'latin_1',
    'iso_8859_1:1987'    : 'latin_1',
    'iso_ir_100'         : 'latin_1',
    'l1'                 : 'latin_1',
    'latin'              : 'latin_1',
    'latin1'             : 'latin_1',

    # mac_cyrillic codec
    'maccyrillic'        : 'mac_cyrillic',

    # mac_greek codec
    'macgreek'           : 'mac_greek',

    # mac_iceland codec
    'maciceland'         : 'mac_iceland',

    # mac_latin2 codec
    'maccentraleurope'   : 'mac_latin2',
    'maclatin2'          : 'mac_latin2',

    # mac_roman codec
    'macroman'           : 'mac_roman',

    # mac_turkish codec
    'macturkish'         : 'mac_turkish',

    # mbcs codec
    'dbcs'               : 'mbcs',

    # quopri_codec codec
    'quopri'             : 'quopri_codec',
    'quoted_printable'   : 'quopri_codec',
    'quotedprintable'    : 'quopri_codec',

    # rot_13 codec
    'rot13'              : 'rot_13',

    # tactis codec
    'tis260'             : 'tactis',

    # utf_16 codec
    'u16'                : 'utf_16',
    'utf16'              : 'utf_16',

    # utf_16_be codec
    'unicodebigunmarked' : 'utf_16_be',
    'utf_16be'           : 'utf_16_be',

    # utf_16_le codec
    'unicodelittleunmarked' : 'utf_16_le',
    'utf_16le'           : 'utf_16_le',

    # utf_7 codec
    'u7'                 : 'utf_7',
    'utf7'               : 'utf_7',

    # utf_8 codec
    'u8'                 : 'utf_8',
    'utf'                : 'utf_8',
    'utf8'               : 'utf_8',
    'utf8@ucs2'          : 'utf_8',
    'utf8@ucs4'          : 'utf_8',

    # uu_codec codec
    'uu'                 : 'uu_codec',

    # zlib_codec codec
    'zip'                : 'zlib_codec',
    'zlib'               : 'zlib_codec',

}
