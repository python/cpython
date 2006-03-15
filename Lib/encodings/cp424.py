""" Python Character Mapping Codec cp424 generated from 'MAPPINGS/VENDORS/MISC/CP424.TXT' with gencodec.py.

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
        name='cp424',
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
    u'\x9c'     #  0x04 -> SELECT
    u'\t'       #  0x05 -> HORIZONTAL TABULATION
    u'\x86'     #  0x06 -> REQUIRED NEW LINE
    u'\x7f'     #  0x07 -> DELETE
    u'\x97'     #  0x08 -> GRAPHIC ESCAPE
    u'\x8d'     #  0x09 -> SUPERSCRIPT
    u'\x8e'     #  0x0A -> REPEAT
    u'\x0b'     #  0x0B -> VERTICAL TABULATION
    u'\x0c'     #  0x0C -> FORM FEED
    u'\r'       #  0x0D -> CARRIAGE RETURN
    u'\x0e'     #  0x0E -> SHIFT OUT
    u'\x0f'     #  0x0F -> SHIFT IN
    u'\x10'     #  0x10 -> DATA LINK ESCAPE
    u'\x11'     #  0x11 -> DEVICE CONTROL ONE
    u'\x12'     #  0x12 -> DEVICE CONTROL TWO
    u'\x13'     #  0x13 -> DEVICE CONTROL THREE
    u'\x9d'     #  0x14 -> RESTORE/ENABLE PRESENTATION
    u'\x85'     #  0x15 -> NEW LINE
    u'\x08'     #  0x16 -> BACKSPACE
    u'\x87'     #  0x17 -> PROGRAM OPERATOR COMMUNICATION
    u'\x18'     #  0x18 -> CANCEL
    u'\x19'     #  0x19 -> END OF MEDIUM
    u'\x92'     #  0x1A -> UNIT BACK SPACE
    u'\x8f'     #  0x1B -> CUSTOMER USE ONE
    u'\x1c'     #  0x1C -> FILE SEPARATOR
    u'\x1d'     #  0x1D -> GROUP SEPARATOR
    u'\x1e'     #  0x1E -> RECORD SEPARATOR
    u'\x1f'     #  0x1F -> UNIT SEPARATOR
    u'\x80'     #  0x20 -> DIGIT SELECT
    u'\x81'     #  0x21 -> START OF SIGNIFICANCE
    u'\x82'     #  0x22 -> FIELD SEPARATOR
    u'\x83'     #  0x23 -> WORD UNDERSCORE
    u'\x84'     #  0x24 -> BYPASS OR INHIBIT PRESENTATION
    u'\n'       #  0x25 -> LINE FEED
    u'\x17'     #  0x26 -> END OF TRANSMISSION BLOCK
    u'\x1b'     #  0x27 -> ESCAPE
    u'\x88'     #  0x28 -> SET ATTRIBUTE
    u'\x89'     #  0x29 -> START FIELD EXTENDED
    u'\x8a'     #  0x2A -> SET MODE OR SWITCH
    u'\x8b'     #  0x2B -> CONTROL SEQUENCE PREFIX
    u'\x8c'     #  0x2C -> MODIFY FIELD ATTRIBUTE
    u'\x05'     #  0x2D -> ENQUIRY
    u'\x06'     #  0x2E -> ACKNOWLEDGE
    u'\x07'     #  0x2F -> BELL
    u'\x90'     #  0x30 -> <reserved>
    u'\x91'     #  0x31 -> <reserved>
    u'\x16'     #  0x32 -> SYNCHRONOUS IDLE
    u'\x93'     #  0x33 -> INDEX RETURN
    u'\x94'     #  0x34 -> PRESENTATION POSITION
    u'\x95'     #  0x35 -> TRANSPARENT
    u'\x96'     #  0x36 -> NUMERIC BACKSPACE
    u'\x04'     #  0x37 -> END OF TRANSMISSION
    u'\x98'     #  0x38 -> SUBSCRIPT
    u'\x99'     #  0x39 -> INDENT TABULATION
    u'\x9a'     #  0x3A -> REVERSE FORM FEED
    u'\x9b'     #  0x3B -> CUSTOMER USE THREE
    u'\x14'     #  0x3C -> DEVICE CONTROL FOUR
    u'\x15'     #  0x3D -> NEGATIVE ACKNOWLEDGE
    u'\x9e'     #  0x3E -> <reserved>
    u'\x1a'     #  0x3F -> SUBSTITUTE
    u' '        #  0x40 -> SPACE
    u'\u05d0'   #  0x41 -> HEBREW LETTER ALEF
    u'\u05d1'   #  0x42 -> HEBREW LETTER BET
    u'\u05d2'   #  0x43 -> HEBREW LETTER GIMEL
    u'\u05d3'   #  0x44 -> HEBREW LETTER DALET
    u'\u05d4'   #  0x45 -> HEBREW LETTER HE
    u'\u05d5'   #  0x46 -> HEBREW LETTER VAV
    u'\u05d6'   #  0x47 -> HEBREW LETTER ZAYIN
    u'\u05d7'   #  0x48 -> HEBREW LETTER HET
    u'\u05d8'   #  0x49 -> HEBREW LETTER TET
    u'\xa2'     #  0x4A -> CENT SIGN
    u'.'        #  0x4B -> FULL STOP
    u'<'        #  0x4C -> LESS-THAN SIGN
    u'('        #  0x4D -> LEFT PARENTHESIS
    u'+'        #  0x4E -> PLUS SIGN
    u'|'        #  0x4F -> VERTICAL LINE
    u'&'        #  0x50 -> AMPERSAND
    u'\u05d9'   #  0x51 -> HEBREW LETTER YOD
    u'\u05da'   #  0x52 -> HEBREW LETTER FINAL KAF
    u'\u05db'   #  0x53 -> HEBREW LETTER KAF
    u'\u05dc'   #  0x54 -> HEBREW LETTER LAMED
    u'\u05dd'   #  0x55 -> HEBREW LETTER FINAL MEM
    u'\u05de'   #  0x56 -> HEBREW LETTER MEM
    u'\u05df'   #  0x57 -> HEBREW LETTER FINAL NUN
    u'\u05e0'   #  0x58 -> HEBREW LETTER NUN
    u'\u05e1'   #  0x59 -> HEBREW LETTER SAMEKH
    u'!'        #  0x5A -> EXCLAMATION MARK
    u'$'        #  0x5B -> DOLLAR SIGN
    u'*'        #  0x5C -> ASTERISK
    u')'        #  0x5D -> RIGHT PARENTHESIS
    u';'        #  0x5E -> SEMICOLON
    u'\xac'     #  0x5F -> NOT SIGN
    u'-'        #  0x60 -> HYPHEN-MINUS
    u'/'        #  0x61 -> SOLIDUS
    u'\u05e2'   #  0x62 -> HEBREW LETTER AYIN
    u'\u05e3'   #  0x63 -> HEBREW LETTER FINAL PE
    u'\u05e4'   #  0x64 -> HEBREW LETTER PE
    u'\u05e5'   #  0x65 -> HEBREW LETTER FINAL TSADI
    u'\u05e6'   #  0x66 -> HEBREW LETTER TSADI
    u'\u05e7'   #  0x67 -> HEBREW LETTER QOF
    u'\u05e8'   #  0x68 -> HEBREW LETTER RESH
    u'\u05e9'   #  0x69 -> HEBREW LETTER SHIN
    u'\xa6'     #  0x6A -> BROKEN BAR
    u','        #  0x6B -> COMMA
    u'%'        #  0x6C -> PERCENT SIGN
    u'_'        #  0x6D -> LOW LINE
    u'>'        #  0x6E -> GREATER-THAN SIGN
    u'?'        #  0x6F -> QUESTION MARK
    u'\ufffe'   #  0x70 -> UNDEFINED
    u'\u05ea'   #  0x71 -> HEBREW LETTER TAV
    u'\ufffe'   #  0x72 -> UNDEFINED
    u'\ufffe'   #  0x73 -> UNDEFINED
    u'\xa0'     #  0x74 -> NO-BREAK SPACE
    u'\ufffe'   #  0x75 -> UNDEFINED
    u'\ufffe'   #  0x76 -> UNDEFINED
    u'\ufffe'   #  0x77 -> UNDEFINED
    u'\u2017'   #  0x78 -> DOUBLE LOW LINE
    u'`'        #  0x79 -> GRAVE ACCENT
    u':'        #  0x7A -> COLON
    u'#'        #  0x7B -> NUMBER SIGN
    u'@'        #  0x7C -> COMMERCIAL AT
    u"'"        #  0x7D -> APOSTROPHE
    u'='        #  0x7E -> EQUALS SIGN
    u'"'        #  0x7F -> QUOTATION MARK
    u'\ufffe'   #  0x80 -> UNDEFINED
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
    u'\ufffe'   #  0x8C -> UNDEFINED
    u'\ufffe'   #  0x8D -> UNDEFINED
    u'\ufffe'   #  0x8E -> UNDEFINED
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
    u'\ufffe'   #  0x9A -> UNDEFINED
    u'\ufffe'   #  0x9B -> UNDEFINED
    u'\ufffe'   #  0x9C -> UNDEFINED
    u'\xb8'     #  0x9D -> CEDILLA
    u'\ufffe'   #  0x9E -> UNDEFINED
    u'\xa4'     #  0x9F -> CURRENCY SIGN
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
    u'\ufffe'   #  0xAA -> UNDEFINED
    u'\ufffe'   #  0xAB -> UNDEFINED
    u'\ufffe'   #  0xAC -> UNDEFINED
    u'\ufffe'   #  0xAD -> UNDEFINED
    u'\ufffe'   #  0xAE -> UNDEFINED
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
    u'\ufffe'   #  0xCB -> UNDEFINED
    u'\ufffe'   #  0xCC -> UNDEFINED
    u'\ufffe'   #  0xCD -> UNDEFINED
    u'\ufffe'   #  0xCE -> UNDEFINED
    u'\ufffe'   #  0xCF -> UNDEFINED
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
    u'\ufffe'   #  0xDB -> UNDEFINED
    u'\ufffe'   #  0xDC -> UNDEFINED
    u'\ufffe'   #  0xDD -> UNDEFINED
    u'\ufffe'   #  0xDE -> UNDEFINED
    u'\ufffe'   #  0xDF -> UNDEFINED
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
    u'\ufffe'   #  0xEB -> UNDEFINED
    u'\ufffe'   #  0xEC -> UNDEFINED
    u'\ufffe'   #  0xED -> UNDEFINED
    u'\ufffe'   #  0xEE -> UNDEFINED
    u'\ufffe'   #  0xEF -> UNDEFINED
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
    u'\ufffe'   #  0xFB -> UNDEFINED
    u'\ufffe'   #  0xFC -> UNDEFINED
    u'\ufffe'   #  0xFD -> UNDEFINED
    u'\ufffe'   #  0xFE -> UNDEFINED
    u'\x9f'     #  0xFF -> EIGHT ONES
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
    0x0080: 0x20,       #  DIGIT SELECT
    0x0081: 0x21,       #  START OF SIGNIFICANCE
    0x0082: 0x22,       #  FIELD SEPARATOR
    0x0083: 0x23,       #  WORD UNDERSCORE
    0x0084: 0x24,       #  BYPASS OR INHIBIT PRESENTATION
    0x0085: 0x15,       #  NEW LINE
    0x0086: 0x06,       #  REQUIRED NEW LINE
    0x0087: 0x17,       #  PROGRAM OPERATOR COMMUNICATION
    0x0088: 0x28,       #  SET ATTRIBUTE
    0x0089: 0x29,       #  START FIELD EXTENDED
    0x008A: 0x2A,       #  SET MODE OR SWITCH
    0x008B: 0x2B,       #  CONTROL SEQUENCE PREFIX
    0x008C: 0x2C,       #  MODIFY FIELD ATTRIBUTE
    0x008D: 0x09,       #  SUPERSCRIPT
    0x008E: 0x0A,       #  REPEAT
    0x008F: 0x1B,       #  CUSTOMER USE ONE
    0x0090: 0x30,       #  <reserved>
    0x0091: 0x31,       #  <reserved>
    0x0092: 0x1A,       #  UNIT BACK SPACE
    0x0093: 0x33,       #  INDEX RETURN
    0x0094: 0x34,       #  PRESENTATION POSITION
    0x0095: 0x35,       #  TRANSPARENT
    0x0096: 0x36,       #  NUMERIC BACKSPACE
    0x0097: 0x08,       #  GRAPHIC ESCAPE
    0x0098: 0x38,       #  SUBSCRIPT
    0x0099: 0x39,       #  INDENT TABULATION
    0x009A: 0x3A,       #  REVERSE FORM FEED
    0x009B: 0x3B,       #  CUSTOMER USE THREE
    0x009C: 0x04,       #  SELECT
    0x009D: 0x14,       #  RESTORE/ENABLE PRESENTATION
    0x009E: 0x3E,       #  <reserved>
    0x009F: 0xFF,       #  EIGHT ONES
    0x00A0: 0x74,       #  NO-BREAK SPACE
    0x00A2: 0x4A,       #  CENT SIGN
    0x00A3: 0xB1,       #  POUND SIGN
    0x00A4: 0x9F,       #  CURRENCY SIGN
    0x00A5: 0xB2,       #  YEN SIGN
    0x00A6: 0x6A,       #  BROKEN BAR
    0x00A7: 0xB5,       #  SECTION SIGN
    0x00A8: 0xBD,       #  DIAERESIS
    0x00A9: 0xB4,       #  COPYRIGHT SIGN
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
    0x00BB: 0x8B,       #  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BC: 0xB7,       #  VULGAR FRACTION ONE QUARTER
    0x00BD: 0xB8,       #  VULGAR FRACTION ONE HALF
    0x00BE: 0xB9,       #  VULGAR FRACTION THREE QUARTERS
    0x00D7: 0xBF,       #  MULTIPLICATION SIGN
    0x00F7: 0xE1,       #  DIVISION SIGN
    0x05D0: 0x41,       #  HEBREW LETTER ALEF
    0x05D1: 0x42,       #  HEBREW LETTER BET
    0x05D2: 0x43,       #  HEBREW LETTER GIMEL
    0x05D3: 0x44,       #  HEBREW LETTER DALET
    0x05D4: 0x45,       #  HEBREW LETTER HE
    0x05D5: 0x46,       #  HEBREW LETTER VAV
    0x05D6: 0x47,       #  HEBREW LETTER ZAYIN
    0x05D7: 0x48,       #  HEBREW LETTER HET
    0x05D8: 0x49,       #  HEBREW LETTER TET
    0x05D9: 0x51,       #  HEBREW LETTER YOD
    0x05DA: 0x52,       #  HEBREW LETTER FINAL KAF
    0x05DB: 0x53,       #  HEBREW LETTER KAF
    0x05DC: 0x54,       #  HEBREW LETTER LAMED
    0x05DD: 0x55,       #  HEBREW LETTER FINAL MEM
    0x05DE: 0x56,       #  HEBREW LETTER MEM
    0x05DF: 0x57,       #  HEBREW LETTER FINAL NUN
    0x05E0: 0x58,       #  HEBREW LETTER NUN
    0x05E1: 0x59,       #  HEBREW LETTER SAMEKH
    0x05E2: 0x62,       #  HEBREW LETTER AYIN
    0x05E3: 0x63,       #  HEBREW LETTER FINAL PE
    0x05E4: 0x64,       #  HEBREW LETTER PE
    0x05E5: 0x65,       #  HEBREW LETTER FINAL TSADI
    0x05E6: 0x66,       #  HEBREW LETTER TSADI
    0x05E7: 0x67,       #  HEBREW LETTER QOF
    0x05E8: 0x68,       #  HEBREW LETTER RESH
    0x05E9: 0x69,       #  HEBREW LETTER SHIN
    0x05EA: 0x71,       #  HEBREW LETTER TAV
    0x2017: 0x78,       #  DOUBLE LOW LINE
}

