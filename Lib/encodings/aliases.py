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

}
