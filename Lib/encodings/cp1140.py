""" Python Character Mapping Codec cp1140 generated from 'python-mappings/CP1140.TXT' with gencodec.py.

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
        name='cp1140',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )


### Decoding Table

decoding_table = (
    u'\x00'     #  0x00 -> NULL
    u'\x01'     #  0x01 -> START OF HEADING
    u'\x02'     #  0x02 -> START OF TEXT
    u'\x03'     #  0x03 -> END OF TEXT
    u'\x9c'     #  0x04 -> CONTROL
    u'\t'       #  0x05 -> HORIZONTAL TABULATION
    u'\x86'     #  0x06 -> CONTROL
    u'\x7f'     #  0x07 -> DELETE
    u'\x97'     #  0x08 -> CONTROL
    u'\x8d'     #  0x09 -> CONTROL
    u'\x8e'     #  0x0A -> CONTROL
    u'\x0b'     #  0x0B -> VERTICAL TABULATION
    u'\x0c'     #  0x0C -> FORM FEED
    u'\r'       #  0x0D -> CARRIAGE RETURN
    u'\x0e'     #  0x0E -> SHIFT OUT
    u'\x0f'     #  0x0F -> SHIFT IN
    u'\x10'     #  0x10 -> DATA LINK ESCAPE
    u'\x11'     #  0x11 -> DEVICE CONTROL ONE
    u'\x12'     #  0x12 -> DEVICE CONTROL TWO
    u'\x13'     #  0x13 -> DEVICE CONTROL THREE
    u'\x9d'     #  0x14 -> CONTROL
    u'\x85'     #  0x15 -> CONTROL
    u'\x08'     #  0x16 -> BACKSPACE
    u'\x87'     #  0x17 -> CONTROL
    u'\x18'     #  0x18 -> CANCEL
    u'\x19'     #  0x19 -> END OF MEDIUM
    u'\x92'     #  0x1A -> CONTROL
    u'\x8f'     #  0x1B -> CONTROL
    u'\x1c'     #  0x1C -> FILE SEPARATOR
    u'\x1d'     #  0x1D -> GROUP SEPARATOR
    u'\x1e'     #  0x1E -> RECORD SEPARATOR
    u'\x1f'     #  0x1F -> UNIT SEPARATOR
    u'\x80'     #  0x20 -> CONTROL
    u'\x81'     #  0x21 -> CONTROL
    u'\x82'     #  0x22 -> CONTROL
    u'\x83'     #  0x23 -> CONTROL
    u'\x84'     #  0x24 -> CONTROL
    u'\n'       #  0x25 -> LINE FEED
    u'\x17'     #  0x26 -> END OF TRANSMISSION BLOCK
    u'\x1b'     #  0x27 -> ESCAPE
    u'\x88'     #  0x28 -> CONTROL
    u'\x89'     #  0x29 -> CONTROL
    u'\x8a'     #  0x2A -> CONTROL
    u'\x8b'     #  0x2B -> CONTROL
    u'\x8c'     #  0x2C -> CONTROL
    u'\x05'     #  0x2D -> ENQUIRY
    u'\x06'     #  0x2E -> ACKNOWLEDGE
    u'\x07'     #  0x2F -> BELL
    u'\x90'     #  0x30 -> CONTROL
    u'\x91'     #  0x31 -> CONTROL
    u'\x16'     #  0x32 -> SYNCHRONOUS IDLE
    u'\x93'     #  0x33 -> CONTROL
    u'\x94'     #  0x34 -> CONTROL
    u'\x95'     #  0x35 -> CONTROL
    u'\x96'     #  0x36 -> CONTROL
    u'\x04'     #  0x37 -> END OF TRANSMISSION
    u'\x98'     #  0x38 -> CONTROL
    u'\x99'     #  0x39 -> CONTROL
    u'\x9a'     #  0x3A -> CONTROL
    u'\x9b'     #  0x3B -> CONTROL
    u'\x14'     #  0x3C -> DEVICE CONTROL FOUR
    u'\x15'     #  0x3D -> NEGATIVE ACKNOWLEDGE
    u'\x9e'     #  0x3E -> CONTROL
    u'\x1a'     #  0x3F -> SUBSTITUTE
    u' '        #  0x40 -> SPACE
    u'\xa0'     #  0x41 -> NO-BREAK SPACE
    u'\xe2'     #  0x42 -> LATIN SMALL LETTER A WITH CIRCUMFLEX
    u'\xe4'     #  0x43 -> LATIN SMALL LETTER A WITH DIAERESIS
    u'\xe0'     #  0x44 -> LATIN SMALL LETTER A WITH GRAVE
    u'\xe1'     #  0x45 -> LATIN SMALL LETTER A WITH ACUTE
    u'\xe3'     #  0x46 -> LATIN SMALL LETTER A WITH TILDE
    u'\xe5'     #  0x47 -> LATIN SMALL LETTER A WITH RING ABOVE
    u'\xe7'     #  0x48 -> LATIN SMALL LETTER C WITH CEDILLA
    u'\xf1'     #  0x49 -> LATIN SMALL LETTER N WITH TILDE
    u'\xa2'     #  0x4A -> CENT SIGN
    u'.'        #  0x4B -> FULL STOP
    u'<'        #  0x4C -> LESS-THAN SIGN
    u'('        #  0x4D -> LEFT PARENTHESIS
    u'+'        #  0x4E -> PLUS SIGN
    u'|'        #  0x4F -> VERTICAL LINE
    u'&'        #  0x50 -> AMPERSAND
    u'\xe9'     #  0x51 -> LATIN SMALL LETTER E WITH ACUTE
    u'\xea'     #  0x52 -> LATIN SMALL LETTER E WITH CIRCUMFLEX
    u'\xeb'     #  0x53 -> LATIN SMALL LETTER E WITH DIAERESIS
    u'\xe8'     #  0x54 -> LATIN SMALL LETTER E WITH GRAVE
    u'\xed'     #  0x55 -> LATIN SMALL LETTER I WITH ACUTE
    u'\xee'     #  0x56 -> LATIN SMALL LETTER I WITH CIRCUMFLEX
    u'\xef'     #  0x57 -> LATIN SMALL LETTER I WITH DIAERESIS
    u'\xec'     #  0x58 -> LATIN SMALL LETTER I WITH GRAVE
    u'\xdf'     #  0x59 -> LATIN SMALL LETTER SHARP S (GERMAN)
    u'!'        #  0x5A -> EXCLAMATION MARK
    u'$'        #  0x5B -> DOLLAR SIGN
    u'*'        #  0x5C -> ASTERISK
    u')'        #  0x5D -> RIGHT PARENTHESIS
    u';'        #  0x5E -> SEMICOLON
    u'\xac'     #  0x5F -> NOT SIGN
    u'-'        #  0x60 -> HYPHEN-MINUS
    u'/'        #  0x61 -> SOLIDUS
    u'\xc2'     #  0x62 -> LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    u'\xc4'     #  0x63 -> LATIN CAPITAL LETTER A WITH DIAERESIS
    u'\xc0'     #  0x64 -> LATIN CAPITAL LETTER A WITH GRAVE
    u'\xc1'     #  0x65 -> LATIN CAPITAL LETTER A WITH ACUTE
    u'\xc3'     #  0x66 -> LATIN CAPITAL LETTER A WITH TILDE
    u'\xc5'     #  0x67 -> LATIN CAPITAL LETTER A WITH RING ABOVE
    u'\xc7'     #  0x68 -> LATIN CAPITAL LETTER C WITH CEDILLA
    u'\xd1'     #  0x69 -> LATIN CAPITAL LETTER N WITH TILDE
    u'\xa6'     #  0x6A -> BROKEN BAR
    u','        #  0x6B -> COMMA
    u'%'        #  0x6C -> PERCENT SIGN
    u'_'        #  0x6D -> LOW LINE
    u'>'        #  0x6E -> GREATER-THAN SIGN
    u'?'        #  0x6F -> QUESTION MARK
    u'\xf8'     #  0x70 -> LATIN SMALL LETTER O WITH STROKE
    u'\xc9'     #  0x71 -> LATIN CAPITAL LETTER E WITH ACUTE
    u'\xca'     #  0x72 -> LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    u'\xcb'     #  0x73 -> LATIN CAPITAL LETTER E WITH DIAERESIS
    u'\xc8'     #  0x74 -> LATIN CAPITAL LETTER E WITH GRAVE
    u'\xcd'     #  0x75 -> LATIN CAPITAL LETTER I WITH ACUTE
    u'\xce'     #  0x76 -> LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    u'\xcf'     #  0x77 -> LATIN CAPITAL LETTER I WITH DIAERESIS
    u'\xcc'     #  0x78 -> LATIN CAPITAL LETTER I WITH GRAVE
    u'`'        #  0x79 -> GRAVE ACCENT
    u':'        #  0x7A -> COLON
    u'#'        #  0x7B -> NUMBER SIGN
    u'@'        #  0x7C -> COMMERCIAL AT
    u"'"        #  0x7D -> APOSTROPHE
    u'='        #  0x7E -> EQUALS SIGN
    u'"'        #  0x7F -> QUOTATION MARK
    u'\xd8'     #  0x80 -> LATIN CAPITAL LETTER O WITH STROKE
    u'a'        #  0x81 -> LATIN SMALL LETTER A
    u'b'        #  0x82 -> LATIN SMALL LETTER B
    u'c'        #  0x83 -> LATIN SMALL LETTER C
    u'd'        #  0x84 -> LATIN SMALL LETTER D
    u'e'        #  0x85 -> LATIN SMALL LETTER E
    u'f'        #  0x86 -> LATIN SMALL LETTER F
    u'g'        #  0x87 -> LATIN SMALL LETTER G
    u'h'        #  0x88 -> LATIN SMALL LETTER H
    u'i'        #  0x89 -> LATIN SMALL LETTER I
    u'\xab'     #  0x8A -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xbb'     #  0x8B -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xf0'     #  0x8C -> LATIN SMALL LETTER ETH (ICELANDIC)
    u'\xfd'     #  0x8D -> LATIN SMALL LETTER Y WITH ACUTE
    u'\xfe'     #  0x8E -> LATIN SMALL LETTER THORN (ICELANDIC)
    u'\xb1'     #  0x8F -> PLUS-MINUS SIGN
    u'\xb0'     #  0x90 -> DEGREE SIGN
    u'j'        #  0x91 -> LATIN SMALL LETTER J
    u'k'        #  0x92 -> LATIN SMALL LETTER K
    u'l'        #  0x93 -> LATIN SMALL LETTER L
    u'm'        #  0x94 -> LATIN SMALL LETTER M
    u'n'        #  0x95 -> LATIN SMALL LETTER N
    u'o'        #  0x96 -> LATIN SMALL LETTER O
    u'p'        #  0x97 -> LATIN SMALL LETTER P
    u'q'        #  0x98 -> LATIN SMALL LETTER Q
    u'r'        #  0x99 -> LATIN SMALL LETTER R
    u'\xaa'     #  0x9A -> FEMININE ORDINAL INDICATOR
    u'\xba'     #  0x9B -> MASCULINE ORDINAL INDICATOR
    u'\xe6'     #  0x9C -> LATIN SMALL LIGATURE AE
    u'\xb8'     #  0x9D -> CEDILLA
    u'\xc6'     #  0x9E -> LATIN CAPITAL LIGATURE AE
    u'\u20ac'   #  0x9F -> EURO SIGN
    u'\xb5'     #  0xA0 -> MICRO SIGN
    u'~'        #  0xA1 -> TILDE
    u's'        #  0xA2 -> LATIN SMALL LETTER S
    u't'        #  0xA3 -> LATIN SMALL LETTER T
    u'u'        #  0xA4 -> LATIN SMALL LETTER U
    u'v'        #  0xA5 -> LATIN SMALL LETTER V
    u'w'        #  0xA6 -> LATIN SMALL LETTER W
    u'x'        #  0xA7 -> LATIN SMALL LETTER X
    u'y'        #  0xA8 -> LATIN SMALL LETTER Y
    u'z'        #  0xA9 -> LATIN SMALL LETTER Z
    u'\xa1'     #  0xAA -> INVERTED EXCLAMATION MARK
    u'\xbf'     #  0xAB -> INVERTED QUESTION MARK
    u'\xd0'     #  0xAC -> LATIN CAPITAL LETTER ETH (ICELANDIC)
    u'\xdd'     #  0xAD -> LATIN CAPITAL LETTER Y WITH ACUTE
    u'\xde'     #  0xAE -> LATIN CAPITAL LETTER THORN (ICELANDIC)
    u'\xae'     #  0xAF -> REGISTERED SIGN
    u'^'        #  0xB0 -> CIRCUMFLEX ACCENT
    u'\xa3'     #  0xB1 -> POUND SIGN
    u'\xa5'     #  0xB2 -> YEN SIGN
    u'\xb7'     #  0xB3 -> MIDDLE DOT
    u'\xa9'     #  0xB4 -> COPYRIGHT SIGN
    u'\xa7'     #  0xB5 -> SECTION SIGN
    u'\xb6'     #  0xB6 -> PILCROW SIGN
    u'\xbc'     #  0xB7 -> VULGAR FRACTION ONE QUARTER
    u'\xbd'     #  0xB8 -> VULGAR FRACTION ONE HALF
    u'\xbe'     #  0xB9 -> VULGAR FRACTION THREE QUARTERS
    u'['        #  0xBA -> LEFT SQUARE BRACKET
    u']'        #  0xBB -> RIGHT SQUARE BRACKET
    u'\xaf'     #  0xBC -> MACRON
    u'\xa8'     #  0xBD -> DIAERESIS
    u'\xb4'     #  0xBE -> ACUTE ACCENT
    u'\xd7'     #  0xBF -> MULTIPLICATION SIGN
    u'{'        #  0xC0 -> LEFT CURLY BRACKET
    u'A'        #  0xC1 -> LATIN CAPITAL LETTER A
    u'B'        #  0xC2 -> LATIN CAPITAL LETTER B
    u'C'        #  0xC3 -> LATIN CAPITAL LETTER C
    u'D'        #  0xC4 -> LATIN CAPITAL LETTER D
    u'E'        #  0xC5 -> LATIN CAPITAL LETTER E
    u'F'        #  0xC6 -> LATIN CAPITAL LETTER F
    u'G'        #  0xC7 -> LATIN CAPITAL LETTER G
    u'H'        #  0xC8 -> LATIN CAPITAL LETTER H
    u'I'        #  0xC9 -> LATIN CAPITAL LETTER I
    u'\xad'     #  0xCA -> SOFT HYPHEN
    u'\xf4'     #  0xCB -> LATIN SMALL LETTER O WITH CIRCUMFLEX
    u'\xf6'     #  0xCC -> LATIN SMALL LETTER O WITH DIAERESIS
    u'\xf2'     #  0xCD -> LATIN SMALL LETTER O WITH GRAVE
    u'\xf3'     #  0xCE -> LATIN SMALL LETTER O WITH ACUTE
    u'\xf5'     #  0xCF -> LATIN SMALL LETTER O WITH TILDE
    u'}'        #  0xD0 -> RIGHT CURLY BRACKET
    u'J'        #  0xD1 -> LATIN CAPITAL LETTER J
    u'K'        #  0xD2 -> LATIN CAPITAL LETTER K
    u'L'        #  0xD3 -> LATIN CAPITAL LETTER L
    u'M'        #  0xD4 -> LATIN CAPITAL LETTER M
    u'N'        #  0xD5 -> LATIN CAPITAL LETTER N
    u'O'        #  0xD6 -> LATIN CAPITAL LETTER O
    u'P'        #  0xD7 -> LATIN CAPITAL LETTER P
    u'Q'        #  0xD8 -> LATIN CAPITAL LETTER Q
    u'R'        #  0xD9 -> LATIN CAPITAL LETTER R
    u'\xb9'     #  0xDA -> SUPERSCRIPT ONE
    u'\xfb'     #  0xDB -> LATIN SMALL LETTER U WITH CIRCUMFLEX
    u'\xfc'     #  0xDC -> LATIN SMALL LETTER U WITH DIAERESIS
    u'\xf9'     #  0xDD -> LATIN SMALL LETTER U WITH GRAVE
    u'\xfa'     #  0xDE -> LATIN SMALL LETTER U WITH ACUTE
    u'\xff'     #  0xDF -> LATIN SMALL LETTER Y WITH DIAERESIS
    u'\\'       #  0xE0 -> REVERSE SOLIDUS
    u'\xf7'     #  0xE1 -> DIVISION SIGN
    u'S'        #  0xE2 -> LATIN CAPITAL LETTER S
    u'T'        #  0xE3 -> LATIN CAPITAL LETTER T
    u'U'        #  0xE4 -> LATIN CAPITAL LETTER U
    u'V'        #  0xE5 -> LATIN CAPITAL LETTER V
    u'W'        #  0xE6 -> LATIN CAPITAL LETTER W
    u'X'        #  0xE7 -> LATIN CAPITAL LETTER X
    u'Y'        #  0xE8 -> LATIN CAPITAL LETTER Y
    u'Z'        #  0xE9 -> LATIN CAPITAL LETTER Z
    u'\xb2'     #  0xEA -> SUPERSCRIPT TWO
    u'\xd4'     #  0xEB -> LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    u'\xd6'     #  0xEC -> LATIN CAPITAL LETTER O WITH DIAERESIS
    u'\xd2'     #  0xED -> LATIN CAPITAL LETTER O WITH GRAVE
    u'\xd3'     #  0xEE -> LATIN CAPITAL LETTER O WITH ACUTE
    u'\xd5'     #  0xEF -> LATIN CAPITAL LETTER O WITH TILDE
    u'0'        #  0xF0 -> DIGIT ZERO
    u'1'        #  0xF1 -> DIGIT ONE
    u'2'        #  0xF2 -> DIGIT TWO
    u'3'        #  0xF3 -> DIGIT THREE
    u'4'        #  0xF4 -> DIGIT FOUR
    u'5'        #  0xF5 -> DIGIT FIVE
    u'6'        #  0xF6 -> DIGIT SIX
    u'7'        #  0xF7 -> DIGIT SEVEN
    u'8'        #  0xF8 -> DIGIT EIGHT
    u'9'        #  0xF9 -> DIGIT NINE
    u'\xb3'     #  0xFA -> SUPERSCRIPT THREE
    u'\xdb'     #  0xFB -> LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    u'\xdc'     #  0xFC -> LATIN CAPITAL LETTER U WITH DIAERESIS
    u'\xd9'     #  0xFD -> LATIN CAPITAL LETTER U WITH GRAVE
    u'\xda'     #  0xFE -> LATIN CAPITAL LETTER U WITH ACUTE
    u'\x9f'     #  0xFF -> CONTROL
)

### Encoding Map

encoding_map = {
    0x0000: 0x00,       #  NULL
    0x0001: 0x01,       #  START OF HEADING
    0x0002: 0x02,       #  START OF TEXT
    0x0003: 0x03,       #  END OF TEXT
    0x0004: 0x37,       #  END OF TRANSMISSION
    0x0005: 0x2D,       #  ENQUIRY
    0x0006: 0x2E,       #  ACKNOWLEDGE
    0x0007: 0x2F,       #  BELL
    0x0008: 0x16,       #  BACKSPACE
    0x0009: 0x05,       #  HORIZONTAL TABULATION
    0x000A: 0x25,       #  LINE FEED
    0x000B: 0x0B,       #  VERTICAL TABULATION
    0x000C: 0x0C,       #  FORM FEED
    0x000D: 0x0D,       #  CARRIAGE RETURN
    0x000E: 0x0E,       #  SHIFT OUT
    0x000F: 0x0F,       #  SHIFT IN
    0x0010: 0x10,       #  DATA LINK ESCAPE
    0x0011: 0x11,       #  DEVICE CONTROL ONE
    0x0012: 0x12,       #  DEVICE CONTROL TWO
    0x0013: 0x13,       #  DEVICE CONTROL THREE
    0x0014: 0x3C,       #  DEVICE CONTROL FOUR
    0x0015: 0x3D,       #  NEGATIVE ACKNOWLEDGE
    0x0016: 0x32,       #  SYNCHRONOUS IDLE
    0x0017: 0x26,       #  END OF TRANSMISSION BLOCK
    0x0018: 0x18,       #  CANCEL
    0x0019: 0x19,       #  END OF MEDIUM
    0x001A: 0x3F,       #  SUBSTITUTE
    0x001B: 0x27,       #  ESCAPE
    0x001C: 0x1C,       #  FILE SEPARATOR
    0x001D: 0x1D,       #  GROUP SEPARATOR
    0x001E: 0x1E,       #  RECORD SEPARATOR
    0x001F: 0x1F,       #  UNIT SEPARATOR
    0x0020: 0x40,       #  SPACE
    0x0021: 0x5A,       #  EXCLAMATION MARK
    0x0022: 0x7F,       #  QUOTATION MARK
    0x0023: 0x7B,       #  NUMBER SIGN
    0x0024: 0x5B,       #  DOLLAR SIGN
    0x0025: 0x6C,       #  PERCENT SIGN
    0x0026: 0x50,       #  AMPERSAND
    0x0027: 0x7D,       #  APOSTROPHE
    0x0028: 0x4D,       #  LEFT PARENTHESIS
    0x0029: 0x5D,       #  RIGHT PARENTHESIS
    0x002A: 0x5C,       #  ASTERISK
    0x002B: 0x4E,       #  PLUS SIGN
    0x002C: 0x6B,       #  COMMA
    0x002D: 0x60,       #  HYPHEN-MINUS
    0x002E: 0x4B,       #  FULL STOP
    0x002F: 0x61,       #  SOLIDUS
    0x0030: 0xF0,       #  DIGIT ZERO
    0x0031: 0xF1,       #  DIGIT ONE
    0x0032: 0xF2,       #  DIGIT TWO
    0x0033: 0xF3,       #  DIGIT THREE
    0x0034: 0xF4,       #  DIGIT FOUR
    0x0035: 0xF5,       #  DIGIT FIVE
    0x0036: 0xF6,       #  DIGIT SIX
    0x0037: 0xF7,       #  DIGIT SEVEN
    0x0038: 0xF8,       #  DIGIT EIGHT
    0x0039: 0xF9,       #  DIGIT NINE
    0x003A: 0x7A,       #  COLON
    0x003B: 0x5E,       #  SEMICOLON
    0x003C: 0x4C,       #  LESS-THAN SIGN
    0x003D: 0x7E,       #  EQUALS SIGN
    0x003E: 0x6E,       #  GREATER-THAN SIGN
    0x003F: 0x6F,       #  QUESTION MARK
    0x0040: 0x7C,       #  COMMERCIAL AT
    0x0041: 0xC1,       #  LATIN CAPITAL LETTER A
    0x0042: 0xC2,       #  LATIN CAPITAL LETTER B
    0x0043: 0xC3,       #  LATIN CAPITAL LETTER C
    0x0044: 0xC4,       #  LATIN CAPITAL LETTER D
    0x0045: 0xC5,       #  LATIN CAPITAL LETTER E
    0x0046: 0xC6,       #  LATIN CAPITAL LETTER F
    0x0047: 0xC7,       #  LATIN CAPITAL LETTER G
    0x0048: 0xC8,       #  LATIN CAPITAL LETTER H
    0x0049: 0xC9,       #  LATIN CAPITAL LETTER I
    0x004A: 0xD1,       #  LATIN CAPITAL LETTER J
    0x004B: 0xD2,       #  LATIN CAPITAL LETTER K
    0x004C: 0xD3,       #  LATIN CAPITAL LETTER L
    0x004D: 0xD4,       #  LATIN CAPITAL LETTER M
    0x004E: 0xD5,       #  LATIN CAPITAL LETTER N
    0x004F: 0xD6,       #  LATIN CAPITAL LETTER O
    0x0050: 0xD7,       #  LATIN CAPITAL LETTER P
    0x0051: 0xD8,       #  LATIN CAPITAL LETTER Q
    0x0052: 0xD9,       #  LATIN CAPITAL LETTER R
    0x0053: 0xE2,       #  LATIN CAPITAL LETTER S
    0x0054: 0xE3,       #  LATIN CAPITAL LETTER T
    0x0055: 0xE4,       #  LATIN CAPITAL LETTER U
    0x0056: 0xE5,       #  LATIN CAPITAL LETTER V
    0x0057: 0xE6,       #  LATIN CAPITAL LETTER W
    0x0058: 0xE7,       #  LATIN CAPITAL LETTER X
    0x0059: 0xE8,       #  LATIN CAPITAL LETTER Y
    0x005A: 0xE9,       #  LATIN CAPITAL LETTER Z
    0x005B: 0xBA,       #  LEFT SQUARE BRACKET
    0x005C: 0xE0,       #  REVERSE SOLIDUS
    0x005D: 0xBB,       #  RIGHT SQUARE BRACKET
    0x005E: 0xB0,       #  CIRCUMFLEX ACCENT
    0x005F: 0x6D,       #  LOW LINE
    0x0060: 0x79,       #  GRAVE ACCENT
    0x0061: 0x81,       #  LATIN SMALL LETTER A
    0x0062: 0x82,       #  LATIN SMALL LETTER B
    0x0063: 0x83,       #  LATIN SMALL LETTER C
    0x0064: 0x84,       #  LATIN SMALL LETTER D
    0x0065: 0x85,       #  LATIN SMALL LETTER E
    0x0066: 0x86,       #  LATIN SMALL LETTER F
    0x0067: 0x87,       #  LATIN SMALL LETTER G
    0x0068: 0x88,       #  LATIN SMALL LETTER H
    0x0069: 0x89,       #  LATIN SMALL LETTER I
    0x006A: 0x91,       #  LATIN SMALL LETTER J
    0x006B: 0x92,       #  LATIN SMALL LETTER K
    0x006C: 0x93,       #  LATIN SMALL LETTER L
    0x006D: 0x94,       #  LATIN SMALL LETTER M
    0x006E: 0x95,       #  LATIN SMALL LETTER N
    0x006F: 0x96,       #  LATIN SMALL LETTER O
    0x0070: 0x97,       #  LATIN SMALL LETTER P
    0x0071: 0x98,       #  LATIN SMALL LETTER Q
    0x0072: 0x99,       #  LATIN SMALL LETTER R
    0x0073: 0xA2,       #  LATIN SMALL LETTER S
    0x0074: 0xA3,       #  LATIN SMALL LETTER T
    0x0075: 0xA4,       #  LATIN SMALL LETTER U
    0x0076: 0xA5,       #  LATIN SMALL LETTER V
    0x0077: 0xA6,       #  LATIN SMALL LETTER W
    0x0078: 0xA7,       #  LATIN SMALL LETTER X
    0x0079: 0xA8,       #  LATIN SMALL LETTER Y
    0x007A: 0xA9,       #  LATIN SMALL LETTER Z
    0x007B: 0xC0,       #  LEFT CURLY BRACKET
    0x007C: 0x4F,       #  VERTICAL LINE
    0x007D: 0xD0,       #  RIGHT CURLY BRACKET
    0x007E: 0xA1,       #  TILDE
    0x007F: 0x07,       #  DELETE
    0x0080: 0x20,       #  CONTROL
    0x0081: 0x21,       #  CONTROL
    0x0082: 0x22,       #  CONTROL
    0x0083: 0x23,       #  CONTROL
    0x0084: 0x24,       #  CONTROL
    0x0085: 0x15,       #  CONTROL
    0x0086: 0x06,       #  CONTROL
    0x0087: 0x17,       #  CONTROL
    0x0088: 0x28,       #  CONTROL
    0x0089: 0x29,       #  CONTROL
    0x008A: 0x2A,       #  CONTROL
    0x008B: 0x2B,       #  CONTROL
    0x008C: 0x2C,       #  CONTROL
    0x008D: 0x09,       #  CONTROL
    0x008E: 0x0A,       #  CONTROL
    0x008F: 0x1B,       #  CONTROL
    0x0090: 0x30,       #  CONTROL
    0x0091: 0x31,       #  CONTROL
    0x0092: 0x1A,       #  CONTROL
    0x0093: 0x33,       #  CONTROL
    0x0094: 0x34,       #  CONTROL
    0x0095: 0x35,       #  CONTROL
    0x0096: 0x36,       #  CONTROL
    0x0097: 0x08,       #  CONTROL
    0x0098: 0x38,       #  CONTROL
    0x0099: 0x39,       #  CONTROL
    0x009A: 0x3A,       #  CONTROL
    0x009B: 0x3B,       #  CONTROL
    0x009C: 0x04,       #  CONTROL
    0x009D: 0x14,       #  CONTROL
    0x009E: 0x3E,       #  CONTROL
    0x009F: 0xFF,       #  CONTROL
    0x00A0: 0x41,       #  NO-BREAK SPACE
    0x00A1: 0xAA,       #  INVERTED EXCLAMATION MARK
    0x00A2: 0x4A,       #  CENT SIGN
    0x00A3: 0xB1,       #  POUND SIGN
    0x00A5: 0xB2,       #  YEN SIGN
    0x00A6: 0x6A,       #  BROKEN BAR
    0x00A7: 0xB5,       #  SECTION SIGN
    0x00A8: 0xBD,       #  DIAERESIS
    0x00A9: 0xB4,       #  COPYRIGHT SIGN
    0x00AA: 0x9A,       #  FEMININE ORDINAL INDICATOR
    0x00AB: 0x8A,       #  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC: 0x5F,       #  NOT SIGN
    0x00AD: 0xCA,       #  SOFT HYPHEN
    0x00AE: 0xAF,       #  REGISTERED SIGN
    0x00AF: 0xBC,       #  MACRON
    0x00B0: 0x90,       #  DEGREE SIGN
    0x00B1: 0x8F,       #  PLUS-MINUS SIGN
    0x00B2: 0xEA,       #  SUPERSCRIPT TWO
    0x00B3: 0xFA,       #  SUPERSCRIPT THREE
    0x00B4: 0xBE,       #  ACUTE ACCENT
    0x00B5: 0xA0,       #  MICRO SIGN
    0x00B6: 0xB6,       #  PILCROW SIGN
    0x00B7: 0xB3,       #  MIDDLE DOT
    0x00B8: 0x9D,       #  CEDILLA
    0x00B9: 0xDA,       #  SUPERSCRIPT ONE
    0x00BA: 0x9B,       #  MASCULINE ORDINAL INDICATOR
    0x00BB: 0x8B,       #  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BC: 0xB7,       #  VULGAR FRACTION ONE QUARTER
    0x00BD: 0xB8,       #  VULGAR FRACTION ONE HALF
    0x00BE: 0xB9,       #  VULGAR FRACTION THREE QUARTERS
    0x00BF: 0xAB,       #  INVERTED QUESTION MARK
    0x00C0: 0x64,       #  LATIN CAPITAL LETTER A WITH GRAVE
    0x00C1: 0x65,       #  LATIN CAPITAL LETTER A WITH ACUTE
    0x00C2: 0x62,       #  LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    0x00C3: 0x66,       #  LATIN CAPITAL LETTER A WITH TILDE
    0x00C4: 0x63,       #  LATIN CAPITAL LETTER A WITH DIAERESIS
    0x00C5: 0x67,       #  LATIN CAPITAL LETTER A WITH RING ABOVE
    0x00C6: 0x9E,       #  LATIN CAPITAL LIGATURE AE
    0x00C7: 0x68,       #  LATIN CAPITAL LETTER C WITH CEDILLA
    0x00C8: 0x74,       #  LATIN CAPITAL LETTER E WITH GRAVE
    0x00C9: 0x71,       #  LATIN CAPITAL LETTER E WITH ACUTE
    0x00CA: 0x72,       #  LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    0x00CB: 0x73,       #  LATIN CAPITAL LETTER E WITH DIAERESIS
    0x00CC: 0x78,       #  LATIN CAPITAL LETTER I WITH GRAVE
    0x00CD: 0x75,       #  LATIN CAPITAL LETTER I WITH ACUTE
    0x00CE: 0x76,       #  LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    0x00CF: 0x77,       #  LATIN CAPITAL LETTER I WITH DIAERESIS
    0x00D0: 0xAC,       #  LATIN CAPITAL LETTER ETH (ICELANDIC)
    0x00D1: 0x69,       #  LATIN CAPITAL LETTER N WITH TILDE
    0x00D2: 0xED,       #  LATIN CAPITAL LETTER O WITH GRAVE
    0x00D3: 0xEE,       #  LATIN CAPITAL LETTER O WITH ACUTE
    0x00D4: 0xEB,       #  LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    0x00D5: 0xEF,       #  LATIN CAPITAL LETTER O WITH TILDE
    0x00D6: 0xEC,       #  LATIN CAPITAL LETTER O WITH DIAERESIS
    0x00D7: 0xBF,       #  MULTIPLICATION SIGN
    0x00D8: 0x80,       #  LATIN CAPITAL LETTER O WITH STROKE
    0x00D9: 0xFD,       #  LATIN CAPITAL LETTER U WITH GRAVE
    0x00DA: 0xFE,       #  LATIN CAPITAL LETTER U WITH ACUTE
    0x00DB: 0xFB,       #  LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    0x00DC: 0xFC,       #  LATIN CAPITAL LETTER U WITH DIAERESIS
    0x00DD: 0xAD,       #  LATIN CAPITAL LETTER Y WITH ACUTE
    0x00DE: 0xAE,       #  LATIN CAPITAL LETTER THORN (ICELANDIC)
    0x00DF: 0x59,       #  LATIN SMALL LETTER SHARP S (GERMAN)
    0x00E0: 0x44,       #  LATIN SMALL LETTER A WITH GRAVE
    0x00E1: 0x45,       #  LATIN SMALL LETTER A WITH ACUTE
    0x00E2: 0x42,       #  LATIN SMALL LETTER A WITH CIRCUMFLEX
    0x00E3: 0x46,       #  LATIN SMALL LETTER A WITH TILDE
    0x00E4: 0x43,       #  LATIN SMALL LETTER A WITH DIAERESIS
    0x00E5: 0x47,       #  LATIN SMALL LETTER A WITH RING ABOVE
    0x00E6: 0x9C,       #  LATIN SMALL LIGATURE AE
    0x00E7: 0x48,       #  LATIN SMALL LETTER C WITH CEDILLA
    0x00E8: 0x54,       #  LATIN SMALL LETTER E WITH GRAVE
    0x00E9: 0x51,       #  LATIN SMALL LETTER E WITH ACUTE
    0x00EA: 0x52,       #  LATIN SMALL LETTER E WITH CIRCUMFLEX
    0x00EB: 0x53,       #  LATIN SMALL LETTER E WITH DIAERESIS
    0x00EC: 0x58,       #  LATIN SMALL LETTER I WITH GRAVE
    0x00ED: 0x55,       #  LATIN SMALL LETTER I WITH ACUTE
    0x00EE: 0x56,       #  LATIN SMALL LETTER I WITH CIRCUMFLEX
    0x00EF: 0x57,       #  LATIN SMALL LETTER I WITH DIAERESIS
    0x00F0: 0x8C,       #  LATIN SMALL LETTER ETH (ICELANDIC)
    0x00F1: 0x49,       #  LATIN SMALL LETTER N WITH TILDE
    0x00F2: 0xCD,       #  LATIN SMALL LETTER O WITH GRAVE
    0x00F3: 0xCE,       #  LATIN SMALL LETTER O WITH ACUTE
    0x00F4: 0xCB,       #  LATIN SMALL LETTER O WITH CIRCUMFLEX
    0x00F5: 0xCF,       #  LATIN SMALL LETTER O WITH TILDE
    0x00F6: 0xCC,       #  LATIN SMALL LETTER O WITH DIAERESIS
    0x00F7: 0xE1,       #  DIVISION SIGN
    0x00F8: 0x70,       #  LATIN SMALL LETTER O WITH STROKE
    0x00F9: 0xDD,       #  LATIN SMALL LETTER U WITH GRAVE
    0x00FA: 0xDE,       #  LATIN SMALL LETTER U WITH ACUTE
    0x00FB: 0xDB,       #  LATIN SMALL LETTER U WITH CIRCUMFLEX
    0x00FC: 0xDC,       #  LATIN SMALL LETTER U WITH DIAERESIS
    0x00FD: 0x8D,       #  LATIN SMALL LETTER Y WITH ACUTE
    0x00FE: 0x8E,       #  LATIN SMALL LETTER THORN (ICELANDIC)
    0x00FF: 0xDF,       #  LATIN SMALL LETTER Y WITH DIAERESIS
    0x20AC: 0x9F,       #  EURO SIGN
}
