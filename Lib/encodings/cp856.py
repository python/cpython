""" Python Character Mapping Codec generated from 'MAPPINGS/VENDORS/MISC/CP856.TXT' with gencodec.py.

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
    u'\u05d0'	#  0x80 -> HEBREW LETTER ALEF
    u'\u05d1'	#  0x81 -> HEBREW LETTER BET
    u'\u05d2'	#  0x82 -> HEBREW LETTER GIMEL
    u'\u05d3'	#  0x83 -> HEBREW LETTER DALET
    u'\u05d4'	#  0x84 -> HEBREW LETTER HE
    u'\u05d5'	#  0x85 -> HEBREW LETTER VAV
    u'\u05d6'	#  0x86 -> HEBREW LETTER ZAYIN
    u'\u05d7'	#  0x87 -> HEBREW LETTER HET
    u'\u05d8'	#  0x88 -> HEBREW LETTER TET
    u'\u05d9'	#  0x89 -> HEBREW LETTER YOD
    u'\u05da'	#  0x8a -> HEBREW LETTER FINAL KAF
    u'\u05db'	#  0x8b -> HEBREW LETTER KAF
    u'\u05dc'	#  0x8c -> HEBREW LETTER LAMED
    u'\u05dd'	#  0x8d -> HEBREW LETTER FINAL MEM
    u'\u05de'	#  0x8e -> HEBREW LETTER MEM
    u'\u05df'	#  0x8f -> HEBREW LETTER FINAL NUN
    u'\u05e0'	#  0x90 -> HEBREW LETTER NUN
    u'\u05e1'	#  0x91 -> HEBREW LETTER SAMEKH
    u'\u05e2'	#  0x92 -> HEBREW LETTER AYIN
    u'\u05e3'	#  0x93 -> HEBREW LETTER FINAL PE
    u'\u05e4'	#  0x94 -> HEBREW LETTER PE
    u'\u05e5'	#  0x95 -> HEBREW LETTER FINAL TSADI
    u'\u05e6'	#  0x96 -> HEBREW LETTER TSADI
    u'\u05e7'	#  0x97 -> HEBREW LETTER QOF
    u'\u05e8'	#  0x98 -> HEBREW LETTER RESH
    u'\u05e9'	#  0x99 -> HEBREW LETTER SHIN
    u'\u05ea'	#  0x9a -> HEBREW LETTER TAV
    u'\ufffe'	#  0x9b -> UNDEFINED
    u'\xa3'	#  0x9c -> POUND SIGN
    u'\ufffe'	#  0x9d -> UNDEFINED
    u'\xd7'	#  0x9e -> MULTIPLICATION SIGN
    u'\ufffe'	#  0x9f -> UNDEFINED
    u'\ufffe'	#  0xa0 -> UNDEFINED
    u'\ufffe'	#  0xa1 -> UNDEFINED
    u'\ufffe'	#  0xa2 -> UNDEFINED
    u'\ufffe'	#  0xa3 -> UNDEFINED
    u'\ufffe'	#  0xa4 -> UNDEFINED
    u'\ufffe'	#  0xa5 -> UNDEFINED
    u'\ufffe'	#  0xa6 -> UNDEFINED
    u'\ufffe'	#  0xa7 -> UNDEFINED
    u'\ufffe'	#  0xa8 -> UNDEFINED
    u'\xae'	#  0xa9 -> REGISTERED SIGN
    u'\xac'	#  0xaa -> NOT SIGN
    u'\xbd'	#  0xab -> VULGAR FRACTION ONE HALF
    u'\xbc'	#  0xac -> VULGAR FRACTION ONE QUARTER
    u'\ufffe'	#  0xad -> UNDEFINED
    u'\xab'	#  0xae -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xbb'	#  0xaf -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\u2591'	#  0xb0 -> LIGHT SHADE
    u'\u2592'	#  0xb1 -> MEDIUM SHADE
    u'\u2593'	#  0xb2 -> DARK SHADE
    u'\u2502'	#  0xb3 -> BOX DRAWINGS LIGHT VERTICAL
    u'\u2524'	#  0xb4 -> BOX DRAWINGS LIGHT VERTICAL AND LEFT
    u'\ufffe'	#  0xb5 -> UNDEFINED
    u'\ufffe'	#  0xb6 -> UNDEFINED
    u'\ufffe'	#  0xb7 -> UNDEFINED
    u'\xa9'	#  0xb8 -> COPYRIGHT SIGN
    u'\u2563'	#  0xb9 -> BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    u'\u2551'	#  0xba -> BOX DRAWINGS DOUBLE VERTICAL
    u'\u2557'	#  0xbb -> BOX DRAWINGS DOUBLE DOWN AND LEFT
    u'\u255d'	#  0xbc -> BOX DRAWINGS DOUBLE UP AND LEFT
    u'\xa2'	#  0xbd -> CENT SIGN
    u'\xa5'	#  0xbe -> YEN SIGN
    u'\u2510'	#  0xbf -> BOX DRAWINGS LIGHT DOWN AND LEFT
    u'\u2514'	#  0xc0 -> BOX DRAWINGS LIGHT UP AND RIGHT
    u'\u2534'	#  0xc1 -> BOX DRAWINGS LIGHT UP AND HORIZONTAL
    u'\u252c'	#  0xc2 -> BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    u'\u251c'	#  0xc3 -> BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    u'\u2500'	#  0xc4 -> BOX DRAWINGS LIGHT HORIZONTAL
    u'\u253c'	#  0xc5 -> BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    u'\ufffe'	#  0xc6 -> UNDEFINED
    u'\ufffe'	#  0xc7 -> UNDEFINED
    u'\u255a'	#  0xc8 -> BOX DRAWINGS DOUBLE UP AND RIGHT
    u'\u2554'	#  0xc9 -> BOX DRAWINGS DOUBLE DOWN AND RIGHT
    u'\u2569'	#  0xca -> BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    u'\u2566'	#  0xcb -> BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    u'\u2560'	#  0xcc -> BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    u'\u2550'	#  0xcd -> BOX DRAWINGS DOUBLE HORIZONTAL
    u'\u256c'	#  0xce -> BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    u'\xa4'	#  0xcf -> CURRENCY SIGN
    u'\ufffe'	#  0xd0 -> UNDEFINED
    u'\ufffe'	#  0xd1 -> UNDEFINED
    u'\ufffe'	#  0xd2 -> UNDEFINED
    u'\ufffe'	#  0xd3 -> UNDEFINEDS
    u'\ufffe'	#  0xd4 -> UNDEFINED
    u'\ufffe'	#  0xd5 -> UNDEFINED
    u'\ufffe'	#  0xd6 -> UNDEFINEDE
    u'\ufffe'	#  0xd7 -> UNDEFINED
    u'\ufffe'	#  0xd8 -> UNDEFINED
    u'\u2518'	#  0xd9 -> BOX DRAWINGS LIGHT UP AND LEFT
    u'\u250c'	#  0xda -> BOX DRAWINGS LIGHT DOWN AND RIGHT
    u'\u2588'	#  0xdb -> FULL BLOCK
    u'\u2584'	#  0xdc -> LOWER HALF BLOCK
    u'\xa6'	#  0xdd -> BROKEN BAR
    u'\ufffe'	#  0xde -> UNDEFINED
    u'\u2580'	#  0xdf -> UPPER HALF BLOCK
    u'\ufffe'	#  0xe0 -> UNDEFINED
    u'\ufffe'	#  0xe1 -> UNDEFINED
    u'\ufffe'	#  0xe2 -> UNDEFINED
    u'\ufffe'	#  0xe3 -> UNDEFINED
    u'\ufffe'	#  0xe4 -> UNDEFINED
    u'\ufffe'	#  0xe5 -> UNDEFINED
    u'\xb5'	#  0xe6 -> MICRO SIGN
    u'\ufffe'	#  0xe7 -> UNDEFINED
    u'\ufffe'	#  0xe8 -> UNDEFINED
    u'\ufffe'	#  0xe9 -> UNDEFINED
    u'\ufffe'	#  0xea -> UNDEFINED
    u'\ufffe'	#  0xeb -> UNDEFINED
    u'\ufffe'	#  0xec -> UNDEFINED
    u'\ufffe'	#  0xed -> UNDEFINED
    u'\xaf'	#  0xee -> MACRON
    u'\xb4'	#  0xef -> ACUTE ACCENT
    u'\xad'	#  0xf0 -> SOFT HYPHEN
    u'\xb1'	#  0xf1 -> PLUS-MINUS SIGN
    u'\u2017'	#  0xf2 -> DOUBLE LOW LINE
    u'\xbe'	#  0xf3 -> VULGAR FRACTION THREE QUARTERS
    u'\xb6'	#  0xf4 -> PILCROW SIGN
    u'\xa7'	#  0xf5 -> SECTION SIGN
    u'\xf7'	#  0xf6 -> DIVISION SIGN
    u'\xb8'	#  0xf7 -> CEDILLA
    u'\xb0'	#  0xf8 -> DEGREE SIGN
    u'\xa8'	#  0xf9 -> DIAERESIS
    u'\xb7'	#  0xfa -> MIDDLE DOT
    u'\xb9'	#  0xfb -> SUPERSCRIPT ONE
    u'\xb3'	#  0xfc -> SUPERSCRIPT THREE
    u'\xb2'	#  0xfd -> SUPERSCRIPT TWO
    u'\u25a0'	#  0xfe -> BLACK SQUARE
    u'\xa0'	#  0xff -> NO-BREAK SPACE
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
    0x00a0: 0xff,	#  NO-BREAK SPACE
    0x00a2: 0xbd,	#  CENT SIGN
    0x00a3: 0x9c,	#  POUND SIGN
    0x00a4: 0xcf,	#  CURRENCY SIGN
    0x00a5: 0xbe,	#  YEN SIGN
    0x00a6: 0xdd,	#  BROKEN BAR
    0x00a7: 0xf5,	#  SECTION SIGN
    0x00a8: 0xf9,	#  DIAERESIS
    0x00a9: 0xb8,	#  COPYRIGHT SIGN
    0x00ab: 0xae,	#  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00ac: 0xaa,	#  NOT SIGN
    0x00ad: 0xf0,	#  SOFT HYPHEN
    0x00ae: 0xa9,	#  REGISTERED SIGN
    0x00af: 0xee,	#  MACRON
    0x00b0: 0xf8,	#  DEGREE SIGN
    0x00b1: 0xf1,	#  PLUS-MINUS SIGN
    0x00b2: 0xfd,	#  SUPERSCRIPT TWO
    0x00b3: 0xfc,	#  SUPERSCRIPT THREE
    0x00b4: 0xef,	#  ACUTE ACCENT
    0x00b5: 0xe6,	#  MICRO SIGN
    0x00b6: 0xf4,	#  PILCROW SIGN
    0x00b7: 0xfa,	#  MIDDLE DOT
    0x00b8: 0xf7,	#  CEDILLA
    0x00b9: 0xfb,	#  SUPERSCRIPT ONE
    0x00bb: 0xaf,	#  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00bc: 0xac,	#  VULGAR FRACTION ONE QUARTER
    0x00bd: 0xab,	#  VULGAR FRACTION ONE HALF
    0x00be: 0xf3,	#  VULGAR FRACTION THREE QUARTERS
    0x00d7: 0x9e,	#  MULTIPLICATION SIGN
    0x00f7: 0xf6,	#  DIVISION SIGN
    0x05d0: 0x80,	#  HEBREW LETTER ALEF
    0x05d1: 0x81,	#  HEBREW LETTER BET
    0x05d2: 0x82,	#  HEBREW LETTER GIMEL
    0x05d3: 0x83,	#  HEBREW LETTER DALET
    0x05d4: 0x84,	#  HEBREW LETTER HE
    0x05d5: 0x85,	#  HEBREW LETTER VAV
    0x05d6: 0x86,	#  HEBREW LETTER ZAYIN
    0x05d7: 0x87,	#  HEBREW LETTER HET
    0x05d8: 0x88,	#  HEBREW LETTER TET
    0x05d9: 0x89,	#  HEBREW LETTER YOD
    0x05da: 0x8a,	#  HEBREW LETTER FINAL KAF
    0x05db: 0x8b,	#  HEBREW LETTER KAF
    0x05dc: 0x8c,	#  HEBREW LETTER LAMED
    0x05dd: 0x8d,	#  HEBREW LETTER FINAL MEM
    0x05de: 0x8e,	#  HEBREW LETTER MEM
    0x05df: 0x8f,	#  HEBREW LETTER FINAL NUN
    0x05e0: 0x90,	#  HEBREW LETTER NUN
    0x05e1: 0x91,	#  HEBREW LETTER SAMEKH
    0x05e2: 0x92,	#  HEBREW LETTER AYIN
    0x05e3: 0x93,	#  HEBREW LETTER FINAL PE
    0x05e4: 0x94,	#  HEBREW LETTER PE
    0x05e5: 0x95,	#  HEBREW LETTER FINAL TSADI
    0x05e6: 0x96,	#  HEBREW LETTER TSADI
    0x05e7: 0x97,	#  HEBREW LETTER QOF
    0x05e8: 0x98,	#  HEBREW LETTER RESH
    0x05e9: 0x99,	#  HEBREW LETTER SHIN
    0x05ea: 0x9a,	#  HEBREW LETTER TAV
    0x2017: 0xf2,	#  DOUBLE LOW LINE
    0x2500: 0xc4,	#  BOX DRAWINGS LIGHT HORIZONTAL
    0x2502: 0xb3,	#  BOX DRAWINGS LIGHT VERTICAL
    0x250c: 0xda,	#  BOX DRAWINGS LIGHT DOWN AND RIGHT
    0x2510: 0xbf,	#  BOX DRAWINGS LIGHT DOWN AND LEFT
    0x2514: 0xc0,	#  BOX DRAWINGS LIGHT UP AND RIGHT
    0x2518: 0xd9,	#  BOX DRAWINGS LIGHT UP AND LEFT
    0x251c: 0xc3,	#  BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    0x2524: 0xb4,	#  BOX DRAWINGS LIGHT VERTICAL AND LEFT
    0x252c: 0xc2,	#  BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    0x2534: 0xc1,	#  BOX DRAWINGS LIGHT UP AND HORIZONTAL
    0x253c: 0xc5,	#  BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    0x2550: 0xcd,	#  BOX DRAWINGS DOUBLE HORIZONTAL
    0x2551: 0xba,	#  BOX DRAWINGS DOUBLE VERTICAL
    0x2554: 0xc9,	#  BOX DRAWINGS DOUBLE DOWN AND RIGHT
    0x2557: 0xbb,	#  BOX DRAWINGS DOUBLE DOWN AND LEFT
    0x255a: 0xc8,	#  BOX DRAWINGS DOUBLE UP AND RIGHT
    0x255d: 0xbc,	#  BOX DRAWINGS DOUBLE UP AND LEFT
    0x2560: 0xcc,	#  BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    0x2563: 0xb9,	#  BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    0x2566: 0xcb,	#  BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    0x2569: 0xca,	#  BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    0x256c: 0xce,	#  BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    0x2580: 0xdf,	#  UPPER HALF BLOCK
    0x2584: 0xdc,	#  LOWER HALF BLOCK
    0x2588: 0xdb,	#  FULL BLOCK
    0x2591: 0xb0,	#  LIGHT SHADE
    0x2592: 0xb1,	#  MEDIUM SHADE
    0x2593: 0xb2,	#  DARK SHADE
    0x25a0: 0xfe,	#  BLACK SQUARE
}