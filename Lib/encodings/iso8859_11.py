""" Python Character Mapping Codec generated from 'MAPPINGS/ISO8859/8859-11.TXT' with gencodec.py.

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
    u'\x00'	#  0x00 -> NULL
    u'\x01'	#  0x01 -> START OF HEADING
    u'\x02'	#  0x02 -> START OF TEXT
    u'\x03'	#  0x03 -> END OF TEXT
    u'\x04'	#  0x04 -> END OF TRANSMISSION
    u'\x05'	#  0x05 -> ENQUIRY
    u'\x06'	#  0x06 -> ACKNOWLEDGE
    u'\x07'	#  0x07 -> BELL
    u'\x08'	#  0x08 -> BACKSPACE
    u'\t'	#  0x09 -> HORIZONTAL TABULATION
    u'\n'	#  0x0a -> LINE FEED
    u'\x0b'	#  0x0b -> VERTICAL TABULATION
    u'\x0c'	#  0x0c -> FORM FEED
    u'\r'	#  0x0d -> CARRIAGE RETURN
    u'\x0e'	#  0x0e -> SHIFT OUT
    u'\x0f'	#  0x0f -> SHIFT IN
    u'\x10'	#  0x10 -> DATA LINK ESCAPE
    u'\x11'	#  0x11 -> DEVICE CONTROL ONE
    u'\x12'	#  0x12 -> DEVICE CONTROL TWO
    u'\x13'	#  0x13 -> DEVICE CONTROL THREE
    u'\x14'	#  0x14 -> DEVICE CONTROL FOUR
    u'\x15'	#  0x15 -> NEGATIVE ACKNOWLEDGE
    u'\x16'	#  0x16 -> SYNCHRONOUS IDLE
    u'\x17'	#  0x17 -> END OF TRANSMISSION BLOCK
    u'\x18'	#  0x18 -> CANCEL
    u'\x19'	#  0x19 -> END OF MEDIUM
    u'\x1a'	#  0x1a -> SUBSTITUTE
    u'\x1b'	#  0x1b -> ESCAPE
    u'\x1c'	#  0x1c -> FILE SEPARATOR
    u'\x1d'	#  0x1d -> GROUP SEPARATOR
    u'\x1e'	#  0x1e -> RECORD SEPARATOR
    u'\x1f'	#  0x1f -> UNIT SEPARATOR
    u' '	#  0x20 -> SPACE
    u'!'	#  0x21 -> EXCLAMATION MARK
    u'"'	#  0x22 -> QUOTATION MARK
    u'#'	#  0x23 -> NUMBER SIGN
    u'$'	#  0x24 -> DOLLAR SIGN
    u'%'	#  0x25 -> PERCENT SIGN
    u'&'	#  0x26 -> AMPERSAND
    u"'"	#  0x27 -> APOSTROPHE
    u'('	#  0x28 -> LEFT PARENTHESIS
    u')'	#  0x29 -> RIGHT PARENTHESIS
    u'*'	#  0x2a -> ASTERISK
    u'+'	#  0x2b -> PLUS SIGN
    u','	#  0x2c -> COMMA
    u'-'	#  0x2d -> HYPHEN-MINUS
    u'.'	#  0x2e -> FULL STOP
    u'/'	#  0x2f -> SOLIDUS
    u'0'	#  0x30 -> DIGIT ZERO
    u'1'	#  0x31 -> DIGIT ONE
    u'2'	#  0x32 -> DIGIT TWO
    u'3'	#  0x33 -> DIGIT THREE
    u'4'	#  0x34 -> DIGIT FOUR
    u'5'	#  0x35 -> DIGIT FIVE
    u'6'	#  0x36 -> DIGIT SIX
    u'7'	#  0x37 -> DIGIT SEVEN
    u'8'	#  0x38 -> DIGIT EIGHT
    u'9'	#  0x39 -> DIGIT NINE
    u':'	#  0x3a -> COLON
    u';'	#  0x3b -> SEMICOLON
    u'<'	#  0x3c -> LESS-THAN SIGN
    u'='	#  0x3d -> EQUALS SIGN
    u'>'	#  0x3e -> GREATER-THAN SIGN
    u'?'	#  0x3f -> QUESTION MARK
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
    u'['	#  0x5b -> LEFT SQUARE BRACKET
    u'\\'	#  0x5c -> REVERSE SOLIDUS
    u']'	#  0x5d -> RIGHT SQUARE BRACKET
    u'^'	#  0x5e -> CIRCUMFLEX ACCENT
    u'_'	#  0x5f -> LOW LINE
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
    u'{'	#  0x7b -> LEFT CURLY BRACKET
    u'|'	#  0x7c -> VERTICAL LINE
    u'}'	#  0x7d -> RIGHT CURLY BRACKET
    u'~'	#  0x7e -> TILDE
    u'\x7f'	#  0x7f -> DELETE
    u'\x80'	#  0x80 -> <control>
    u'\x81'	#  0x81 -> <control>
    u'\x82'	#  0x82 -> <control>
    u'\x83'	#  0x83 -> <control>
    u'\x84'	#  0x84 -> <control>
    u'\x85'	#  0x85 -> <control>
    u'\x86'	#  0x86 -> <control>
    u'\x87'	#  0x87 -> <control>
    u'\x88'	#  0x88 -> <control>
    u'\x89'	#  0x89 -> <control>
    u'\x8a'	#  0x8a -> <control>
    u'\x8b'	#  0x8b -> <control>
    u'\x8c'	#  0x8c -> <control>
    u'\x8d'	#  0x8d -> <control>
    u'\x8e'	#  0x8e -> <control>
    u'\x8f'	#  0x8f -> <control>
    u'\x90'	#  0x90 -> <control>
    u'\x91'	#  0x91 -> <control>
    u'\x92'	#  0x92 -> <control>
    u'\x93'	#  0x93 -> <control>
    u'\x94'	#  0x94 -> <control>
    u'\x95'	#  0x95 -> <control>
    u'\x96'	#  0x96 -> <control>
    u'\x97'	#  0x97 -> <control>
    u'\x98'	#  0x98 -> <control>
    u'\x99'	#  0x99 -> <control>
    u'\x9a'	#  0x9a -> <control>
    u'\x9b'	#  0x9b -> <control>
    u'\x9c'	#  0x9c -> <control>
    u'\x9d'	#  0x9d -> <control>
    u'\x9e'	#  0x9e -> <control>
    u'\x9f'	#  0x9f -> <control>
    u'\xa0'	#  0xa0 -> NO-BREAK SPACE
    u'\u0e01'	#  0xa1 -> THAI CHARACTER KO KAI
    u'\u0e02'	#  0xa2 -> THAI CHARACTER KHO KHAI
    u'\u0e03'	#  0xa3 -> THAI CHARACTER KHO KHUAT
    u'\u0e04'	#  0xa4 -> THAI CHARACTER KHO KHWAI
    u'\u0e05'	#  0xa5 -> THAI CHARACTER KHO KHON
    u'\u0e06'	#  0xa6 -> THAI CHARACTER KHO RAKHANG
    u'\u0e07'	#  0xa7 -> THAI CHARACTER NGO NGU
    u'\u0e08'	#  0xa8 -> THAI CHARACTER CHO CHAN
    u'\u0e09'	#  0xa9 -> THAI CHARACTER CHO CHING
    u'\u0e0a'	#  0xaa -> THAI CHARACTER CHO CHANG
    u'\u0e0b'	#  0xab -> THAI CHARACTER SO SO
    u'\u0e0c'	#  0xac -> THAI CHARACTER CHO CHOE
    u'\u0e0d'	#  0xad -> THAI CHARACTER YO YING
    u'\u0e0e'	#  0xae -> THAI CHARACTER DO CHADA
    u'\u0e0f'	#  0xaf -> THAI CHARACTER TO PATAK
    u'\u0e10'	#  0xb0 -> THAI CHARACTER THO THAN
    u'\u0e11'	#  0xb1 -> THAI CHARACTER THO NANGMONTHO
    u'\u0e12'	#  0xb2 -> THAI CHARACTER THO PHUTHAO
    u'\u0e13'	#  0xb3 -> THAI CHARACTER NO NEN
    u'\u0e14'	#  0xb4 -> THAI CHARACTER DO DEK
    u'\u0e15'	#  0xb5 -> THAI CHARACTER TO TAO
    u'\u0e16'	#  0xb6 -> THAI CHARACTER THO THUNG
    u'\u0e17'	#  0xb7 -> THAI CHARACTER THO THAHAN
    u'\u0e18'	#  0xb8 -> THAI CHARACTER THO THONG
    u'\u0e19'	#  0xb9 -> THAI CHARACTER NO NU
    u'\u0e1a'	#  0xba -> THAI CHARACTER BO BAIMAI
    u'\u0e1b'	#  0xbb -> THAI CHARACTER PO PLA
    u'\u0e1c'	#  0xbc -> THAI CHARACTER PHO PHUNG
    u'\u0e1d'	#  0xbd -> THAI CHARACTER FO FA
    u'\u0e1e'	#  0xbe -> THAI CHARACTER PHO PHAN
    u'\u0e1f'	#  0xbf -> THAI CHARACTER FO FAN
    u'\u0e20'	#  0xc0 -> THAI CHARACTER PHO SAMPHAO
    u'\u0e21'	#  0xc1 -> THAI CHARACTER MO MA
    u'\u0e22'	#  0xc2 -> THAI CHARACTER YO YAK
    u'\u0e23'	#  0xc3 -> THAI CHARACTER RO RUA
    u'\u0e24'	#  0xc4 -> THAI CHARACTER RU
    u'\u0e25'	#  0xc5 -> THAI CHARACTER LO LING
    u'\u0e26'	#  0xc6 -> THAI CHARACTER LU
    u'\u0e27'	#  0xc7 -> THAI CHARACTER WO WAEN
    u'\u0e28'	#  0xc8 -> THAI CHARACTER SO SALA
    u'\u0e29'	#  0xc9 -> THAI CHARACTER SO RUSI
    u'\u0e2a'	#  0xca -> THAI CHARACTER SO SUA
    u'\u0e2b'	#  0xcb -> THAI CHARACTER HO HIP
    u'\u0e2c'	#  0xcc -> THAI CHARACTER LO CHULA
    u'\u0e2d'	#  0xcd -> THAI CHARACTER O ANG
    u'\u0e2e'	#  0xce -> THAI CHARACTER HO NOKHUK
    u'\u0e2f'	#  0xcf -> THAI CHARACTER PAIYANNOI
    u'\u0e30'	#  0xd0 -> THAI CHARACTER SARA A
    u'\u0e31'	#  0xd1 -> THAI CHARACTER MAI HAN-AKAT
    u'\u0e32'	#  0xd2 -> THAI CHARACTER SARA AA
    u'\u0e33'	#  0xd3 -> THAI CHARACTER SARA AM
    u'\u0e34'	#  0xd4 -> THAI CHARACTER SARA I
    u'\u0e35'	#  0xd5 -> THAI CHARACTER SARA II
    u'\u0e36'	#  0xd6 -> THAI CHARACTER SARA UE
    u'\u0e37'	#  0xd7 -> THAI CHARACTER SARA UEE
    u'\u0e38'	#  0xd8 -> THAI CHARACTER SARA U
    u'\u0e39'	#  0xd9 -> THAI CHARACTER SARA UU
    u'\u0e3a'	#  0xda -> THAI CHARACTER PHINTHU
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u0e3f'	#  0xdf -> THAI CURRENCY SYMBOL BAHT
    u'\u0e40'	#  0xe0 -> THAI CHARACTER SARA E
    u'\u0e41'	#  0xe1 -> THAI CHARACTER SARA AE
    u'\u0e42'	#  0xe2 -> THAI CHARACTER SARA O
    u'\u0e43'	#  0xe3 -> THAI CHARACTER SARA AI MAIMUAN
    u'\u0e44'	#  0xe4 -> THAI CHARACTER SARA AI MAIMALAI
    u'\u0e45'	#  0xe5 -> THAI CHARACTER LAKKHANGYAO
    u'\u0e46'	#  0xe6 -> THAI CHARACTER MAIYAMOK
    u'\u0e47'	#  0xe7 -> THAI CHARACTER MAITAIKHU
    u'\u0e48'	#  0xe8 -> THAI CHARACTER MAI EK
    u'\u0e49'	#  0xe9 -> THAI CHARACTER MAI THO
    u'\u0e4a'	#  0xea -> THAI CHARACTER MAI TRI
    u'\u0e4b'	#  0xeb -> THAI CHARACTER MAI CHATTAWA
    u'\u0e4c'	#  0xec -> THAI CHARACTER THANTHAKHAT
    u'\u0e4d'	#  0xed -> THAI CHARACTER NIKHAHIT
    u'\u0e4e'	#  0xee -> THAI CHARACTER YAMAKKAN
    u'\u0e4f'	#  0xef -> THAI CHARACTER FONGMAN
    u'\u0e50'	#  0xf0 -> THAI DIGIT ZERO
    u'\u0e51'	#  0xf1 -> THAI DIGIT ONE
    u'\u0e52'	#  0xf2 -> THAI DIGIT TWO
    u'\u0e53'	#  0xf3 -> THAI DIGIT THREE
    u'\u0e54'	#  0xf4 -> THAI DIGIT FOUR
    u'\u0e55'	#  0xf5 -> THAI DIGIT FIVE
    u'\u0e56'	#  0xf6 -> THAI DIGIT SIX
    u'\u0e57'	#  0xf7 -> THAI DIGIT SEVEN
    u'\u0e58'	#  0xf8 -> THAI DIGIT EIGHT
    u'\u0e59'	#  0xf9 -> THAI DIGIT NINE
    u'\u0e5a'	#  0xfa -> THAI CHARACTER ANGKHANKHU
    u'\u0e5b'	#  0xfb -> THAI CHARACTER KHOMUT
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
)

### Encoding Map

encoding_map = {
    0x0000: 0x00,	#  NULL
    0x0001: 0x01,	#  START OF HEADING
    0x0002: 0x02,	#  START OF TEXT
    0x0003: 0x03,	#  END OF TEXT
    0x0004: 0x04,	#  END OF TRANSMISSION
    0x0005: 0x05,	#  ENQUIRY
    0x0006: 0x06,	#  ACKNOWLEDGE
    0x0007: 0x07,	#  BELL
    0x0008: 0x08,	#  BACKSPACE
    0x0009: 0x09,	#  HORIZONTAL TABULATION
    0x000a: 0x0a,	#  LINE FEED
    0x000b: 0x0b,	#  VERTICAL TABULATION
    0x000c: 0x0c,	#  FORM FEED
    0x000d: 0x0d,	#  CARRIAGE RETURN
    0x000e: 0x0e,	#  SHIFT OUT
    0x000f: 0x0f,	#  SHIFT IN
    0x0010: 0x10,	#  DATA LINK ESCAPE
    0x0011: 0x11,	#  DEVICE CONTROL ONE
    0x0012: 0x12,	#  DEVICE CONTROL TWO
    0x0013: 0x13,	#  DEVICE CONTROL THREE
    0x0014: 0x14,	#  DEVICE CONTROL FOUR
    0x0015: 0x15,	#  NEGATIVE ACKNOWLEDGE
    0x0016: 0x16,	#  SYNCHRONOUS IDLE
    0x0017: 0x17,	#  END OF TRANSMISSION BLOCK
    0x0018: 0x18,	#  CANCEL
    0x0019: 0x19,	#  END OF MEDIUM
    0x001a: 0x1a,	#  SUBSTITUTE
    0x001b: 0x1b,	#  ESCAPE
    0x001c: 0x1c,	#  FILE SEPARATOR
    0x001d: 0x1d,	#  GROUP SEPARATOR
    0x001e: 0x1e,	#  RECORD SEPARATOR
    0x001f: 0x1f,	#  UNIT SEPARATOR
    0x0020: 0x20,	#  SPACE
    0x0021: 0x21,	#  EXCLAMATION MARK
    0x0022: 0x22,	#  QUOTATION MARK
    0x0023: 0x23,	#  NUMBER SIGN
    0x0024: 0x24,	#  DOLLAR SIGN
    0x0025: 0x25,	#  PERCENT SIGN
    0x0026: 0x26,	#  AMPERSAND
    0x0027: 0x27,	#  APOSTROPHE
    0x0028: 0x28,	#  LEFT PARENTHESIS
    0x0029: 0x29,	#  RIGHT PARENTHESIS
    0x002a: 0x2a,	#  ASTERISK
    0x002b: 0x2b,	#  PLUS SIGN
    0x002c: 0x2c,	#  COMMA
    0x002d: 0x2d,	#  HYPHEN-MINUS
    0x002e: 0x2e,	#  FULL STOP
    0x002f: 0x2f,	#  SOLIDUS
    0x0030: 0x30,	#  DIGIT ZERO
    0x0031: 0x31,	#  DIGIT ONE
    0x0032: 0x32,	#  DIGIT TWO
    0x0033: 0x33,	#  DIGIT THREE
    0x0034: 0x34,	#  DIGIT FOUR
    0x0035: 0x35,	#  DIGIT FIVE
    0x0036: 0x36,	#  DIGIT SIX
    0x0037: 0x37,	#  DIGIT SEVEN
    0x0038: 0x38,	#  DIGIT EIGHT
    0x0039: 0x39,	#  DIGIT NINE
    0x003a: 0x3a,	#  COLON
    0x003b: 0x3b,	#  SEMICOLON
    0x003c: 0x3c,	#  LESS-THAN SIGN
    0x003d: 0x3d,	#  EQUALS SIGN
    0x003e: 0x3e,	#  GREATER-THAN SIGN
    0x003f: 0x3f,	#  QUESTION MARK
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
    0x005b: 0x5b,	#  LEFT SQUARE BRACKET
    0x005c: 0x5c,	#  REVERSE SOLIDUS
    0x005d: 0x5d,	#  RIGHT SQUARE BRACKET
    0x005e: 0x5e,	#  CIRCUMFLEX ACCENT
    0x005f: 0x5f,	#  LOW LINE
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
    0x007b: 0x7b,	#  LEFT CURLY BRACKET
    0x007c: 0x7c,	#  VERTICAL LINE
    0x007d: 0x7d,	#  RIGHT CURLY BRACKET
    0x007e: 0x7e,	#  TILDE
    0x007f: 0x7f,	#  DELETE
    0x0080: 0x80,	#  <control>
    0x0081: 0x81,	#  <control>
    0x0082: 0x82,	#  <control>
    0x0083: 0x83,	#  <control>
    0x0084: 0x84,	#  <control>
    0x0085: 0x85,	#  <control>
    0x0086: 0x86,	#  <control>
    0x0087: 0x87,	#  <control>
    0x0088: 0x88,	#  <control>
    0x0089: 0x89,	#  <control>
    0x008a: 0x8a,	#  <control>
    0x008b: 0x8b,	#  <control>
    0x008c: 0x8c,	#  <control>
    0x008d: 0x8d,	#  <control>
    0x008e: 0x8e,	#  <control>
    0x008f: 0x8f,	#  <control>
    0x0090: 0x90,	#  <control>
    0x0091: 0x91,	#  <control>
    0x0092: 0x92,	#  <control>
    0x0093: 0x93,	#  <control>
    0x0094: 0x94,	#  <control>
    0x0095: 0x95,	#  <control>
    0x0096: 0x96,	#  <control>
    0x0097: 0x97,	#  <control>
    0x0098: 0x98,	#  <control>
    0x0099: 0x99,	#  <control>
    0x009a: 0x9a,	#  <control>
    0x009b: 0x9b,	#  <control>
    0x009c: 0x9c,	#  <control>
    0x009d: 0x9d,	#  <control>
    0x009e: 0x9e,	#  <control>
    0x009f: 0x9f,	#  <control>
    0x00a0: 0xa0,	#  NO-BREAK SPACE
    0x0e01: 0xa1,	#  THAI CHARACTER KO KAI
    0x0e02: 0xa2,	#  THAI CHARACTER KHO KHAI
    0x0e03: 0xa3,	#  THAI CHARACTER KHO KHUAT
    0x0e04: 0xa4,	#  THAI CHARACTER KHO KHWAI
    0x0e05: 0xa5,	#  THAI CHARACTER KHO KHON
    0x0e06: 0xa6,	#  THAI CHARACTER KHO RAKHANG
    0x0e07: 0xa7,	#  THAI CHARACTER NGO NGU
    0x0e08: 0xa8,	#  THAI CHARACTER CHO CHAN
    0x0e09: 0xa9,	#  THAI CHARACTER CHO CHING
    0x0e0a: 0xaa,	#  THAI CHARACTER CHO CHANG
    0x0e0b: 0xab,	#  THAI CHARACTER SO SO
    0x0e0c: 0xac,	#  THAI CHARACTER CHO CHOE
    0x0e0d: 0xad,	#  THAI CHARACTER YO YING
    0x0e0e: 0xae,	#  THAI CHARACTER DO CHADA
    0x0e0f: 0xaf,	#  THAI CHARACTER TO PATAK
    0x0e10: 0xb0,	#  THAI CHARACTER THO THAN
    0x0e11: 0xb1,	#  THAI CHARACTER THO NANGMONTHO
    0x0e12: 0xb2,	#  THAI CHARACTER THO PHUTHAO
    0x0e13: 0xb3,	#  THAI CHARACTER NO NEN
    0x0e14: 0xb4,	#  THAI CHARACTER DO DEK
    0x0e15: 0xb5,	#  THAI CHARACTER TO TAO
    0x0e16: 0xb6,	#  THAI CHARACTER THO THUNG
    0x0e17: 0xb7,	#  THAI CHARACTER THO THAHAN
    0x0e18: 0xb8,	#  THAI CHARACTER THO THONG
    0x0e19: 0xb9,	#  THAI CHARACTER NO NU
    0x0e1a: 0xba,	#  THAI CHARACTER BO BAIMAI
    0x0e1b: 0xbb,	#  THAI CHARACTER PO PLA
    0x0e1c: 0xbc,	#  THAI CHARACTER PHO PHUNG
    0x0e1d: 0xbd,	#  THAI CHARACTER FO FA
    0x0e1e: 0xbe,	#  THAI CHARACTER PHO PHAN
    0x0e1f: 0xbf,	#  THAI CHARACTER FO FAN
    0x0e20: 0xc0,	#  THAI CHARACTER PHO SAMPHAO
    0x0e21: 0xc1,	#  THAI CHARACTER MO MA
    0x0e22: 0xc2,	#  THAI CHARACTER YO YAK
    0x0e23: 0xc3,	#  THAI CHARACTER RO RUA
    0x0e24: 0xc4,	#  THAI CHARACTER RU
    0x0e25: 0xc5,	#  THAI CHARACTER LO LING
    0x0e26: 0xc6,	#  THAI CHARACTER LU
    0x0e27: 0xc7,	#  THAI CHARACTER WO WAEN
    0x0e28: 0xc8,	#  THAI CHARACTER SO SALA
    0x0e29: 0xc9,	#  THAI CHARACTER SO RUSI
    0x0e2a: 0xca,	#  THAI CHARACTER SO SUA
    0x0e2b: 0xcb,	#  THAI CHARACTER HO HIP
    0x0e2c: 0xcc,	#  THAI CHARACTER LO CHULA
    0x0e2d: 0xcd,	#  THAI CHARACTER O ANG
    0x0e2e: 0xce,	#  THAI CHARACTER HO NOKHUK
    0x0e2f: 0xcf,	#  THAI CHARACTER PAIYANNOI
    0x0e30: 0xd0,	#  THAI CHARACTER SARA A
    0x0e31: 0xd1,	#  THAI CHARACTER MAI HAN-AKAT
    0x0e32: 0xd2,	#  THAI CHARACTER SARA AA
    0x0e33: 0xd3,	#  THAI CHARACTER SARA AM
    0x0e34: 0xd4,	#  THAI CHARACTER SARA I
    0x0e35: 0xd5,	#  THAI CHARACTER SARA II
    0x0e36: 0xd6,	#  THAI CHARACTER SARA UE
    0x0e37: 0xd7,	#  THAI CHARACTER SARA UEE
    0x0e38: 0xd8,	#  THAI CHARACTER SARA U
    0x0e39: 0xd9,	#  THAI CHARACTER SARA UU
    0x0e3a: 0xda,	#  THAI CHARACTER PHINTHU
    0x0e3f: 0xdf,	#  THAI CURRENCY SYMBOL BAHT
    0x0e40: 0xe0,	#  THAI CHARACTER SARA E
    0x0e41: 0xe1,	#  THAI CHARACTER SARA AE
    0x0e42: 0xe2,	#  THAI CHARACTER SARA O
    0x0e43: 0xe3,	#  THAI CHARACTER SARA AI MAIMUAN
    0x0e44: 0xe4,	#  THAI CHARACTER SARA AI MAIMALAI
    0x0e45: 0xe5,	#  THAI CHARACTER LAKKHANGYAO
    0x0e46: 0xe6,	#  THAI CHARACTER MAIYAMOK
    0x0e47: 0xe7,	#  THAI CHARACTER MAITAIKHU
    0x0e48: 0xe8,	#  THAI CHARACTER MAI EK
    0x0e49: 0xe9,	#  THAI CHARACTER MAI THO
    0x0e4a: 0xea,	#  THAI CHARACTER MAI TRI
    0x0e4b: 0xeb,	#  THAI CHARACTER MAI CHATTAWA
    0x0e4c: 0xec,	#  THAI CHARACTER THANTHAKHAT
    0x0e4d: 0xed,	#  THAI CHARACTER NIKHAHIT
    0x0e4e: 0xee,	#  THAI CHARACTER YAMAKKAN
    0x0e4f: 0xef,	#  THAI CHARACTER FONGMAN
    0x0e50: 0xf0,	#  THAI DIGIT ZERO
    0x0e51: 0xf1,	#  THAI DIGIT ONE
    0x0e52: 0xf2,	#  THAI DIGIT TWO
    0x0e53: 0xf3,	#  THAI DIGIT THREE
    0x0e54: 0xf4,	#  THAI DIGIT FOUR
    0x0e55: 0xf5,	#  THAI DIGIT FIVE
    0x0e56: 0xf6,	#  THAI DIGIT SIX
    0x0e57: 0xf7,	#  THAI DIGIT SEVEN
    0x0e58: 0xf8,	#  THAI DIGIT EIGHT
    0x0e59: 0xf9,	#  THAI DIGIT NINE
    0x0e5a: 0xfa,	#  THAI CHARACTER ANGKHANKHU
    0x0e5b: 0xfb,	#  THAI CHARACTER KHOMUT
}