""" Python Character Mapping Codec mac_farsi generated from 'MAPPINGS/VENDORS/APPLE/FARSI.TXT' with gencodec.py.

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):
        return codecs.charmap_encode(input,errors,encoding_map)

    def decode(self,input,errors='strict'):
        return codecs.charmap_decode(input,errors,decoding_table)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return codecs.charmap_encode(input,self.errors,encoding_map)[0]

class IncrementalDecoder(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        return codecs.charmap_decode(input,self.errors,decoding_table)[0]

class StreamWriter(Codec,codecs.StreamWriter):
    pass

class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='mac-farsi',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )


### Decoding Table

decoding_table = (
    u'\x00'     #  0x00 -> CONTROL CHARACTER
    u'\x01'     #  0x01 -> CONTROL CHARACTER
    u'\x02'     #  0x02 -> CONTROL CHARACTER
    u'\x03'     #  0x03 -> CONTROL CHARACTER
    u'\x04'     #  0x04 -> CONTROL CHARACTER
    u'\x05'     #  0x05 -> CONTROL CHARACTER
    u'\x06'     #  0x06 -> CONTROL CHARACTER
    u'\x07'     #  0x07 -> CONTROL CHARACTER
    u'\x08'     #  0x08 -> CONTROL CHARACTER
    u'\t'       #  0x09 -> CONTROL CHARACTER
    u'\n'       #  0x0A -> CONTROL CHARACTER
    u'\x0b'     #  0x0B -> CONTROL CHARACTER
    u'\x0c'     #  0x0C -> CONTROL CHARACTER
    u'\r'       #  0x0D -> CONTROL CHARACTER
    u'\x0e'     #  0x0E -> CONTROL CHARACTER
    u'\x0f'     #  0x0F -> CONTROL CHARACTER
    u'\x10'     #  0x10 -> CONTROL CHARACTER
    u'\x11'     #  0x11 -> CONTROL CHARACTER
    u'\x12'     #  0x12 -> CONTROL CHARACTER
    u'\x13'     #  0x13 -> CONTROL CHARACTER
    u'\x14'     #  0x14 -> CONTROL CHARACTER
    u'\x15'     #  0x15 -> CONTROL CHARACTER
    u'\x16'     #  0x16 -> CONTROL CHARACTER
    u'\x17'     #  0x17 -> CONTROL CHARACTER
    u'\x18'     #  0x18 -> CONTROL CHARACTER
    u'\x19'     #  0x19 -> CONTROL CHARACTER
    u'\x1a'     #  0x1A -> CONTROL CHARACTER
    u'\x1b'     #  0x1B -> CONTROL CHARACTER
    u'\x1c'     #  0x1C -> CONTROL CHARACTER
    u'\x1d'     #  0x1D -> CONTROL CHARACTER
    u'\x1e'     #  0x1E -> CONTROL CHARACTER
    u'\x1f'     #  0x1F -> CONTROL CHARACTER
    u' '        #  0x20 -> SPACE, left-right
    u'!'        #  0x21 -> EXCLAMATION MARK, left-right
    u'"'        #  0x22 -> QUOTATION MARK, left-right
    u'#'        #  0x23 -> NUMBER SIGN, left-right
    u'$'        #  0x24 -> DOLLAR SIGN, left-right
    u'%'        #  0x25 -> PERCENT SIGN, left-right
    u'&'        #  0x26 -> AMPERSAND, left-right
    u"'"        #  0x27 -> APOSTROPHE, left-right
    u'('        #  0x28 -> LEFT PARENTHESIS, left-right
    u')'        #  0x29 -> RIGHT PARENTHESIS, left-right
    u'*'        #  0x2A -> ASTERISK, left-right
    u'+'        #  0x2B -> PLUS SIGN, left-right
    u','        #  0x2C -> COMMA, left-right; in Arabic-script context, displayed as 0x066C ARABIC THOUSANDS SEPARATOR
    u'-'        #  0x2D -> HYPHEN-MINUS, left-right
    u'.'        #  0x2E -> FULL STOP, left-right; in Arabic-script context, displayed as 0x066B ARABIC DECIMAL SEPARATOR
    u'/'        #  0x2F -> SOLIDUS, left-right
    u'0'        #  0x30 -> DIGIT ZERO;  in Arabic-script context, displayed as 0x06F0 EXTENDED ARABIC-INDIC DIGIT ZERO
    u'1'        #  0x31 -> DIGIT ONE;   in Arabic-script context, displayed as 0x06F1 EXTENDED ARABIC-INDIC DIGIT ONE
    u'2'        #  0x32 -> DIGIT TWO;   in Arabic-script context, displayed as 0x06F2 EXTENDED ARABIC-INDIC DIGIT TWO
    u'3'        #  0x33 -> DIGIT THREE; in Arabic-script context, displayed as 0x06F3 EXTENDED ARABIC-INDIC DIGIT THREE
    u'4'        #  0x34 -> DIGIT FOUR;  in Arabic-script context, displayed as 0x06F4 EXTENDED ARABIC-INDIC DIGIT FOUR
    u'5'        #  0x35 -> DIGIT FIVE;  in Arabic-script context, displayed as 0x06F5 EXTENDED ARABIC-INDIC DIGIT FIVE
    u'6'        #  0x36 -> DIGIT SIX;   in Arabic-script context, displayed as 0x06F6 EXTENDED ARABIC-INDIC DIGIT SIX
    u'7'        #  0x37 -> DIGIT SEVEN; in Arabic-script context, displayed as 0x06F7 EXTENDED ARABIC-INDIC DIGIT SEVEN
    u'8'        #  0x38 -> DIGIT EIGHT; in Arabic-script context, displayed as 0x06F8 EXTENDED ARABIC-INDIC DIGIT EIGHT
    u'9'        #  0x39 -> DIGIT NINE;  in Arabic-script context, displayed as 0x06F9 EXTENDED ARABIC-INDIC DIGIT NINE
    u':'        #  0x3A -> COLON, left-right
    u';'        #  0x3B -> SEMICOLON, left-right
    u'<'        #  0x3C -> LESS-THAN SIGN, left-right
    u'='        #  0x3D -> EQUALS SIGN, left-right
    u'>'        #  0x3E -> GREATER-THAN SIGN, left-right
    u'?'        #  0x3F -> QUESTION MARK, left-right
    u'@'        #  0x40 -> COMMERCIAL AT
    u'A'        #  0x41 -> LATIN CAPITAL LETTER A
    u'B'        #  0x42 -> LATIN CAPITAL LETTER B
    u'C'        #  0x43 -> LATIN CAPITAL LETTER C
    u'D'        #  0x44 -> LATIN CAPITAL LETTER D
    u'E'        #  0x45 -> LATIN CAPITAL LETTER E
    u'F'        #  0x46 -> LATIN CAPITAL LETTER F
    u'G'        #  0x47 -> LATIN CAPITAL LETTER G
    u'H'        #  0x48 -> LATIN CAPITAL LETTER H
    u'I'        #  0x49 -> LATIN CAPITAL LETTER I
    u'J'        #  0x4A -> LATIN CAPITAL LETTER J
    u'K'        #  0x4B -> LATIN CAPITAL LETTER K
    u'L'        #  0x4C -> LATIN CAPITAL LETTER L
    u'M'        #  0x4D -> LATIN CAPITAL LETTER M
    u'N'        #  0x4E -> LATIN CAPITAL LETTER N
    u'O'        #  0x4F -> LATIN CAPITAL LETTER O
    u'P'        #  0x50 -> LATIN CAPITAL LETTER P
    u'Q'        #  0x51 -> LATIN CAPITAL LETTER Q
    u'R'        #  0x52 -> LATIN CAPITAL LETTER R
    u'S'        #  0x53 -> LATIN CAPITAL LETTER S
    u'T'        #  0x54 -> LATIN CAPITAL LETTER T
    u'U'        #  0x55 -> LATIN CAPITAL LETTER U
    u'V'        #  0x56 -> LATIN CAPITAL LETTER V
    u'W'        #  0x57 -> LATIN CAPITAL LETTER W
    u'X'        #  0x58 -> LATIN CAPITAL LETTER X
    u'Y'        #  0x59 -> LATIN CAPITAL LETTER Y
    u'Z'        #  0x5A -> LATIN CAPITAL LETTER Z
    u'['        #  0x5B -> LEFT SQUARE BRACKET, left-right
    u'\\'       #  0x5C -> REVERSE SOLIDUS, left-right
    u']'        #  0x5D -> RIGHT SQUARE BRACKET, left-right
    u'^'        #  0x5E -> CIRCUMFLEX ACCENT, left-right
    u'_'        #  0x5F -> LOW LINE, left-right
    u'`'        #  0x60 -> GRAVE ACCENT
    u'a'        #  0x61 -> LATIN SMALL LETTER A
    u'b'        #  0x62 -> LATIN SMALL LETTER B
    u'c'        #  0x63 -> LATIN SMALL LETTER C
    u'd'        #  0x64 -> LATIN SMALL LETTER D
    u'e'        #  0x65 -> LATIN SMALL LETTER E
    u'f'        #  0x66 -> LATIN SMALL LETTER F
    u'g'        #  0x67 -> LATIN SMALL LETTER G
    u'h'        #  0x68 -> LATIN SMALL LETTER H
    u'i'        #  0x69 -> LATIN SMALL LETTER I
    u'j'        #  0x6A -> LATIN SMALL LETTER J
    u'k'        #  0x6B -> LATIN SMALL LETTER K
    u'l'        #  0x6C -> LATIN SMALL LETTER L
    u'm'        #  0x6D -> LATIN SMALL LETTER M
    u'n'        #  0x6E -> LATIN SMALL LETTER N
    u'o'        #  0x6F -> LATIN SMALL LETTER O
    u'p'        #  0x70 -> LATIN SMALL LETTER P
    u'q'        #  0x71 -> LATIN SMALL LETTER Q
    u'r'        #  0x72 -> LATIN SMALL LETTER R
    u's'        #  0x73 -> LATIN SMALL LETTER S
    u't'        #  0x74 -> LATIN SMALL LETTER T
    u'u'        #  0x75 -> LATIN SMALL LETTER U
    u'v'        #  0x76 -> LATIN SMALL LETTER V
    u'w'        #  0x77 -> LATIN SMALL LETTER W
    u'x'        #  0x78 -> LATIN SMALL LETTER X
    u'y'        #  0x79 -> LATIN SMALL LETTER Y
    u'z'        #  0x7A -> LATIN SMALL LETTER Z
    u'{'        #  0x7B -> LEFT CURLY BRACKET, left-right
    u'|'        #  0x7C -> VERTICAL LINE, left-right
    u'}'        #  0x7D -> RIGHT CURLY BRACKET, left-right
    u'~'        #  0x7E -> TILDE
    u'\x7f'     #  0x7F -> CONTROL CHARACTER
    u'\xc4'     #  0x80 -> LATIN CAPITAL LETTER A WITH DIAERESIS
    u'\xa0'     #  0x81 -> NO-BREAK SPACE, right-left
    u'\xc7'     #  0x82 -> LATIN CAPITAL LETTER C WITH CEDILLA
    u'\xc9'     #  0x83 -> LATIN CAPITAL LETTER E WITH ACUTE
    u'\xd1'     #  0x84 -> LATIN CAPITAL LETTER N WITH TILDE
    u'\xd6'     #  0x85 -> LATIN CAPITAL LETTER O WITH DIAERESIS
    u'\xdc'     #  0x86 -> LATIN CAPITAL LETTER U WITH DIAERESIS
    u'\xe1'     #  0x87 -> LATIN SMALL LETTER A WITH ACUTE
    u'\xe0'     #  0x88 -> LATIN SMALL LETTER A WITH GRAVE
    u'\xe2'     #  0x89 -> LATIN SMALL LETTER A WITH CIRCUMFLEX
    u'\xe4'     #  0x8A -> LATIN SMALL LETTER A WITH DIAERESIS
    u'\u06ba'   #  0x8B -> ARABIC LETTER NOON GHUNNA
    u'\xab'     #  0x8C -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    u'\xe7'     #  0x8D -> LATIN SMALL LETTER C WITH CEDILLA
    u'\xe9'     #  0x8E -> LATIN SMALL LETTER E WITH ACUTE
    u'\xe8'     #  0x8F -> LATIN SMALL LETTER E WITH GRAVE
    u'\xea'     #  0x90 -> LATIN SMALL LETTER E WITH CIRCUMFLEX
    u'\xeb'     #  0x91 -> LATIN SMALL LETTER E WITH DIAERESIS
    u'\xed'     #  0x92 -> LATIN SMALL LETTER I WITH ACUTE
    u'\u2026'   #  0x93 -> HORIZONTAL ELLIPSIS, right-left
    u'\xee'     #  0x94 -> LATIN SMALL LETTER I WITH CIRCUMFLEX
    u'\xef'     #  0x95 -> LATIN SMALL LETTER I WITH DIAERESIS
    u'\xf1'     #  0x96 -> LATIN SMALL LETTER N WITH TILDE
    u'\xf3'     #  0x97 -> LATIN SMALL LETTER O WITH ACUTE
    u'\xbb'     #  0x98 -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    u'\xf4'     #  0x99 -> LATIN SMALL LETTER O WITH CIRCUMFLEX
    u'\xf6'     #  0x9A -> LATIN SMALL LETTER O WITH DIAERESIS
    u'\xf7'     #  0x9B -> DIVISION SIGN, right-left
    u'\xfa'     #  0x9C -> LATIN SMALL LETTER U WITH ACUTE
    u'\xf9'     #  0x9D -> LATIN SMALL LETTER U WITH GRAVE
    u'\xfb'     #  0x9E -> LATIN SMALL LETTER U WITH CIRCUMFLEX
    u'\xfc'     #  0x9F -> LATIN SMALL LETTER U WITH DIAERESIS
    u' '        #  0xA0 -> SPACE, right-left
    u'!'        #  0xA1 -> EXCLAMATION MARK, right-left
    u'"'        #  0xA2 -> QUOTATION MARK, right-left
    u'#'        #  0xA3 -> NUMBER SIGN, right-left
    u'$'        #  0xA4 -> DOLLAR SIGN, right-left
    u'\u066a'   #  0xA5 -> ARABIC PERCENT SIGN
    u'&'        #  0xA6 -> AMPERSAND, right-left
    u"'"        #  0xA7 -> APOSTROPHE, right-left
    u'('        #  0xA8 -> LEFT PARENTHESIS, right-left
    u')'        #  0xA9 -> RIGHT PARENTHESIS, right-left
    u'*'        #  0xAA -> ASTERISK, right-left
    u'+'        #  0xAB -> PLUS SIGN, right-left
    u'\u060c'   #  0xAC -> ARABIC COMMA
    u'-'        #  0xAD -> HYPHEN-MINUS, right-left
    u'.'        #  0xAE -> FULL STOP, right-left
    u'/'        #  0xAF -> SOLIDUS, right-left
    u'\u06f0'   #  0xB0 -> EXTENDED ARABIC-INDIC DIGIT ZERO, right-left (need override)
    u'\u06f1'   #  0xB1 -> EXTENDED ARABIC-INDIC DIGIT ONE, right-left (need override)
    u'\u06f2'   #  0xB2 -> EXTENDED ARABIC-INDIC DIGIT TWO, right-left (need override)
    u'\u06f3'   #  0xB3 -> EXTENDED ARABIC-INDIC DIGIT THREE, right-left (need override)
    u'\u06f4'   #  0xB4 -> EXTENDED ARABIC-INDIC DIGIT FOUR, right-left (need override)
    u'\u06f5'   #  0xB5 -> EXTENDED ARABIC-INDIC DIGIT FIVE, right-left (need override)
    u'\u06f6'   #  0xB6 -> EXTENDED ARABIC-INDIC DIGIT SIX, right-left (need override)
    u'\u06f7'   #  0xB7 -> EXTENDED ARABIC-INDIC DIGIT SEVEN, right-left (need override)
    u'\u06f8'   #  0xB8 -> EXTENDED ARABIC-INDIC DIGIT EIGHT, right-left (need override)
    u'\u06f9'   #  0xB9 -> EXTENDED ARABIC-INDIC DIGIT NINE, right-left (need override)
    u':'        #  0xBA -> COLON, right-left
    u'\u061b'   #  0xBB -> ARABIC SEMICOLON
    u'<'        #  0xBC -> LESS-THAN SIGN, right-left
    u'='        #  0xBD -> EQUALS SIGN, right-left
    u'>'        #  0xBE -> GREATER-THAN SIGN, right-left
    u'\u061f'   #  0xBF -> ARABIC QUESTION MARK
    u'\u274a'   #  0xC0 -> EIGHT TEARDROP-SPOKED PROPELLER ASTERISK, right-left
    u'\u0621'   #  0xC1 -> ARABIC LETTER HAMZA
    u'\u0622'   #  0xC2 -> ARABIC LETTER ALEF WITH MADDA ABOVE
    u'\u0623'   #  0xC3 -> ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0624'   #  0xC4 -> ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0625'   #  0xC5 -> ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0626'   #  0xC6 -> ARABIC LETTER YEH WITH HAMZA ABOVE
    u'\u0627'   #  0xC7 -> ARABIC LETTER ALEF
    u'\u0628'   #  0xC8 -> ARABIC LETTER BEH
    u'\u0629'   #  0xC9 -> ARABIC LETTER TEH MARBUTA
    u'\u062a'   #  0xCA -> ARABIC LETTER TEH
    u'\u062b'   #  0xCB -> ARABIC LETTER THEH
    u'\u062c'   #  0xCC -> ARABIC LETTER JEEM
    u'\u062d'   #  0xCD -> ARABIC LETTER HAH
    u'\u062e'   #  0xCE -> ARABIC LETTER KHAH
    u'\u062f'   #  0xCF -> ARABIC LETTER DAL
    u'\u0630'   #  0xD0 -> ARABIC LETTER THAL
    u'\u0631'   #  0xD1 -> ARABIC LETTER REH
    u'\u0632'   #  0xD2 -> ARABIC LETTER ZAIN
    u'\u0633'   #  0xD3 -> ARABIC LETTER SEEN
    u'\u0634'   #  0xD4 -> ARABIC LETTER SHEEN
    u'\u0635'   #  0xD5 -> ARABIC LETTER SAD
    u'\u0636'   #  0xD6 -> ARABIC LETTER DAD
    u'\u0637'   #  0xD7 -> ARABIC LETTER TAH
    u'\u0638'   #  0xD8 -> ARABIC LETTER ZAH
    u'\u0639'   #  0xD9 -> ARABIC LETTER AIN
    u'\u063a'   #  0xDA -> ARABIC LETTER GHAIN
    u'['        #  0xDB -> LEFT SQUARE BRACKET, right-left
    u'\\'       #  0xDC -> REVERSE SOLIDUS, right-left
    u']'        #  0xDD -> RIGHT SQUARE BRACKET, right-left
    u'^'        #  0xDE -> CIRCUMFLEX ACCENT, right-left
    u'_'        #  0xDF -> LOW LINE, right-left
    u'\u0640'   #  0xE0 -> ARABIC TATWEEL
    u'\u0641'   #  0xE1 -> ARABIC LETTER FEH
    u'\u0642'   #  0xE2 -> ARABIC LETTER QAF
    u'\u0643'   #  0xE3 -> ARABIC LETTER KAF
    u'\u0644'   #  0xE4 -> ARABIC LETTER LAM
    u'\u0645'   #  0xE5 -> ARABIC LETTER MEEM
    u'\u0646'   #  0xE6 -> ARABIC LETTER NOON
    u'\u0647'   #  0xE7 -> ARABIC LETTER HEH
    u'\u0648'   #  0xE8 -> ARABIC LETTER WAW
    u'\u0649'   #  0xE9 -> ARABIC LETTER ALEF MAKSURA
    u'\u064a'   #  0xEA -> ARABIC LETTER YEH
    u'\u064b'   #  0xEB -> ARABIC FATHATAN
    u'\u064c'   #  0xEC -> ARABIC DAMMATAN
    u'\u064d'   #  0xED -> ARABIC KASRATAN
    u'\u064e'   #  0xEE -> ARABIC FATHA
    u'\u064f'   #  0xEF -> ARABIC DAMMA
    u'\u0650'   #  0xF0 -> ARABIC KASRA
    u'\u0651'   #  0xF1 -> ARABIC SHADDA
    u'\u0652'   #  0xF2 -> ARABIC SUKUN
    u'\u067e'   #  0xF3 -> ARABIC LETTER PEH
    u'\u0679'   #  0xF4 -> ARABIC LETTER TTEH
    u'\u0686'   #  0xF5 -> ARABIC LETTER TCHEH
    u'\u06d5'   #  0xF6 -> ARABIC LETTER AE
    u'\u06a4'   #  0xF7 -> ARABIC LETTER VEH
    u'\u06af'   #  0xF8 -> ARABIC LETTER GAF
    u'\u0688'   #  0xF9 -> ARABIC LETTER DDAL
    u'\u0691'   #  0xFA -> ARABIC LETTER RREH
    u'{'        #  0xFB -> LEFT CURLY BRACKET, right-left
    u'|'        #  0xFC -> VERTICAL LINE, right-left
    u'}'        #  0xFD -> RIGHT CURLY BRACKET, right-left
    u'\u0698'   #  0xFE -> ARABIC LETTER JEH
    u'\u06d2'   #  0xFF -> ARABIC LETTER YEH BARREE
)

### Encoding Map

encoding_map = {
    0x0000: 0x00,       #  CONTROL CHARACTER
    0x0001: 0x01,       #  CONTROL CHARACTER
    0x0002: 0x02,       #  CONTROL CHARACTER
    0x0003: 0x03,       #  CONTROL CHARACTER
    0x0004: 0x04,       #  CONTROL CHARACTER
    0x0005: 0x05,       #  CONTROL CHARACTER
    0x0006: 0x06,       #  CONTROL CHARACTER
    0x0007: 0x07,       #  CONTROL CHARACTER
    0x0008: 0x08,       #  CONTROL CHARACTER
    0x0009: 0x09,       #  CONTROL CHARACTER
    0x000A: 0x0A,       #  CONTROL CHARACTER
    0x000B: 0x0B,       #  CONTROL CHARACTER
    0x000C: 0x0C,       #  CONTROL CHARACTER
    0x000D: 0x0D,       #  CONTROL CHARACTER
    0x000E: 0x0E,       #  CONTROL CHARACTER
    0x000F: 0x0F,       #  CONTROL CHARACTER
    0x0010: 0x10,       #  CONTROL CHARACTER
    0x0011: 0x11,       #  CONTROL CHARACTER
    0x0012: 0x12,       #  CONTROL CHARACTER
    0x0013: 0x13,       #  CONTROL CHARACTER
    0x0014: 0x14,       #  CONTROL CHARACTER
    0x0015: 0x15,       #  CONTROL CHARACTER
    0x0016: 0x16,       #  CONTROL CHARACTER
    0x0017: 0x17,       #  CONTROL CHARACTER
    0x0018: 0x18,       #  CONTROL CHARACTER
    0x0019: 0x19,       #  CONTROL CHARACTER
    0x001A: 0x1A,       #  CONTROL CHARACTER
    0x001B: 0x1B,       #  CONTROL CHARACTER
    0x001C: 0x1C,       #  CONTROL CHARACTER
    0x001D: 0x1D,       #  CONTROL CHARACTER
    0x001E: 0x1E,       #  CONTROL CHARACTER
    0x001F: 0x1F,       #  CONTROL CHARACTER
    0x0020: 0x20,       #  SPACE, left-right
    0x0020: 0xA0,       #  SPACE, right-left
    0x0021: 0x21,       #  EXCLAMATION MARK, left-right
    0x0021: 0xA1,       #  EXCLAMATION MARK, right-left
    0x0022: 0x22,       #  QUOTATION MARK, left-right
    0x0022: 0xA2,       #  QUOTATION MARK, right-left
    0x0023: 0x23,       #  NUMBER SIGN, left-right
    0x0023: 0xA3,       #  NUMBER SIGN, right-left
    0x0024: 0x24,       #  DOLLAR SIGN, left-right
    0x0024: 0xA4,       #  DOLLAR SIGN, right-left
    0x0025: 0x25,       #  PERCENT SIGN, left-right
    0x0026: 0x26,       #  AMPERSAND, left-right
    0x0026: 0xA6,       #  AMPERSAND, right-left
    0x0027: 0x27,       #  APOSTROPHE, left-right
    0x0027: 0xA7,       #  APOSTROPHE, right-left
    0x0028: 0x28,       #  LEFT PARENTHESIS, left-right
    0x0028: 0xA8,       #  LEFT PARENTHESIS, right-left
    0x0029: 0x29,       #  RIGHT PARENTHESIS, left-right
    0x0029: 0xA9,       #  RIGHT PARENTHESIS, right-left
    0x002A: 0x2A,       #  ASTERISK, left-right
    0x002A: 0xAA,       #  ASTERISK, right-left
    0x002B: 0x2B,       #  PLUS SIGN, left-right
    0x002B: 0xAB,       #  PLUS SIGN, right-left
    0x002C: 0x2C,       #  COMMA, left-right; in Arabic-script context, displayed as 0x066C ARABIC THOUSANDS SEPARATOR
    0x002D: 0x2D,       #  HYPHEN-MINUS, left-right
    0x002D: 0xAD,       #  HYPHEN-MINUS, right-left
    0x002E: 0x2E,       #  FULL STOP, left-right; in Arabic-script context, displayed as 0x066B ARABIC DECIMAL SEPARATOR
    0x002E: 0xAE,       #  FULL STOP, right-left
    0x002F: 0x2F,       #  SOLIDUS, left-right
    0x002F: 0xAF,       #  SOLIDUS, right-left
    0x0030: 0x30,       #  DIGIT ZERO;  in Arabic-script context, displayed as 0x06F0 EXTENDED ARABIC-INDIC DIGIT ZERO
    0x0031: 0x31,       #  DIGIT ONE;   in Arabic-script context, displayed as 0x06F1 EXTENDED ARABIC-INDIC DIGIT ONE
    0x0032: 0x32,       #  DIGIT TWO;   in Arabic-script context, displayed as 0x06F2 EXTENDED ARABIC-INDIC DIGIT TWO
    0x0033: 0x33,       #  DIGIT THREE; in Arabic-script context, displayed as 0x06F3 EXTENDED ARABIC-INDIC DIGIT THREE
    0x0034: 0x34,       #  DIGIT FOUR;  in Arabic-script context, displayed as 0x06F4 EXTENDED ARABIC-INDIC DIGIT FOUR
    0x0035: 0x35,       #  DIGIT FIVE;  in Arabic-script context, displayed as 0x06F5 EXTENDED ARABIC-INDIC DIGIT FIVE
    0x0036: 0x36,       #  DIGIT SIX;   in Arabic-script context, displayed as 0x06F6 EXTENDED ARABIC-INDIC DIGIT SIX
    0x0037: 0x37,       #  DIGIT SEVEN; in Arabic-script context, displayed as 0x06F7 EXTENDED ARABIC-INDIC DIGIT SEVEN
    0x0038: 0x38,       #  DIGIT EIGHT; in Arabic-script context, displayed as 0x06F8 EXTENDED ARABIC-INDIC DIGIT EIGHT
    0x0039: 0x39,       #  DIGIT NINE;  in Arabic-script context, displayed as 0x06F9 EXTENDED ARABIC-INDIC DIGIT NINE
    0x003A: 0x3A,       #  COLON, left-right
    0x003A: 0xBA,       #  COLON, right-left
    0x003B: 0x3B,       #  SEMICOLON, left-right
    0x003C: 0x3C,       #  LESS-THAN SIGN, left-right
    0x003C: 0xBC,       #  LESS-THAN SIGN, right-left
    0x003D: 0x3D,       #  EQUALS SIGN, left-right
    0x003D: 0xBD,       #  EQUALS SIGN, right-left
    0x003E: 0x3E,       #  GREATER-THAN SIGN, left-right
    0x003E: 0xBE,       #  GREATER-THAN SIGN, right-left
    0x003F: 0x3F,       #  QUESTION MARK, left-right
    0x0040: 0x40,       #  COMMERCIAL AT
    0x0041: 0x41,       #  LATIN CAPITAL LETTER A
    0x0042: 0x42,       #  LATIN CAPITAL LETTER B
    0x0043: 0x43,       #  LATIN CAPITAL LETTER C
    0x0044: 0x44,       #  LATIN CAPITAL LETTER D
    0x0045: 0x45,       #  LATIN CAPITAL LETTER E
    0x0046: 0x46,       #  LATIN CAPITAL LETTER F
    0x0047: 0x47,       #  LATIN CAPITAL LETTER G
    0x0048: 0x48,       #  LATIN CAPITAL LETTER H
    0x0049: 0x49,       #  LATIN CAPITAL LETTER I
    0x004A: 0x4A,       #  LATIN CAPITAL LETTER J
    0x004B: 0x4B,       #  LATIN CAPITAL LETTER K
    0x004C: 0x4C,       #  LATIN CAPITAL LETTER L
    0x004D: 0x4D,       #  LATIN CAPITAL LETTER M
    0x004E: 0x4E,       #  LATIN CAPITAL LETTER N
    0x004F: 0x4F,       #  LATIN CAPITAL LETTER O
    0x0050: 0x50,       #  LATIN CAPITAL LETTER P
    0x0051: 0x51,       #  LATIN CAPITAL LETTER Q
    0x0052: 0x52,       #  LATIN CAPITAL LETTER R
    0x0053: 0x53,       #  LATIN CAPITAL LETTER S
    0x0054: 0x54,       #  LATIN CAPITAL LETTER T
    0x0055: 0x55,       #  LATIN CAPITAL LETTER U
    0x0056: 0x56,       #  LATIN CAPITAL LETTER V
    0x0057: 0x57,       #  LATIN CAPITAL LETTER W
    0x0058: 0x58,       #  LATIN CAPITAL LETTER X
    0x0059: 0x59,       #  LATIN CAPITAL LETTER Y
    0x005A: 0x5A,       #  LATIN CAPITAL LETTER Z
    0x005B: 0x5B,       #  LEFT SQUARE BRACKET, left-right
    0x005B: 0xDB,       #  LEFT SQUARE BRACKET, right-left
    0x005C: 0x5C,       #  REVERSE SOLIDUS, left-right
    0x005C: 0xDC,       #  REVERSE SOLIDUS, right-left
    0x005D: 0x5D,       #  RIGHT SQUARE BRACKET, left-right
    0x005D: 0xDD,       #  RIGHT SQUARE BRACKET, right-left
    0x005E: 0x5E,       #  CIRCUMFLEX ACCENT, left-right
    0x005E: 0xDE,       #  CIRCUMFLEX ACCENT, right-left
    0x005F: 0x5F,       #  LOW LINE, left-right
    0x005F: 0xDF,       #  LOW LINE, right-left
    0x0060: 0x60,       #  GRAVE ACCENT
    0x0061: 0x61,       #  LATIN SMALL LETTER A
    0x0062: 0x62,       #  LATIN SMALL LETTER B
    0x0063: 0x63,       #  LATIN SMALL LETTER C
    0x0064: 0x64,       #  LATIN SMALL LETTER D
    0x0065: 0x65,       #  LATIN SMALL LETTER E
    0x0066: 0x66,       #  LATIN SMALL LETTER F
    0x0067: 0x67,       #  LATIN SMALL LETTER G
    0x0068: 0x68,       #  LATIN SMALL LETTER H
    0x0069: 0x69,       #  LATIN SMALL LETTER I
    0x006A: 0x6A,       #  LATIN SMALL LETTER J
    0x006B: 0x6B,       #  LATIN SMALL LETTER K
    0x006C: 0x6C,       #  LATIN SMALL LETTER L
    0x006D: 0x6D,       #  LATIN SMALL LETTER M
    0x006E: 0x6E,       #  LATIN SMALL LETTER N
    0x006F: 0x6F,       #  LATIN SMALL LETTER O
    0x0070: 0x70,       #  LATIN SMALL LETTER P
    0x0071: 0x71,       #  LATIN SMALL LETTER Q
    0x0072: 0x72,       #  LATIN SMALL LETTER R
    0x0073: 0x73,       #  LATIN SMALL LETTER S
    0x0074: 0x74,       #  LATIN SMALL LETTER T
    0x0075: 0x75,       #  LATIN SMALL LETTER U
    0x0076: 0x76,       #  LATIN SMALL LETTER V
    0x0077: 0x77,       #  LATIN SMALL LETTER W
    0x0078: 0x78,       #  LATIN SMALL LETTER X
    0x0079: 0x79,       #  LATIN SMALL LETTER Y
    0x007A: 0x7A,       #  LATIN SMALL LETTER Z
    0x007B: 0x7B,       #  LEFT CURLY BRACKET, left-right
    0x007B: 0xFB,       #  LEFT CURLY BRACKET, right-left
    0x007C: 0x7C,       #  VERTICAL LINE, left-right
    0x007C: 0xFC,       #  VERTICAL LINE, right-left
    0x007D: 0x7D,       #  RIGHT CURLY BRACKET, left-right
    0x007D: 0xFD,       #  RIGHT CURLY BRACKET, right-left
    0x007E: 0x7E,       #  TILDE
    0x007F: 0x7F,       #  CONTROL CHARACTER
    0x00A0: 0x81,       #  NO-BREAK SPACE, right-left
    0x00AB: 0x8C,       #  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    0x00BB: 0x98,       #  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    0x00C4: 0x80,       #  LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C7: 0x82,       #  LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C9: 0x83,       #  LATIN CAPITAL LETTER E WITH ACUTE
    0x00D1: 0x84,       #  LATIN CAPITAL LETTER N WITH TILDE
    0x00D6: 0x85,       #  LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00DC: 0x86,       #  LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00E0: 0x88,       #  LATIN SMALL LETTER A WITH GRAVE
    0x00E1: 0x87,       #  LATIN SMALL LETTER A WITH ACUTE
    0x00E2: 0x89,       #  LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E4: 0x8A,       #  LATIN SMALL LETTER A WITH DIAERESIS
    0x00E7: 0x8D,       #  LATIN SMALL LETTER C WITH CEDILLA
    0x00E8: 0x8F,       #  LATIN SMALL LETTER E WITH GRAVE
    0x00E9: 0x8E,       #  LATIN SMALL LETTER E WITH ACUTE
    0x00EA: 0x90,       #  LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB: 0x91,       #  LATIN SMALL LETTER E WITH DIAERESIS
    0x00ED: 0x92,       #  LATIN SMALL LETTER I WITH ACUTE
    0x00EE: 0x94,       #  LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF: 0x95,       #  LATIN SMALL LETTER I WITH DIAERESIS
    0x00F1: 0x96,       #  LATIN SMALL LETTER N WITH TILDE
    0x00F3: 0x97,       #  LATIN SMALL LETTER O WITH ACUTE
    0x00F4: 0x99,       #  LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F6: 0x9A,       #  LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7: 0x9B,       #  DIVISION SIGN, right-left
    0x00F9: 0x9D,       #  LATIN SMALL LETTER U WITH GRAVE
    0x00FA: 0x9C,       #  LATIN SMALL LETTER U WITH ACUTE
    0x00FB: 0x9E,       #  LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC: 0x9F,       #  LATIN SMALL LETTER U WITH DIAERESIS
    0x060C: 0xAC,       #  ARABIC COMMA
    0x061B: 0xBB,       #  ARABIC SEMICOLON
    0x061F: 0xBF,       #  ARABIC QUESTION MARK
    0x0621: 0xC1,       #  ARABIC LETTER HAMZA
    0x0622: 0xC2,       #  ARABIC LETTER ALEF WITH MADDA ABOVE
    0x0623: 0xC3,       #  ARABIC LETTER ALEF WITH HAMZA ABOVE
    0x0624: 0xC4,       #  ARABIC LETTER WAW WITH HAMZA ABOVE
    0x0625: 0xC5,       #  ARABIC LETTER ALEF WITH HAMZA BELOW
    0x0626: 0xC6,       #  ARABIC LETTER YEH WITH HAMZA ABOVE
    0x0627: 0xC7,       #  ARABIC LETTER ALEF
    0x0628: 0xC8,       #  ARABIC LETTER BEH
    0x0629: 0xC9,       #  ARABIC LETTER TEH MARBUTA
    0x062A: 0xCA,       #  ARABIC LETTER TEH
    0x062B: 0xCB,       #  ARABIC LETTER THEH
    0x062C: 0xCC,       #  ARABIC LETTER JEEM
    0x062D: 0xCD,       #  ARABIC LETTER HAH
    0x062E: 0xCE,       #  ARABIC LETTER KHAH
    0x062F: 0xCF,       #  ARABIC LETTER DAL
    0x0630: 0xD0,       #  ARABIC LETTER THAL
    0x0631: 0xD1,       #  ARABIC LETTER REH
    0x0632: 0xD2,       #  ARABIC LETTER ZAIN
    0x0633: 0xD3,       #  ARABIC LETTER SEEN
    0x0634: 0xD4,       #  ARABIC LETTER SHEEN
    0x0635: 0xD5,       #  ARABIC LETTER SAD
    0x0636: 0xD6,       #  ARABIC LETTER DAD
    0x0637: 0xD7,       #  ARABIC LETTER TAH
    0x0638: 0xD8,       #  ARABIC LETTER ZAH
    0x0639: 0xD9,       #  ARABIC LETTER AIN
    0x063A: 0xDA,       #  ARABIC LETTER GHAIN
    0x0640: 0xE0,       #  ARABIC TATWEEL
    0x0641: 0xE1,       #  ARABIC LETTER FEH
    0x0642: 0xE2,       #  ARABIC LETTER QAF
    0x0643: 0xE3,       #  ARABIC LETTER KAF
    0x0644: 0xE4,       #  ARABIC LETTER LAM
    0x0645: 0xE5,       #  ARABIC LETTER MEEM
    0x0646: 0xE6,       #  ARABIC LETTER NOON
    0x0647: 0xE7,       #  ARABIC LETTER HEH
    0x0648: 0xE8,       #  ARABIC LETTER WAW
    0x0649: 0xE9,       #  ARABIC LETTER ALEF MAKSURA
    0x064A: 0xEA,       #  ARABIC LETTER YEH
    0x064B: 0xEB,       #  ARABIC FATHATAN
    0x064C: 0xEC,       #  ARABIC DAMMATAN
    0x064D: 0xED,       #  ARABIC KASRATAN
    0x064E: 0xEE,       #  ARABIC FATHA
    0x064F: 0xEF,       #  ARABIC DAMMA
    0x0650: 0xF0,       #  ARABIC KASRA
    0x0651: 0xF1,       #  ARABIC SHADDA
    0x0652: 0xF2,       #  ARABIC SUKUN
    0x066A: 0xA5,       #  ARABIC PERCENT SIGN
    0x0679: 0xF4,       #  ARABIC LETTER TTEH
    0x067E: 0xF3,       #  ARABIC LETTER PEH
    0x0686: 0xF5,       #  ARABIC LETTER TCHEH
    0x0688: 0xF9,       #  ARABIC LETTER DDAL
    0x0691: 0xFA,       #  ARABIC LETTER RREH
    0x0698: 0xFE,       #  ARABIC LETTER JEH
    0x06A4: 0xF7,       #  ARABIC LETTER VEH
    0x06AF: 0xF8,       #  ARABIC LETTER GAF
    0x06BA: 0x8B,       #  ARABIC LETTER NOON GHUNNA
    0x06D2: 0xFF,       #  ARABIC LETTER YEH BARREE
    0x06D5: 0xF6,       #  ARABIC LETTER AE
    0x06F0: 0xB0,       #  EXTENDED ARABIC-INDIC DIGIT ZERO, right-left (need override)
    0x06F1: 0xB1,       #  EXTENDED ARABIC-INDIC DIGIT ONE, right-left (need override)
    0x06F2: 0xB2,       #  EXTENDED ARABIC-INDIC DIGIT TWO, right-left (need override)
    0x06F3: 0xB3,       #  EXTENDED ARABIC-INDIC DIGIT THREE, right-left (need override)
    0x06F4: 0xB4,       #  EXTENDED ARABIC-INDIC DIGIT FOUR, right-left (need override)
    0x06F5: 0xB5,       #  EXTENDED ARABIC-INDIC DIGIT FIVE, right-left (need override)
    0x06F6: 0xB6,       #  EXTENDED ARABIC-INDIC DIGIT SIX, right-left (need override)
    0x06F7: 0xB7,       #  EXTENDED ARABIC-INDIC DIGIT SEVEN, right-left (need override)
    0x06F8: 0xB8,       #  EXTENDED ARABIC-INDIC DIGIT EIGHT, right-left (need override)
    0x06F9: 0xB9,       #  EXTENDED ARABIC-INDIC DIGIT NINE, right-left (need override)
    0x2026: 0x93,       #  HORIZONTAL ELLIPSIS, right-left
    0x274A: 0xC0,       #  EIGHT TEARDROP-SPOKED PROPELLER ASTERISK, right-left
}

