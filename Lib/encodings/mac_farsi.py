""" Python Character Mapping Codec generated from 'MAPPINGS/VENDORS/APPLE/FARSI.TXT' with gencodec.py.

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):

        return codecs.charmap_encode(input,errors,encoding_map)

    def decode(self,input,errors='strict'):

        return codecs.charmap_decode(input,errors,decoding_table)
    
class StreamWriter(Codec,codecs.StreamWriter):
    pass

class StreamReader(Codec,codecs.StreamReader):
    pass

### encodings module API

def getregentry():

    return (Codec().encode,Codec().decode,StreamReader,StreamWriter)


### Decoding Table

decoding_table = (
    u'\x00'	#  0x00 -> CONTROL CHARACTER
    u'\x01'	#  0x01 -> CONTROL CHARACTER
    u'\x02'	#  0x02 -> CONTROL CHARACTER
    u'\x03'	#  0x03 -> CONTROL CHARACTER
    u'\x04'	#  0x04 -> CONTROL CHARACTER
    u'\x05'	#  0x05 -> CONTROL CHARACTER
    u'\x06'	#  0x06 -> CONTROL CHARACTER
    u'\x07'	#  0x07 -> CONTROL CHARACTER
    u'\x08'	#  0x08 -> CONTROL CHARACTER
    u'\t'	#  0x09 -> CONTROL CHARACTER
    u'\n'	#  0x0a -> CONTROL CHARACTER
    u'\x0b'	#  0x0b -> CONTROL CHARACTER
    u'\x0c'	#  0x0c -> CONTROL CHARACTER
    u'\r'	#  0x0d -> CONTROL CHARACTER
    u'\x0e'	#  0x0e -> CONTROL CHARACTER
    u'\x0f'	#  0x0f -> CONTROL CHARACTER
    u'\x10'	#  0x10 -> CONTROL CHARACTER
    u'\x11'	#  0x11 -> CONTROL CHARACTER
    u'\x12'	#  0x12 -> CONTROL CHARACTER
    u'\x13'	#  0x13 -> CONTROL CHARACTER
    u'\x14'	#  0x14 -> CONTROL CHARACTER
    u'\x15'	#  0x15 -> CONTROL CHARACTER
    u'\x16'	#  0x16 -> CONTROL CHARACTER
    u'\x17'	#  0x17 -> CONTROL CHARACTER
    u'\x18'	#  0x18 -> CONTROL CHARACTER
    u'\x19'	#  0x19 -> CONTROL CHARACTER
    u'\x1a'	#  0x1a -> CONTROL CHARACTER
    u'\x1b'	#  0x1b -> CONTROL CHARACTER
    u'\x1c'	#  0x1c -> CONTROL CHARACTER
    u'\x1d'	#  0x1d -> CONTROL CHARACTER
    u'\x1e'	#  0x1e -> CONTROL CHARACTER
    u'\x1f'	#  0x1f -> CONTROL CHARACTER
    u' '	#  0x20 -> SPACE, left-right
    u'!'	#  0x21 -> EXCLAMATION MARK, left-right
    u'"'	#  0x22 -> QUOTATION MARK, left-right
    u'#'	#  0x23 -> NUMBER SIGN, left-right
    u'$'	#  0x24 -> DOLLAR SIGN, left-right
    u'%'	#  0x25 -> PERCENT SIGN, left-right
    u'&'	#  0x26 -> AMPERSAND, left-right
    u"'"	#  0x27 -> APOSTROPHE, left-right
    u'('	#  0x28 -> LEFT PARENTHESIS, left-right
    u')'	#  0x29 -> RIGHT PARENTHESIS, left-right
    u'*'	#  0x2a -> ASTERISK, left-right
    u'+'	#  0x2b -> PLUS SIGN, left-right
    u','	#  0x2c -> COMMA, left-right; in Arabic-script context, displayed as 0x066C ARABIC THOUSANDS SEPARATOR
    u'-'	#  0x2d -> HYPHEN-MINUS, left-right
    u'.'	#  0x2e -> FULL STOP, left-right; in Arabic-script context, displayed as 0x066B ARABIC DECIMAL SEPARATOR
    u'/'	#  0x2f -> SOLIDUS, left-right
    u'0'	#  0x30 -> DIGIT ZERO;  in Arabic-script context, displayed as 0x06F0 EXTENDED ARABIC-INDIC DIGIT ZERO
    u'1'	#  0x31 -> DIGIT ONE;   in Arabic-script context, displayed as 0x06F1 EXTENDED ARABIC-INDIC DIGIT ONE
    u'2'	#  0x32 -> DIGIT TWO;   in Arabic-script context, displayed as 0x06F2 EXTENDED ARABIC-INDIC DIGIT TWO
    u'3'	#  0x33 -> DIGIT THREE; in Arabic-script context, displayed as 0x06F3 EXTENDED ARABIC-INDIC DIGIT THREE
    u'4'	#  0x34 -> DIGIT FOUR;  in Arabic-script context, displayed as 0x06F4 EXTENDED ARABIC-INDIC DIGIT FOUR
    u'5'	#  0x35 -> DIGIT FIVE;  in Arabic-script context, displayed as 0x06F5 EXTENDED ARABIC-INDIC DIGIT FIVE
    u'6'	#  0x36 -> DIGIT SIX;   in Arabic-script context, displayed as 0x06F6 EXTENDED ARABIC-INDIC DIGIT SIX
    u'7'	#  0x37 -> DIGIT SEVEN; in Arabic-script context, displayed as 0x06F7 EXTENDED ARABIC-INDIC DIGIT SEVEN
    u'8'	#  0x38 -> DIGIT EIGHT; in Arabic-script context, displayed as 0x06F8 EXTENDED ARABIC-INDIC DIGIT EIGHT
    u'9'	#  0x39 -> DIGIT NINE;  in Arabic-script context, displayed as 0x06F9 EXTENDED ARABIC-INDIC DIGIT NINE
    u':'	#  0x3a -> COLON, left-right
    u';'	#  0x3b -> SEMICOLON, left-right
    u'<'	#  0x3c -> LESS-THAN SIGN, left-right
    u'='	#  0x3d -> EQUALS SIGN, left-right
    u'>'	#  0x3e -> GREATER-THAN SIGN, left-right
    u'?'	#  0x3f -> QUESTION MARK, left-right
    u'@'	#  0x40 -> COMMERCIAL AT
    u'A'	#  0x41 -> LATIN CAPITAL LETTER A
    u'B'	#  0x42 -> LATIN CAPITAL LETTER B
    u'C'	#  0x43 -> LATIN CAPITAL LETTER C
    u'D'	#  0x44 -> LATIN CAPITAL LETTER D
    u'E'	#  0x45 -> LATIN CAPITAL LETTER E
    u'F'	#  0x46 -> LATIN CAPITAL LETTER F
    u'G'	#  0x47 -> LATIN CAPITAL LETTER G
    u'H'	#  0x48 -> LATIN CAPITAL LETTER H
    u'I'	#  0x49 -> LATIN CAPITAL LETTER I
    u'J'	#  0x4a -> LATIN CAPITAL LETTER J
    u'K'	#  0x4b -> LATIN CAPITAL LETTER K
    u'L'	#  0x4c -> LATIN CAPITAL LETTER L
    u'M'	#  0x4d -> LATIN CAPITAL LETTER M
    u'N'	#  0x4e -> LATIN CAPITAL LETTER N
    u'O'	#  0x4f -> LATIN CAPITAL LETTER O
    u'P'	#  0x50 -> LATIN CAPITAL LETTER P
    u'Q'	#  0x51 -> LATIN CAPITAL LETTER Q
    u'R'	#  0x52 -> LATIN CAPITAL LETTER R
    u'S'	#  0x53 -> LATIN CAPITAL LETTER S
    u'T'	#  0x54 -> LATIN CAPITAL LETTER T
    u'U'	#  0x55 -> LATIN CAPITAL LETTER U
    u'V'	#  0x56 -> LATIN CAPITAL LETTER V
    u'W'	#  0x57 -> LATIN CAPITAL LETTER W
    u'X'	#  0x58 -> LATIN CAPITAL LETTER X
    u'Y'	#  0x59 -> LATIN CAPITAL LETTER Y
    u'Z'	#  0x5a -> LATIN CAPITAL LETTER Z
    u'['	#  0x5b -> LEFT SQUARE BRACKET, left-right
    u'\\'	#  0x5c -> REVERSE SOLIDUS, left-right
    u']'	#  0x5d -> RIGHT SQUARE BRACKET, left-right
    u'^'	#  0x5e -> CIRCUMFLEX ACCENT, left-right
    u'_'	#  0x5f -> LOW LINE, left-right
    u'`'	#  0x60 -> GRAVE ACCENT
    u'a'	#  0x61 -> LATIN SMALL LETTER A
    u'b'	#  0x62 -> LATIN SMALL LETTER B
    u'c'	#  0x63 -> LATIN SMALL LETTER C
    u'd'	#  0x64 -> LATIN SMALL LETTER D
    u'e'	#  0x65 -> LATIN SMALL LETTER E
    u'f'	#  0x66 -> LATIN SMALL LETTER F
    u'g'	#  0x67 -> LATIN SMALL LETTER G
    u'h'	#  0x68 -> LATIN SMALL LETTER H
    u'i'	#  0x69 -> LATIN SMALL LETTER I
    u'j'	#  0x6a -> LATIN SMALL LETTER J
    u'k'	#  0x6b -> LATIN SMALL LETTER K
    u'l'	#  0x6c -> LATIN SMALL LETTER L
    u'm'	#  0x6d -> LATIN SMALL LETTER M
    u'n'	#  0x6e -> LATIN SMALL LETTER N
    u'o'	#  0x6f -> LATIN SMALL LETTER O
    u'p'	#  0x70 -> LATIN SMALL LETTER P
    u'q'	#  0x71 -> LATIN SMALL LETTER Q
    u'r'	#  0x72 -> LATIN SMALL LETTER R
    u's'	#  0x73 -> LATIN SMALL LETTER S
    u't'	#  0x74 -> LATIN SMALL LETTER T
    u'u'	#  0x75 -> LATIN SMALL LETTER U
    u'v'	#  0x76 -> LATIN SMALL LETTER V
    u'w'	#  0x77 -> LATIN SMALL LETTER W
    u'x'	#  0x78 -> LATIN SMALL LETTER X
    u'y'	#  0x79 -> LATIN SMALL LETTER Y
    u'z'	#  0x7a -> LATIN SMALL LETTER Z
    u'{'	#  0x7b -> LEFT CURLY BRACKET, left-right
    u'|'	#  0x7c -> VERTICAL LINE, left-right
    u'}'	#  0x7d -> RIGHT CURLY BRACKET, left-right
    u'~'	#  0x7e -> TILDE
    u'\x7f'	#  0x7f -> CONTROL CHARACTER
    u'\xc4'	#  0x80 -> LATIN CAPITAL LETTER A WITH DIAERESIS
    u'\xa0'	#  0x81 -> NO-BREAK SPACE, right-left
    u'\xc7'	#  0x82 -> LATIN CAPITAL LETTER C WITH CEDILLA
    u'\xc9'	#  0x83 -> LATIN CAPITAL LETTER E WITH ACUTE
    u'\xd1'	#  0x84 -> LATIN CAPITAL LETTER N WITH TILDE
    u'\xd6'	#  0x85 -> LATIN CAPITAL LETTER O WITH DIAERESIS
    u'\xdc'	#  0x86 -> LATIN CAPITAL LETTER U WITH DIAERESIS
    u'\xe1'	#  0x87 -> LATIN SMALL LETTER A WITH ACUTE
    u'\xe0'	#  0x88 -> LATIN SMALL LETTER A WITH GRAVE
    u'\xe2'	#  0x89 -> LATIN SMALL LETTER A WITH CIRCUMFLEX
    u'\xe4'	#  0x8a -> LATIN SMALL LETTER A WITH DIAERESIS
    u'\u06ba'	#  0x8b -> ARABIC LETTER NOON GHUNNA
    u'\xab'	#  0x8c -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    u'\xe7'	#  0x8d -> LATIN SMALL LETTER C WITH CEDILLA
    u'\xe9'	#  0x8e -> LATIN SMALL LETTER E WITH ACUTE
    u'\xe8'	#  0x8f -> LATIN SMALL LETTER E WITH GRAVE
    u'\xea'	#  0x90 -> LATIN SMALL LETTER E WITH CIRCUMFLEX
    u'\xeb'	#  0x91 -> LATIN SMALL LETTER E WITH DIAERESIS
    u'\xed'	#  0x92 -> LATIN SMALL LETTER I WITH ACUTE
    u'\u2026'	#  0x93 -> HORIZONTAL ELLIPSIS, right-left
    u'\xee'	#  0x94 -> LATIN SMALL LETTER I WITH CIRCUMFLEX
    u'\xef'	#  0x95 -> LATIN SMALL LETTER I WITH DIAERESIS
    u'\xf1'	#  0x96 -> LATIN SMALL LETTER N WITH TILDE
    u'\xf3'	#  0x97 -> LATIN SMALL LETTER O WITH ACUTE
    u'\xbb'	#  0x98 -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    u'\xf4'	#  0x99 -> LATIN SMALL LETTER O WITH CIRCUMFLEX
    u'\xf6'	#  0x9a -> LATIN SMALL LETTER O WITH DIAERESIS
    u'\xf7'	#  0x9b -> DIVISION SIGN, right-left
    u'\xfa'	#  0x9c -> LATIN SMALL LETTER U WITH ACUTE
    u'\xf9'	#  0x9d -> LATIN SMALL LETTER U WITH GRAVE
    u'\xfb'	#  0x9e -> LATIN SMALL LETTER U WITH CIRCUMFLEX
    u'\xfc'	#  0x9f -> LATIN SMALL LETTER U WITH DIAERESIS
    u' '	#  0xa0 -> SPACE, right-left
    u'!'	#  0xa1 -> EXCLAMATION MARK, right-left
    u'"'	#  0xa2 -> QUOTATION MARK, right-left
    u'#'	#  0xa3 -> NUMBER SIGN, right-left
    u'$'	#  0xa4 -> DOLLAR SIGN, right-left
    u'\u066a'	#  0xa5 -> ARABIC PERCENT SIGN
    u'&'	#  0xa6 -> AMPERSAND, right-left
    u"'"	#  0xa7 -> APOSTROPHE, right-left
    u'('	#  0xa8 -> LEFT PARENTHESIS, right-left
    u')'	#  0xa9 -> RIGHT PARENTHESIS, right-left
    u'*'	#  0xaa -> ASTERISK, right-left
    u'+'	#  0xab -> PLUS SIGN, right-left
    u'\u060c'	#  0xac -> ARABIC COMMA
    u'-'	#  0xad -> HYPHEN-MINUS, right-left
    u'.'	#  0xae -> FULL STOP, right-left
    u'/'	#  0xaf -> SOLIDUS, right-left
    u'\u06f0'	#  0xb0 -> EXTENDED ARABIC-INDIC DIGIT ZERO, right-left (need override)
    u'\u06f1'	#  0xb1 -> EXTENDED ARABIC-INDIC DIGIT ONE, right-left (need override)
    u'\u06f2'	#  0xb2 -> EXTENDED ARABIC-INDIC DIGIT TWO, right-left (need override)
    u'\u06f3'	#  0xb3 -> EXTENDED ARABIC-INDIC DIGIT THREE, right-left (need override)
    u'\u06f4'	#  0xb4 -> EXTENDED ARABIC-INDIC DIGIT FOUR, right-left (need override)
    u'\u06f5'	#  0xb5 -> EXTENDED ARABIC-INDIC DIGIT FIVE, right-left (need override)
    u'\u06f6'	#  0xb6 -> EXTENDED ARABIC-INDIC DIGIT SIX, right-left (need override)
    u'\u06f7'	#  0xb7 -> EXTENDED ARABIC-INDIC DIGIT SEVEN, right-left (need override)
    u'\u06f8'	#  0xb8 -> EXTENDED ARABIC-INDIC DIGIT EIGHT, right-left (need override)
    u'\u06f9'	#  0xb9 -> EXTENDED ARABIC-INDIC DIGIT NINE, right-left (need override)
    u':'	#  0xba -> COLON, right-left
    u'\u061b'	#  0xbb -> ARABIC SEMICOLON
    u'<'	#  0xbc -> LESS-THAN SIGN, right-left
    u'='	#  0xbd -> EQUALS SIGN, right-left
    u'>'	#  0xbe -> GREATER-THAN SIGN, right-left
    u'\u061f'	#  0xbf -> ARABIC QUESTION MARK
    u'\u274a'	#  0xc0 -> EIGHT TEARDROP-SPOKED PROPELLER ASTERISK, right-left
    u'\u0621'	#  0xc1 -> ARABIC LETTER HAMZA
    u'\u0622'	#  0xc2 -> ARABIC LETTER ALEF WITH MADDA ABOVE
    u'\u0623'	#  0xc3 -> ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0624'	#  0xc4 -> ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0625'	#  0xc5 -> ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0626'	#  0xc6 -> ARABIC LETTER YEH WITH HAMZA ABOVE
    u'\u0627'	#  0xc7 -> ARABIC LETTER ALEF
    u'\u0628'	#  0xc8 -> ARABIC LETTER BEH
    u'\u0629'	#  0xc9 -> ARABIC LETTER TEH MARBUTA
    u'\u062a'	#  0xca -> ARABIC LETTER TEH
    u'\u062b'	#  0xcb -> ARABIC LETTER THEH
    u'\u062c'	#  0xcc -> ARABIC LETTER JEEM
    u'\u062d'	#  0xcd -> ARABIC LETTER HAH
    u'\u062e'	#  0xce -> ARABIC LETTER KHAH
    u'\u062f'	#  0xcf -> ARABIC LETTER DAL
    u'\u0630'	#  0xd0 -> ARABIC LETTER THAL
    u'\u0631'	#  0xd1 -> ARABIC LETTER REH
    u'\u0632'	#  0xd2 -> ARABIC LETTER ZAIN
    u'\u0633'	#  0xd3 -> ARABIC LETTER SEEN
    u'\u0634'	#  0xd4 -> ARABIC LETTER SHEEN
    u'\u0635'	#  0xd5 -> ARABIC LETTER SAD
    u'\u0636'	#  0xd6 -> ARABIC LETTER DAD
    u'\u0637'	#  0xd7 -> ARABIC LETTER TAH
    u'\u0638'	#  0xd8 -> ARABIC LETTER ZAH
    u'\u0639'	#  0xd9 -> ARABIC LETTER AIN
    u'\u063a'	#  0xda -> ARABIC LETTER GHAIN
    u'['	#  0xdb -> LEFT SQUARE BRACKET, right-left
    u'\\'	#  0xdc -> REVERSE SOLIDUS, right-left
    u']'	#  0xdd -> RIGHT SQUARE BRACKET, right-left
    u'^'	#  0xde -> CIRCUMFLEX ACCENT, right-left
    u'_'	#  0xdf -> LOW LINE, right-left
    u'\u0640'	#  0xe0 -> ARABIC TATWEEL
    u'\u0641'	#  0xe1 -> ARABIC LETTER FEH
    u'\u0642'	#  0xe2 -> ARABIC LETTER QAF
    u'\u0643'	#  0xe3 -> ARABIC LETTER KAF
    u'\u0644'	#  0xe4 -> ARABIC LETTER LAM
    u'\u0645'	#  0xe5 -> ARABIC LETTER MEEM
    u'\u0646'	#  0xe6 -> ARABIC LETTER NOON
    u'\u0647'	#  0xe7 -> ARABIC LETTER HEH
    u'\u0648'	#  0xe8 -> ARABIC LETTER WAW
    u'\u0649'	#  0xe9 -> ARABIC LETTER ALEF MAKSURA
    u'\u064a'	#  0xea -> ARABIC LETTER YEH
    u'\u064b'	#  0xeb -> ARABIC FATHATAN
    u'\u064c'	#  0xec -> ARABIC DAMMATAN
    u'\u064d'	#  0xed -> ARABIC KASRATAN
    u'\u064e'	#  0xee -> ARABIC FATHA
    u'\u064f'	#  0xef -> ARABIC DAMMA
    u'\u0650'	#  0xf0 -> ARABIC KASRA
    u'\u0651'	#  0xf1 -> ARABIC SHADDA
    u'\u0652'	#  0xf2 -> ARABIC SUKUN
    u'\u067e'	#  0xf3 -> ARABIC LETTER PEH
    u'\u0679'	#  0xf4 -> ARABIC LETTER TTEH
    u'\u0686'	#  0xf5 -> ARABIC LETTER TCHEH
    u'\u06d5'	#  0xf6 -> ARABIC LETTER AE
    u'\u06a4'	#  0xf7 -> ARABIC LETTER VEH
    u'\u06af'	#  0xf8 -> ARABIC LETTER GAF
    u'\u0688'	#  0xf9 -> ARABIC LETTER DDAL
    u'\u0691'	#  0xfa -> ARABIC LETTER RREH
    u'{'	#  0xfb -> LEFT CURLY BRACKET, right-left
    u'|'	#  0xfc -> VERTICAL LINE, right-left
    u'}'	#  0xfd -> RIGHT CURLY BRACKET, right-left
    u'\u0698'	#  0xfe -> ARABIC LETTER JEH
    u'\u06d2'	#  0xff -> ARABIC LETTER YEH BARREE
)

### Encoding Map

encoding_map = {
    0x0000: 0x00,	#  CONTROL CHARACTER
    0x0001: 0x01,	#  CONTROL CHARACTER
    0x0002: 0x02,	#  CONTROL CHARACTER
    0x0003: 0x03,	#  CONTROL CHARACTER
    0x0004: 0x04,	#  CONTROL CHARACTER
    0x0005: 0x05,	#  CONTROL CHARACTER
    0x0006: 0x06,	#  CONTROL CHARACTER
    0x0007: 0x07,	#  CONTROL CHARACTER
    0x0008: 0x08,	#  CONTROL CHARACTER
    0x0009: 0x09,	#  CONTROL CHARACTER
    0x000a: 0x0a,	#  CONTROL CHARACTER
    0x000b: 0x0b,	#  CONTROL CHARACTER
    0x000c: 0x0c,	#  CONTROL CHARACTER
    0x000d: 0x0d,	#  CONTROL CHARACTER
    0x000e: 0x0e,	#  CONTROL CHARACTER
    0x000f: 0x0f,	#  CONTROL CHARACTER
    0x0010: 0x10,	#  CONTROL CHARACTER
    0x0011: 0x11,	#  CONTROL CHARACTER
    0x0012: 0x12,	#  CONTROL CHARACTER
    0x0013: 0x13,	#  CONTROL CHARACTER
    0x0014: 0x14,	#  CONTROL CHARACTER
    0x0015: 0x15,	#  CONTROL CHARACTER
    0x0016: 0x16,	#  CONTROL CHARACTER
    0x0017: 0x17,	#  CONTROL CHARACTER
    0x0018: 0x18,	#  CONTROL CHARACTER
    0x0019: 0x19,	#  CONTROL CHARACTER
    0x001a: 0x1a,	#  CONTROL CHARACTER
    0x001b: 0x1b,	#  CONTROL CHARACTER
    0x001c: 0x1c,	#  CONTROL CHARACTER
    0x001d: 0x1d,	#  CONTROL CHARACTER
    0x001e: 0x1e,	#  CONTROL CHARACTER
    0x001f: 0x1f,	#  CONTROL CHARACTER
    0x0020: 0x20,	#  SPACE, left-right
    0x0020: 0xa0,	#  SPACE, right-left
    0x0021: 0x21,	#  EXCLAMATION MARK, left-right
    0x0021: 0xa1,	#  EXCLAMATION MARK, right-left
    0x0022: 0x22,	#  QUOTATION MARK, left-right
    0x0022: 0xa2,	#  QUOTATION MARK, right-left
    0x0023: 0x23,	#  NUMBER SIGN, left-right
    0x0023: 0xa3,	#  NUMBER SIGN, right-left
    0x0024: 0x24,	#  DOLLAR SIGN, left-right
    0x0024: 0xa4,	#  DOLLAR SIGN, right-left
    0x0025: 0x25,	#  PERCENT SIGN, left-right
    0x0026: 0x26,	#  AMPERSAND, left-right
    0x0026: 0xa6,	#  AMPERSAND, right-left
    0x0027: 0x27,	#  APOSTROPHE, left-right
    0x0027: 0xa7,	#  APOSTROPHE, right-left
    0x0028: 0x28,	#  LEFT PARENTHESIS, left-right
    0x0028: 0xa8,	#  LEFT PARENTHESIS, right-left
    0x0029: 0x29,	#  RIGHT PARENTHESIS, left-right
    0x0029: 0xa9,	#  RIGHT PARENTHESIS, right-left
    0x002a: 0x2a,	#  ASTERISK, left-right
    0x002a: 0xaa,	#  ASTERISK, right-left
    0x002b: 0x2b,	#  PLUS SIGN, left-right
    0x002b: 0xab,	#  PLUS SIGN, right-left
    0x002c: 0x2c,	#  COMMA, left-right; in Arabic-script context, displayed as 0x066C ARABIC THOUSANDS SEPARATOR
    0x002d: 0x2d,	#  HYPHEN-MINUS, left-right
    0x002d: 0xad,	#  HYPHEN-MINUS, right-left
    0x002e: 0x2e,	#  FULL STOP, left-right; in Arabic-script context, displayed as 0x066B ARABIC DECIMAL SEPARATOR
    0x002e: 0xae,	#  FULL STOP, right-left
    0x002f: 0x2f,	#  SOLIDUS, left-right
    0x002f: 0xaf,	#  SOLIDUS, right-left
    0x0030: 0x30,	#  DIGIT ZERO;  in Arabic-script context, displayed as 0x06F0 EXTENDED ARABIC-INDIC DIGIT ZERO
    0x0031: 0x31,	#  DIGIT ONE;   in Arabic-script context, displayed as 0x06F1 EXTENDED ARABIC-INDIC DIGIT ONE
    0x0032: 0x32,	#  DIGIT TWO;   in Arabic-script context, displayed as 0x06F2 EXTENDED ARABIC-INDIC DIGIT TWO
    0x0033: 0x33,	#  DIGIT THREE; in Arabic-script context, displayed as 0x06F3 EXTENDED ARABIC-INDIC DIGIT THREE
    0x0034: 0x34,	#  DIGIT FOUR;  in Arabic-script context, displayed as 0x06F4 EXTENDED ARABIC-INDIC DIGIT FOUR
    0x0035: 0x35,	#  DIGIT FIVE;  in Arabic-script context, displayed as 0x06F5 EXTENDED ARABIC-INDIC DIGIT FIVE
    0x0036: 0x36,	#  DIGIT SIX;   in Arabic-script context, displayed as 0x06F6 EXTENDED ARABIC-INDIC DIGIT SIX
    0x0037: 0x37,	#  DIGIT SEVEN; in Arabic-script context, displayed as 0x06F7 EXTENDED ARABIC-INDIC DIGIT SEVEN
    0x0038: 0x38,	#  DIGIT EIGHT; in Arabic-script context, displayed as 0x06F8 EXTENDED ARABIC-INDIC DIGIT EIGHT
    0x0039: 0x39,	#  DIGIT NINE;  in Arabic-script context, displayed as 0x06F9 EXTENDED ARABIC-INDIC DIGIT NINE
    0x003a: 0x3a,	#  COLON, left-right
    0x003a: 0xba,	#  COLON, right-left
    0x003b: 0x3b,	#  SEMICOLON, left-right
    0x003c: 0x3c,	#  LESS-THAN SIGN, left-right
    0x003c: 0xbc,	#  LESS-THAN SIGN, right-left
    0x003d: 0x3d,	#  EQUALS SIGN, left-right
    0x003d: 0xbd,	#  EQUALS SIGN, right-left
    0x003e: 0x3e,	#  GREATER-THAN SIGN, left-right
    0x003e: 0xbe,	#  GREATER-THAN SIGN, right-left
    0x003f: 0x3f,	#  QUESTION MARK, left-right
    0x0040: 0x40,	#  COMMERCIAL AT
    0x0041: 0x41,	#  LATIN CAPITAL LETTER A
    0x0042: 0x42,	#  LATIN CAPITAL LETTER B
    0x0043: 0x43,	#  LATIN CAPITAL LETTER C
    0x0044: 0x44,	#  LATIN CAPITAL LETTER D
    0x0045: 0x45,	#  LATIN CAPITAL LETTER E
    0x0046: 0x46,	#  LATIN CAPITAL LETTER F
    0x0047: 0x47,	#  LATIN CAPITAL LETTER G
    0x0048: 0x48,	#  LATIN CAPITAL LETTER H
    0x0049: 0x49,	#  LATIN CAPITAL LETTER I
    0x004a: 0x4a,	#  LATIN CAPITAL LETTER J
    0x004b: 0x4b,	#  LATIN CAPITAL LETTER K
    0x004c: 0x4c,	#  LATIN CAPITAL LETTER L
    0x004d: 0x4d,	#  LATIN CAPITAL LETTER M
    0x004e: 0x4e,	#  LATIN CAPITAL LETTER N
    0x004f: 0x4f,	#  LATIN CAPITAL LETTER O
    0x0050: 0x50,	#  LATIN CAPITAL LETTER P
    0x0051: 0x51,	#  LATIN CAPITAL LETTER Q
    0x0052: 0x52,	#  LATIN CAPITAL LETTER R
    0x0053: 0x53,	#  LATIN CAPITAL LETTER S
    0x0054: 0x54,	#  LATIN CAPITAL LETTER T
    0x0055: 0x55,	#  LATIN CAPITAL LETTER U
    0x0056: 0x56,	#  LATIN CAPITAL LETTER V
    0x0057: 0x57,	#  LATIN CAPITAL LETTER W
    0x0058: 0x58,	#  LATIN CAPITAL LETTER X
    0x0059: 0x59,	#  LATIN CAPITAL LETTER Y
    0x005a: 0x5a,	#  LATIN CAPITAL LETTER Z
    0x005b: 0x5b,	#  LEFT SQUARE BRACKET, left-right
    0x005b: 0xdb,	#  LEFT SQUARE BRACKET, right-left
    0x005c: 0x5c,	#  REVERSE SOLIDUS, left-right
    0x005c: 0xdc,	#  REVERSE SOLIDUS, right-left
    0x005d: 0x5d,	#  RIGHT SQUARE BRACKET, left-right
    0x005d: 0xdd,	#  RIGHT SQUARE BRACKET, right-left
    0x005e: 0x5e,	#  CIRCUMFLEX ACCENT, left-right
    0x005e: 0xde,	#  CIRCUMFLEX ACCENT, right-left
    0x005f: 0x5f,	#  LOW LINE, left-right
    0x005f: 0xdf,	#  LOW LINE, right-left
    0x0060: 0x60,	#  GRAVE ACCENT
    0x0061: 0x61,	#  LATIN SMALL LETTER A
    0x0062: 0x62,	#  LATIN SMALL LETTER B
    0x0063: 0x63,	#  LATIN SMALL LETTER C
    0x0064: 0x64,	#  LATIN SMALL LETTER D
    0x0065: 0x65,	#  LATIN SMALL LETTER E
    0x0066: 0x66,	#  LATIN SMALL LETTER F
    0x0067: 0x67,	#  LATIN SMALL LETTER G
    0x0068: 0x68,	#  LATIN SMALL LETTER H
    0x0069: 0x69,	#  LATIN SMALL LETTER I
    0x006a: 0x6a,	#  LATIN SMALL LETTER J
    0x006b: 0x6b,	#  LATIN SMALL LETTER K
    0x006c: 0x6c,	#  LATIN SMALL LETTER L
    0x006d: 0x6d,	#  LATIN SMALL LETTER M
    0x006e: 0x6e,	#  LATIN SMALL LETTER N
    0x006f: 0x6f,	#  LATIN SMALL LETTER O
    0x0070: 0x70,	#  LATIN SMALL LETTER P
    0x0071: 0x71,	#  LATIN SMALL LETTER Q
    0x0072: 0x72,	#  LATIN SMALL LETTER R
    0x0073: 0x73,	#  LATIN SMALL LETTER S
    0x0074: 0x74,	#  LATIN SMALL LETTER T
    0x0075: 0x75,	#  LATIN SMALL LETTER U
    0x0076: 0x76,	#  LATIN SMALL LETTER V
    0x0077: 0x77,	#  LATIN SMALL LETTER W
    0x0078: 0x78,	#  LATIN SMALL LETTER X
    0x0079: 0x79,	#  LATIN SMALL LETTER Y
    0x007a: 0x7a,	#  LATIN SMALL LETTER Z
    0x007b: 0x7b,	#  LEFT CURLY BRACKET, left-right
    0x007b: 0xfb,	#  LEFT CURLY BRACKET, right-left
    0x007c: 0x7c,	#  VERTICAL LINE, left-right
    0x007c: 0xfc,	#  VERTICAL LINE, right-left
    0x007d: 0x7d,	#  RIGHT CURLY BRACKET, left-right
    0x007d: 0xfd,	#  RIGHT CURLY BRACKET, right-left
    0x007e: 0x7e,	#  TILDE
    0x007f: 0x7f,	#  CONTROL CHARACTER
    0x00a0: 0x81,	#  NO-BREAK SPACE, right-left
    0x00ab: 0x8c,	#  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    0x00bb: 0x98,	#  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK, right-left
    0x00c4: 0x80,	#  LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00c7: 0x82,	#  LATIN CAPITAL LETTER C WITH CEDILLA
    0x00c9: 0x83,	#  LATIN CAPITAL LETTER E WITH ACUTE
    0x00d1: 0x84,	#  LATIN CAPITAL LETTER N WITH TILDE
    0x00d6: 0x85,	#  LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00dc: 0x86,	#  LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00e0: 0x88,	#  LATIN SMALL LETTER A WITH GRAVE
    0x00e1: 0x87,	#  LATIN SMALL LETTER A WITH ACUTE
    0x00e2: 0x89,	#  LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00e4: 0x8a,	#  LATIN SMALL LETTER A WITH DIAERESIS
    0x00e7: 0x8d,	#  LATIN SMALL LETTER C WITH CEDILLA
    0x00e8: 0x8f,	#  LATIN SMALL LETTER E WITH GRAVE
    0x00e9: 0x8e,	#  LATIN SMALL LETTER E WITH ACUTE
    0x00ea: 0x90,	#  LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00eb: 0x91,	#  LATIN SMALL LETTER E WITH DIAERESIS
    0x00ed: 0x92,	#  LATIN SMALL LETTER I WITH ACUTE
    0x00ee: 0x94,	#  LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00ef: 0x95,	#  LATIN SMALL LETTER I WITH DIAERESIS
    0x00f1: 0x96,	#  LATIN SMALL LETTER N WITH TILDE
    0x00f3: 0x97,	#  LATIN SMALL LETTER O WITH ACUTE
    0x00f4: 0x99,	#  LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00f6: 0x9a,	#  LATIN SMALL LETTER O WITH DIAERESIS
    0x00f7: 0x9b,	#  DIVISION SIGN, right-left
    0x00f9: 0x9d,	#  LATIN SMALL LETTER U WITH GRAVE
    0x00fa: 0x9c,	#  LATIN SMALL LETTER U WITH ACUTE
    0x00fb: 0x9e,	#  LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00fc: 0x9f,	#  LATIN SMALL LETTER U WITH DIAERESIS
    0x060c: 0xac,	#  ARABIC COMMA
    0x061b: 0xbb,	#  ARABIC SEMICOLON
    0x061f: 0xbf,	#  ARABIC QUESTION MARK
    0x0621: 0xc1,	#  ARABIC LETTER HAMZA
    0x0622: 0xc2,	#  ARABIC LETTER ALEF WITH MADDA ABOVE
    0x0623: 0xc3,	#  ARABIC LETTER ALEF WITH HAMZA ABOVE
    0x0624: 0xc4,	#  ARABIC LETTER WAW WITH HAMZA ABOVE
    0x0625: 0xc5,	#  ARABIC LETTER ALEF WITH HAMZA BELOW
    0x0626: 0xc6,	#  ARABIC LETTER YEH WITH HAMZA ABOVE
    0x0627: 0xc7,	#  ARABIC LETTER ALEF
    0x0628: 0xc8,	#  ARABIC LETTER BEH
    0x0629: 0xc9,	#  ARABIC LETTER TEH MARBUTA
    0x062a: 0xca,	#  ARABIC LETTER TEH
    0x062b: 0xcb,	#  ARABIC LETTER THEH
    0x062c: 0xcc,	#  ARABIC LETTER JEEM
    0x062d: 0xcd,	#  ARABIC LETTER HAH
    0x062e: 0xce,	#  ARABIC LETTER KHAH
    0x062f: 0xcf,	#  ARABIC LETTER DAL
    0x0630: 0xd0,	#  ARABIC LETTER THAL
    0x0631: 0xd1,	#  ARABIC LETTER REH
    0x0632: 0xd2,	#  ARABIC LETTER ZAIN
    0x0633: 0xd3,	#  ARABIC LETTER SEEN
    0x0634: 0xd4,	#  ARABIC LETTER SHEEN
    0x0635: 0xd5,	#  ARABIC LETTER SAD
    0x0636: 0xd6,	#  ARABIC LETTER DAD
    0x0637: 0xd7,	#  ARABIC LETTER TAH
    0x0638: 0xd8,	#  ARABIC LETTER ZAH
    0x0639: 0xd9,	#  ARABIC LETTER AIN
    0x063a: 0xda,	#  ARABIC LETTER GHAIN
    0x0640: 0xe0,	#  ARABIC TATWEEL
    0x0641: 0xe1,	#  ARABIC LETTER FEH
    0x0642: 0xe2,	#  ARABIC LETTER QAF
    0x0643: 0xe3,	#  ARABIC LETTER KAF
    0x0644: 0xe4,	#  ARABIC LETTER LAM
    0x0645: 0xe5,	#  ARABIC LETTER MEEM
    0x0646: 0xe6,	#  ARABIC LETTER NOON
    0x0647: 0xe7,	#  ARABIC LETTER HEH
    0x0648: 0xe8,	#  ARABIC LETTER WAW
    0x0649: 0xe9,	#  ARABIC LETTER ALEF MAKSURA
    0x064a: 0xea,	#  ARABIC LETTER YEH
    0x064b: 0xeb,	#  ARABIC FATHATAN
    0x064c: 0xec,	#  ARABIC DAMMATAN
    0x064d: 0xed,	#  ARABIC KASRATAN
    0x064e: 0xee,	#  ARABIC FATHA
    0x064f: 0xef,	#  ARABIC DAMMA
    0x0650: 0xf0,	#  ARABIC KASRA
    0x0651: 0xf1,	#  ARABIC SHADDA
    0x0652: 0xf2,	#  ARABIC SUKUN
    0x066a: 0xa5,	#  ARABIC PERCENT SIGN
    0x0679: 0xf4,	#  ARABIC LETTER TTEH
    0x067e: 0xf3,	#  ARABIC LETTER PEH
    0x0686: 0xf5,	#  ARABIC LETTER TCHEH
    0x0688: 0xf9,	#  ARABIC LETTER DDAL
    0x0691: 0xfa,	#  ARABIC LETTER RREH
    0x0698: 0xfe,	#  ARABIC LETTER JEH
    0x06a4: 0xf7,	#  ARABIC LETTER VEH
    0x06af: 0xf8,	#  ARABIC LETTER GAF
    0x06ba: 0x8b,	#  ARABIC LETTER NOON GHUNNA
    0x06d2: 0xff,	#  ARABIC LETTER YEH BARREE
    0x06d5: 0xf6,	#  ARABIC LETTER AE
    0x06f0: 0xb0,	#  EXTENDED ARABIC-INDIC DIGIT ZERO, right-left (need override)
    0x06f1: 0xb1,	#  EXTENDED ARABIC-INDIC DIGIT ONE, right-left (need override)
    0x06f2: 0xb2,	#  EXTENDED ARABIC-INDIC DIGIT TWO, right-left (need override)
    0x06f3: 0xb3,	#  EXTENDED ARABIC-INDIC DIGIT THREE, right-left (need override)
    0x06f4: 0xb4,	#  EXTENDED ARABIC-INDIC DIGIT FOUR, right-left (need override)
    0x06f5: 0xb5,	#  EXTENDED ARABIC-INDIC DIGIT FIVE, right-left (need override)
    0x06f6: 0xb6,	#  EXTENDED ARABIC-INDIC DIGIT SIX, right-left (need override)
    0x06f7: 0xb7,	#  EXTENDED ARABIC-INDIC DIGIT SEVEN, right-left (need override)
    0x06f8: 0xb8,	#  EXTENDED ARABIC-INDIC DIGIT EIGHT, right-left (need override)
    0x06f9: 0xb9,	#  EXTENDED ARABIC-INDIC DIGIT NINE, right-left (need override)
    0x2026: 0x93,	#  HORIZONTAL ELLIPSIS, right-left
    0x274a: 0xc0,	#  EIGHT TEARDROP-SPOKED PROPELLER ASTERISK, right-left
}