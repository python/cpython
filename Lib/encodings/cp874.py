""" Python Character Mapping Codec cp874 generated from 'MAPPINGS/VENDORS/MICSFT/WINDOWS/CP874.TXT' with gencodec.py.

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
        name='cp874',
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
    u'\x04'     #  0x04 -> END OF TRANSMISSION
    u'\x05'     #  0x05 -> ENQUIRY
    u'\x06'     #  0x06 -> ACKNOWLEDGE
    u'\x07'     #  0x07 -> BELL
    u'\x08'     #  0x08 -> BACKSPACE
    u'\t'       #  0x09 -> HORIZONTAL TABULATION
    u'\n'       #  0x0A -> LINE FEED
    u'\x0b'     #  0x0B -> VERTICAL TABULATION
    u'\x0c'     #  0x0C -> FORM FEED
    u'\r'       #  0x0D -> CARRIAGE RETURN
    u'\x0e'     #  0x0E -> SHIFT OUT
    u'\x0f'     #  0x0F -> SHIFT IN
    u'\x10'     #  0x10 -> DATA LINK ESCAPE
    u'\x11'     #  0x11 -> DEVICE CONTROL ONE
    u'\x12'     #  0x12 -> DEVICE CONTROL TWO
    u'\x13'     #  0x13 -> DEVICE CONTROL THREE
    u'\x14'     #  0x14 -> DEVICE CONTROL FOUR
    u'\x15'     #  0x15 -> NEGATIVE ACKNOWLEDGE
    u'\x16'     #  0x16 -> SYNCHRONOUS IDLE
    u'\x17'     #  0x17 -> END OF TRANSMISSION BLOCK
    u'\x18'     #  0x18 -> CANCEL
    u'\x19'     #  0x19 -> END OF MEDIUM
    u'\x1a'     #  0x1A -> SUBSTITUTE
    u'\x1b'     #  0x1B -> ESCAPE
    u'\x1c'     #  0x1C -> FILE SEPARATOR
    u'\x1d'     #  0x1D -> GROUP SEPARATOR
    u'\x1e'     #  0x1E -> RECORD SEPARATOR
    u'\x1f'     #  0x1F -> UNIT SEPARATOR
    u' '        #  0x20 -> SPACE
    u'!'        #  0x21 -> EXCLAMATION MARK
    u'"'        #  0x22 -> QUOTATION MARK
    u'#'        #  0x23 -> NUMBER SIGN
    u'$'        #  0x24 -> DOLLAR SIGN
    u'%'        #  0x25 -> PERCENT SIGN
    u'&'        #  0x26 -> AMPERSAND
    u"'"        #  0x27 -> APOSTROPHE
    u'('        #  0x28 -> LEFT PARENTHESIS
    u')'        #  0x29 -> RIGHT PARENTHESIS
    u'*'        #  0x2A -> ASTERISK
    u'+'        #  0x2B -> PLUS SIGN
    u','        #  0x2C -> COMMA
    u'-'        #  0x2D -> HYPHEN-MINUS
    u'.'        #  0x2E -> FULL STOP
    u'/'        #  0x2F -> SOLIDUS
    u'0'        #  0x30 -> DIGIT ZERO
    u'1'        #  0x31 -> DIGIT ONE
    u'2'        #  0x32 -> DIGIT TWO
    u'3'        #  0x33 -> DIGIT THREE
    u'4'        #  0x34 -> DIGIT FOUR
    u'5'        #  0x35 -> DIGIT FIVE
    u'6'        #  0x36 -> DIGIT SIX
    u'7'        #  0x37 -> DIGIT SEVEN
    u'8'        #  0x38 -> DIGIT EIGHT
    u'9'        #  0x39 -> DIGIT NINE
    u':'        #  0x3A -> COLON
    u';'        #  0x3B -> SEMICOLON
    u'<'        #  0x3C -> LESS-THAN SIGN
    u'='        #  0x3D -> EQUALS SIGN
    u'>'        #  0x3E -> GREATER-THAN SIGN
    u'?'        #  0x3F -> QUESTION MARK
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
    u'['        #  0x5B -> LEFT SQUARE BRACKET
    u'\\'       #  0x5C -> REVERSE SOLIDUS
    u']'        #  0x5D -> RIGHT SQUARE BRACKET
    u'^'        #  0x5E -> CIRCUMFLEX ACCENT
    u'_'        #  0x5F -> LOW LINE
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
    u'{'        #  0x7B -> LEFT CURLY BRACKET
    u'|'        #  0x7C -> VERTICAL LINE
    u'}'        #  0x7D -> RIGHT CURLY BRACKET
    u'~'        #  0x7E -> TILDE
    u'\x7f'     #  0x7F -> DELETE
    u'\u20ac'   #  0x80 -> EURO SIGN
    u'\ufffe'   #  0x81 -> UNDEFINED
    u'\ufffe'   #  0x82 -> UNDEFINED
    u'\ufffe'   #  0x83 -> UNDEFINED
    u'\ufffe'   #  0x84 -> UNDEFINED
    u'\u2026'   #  0x85 -> HORIZONTAL ELLIPSIS
    u'\ufffe'   #  0x86 -> UNDEFINED
    u'\ufffe'   #  0x87 -> UNDEFINED
    u'\ufffe'   #  0x88 -> UNDEFINED
    u'\ufffe'   #  0x89 -> UNDEFINED
    u'\ufffe'   #  0x8A -> UNDEFINED
    u'\ufffe'   #  0x8B -> UNDEFINED
    u'\ufffe'   #  0x8C -> UNDEFINED
    u'\ufffe'   #  0x8D -> UNDEFINED
    u'\ufffe'   #  0x8E -> UNDEFINED
    u'\ufffe'   #  0x8F -> UNDEFINED
    u'\ufffe'   #  0x90 -> UNDEFINED
    u'\u2018'   #  0x91 -> LEFT SINGLE QUOTATION MARK
    u'\u2019'   #  0x92 -> RIGHT SINGLE QUOTATION MARK
    u'\u201c'   #  0x93 -> LEFT DOUBLE QUOTATION MARK
    u'\u201d'   #  0x94 -> RIGHT DOUBLE QUOTATION MARK
    u'\u2022'   #  0x95 -> BULLET
    u'\u2013'   #  0x96 -> EN DASH
    u'\u2014'   #  0x97 -> EM DASH
    u'\ufffe'   #  0x98 -> UNDEFINED
    u'\ufffe'   #  0x99 -> UNDEFINED
    u'\ufffe'   #  0x9A -> UNDEFINED
    u'\ufffe'   #  0x9B -> UNDEFINED
    u'\ufffe'   #  0x9C -> UNDEFINED
    u'\ufffe'   #  0x9D -> UNDEFINED
    u'\ufffe'   #  0x9E -> UNDEFINED
    u'\ufffe'   #  0x9F -> UNDEFINED
    u'\xa0'     #  0xA0 -> NO-BREAK SPACE
    u'\u0e01'   #  0xA1 -> THAI CHARACTER KO KAI
    u'\u0e02'   #  0xA2 -> THAI CHARACTER KHO KHAI
    u'\u0e03'   #  0xA3 -> THAI CHARACTER KHO KHUAT
    u'\u0e04'   #  0xA4 -> THAI CHARACTER KHO KHWAI
    u'\u0e05'   #  0xA5 -> THAI CHARACTER KHO KHON
    u'\u0e06'   #  0xA6 -> THAI CHARACTER KHO RAKHANG
    u'\u0e07'   #  0xA7 -> THAI CHARACTER NGO NGU
    u'\u0e08'   #  0xA8 -> THAI CHARACTER CHO CHAN
    u'\u0e09'   #  0xA9 -> THAI CHARACTER CHO CHING
    u'\u0e0a'   #  0xAA -> THAI CHARACTER CHO CHANG
    u'\u0e0b'   #  0xAB -> THAI CHARACTER SO SO
    u'\u0e0c'   #  0xAC -> THAI CHARACTER CHO CHOE
    u'\u0e0d'   #  0xAD -> THAI CHARACTER YO YING
    u'\u0e0e'   #  0xAE -> THAI CHARACTER DO CHADA
    u'\u0e0f'   #  0xAF -> THAI CHARACTER TO PATAK
    u'\u0e10'   #  0xB0 -> THAI CHARACTER THO THAN
    u'\u0e11'   #  0xB1 -> THAI CHARACTER THO NANGMONTHO
    u'\u0e12'   #  0xB2 -> THAI CHARACTER THO PHUTHAO
    u'\u0e13'   #  0xB3 -> THAI CHARACTER NO NEN
    u'\u0e14'   #  0xB4 -> THAI CHARACTER DO DEK
    u'\u0e15'   #  0xB5 -> THAI CHARACTER TO TAO
    u'\u0e16'   #  0xB6 -> THAI CHARACTER THO THUNG
    u'\u0e17'   #  0xB7 -> THAI CHARACTER THO THAHAN
    u'\u0e18'   #  0xB8 -> THAI CHARACTER THO THONG
    u'\u0e19'   #  0xB9 -> THAI CHARACTER NO NU
    u'\u0e1a'   #  0xBA -> THAI CHARACTER BO BAIMAI
    u'\u0e1b'   #  0xBB -> THAI CHARACTER PO PLA
    u'\u0e1c'   #  0xBC -> THAI CHARACTER PHO PHUNG
    u'\u0e1d'   #  0xBD -> THAI CHARACTER FO FA
    u'\u0e1e'   #  0xBE -> THAI CHARACTER PHO PHAN
    u'\u0e1f'   #  0xBF -> THAI CHARACTER FO FAN
    u'\u0e20'   #  0xC0 -> THAI CHARACTER PHO SAMPHAO
    u'\u0e21'   #  0xC1 -> THAI CHARACTER MO MA
    u'\u0e22'   #  0xC2 -> THAI CHARACTER YO YAK
    u'\u0e23'   #  0xC3 -> THAI CHARACTER RO RUA
    u'\u0e24'   #  0xC4 -> THAI CHARACTER RU
    u'\u0e25'   #  0xC5 -> THAI CHARACTER LO LING
    u'\u0e26'   #  0xC6 -> THAI CHARACTER LU
    u'\u0e27'   #  0xC7 -> THAI CHARACTER WO WAEN
    u'\u0e28'   #  0xC8 -> THAI CHARACTER SO SALA
    u'\u0e29'   #  0xC9 -> THAI CHARACTER SO RUSI
    u'\u0e2a'   #  0xCA -> THAI CHARACTER SO SUA
    u'\u0e2b'   #  0xCB -> THAI CHARACTER HO HIP
    u'\u0e2c'   #  0xCC -> THAI CHARACTER LO CHULA
    u'\u0e2d'   #  0xCD -> THAI CHARACTER O ANG
    u'\u0e2e'   #  0xCE -> THAI CHARACTER HO NOKHUK
    u'\u0e2f'   #  0xCF -> THAI CHARACTER PAIYANNOI
    u'\u0e30'   #  0xD0 -> THAI CHARACTER SARA A
    u'\u0e31'   #  0xD1 -> THAI CHARACTER MAI HAN-AKAT
    u'\u0e32'   #  0xD2 -> THAI CHARACTER SARA AA
    u'\u0e33'   #  0xD3 -> THAI CHARACTER SARA AM
    u'\u0e34'   #  0xD4 -> THAI CHARACTER SARA I
    u'\u0e35'   #  0xD5 -> THAI CHARACTER SARA II
    u'\u0e36'   #  0xD6 -> THAI CHARACTER SARA UE
    u'\u0e37'   #  0xD7 -> THAI CHARACTER SARA UEE
    u'\u0e38'   #  0xD8 -> THAI CHARACTER SARA U
    u'\u0e39'   #  0xD9 -> THAI CHARACTER SARA UU
    u'\u0e3a'   #  0xDA -> THAI CHARACTER PHINTHU
    u'\ufffe'   #  0xDB -> UNDEFINED
    u'\ufffe'   #  0xDC -> UNDEFINED
    u'\ufffe'   #  0xDD -> UNDEFINED
    u'\ufffe'   #  0xDE -> UNDEFINED
    u'\u0e3f'   #  0xDF -> THAI CURRENCY SYMBOL BAHT
    u'\u0e40'   #  0xE0 -> THAI CHARACTER SARA E
    u'\u0e41'   #  0xE1 -> THAI CHARACTER SARA AE
    u'\u0e42'   #  0xE2 -> THAI CHARACTER SARA O
    u'\u0e43'   #  0xE3 -> THAI CHARACTER SARA AI MAIMUAN
    u'\u0e44'   #  0xE4 -> THAI CHARACTER SARA AI MAIMALAI
    u'\u0e45'   #  0xE5 -> THAI CHARACTER LAKKHANGYAO
    u'\u0e46'   #  0xE6 -> THAI CHARACTER MAIYAMOK
    u'\u0e47'   #  0xE7 -> THAI CHARACTER MAITAIKHU
    u'\u0e48'   #  0xE8 -> THAI CHARACTER MAI EK
    u'\u0e49'   #  0xE9 -> THAI CHARACTER MAI THO
    u'\u0e4a'   #  0xEA -> THAI CHARACTER MAI TRI
    u'\u0e4b'   #  0xEB -> THAI CHARACTER MAI CHATTAWA
    u'\u0e4c'   #  0xEC -> THAI CHARACTER THANTHAKHAT
    u'\u0e4d'   #  0xED -> THAI CHARACTER NIKHAHIT
    u'\u0e4e'   #  0xEE -> THAI CHARACTER YAMAKKAN
    u'\u0e4f'   #  0xEF -> THAI CHARACTER FONGMAN
    u'\u0e50'   #  0xF0 -> THAI DIGIT ZERO
    u'\u0e51'   #  0xF1 -> THAI DIGIT ONE
    u'\u0e52'   #  0xF2 -> THAI DIGIT TWO
    u'\u0e53'   #  0xF3 -> THAI DIGIT THREE
    u'\u0e54'   #  0xF4 -> THAI DIGIT FOUR
    u'\u0e55'   #  0xF5 -> THAI DIGIT FIVE
    u'\u0e56'   #  0xF6 -> THAI DIGIT SIX
    u'\u0e57'   #  0xF7 -> THAI DIGIT SEVEN
    u'\u0e58'   #  0xF8 -> THAI DIGIT EIGHT
    u'\u0e59'   #  0xF9 -> THAI DIGIT NINE
    u'\u0e5a'   #  0xFA -> THAI CHARACTER ANGKHANKHU
    u'\u0e5b'   #  0xFB -> THAI CHARACTER KHOMUT
    u'\ufffe'   #  0xFC -> UNDEFINED
    u'\ufffe'   #  0xFD -> UNDEFINED
    u'\ufffe'   #  0xFE -> UNDEFINED
    u'\ufffe'   #  0xFF -> UNDEFINED
)

### Encoding Map

encoding_map = {
    0x0000: 0x00,       #  NULL
    0x0001: 0x01,       #  START OF HEADING
    0x0002: 0x02,       #  START OF TEXT
    0x0003: 0x03,       #  END OF TEXT
    0x0004: 0x04,       #  END OF TRANSMISSION
    0x0005: 0x05,       #  ENQUIRY
    0x0006: 0x06,       #  ACKNOWLEDGE
    0x0007: 0x07,       #  BELL
    0x0008: 0x08,       #  BACKSPACE
    0x0009: 0x09,       #  HORIZONTAL TABULATION
    0x000A: 0x0A,       #  LINE FEED
    0x000B: 0x0B,       #  VERTICAL TABULATION
    0x000C: 0x0C,       #  FORM FEED
    0x000D: 0x0D,       #  CARRIAGE RETURN
    0x000E: 0x0E,       #  SHIFT OUT
    0x000F: 0x0F,       #  SHIFT IN
    0x0010: 0x10,       #  DATA LINK ESCAPE
    0x0011: 0x11,       #  DEVICE CONTROL ONE
    0x0012: 0x12,       #  DEVICE CONTROL TWO
    0x0013: 0x13,       #  DEVICE CONTROL THREE
    0x0014: 0x14,       #  DEVICE CONTROL FOUR
    0x0015: 0x15,       #  NEGATIVE ACKNOWLEDGE
    0x0016: 0x16,       #  SYNCHRONOUS IDLE
    0x0017: 0x17,       #  END OF TRANSMISSION BLOCK
    0x0018: 0x18,       #  CANCEL
    0x0019: 0x19,       #  END OF MEDIUM
    0x001A: 0x1A,       #  SUBSTITUTE
    0x001B: 0x1B,       #  ESCAPE
    0x001C: 0x1C,       #  FILE SEPARATOR
    0x001D: 0x1D,       #  GROUP SEPARATOR
    0x001E: 0x1E,       #  RECORD SEPARATOR
    0x001F: 0x1F,       #  UNIT SEPARATOR
    0x0020: 0x20,       #  SPACE
    0x0021: 0x21,       #  EXCLAMATION MARK
    0x0022: 0x22,       #  QUOTATION MARK
    0x0023: 0x23,       #  NUMBER SIGN
    0x0024: 0x24,       #  DOLLAR SIGN
    0x0025: 0x25,       #  PERCENT SIGN
    0x0026: 0x26,       #  AMPERSAND
    0x0027: 0x27,       #  APOSTROPHE
    0x0028: 0x28,       #  LEFT PARENTHESIS
    0x0029: 0x29,       #  RIGHT PARENTHESIS
    0x002A: 0x2A,       #  ASTERISK
    0x002B: 0x2B,       #  PLUS SIGN
    0x002C: 0x2C,       #  COMMA
    0x002D: 0x2D,       #  HYPHEN-MINUS
    0x002E: 0x2E,       #  FULL STOP
    0x002F: 0x2F,       #  SOLIDUS
    0x0030: 0x30,       #  DIGIT ZERO
    0x0031: 0x31,       #  DIGIT ONE
    0x0032: 0x32,       #  DIGIT TWO
    0x0033: 0x33,       #  DIGIT THREE
    0x0034: 0x34,       #  DIGIT FOUR
    0x0035: 0x35,       #  DIGIT FIVE
    0x0036: 0x36,       #  DIGIT SIX
    0x0037: 0x37,       #  DIGIT SEVEN
    0x0038: 0x38,       #  DIGIT EIGHT
    0x0039: 0x39,       #  DIGIT NINE
    0x003A: 0x3A,       #  COLON
    0x003B: 0x3B,       #  SEMICOLON
    0x003C: 0x3C,       #  LESS-THAN SIGN
    0x003D: 0x3D,       #  EQUALS SIGN
    0x003E: 0x3E,       #  GREATER-THAN SIGN
    0x003F: 0x3F,       #  QUESTION MARK
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
    0x005B: 0x5B,       #  LEFT SQUARE BRACKET
    0x005C: 0x5C,       #  REVERSE SOLIDUS
    0x005D: 0x5D,       #  RIGHT SQUARE BRACKET
    0x005E: 0x5E,       #  CIRCUMFLEX ACCENT
    0x005F: 0x5F,       #  LOW LINE
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
    0x007B: 0x7B,       #  LEFT CURLY BRACKET
    0x007C: 0x7C,       #  VERTICAL LINE
    0x007D: 0x7D,       #  RIGHT CURLY BRACKET
    0x007E: 0x7E,       #  TILDE
    0x007F: 0x7F,       #  DELETE
    0x00A0: 0xA0,       #  NO-BREAK SPACE
    0x0E01: 0xA1,       #  THAI CHARACTER KO KAI
    0x0E02: 0xA2,       #  THAI CHARACTER KHO KHAI
    0x0E03: 0xA3,       #  THAI CHARACTER KHO KHUAT
    0x0E04: 0xA4,       #  THAI CHARACTER KHO KHWAI
    0x0E05: 0xA5,       #  THAI CHARACTER KHO KHON
    0x0E06: 0xA6,       #  THAI CHARACTER KHO RAKHANG
    0x0E07: 0xA7,       #  THAI CHARACTER NGO NGU
    0x0E08: 0xA8,       #  THAI CHARACTER CHO CHAN
    0x0E09: 0xA9,       #  THAI CHARACTER CHO CHING
    0x0E0A: 0xAA,       #  THAI CHARACTER CHO CHANG
    0x0E0B: 0xAB,       #  THAI CHARACTER SO SO
    0x0E0C: 0xAC,       #  THAI CHARACTER CHO CHOE
    0x0E0D: 0xAD,       #  THAI CHARACTER YO YING
    0x0E0E: 0xAE,       #  THAI CHARACTER DO CHADA
    0x0E0F: 0xAF,       #  THAI CHARACTER TO PATAK
    0x0E10: 0xB0,       #  THAI CHARACTER THO THAN
    0x0E11: 0xB1,       #  THAI CHARACTER THO NANGMONTHO
    0x0E12: 0xB2,       #  THAI CHARACTER THO PHUTHAO
    0x0E13: 0xB3,       #  THAI CHARACTER NO NEN
    0x0E14: 0xB4,       #  THAI CHARACTER DO DEK
    0x0E15: 0xB5,       #  THAI CHARACTER TO TAO
    0x0E16: 0xB6,       #  THAI CHARACTER THO THUNG
    0x0E17: 0xB7,       #  THAI CHARACTER THO THAHAN
    0x0E18: 0xB8,       #  THAI CHARACTER THO THONG
    0x0E19: 0xB9,       #  THAI CHARACTER NO NU
    0x0E1A: 0xBA,       #  THAI CHARACTER BO BAIMAI
    0x0E1B: 0xBB,       #  THAI CHARACTER PO PLA
    0x0E1C: 0xBC,       #  THAI CHARACTER PHO PHUNG
    0x0E1D: 0xBD,       #  THAI CHARACTER FO FA
    0x0E1E: 0xBE,       #  THAI CHARACTER PHO PHAN
    0x0E1F: 0xBF,       #  THAI CHARACTER FO FAN
    0x0E20: 0xC0,       #  THAI CHARACTER PHO SAMPHAO
    0x0E21: 0xC1,       #  THAI CHARACTER MO MA
    0x0E22: 0xC2,       #  THAI CHARACTER YO YAK
    0x0E23: 0xC3,       #  THAI CHARACTER RO RUA
    0x0E24: 0xC4,       #  THAI CHARACTER RU
    0x0E25: 0xC5,       #  THAI CHARACTER LO LING
    0x0E26: 0xC6,       #  THAI CHARACTER LU
    0x0E27: 0xC7,       #  THAI CHARACTER WO WAEN
    0x0E28: 0xC8,       #  THAI CHARACTER SO SALA
    0x0E29: 0xC9,       #  THAI CHARACTER SO RUSI
    0x0E2A: 0xCA,       #  THAI CHARACTER SO SUA
    0x0E2B: 0xCB,       #  THAI CHARACTER HO HIP
    0x0E2C: 0xCC,       #  THAI CHARACTER LO CHULA
    0x0E2D: 0xCD,       #  THAI CHARACTER O ANG
    0x0E2E: 0xCE,       #  THAI CHARACTER HO NOKHUK
    0x0E2F: 0xCF,       #  THAI CHARACTER PAIYANNOI
    0x0E30: 0xD0,       #  THAI CHARACTER SARA A
    0x0E31: 0xD1,       #  THAI CHARACTER MAI HAN-AKAT
    0x0E32: 0xD2,       #  THAI CHARACTER SARA AA
    0x0E33: 0xD3,       #  THAI CHARACTER SARA AM
    0x0E34: 0xD4,       #  THAI CHARACTER SARA I
    0x0E35: 0xD5,       #  THAI CHARACTER SARA II
    0x0E36: 0xD6,       #  THAI CHARACTER SARA UE
    0x0E37: 0xD7,       #  THAI CHARACTER SARA UEE
    0x0E38: 0xD8,       #  THAI CHARACTER SARA U
    0x0E39: 0xD9,       #  THAI CHARACTER SARA UU
    0x0E3A: 0xDA,       #  THAI CHARACTER PHINTHU
    0x0E3F: 0xDF,       #  THAI CURRENCY SYMBOL BAHT
    0x0E40: 0xE0,       #  THAI CHARACTER SARA E
    0x0E41: 0xE1,       #  THAI CHARACTER SARA AE
    0x0E42: 0xE2,       #  THAI CHARACTER SARA O
    0x0E43: 0xE3,       #  THAI CHARACTER SARA AI MAIMUAN
    0x0E44: 0xE4,       #  THAI CHARACTER SARA AI MAIMALAI
    0x0E45: 0xE5,       #  THAI CHARACTER LAKKHANGYAO
    0x0E46: 0xE6,       #  THAI CHARACTER MAIYAMOK
    0x0E47: 0xE7,       #  THAI CHARACTER MAITAIKHU
    0x0E48: 0xE8,       #  THAI CHARACTER MAI EK
    0x0E49: 0xE9,       #  THAI CHARACTER MAI THO
    0x0E4A: 0xEA,       #  THAI CHARACTER MAI TRI
    0x0E4B: 0xEB,       #  THAI CHARACTER MAI CHATTAWA
    0x0E4C: 0xEC,       #  THAI CHARACTER THANTHAKHAT
    0x0E4D: 0xED,       #  THAI CHARACTER NIKHAHIT
    0x0E4E: 0xEE,       #  THAI CHARACTER YAMAKKAN
    0x0E4F: 0xEF,       #  THAI CHARACTER FONGMAN
    0x0E50: 0xF0,       #  THAI DIGIT ZERO
    0x0E51: 0xF1,       #  THAI DIGIT ONE
    0x0E52: 0xF2,       #  THAI DIGIT TWO
    0x0E53: 0xF3,       #  THAI DIGIT THREE
    0x0E54: 0xF4,       #  THAI DIGIT FOUR
    0x0E55: 0xF5,       #  THAI DIGIT FIVE
    0x0E56: 0xF6,       #  THAI DIGIT SIX
    0x0E57: 0xF7,       #  THAI DIGIT SEVEN
    0x0E58: 0xF8,       #  THAI DIGIT EIGHT
    0x0E59: 0xF9,       #  THAI DIGIT NINE
    0x0E5A: 0xFA,       #  THAI CHARACTER ANGKHANKHU
    0x0E5B: 0xFB,       #  THAI CHARACTER KHOMUT
    0x2013: 0x96,       #  EN DASH
    0x2014: 0x97,       #  EM DASH
    0x2018: 0x91,       #  LEFT SINGLE QUOTATION MARK
    0x2019: 0x92,       #  RIGHT SINGLE QUOTATION MARK
    0x201C: 0x93,       #  LEFT DOUBLE QUOTATION MARK
    0x201D: 0x94,       #  RIGHT DOUBLE QUOTATION MARK
    0x2022: 0x95,       #  BULLET
    0x2026: 0x85,       #  HORIZONTAL ELLIPSIS
    0x20AC: 0x80,       #  EURO SIGN
}
