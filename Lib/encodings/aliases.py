""" Encoding Aliases Support

    This module is used by the encodings package search function to
    map encodings names to module names.

    Note that the search function converts the encoding names to lower
    case and replaces hyphens with underscores *before* performing the
    lookup.

"""
aliases = {

    # Latin-1
    'latin': 'latin_1',
    'latin1': 'latin_1',
    
    # UTF-7
    'utf7': 'utf_7',
    'u7': 'utf_7',
    
    # UTF-8
    'utf': 'utf_8',
    'utf8': 'utf_8',
    'u8': 'utf_8',
    'utf8@ucs2': 'utf_8',
    'utf8@ucs4': 'utf_8',
    
    # UTF-16
    'utf16': 'utf_16',
    'u16': 'utf_16',
    'utf_16be': 'utf_16_be',
    'utf_16le': 'utf_16_le',
    'unicodebigunmarked': 'utf_16_be',
    'unicodelittleunmarked': 'utf_16_le',

    # ASCII
    'us_ascii': 'ascii',
    'ansi_x3.4_1968': 'ascii', # used on Linux
    'ansi_x3_4_1968': 'ascii', # used on BSD?
    '646': 'ascii',            # used on Solaris

    # EBCDIC
    'ebcdic_cp_us': 'cp037',
    'ibm039': 'cp037',
    'ibm1140': 'cp1140',
    
    # ISO
    '8859': 'latin_1',
    'iso8859': 'latin_1',
    'iso8859_1': 'latin_1',
    'iso_8859_1': 'latin_1',
    'iso_8859_10': 'iso8859_10',
    'iso_8859_13': 'iso8859_13',
    'iso_8859_14': 'iso8859_14',
    'iso_8859_15': 'iso8859_15',
    'iso_8859_2': 'iso8859_2',
    'iso_8859_3': 'iso8859_3',
    'iso_8859_4': 'iso8859_4',
    'iso_8859_5': 'iso8859_5',
    'iso_8859_6': 'iso8859_6',
    'iso_8859_7': 'iso8859_7',
    'iso_8859_8': 'iso8859_8',
    'iso_8859_9': 'iso8859_9',

    # Mac
    'maclatin2': 'mac_latin2',
    'maccentraleurope': 'mac_latin2',
    'maccyrillic': 'mac_cyrillic',
    'macgreek': 'mac_greek',
    'maciceland': 'mac_iceland',
    'macroman': 'mac_roman',
    'macturkish': 'mac_turkish',

    # Windows
    'windows_1251': 'cp1251',
    'windows_1252': 'cp1252',
    'windows_1254': 'cp1254',
    'windows_1255': 'cp1255',
    'windows_1256': 'cp1256',
    'windows_1257': 'cp1257',
    'windows_1258': 'cp1258',

    # MBCS
    'dbcs': 'mbcs',

    # Code pages
    '437': 'cp437',

    # CJK
    #
    # The codecs for these encodings are not distributed with the
    # Python core, but are included here for reference, since the
    # locale module relies on having these aliases available.
    #
    'jis_7': 'jis_7',
    'iso_2022_jp': 'jis_7',
    'ujis': 'euc_jp',
    'ajec': 'euc_jp',
    'eucjp': 'euc_jp',
    'tis260': 'tactis',
    'sjis': 'shift_jis',

    # Content transfer/compression encodings
    'rot13': 'rot_13',
    'base64': 'base64_codec',
    'base_64': 'base64_codec',
    'zlib': 'zlib_codec',
    'zip': 'zlib_codec',
    'hex': 'hex_codec',
    'uu': 'uu_codec',
    'quopri': 'quopri_codec',
    'quotedprintable': 'quopri_codec',
    'quoted_printable': 'quopri_codec',

}
