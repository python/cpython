""" Python Character Mapping Codec cp875 generated from 'MAPPINGS/VENDORS/MICSFT/EBCDIC/CP875.TXT' with gencodec.py.

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
        name='cp875',
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
    u'\u0391'   #  0x41 -> GREEK CAPITAL LETTER ALPHA
    u'\u0392'   #  0x42 -> GREEK CAPITAL LETTER BETA
    u'\u0393'   #  0x43 -> GREEK CAPITAL LETTER GAMMA
    u'\u0394'   #  0x44 -> GREEK CAPITAL LETTER DELTA
    u'\u0395'   #  0x45 -> GREEK CAPITAL LETTER EPSILON
    u'\u0396'   #  0x46 -> GREEK CAPITAL LETTER ZETA
    u'\u0397'   #  0x47 -> GREEK CAPITAL LETTER ETA
    u'\u0398'   #  0x48 -> GREEK CAPITAL LETTER THETA
    u'\u0399'   #  0x49 -> GREEK CAPITAL LETTER IOTA
    u'['        #  0x4A -> LEFT SQUARE BRACKET
    u'.'        #  0x4B -> FULL STOP
    u'<'        #  0x4C -> LESS-THAN SIGN
    u'('        #  0x4D -> LEFT PARENTHESIS
    u'+'        #  0x4E -> PLUS SIGN
    u'!'        #  0x4F -> EXCLAMATION MARK
    u'&'        #  0x50 -> AMPERSAND
    u'\u039a'   #  0x51 -> GREEK CAPITAL LETTER KAPPA
    u'\u039b'   #  0x52 -> GREEK CAPITAL LETTER LAMDA
    u'\u039c'   #  0x53 -> GREEK CAPITAL LETTER MU
    u'\u039d'   #  0x54 -> GREEK CAPITAL LETTER NU
    u'\u039e'   #  0x55 -> GREEK CAPITAL LETTER XI
    u'\u039f'   #  0x56 -> GREEK CAPITAL LETTER OMICRON
    u'\u03a0'   #  0x57 -> GREEK CAPITAL LETTER PI
    u'\u03a1'   #  0x58 -> GREEK CAPITAL LETTER RHO
    u'\u03a3'   #  0x59 -> GREEK CAPITAL LETTER SIGMA
    u']'        #  0x5A -> RIGHT SQUARE BRACKET
    u'$'        #  0x5B -> DOLLAR SIGN
    u'*'        #  0x5C -> ASTERISK
    u')'        #  0x5D -> RIGHT PARENTHESIS
    u';'        #  0x5E -> SEMICOLON
    u'^'        #  0x5F -> CIRCUMFLEX ACCENT
    u'-'        #  0x60 -> HYPHEN-MINUS
    u'/'        #  0x61 -> SOLIDUS
    u'\u03a4'   #  0x62 -> GREEK CAPITAL LETTER TAU
    u'\u03a5'   #  0x63 -> GREEK CAPITAL LETTER UPSILON
    u'\u03a6'   #  0x64 -> GREEK CAPITAL LETTER PHI
    u'\u03a7'   #  0x65 -> GREEK CAPITAL LETTER CHI
    u'\u03a8'   #  0x66 -> GREEK CAPITAL LETTER PSI
    u'\u03a9'   #  0x67 -> GREEK CAPITAL LETTER OMEGA
    u'\u03aa'   #  0x68 -> GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
    u'\u03ab'   #  0x69 -> GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
    u'|'        #  0x6A -> VERTICAL LINE
    u','        #  0x6B -> COMMA
    u'%'        #  0x6C -> PERCENT SIGN
    u'_'        #  0x6D -> LOW LINE
    u'>'        #  0x6E -> GREATER-THAN SIGN
    u'?'        #  0x6F -> QUESTION MARK
    u'\xa8'     #  0x70 -> DIAERESIS
    u'\u0386'   #  0x71 -> GREEK CAPITAL LETTER ALPHA WITH TONOS
    u'\u0388'   #  0x72 -> GREEK CAPITAL LETTER EPSILON WITH TONOS
    u'\u0389'   #  0x73 -> GREEK CAPITAL LETTER ETA WITH TONOS
    u'\xa0'     #  0x74 -> NO-BREAK SPACE
    u'\u038a'   #  0x75 -> GREEK CAPITAL LETTER IOTA WITH TONOS
    u'\u038c'   #  0x76 -> GREEK CAPITAL LETTER OMICRON WITH TONOS
    u'\u038e'   #  0x77 -> GREEK CAPITAL LETTER UPSILON WITH TONOS
    u'\u038f'   #  0x78 -> GREEK CAPITAL LETTER OMEGA WITH TONOS
    u'`'        #  0x79 -> GRAVE ACCENT
    u':'        #  0x7A -> COLON
    u'#'        #  0x7B -> NUMBER SIGN
    u'@'        #  0x7C -> COMMERCIAL AT
    u"'"        #  0x7D -> APOSTROPHE
    u'='        #  0x7E -> EQUALS SIGN
    u'"'        #  0x7F -> QUOTATION MARK
    u'\u0385'   #  0x80 -> GREEK DIALYTIKA TONOS
    u'a'        #  0x81 -> LATIN SMALL LETTER A
    u'b'        #  0x82 -> LATIN SMALL LETTER B
    u'c'        #  0x83 -> LATIN SMALL LETTER C
    u'd'        #  0x84 -> LATIN SMALL LETTER D
    u'e'        #  0x85 -> LATIN SMALL LETTER E
    u'f'        #  0x86 -> LATIN SMALL LETTER F
    u'g'        #  0x87 -> LATIN SMALL LETTER G
    u'h'        #  0x88 -> LATIN SMALL LETTER H
    u'i'        #  0x89 -> LATIN SMALL LETTER I
    u'\u03b1'   #  0x8A -> GREEK SMALL LETTER ALPHA
    u'\u03b2'   #  0x8B -> GREEK SMALL LETTER BETA
    u'\u03b3'   #  0x8C -> GREEK SMALL LETTER GAMMA
    u'\u03b4'   #  0x8D -> GREEK SMALL LETTER DELTA
    u'\u03b5'   #  0x8E -> GREEK SMALL LETTER EPSILON
    u'\u03b6'   #  0x8F -> GREEK SMALL LETTER ZETA
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
    u'\u03b7'   #  0x9A -> GREEK SMALL LETTER ETA
    u'\u03b8'   #  0x9B -> GREEK SMALL LETTER THETA
    u'\u03b9'   #  0x9C -> GREEK SMALL LETTER IOTA
    u'\u03ba'   #  0x9D -> GREEK SMALL LETTER KAPPA
    u'\u03bb'   #  0x9E -> GREEK SMALL LETTER LAMDA
    u'\u03bc'   #  0x9F -> GREEK SMALL LETTER MU
    u'\xb4'     #  0xA0 -> ACUTE ACCENT
    u'~'        #  0xA1 -> TILDE
    u's'        #  0xA2 -> LATIN SMALL LETTER S
    u't'        #  0xA3 -> LATIN SMALL LETTER T
    u'u'        #  0xA4 -> LATIN SMALL LETTER U
    u'v'        #  0xA5 -> LATIN SMALL LETTER V
    u'w'        #  0xA6 -> LATIN SMALL LETTER W
    u'x'        #  0xA7 -> LATIN SMALL LETTER X
    u'y'        #  0xA8 -> LATIN SMALL LETTER Y
    u'z'        #  0xA9 -> LATIN SMALL LETTER Z
    u'\u03bd'   #  0xAA -> GREEK SMALL LETTER NU
    u'\u03be'   #  0xAB -> GREEK SMALL LETTER XI
    u'\u03bf'   #  0xAC -> GREEK SMALL LETTER OMICRON
    u'\u03c0'   #  0xAD -> GREEK SMALL LETTER PI
    u'\u03c1'   #  0xAE -> GREEK SMALL LETTER RHO
    u'\u03c3'   #  0xAF -> GREEK SMALL LETTER SIGMA
    u'\xa3'     #  0xB0 -> POUND SIGN
    u'\u03ac'   #  0xB1 -> GREEK SMALL LETTER ALPHA WITH TONOS
    u'\u03ad'   #  0xB2 -> GREEK SMALL LETTER EPSILON WITH TONOS
    u'\u03ae'   #  0xB3 -> GREEK SMALL LETTER ETA WITH TONOS
    u'\u03ca'   #  0xB4 -> GREEK SMALL LETTER IOTA WITH DIALYTIKA
    u'\u03af'   #  0xB5 -> GREEK SMALL LETTER IOTA WITH TONOS
    u'\u03cc'   #  0xB6 -> GREEK SMALL LETTER OMICRON WITH TONOS
    u'\u03cd'   #  0xB7 -> GREEK SMALL LETTER UPSILON WITH TONOS
    u'\u03cb'   #  0xB8 -> GREEK SMALL LETTER UPSILON WITH DIALYTIKA
    u'\u03ce'   #  0xB9 -> GREEK SMALL LETTER OMEGA WITH TONOS
    u'\u03c2'   #  0xBA -> GREEK SMALL LETTER FINAL SIGMA
    u'\u03c4'   #  0xBB -> GREEK SMALL LETTER TAU
    u'\u03c5'   #  0xBC -> GREEK SMALL LETTER UPSILON
    u'\u03c6'   #  0xBD -> GREEK SMALL LETTER PHI
    u'\u03c7'   #  0xBE -> GREEK SMALL LETTER CHI
    u'\u03c8'   #  0xBF -> GREEK SMALL LETTER PSI
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
    u'\u03c9'   #  0xCB -> GREEK SMALL LETTER OMEGA
    u'\u0390'   #  0xCC -> GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
    u'\u03b0'   #  0xCD -> GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
    u'\u2018'   #  0xCE -> LEFT SINGLE QUOTATION MARK
    u'\u2015'   #  0xCF -> HORIZONTAL BAR
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
    u'\xb1'     #  0xDA -> PLUS-MINUS SIGN
    u'\xbd'     #  0xDB -> VULGAR FRACTION ONE HALF
    u'\x1a'     #  0xDC -> SUBSTITUTE
    u'\u0387'   #  0xDD -> GREEK ANO TELEIA
    u'\u2019'   #  0xDE -> RIGHT SINGLE QUOTATION MARK
    u'\xa6'     #  0xDF -> BROKEN BAR
    u'\\'       #  0xE0 -> REVERSE SOLIDUS
    u'\x1a'     #  0xE1 -> SUBSTITUTE
    u'S'        #  0xE2 -> LATIN CAPITAL LETTER S
    u'T'        #  0xE3 -> LATIN CAPITAL LETTER T
    u'U'        #  0xE4 -> LATIN CAPITAL LETTER U
    u'V'        #  0xE5 -> LATIN CAPITAL LETTER V
    u'W'        #  0xE6 -> LATIN CAPITAL LETTER W
    u'X'        #  0xE7 -> LATIN CAPITAL LETTER X
    u'Y'        #  0xE8 -> LATIN CAPITAL LETTER Y
    u'Z'        #  0xE9 -> LATIN CAPITAL LETTER Z
    u'\xb2'     #  0xEA -> SUPERSCRIPT TWO
    u'\xa7'     #  0xEB -> SECTION SIGN
    u'\x1a'     #  0xEC -> SUBSTITUTE
    u'\x1a'     #  0xED -> SUBSTITUTE
    u'\xab'     #  0xEE -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xac'     #  0xEF -> NOT SIGN
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
    u'\xa9'     #  0xFB -> COPYRIGHT SIGN
    u'\x1a'     #  0xFC -> SUBSTITUTE
    u'\x1a'     #  0xFD -> SUBSTITUTE
    u'\xbb'     #  0xFE -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
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
    0x001A: None,       #  SUBSTITUTE
    0x001B: 0x27,       #  ESCAPE
    0x001C: 0x1C,       #  FILE SEPARATOR
    0x001D: 0x1D,       #  GROUP SEPARATOR
    0x001E: 0x1E,       #  RECORD SEPARATOR
    0x001F: 0x1F,       #  UNIT SEPARATOR
    0x0020: 0x40,       #  SPACE
    0x0021: 0x4F,       #  EXCLAMATION MARK
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
    0x005B: 0x4A,       #  LEFT SQUARE BRACKET
    0x005C: 0xE0,       #  REVERSE SOLIDUS
    0x005D: 0x5A,       #  RIGHT SQUARE BRACKET
    0x005E: 0x5F,       #  CIRCUMFLEX ACCENT
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
    0x007C: 0x6A,       #  VERTICAL LINE
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
    0x00A0: 0x74,       #  NO-BREAK SPACE
    0x00A3: 0xB0,       #  POUND SIGN
    0x00A6: 0xDF,       #  BROKEN BAR
    0x00A7: 0xEB,       #  SECTION SIGN
    0x00A8: 0x70,       #  DIAERESIS
    0x00A9: 0xFB,       #  COPYRIGHT SIGN
    0x00AB: 0xEE,       #  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00AC: 0xEF,       #  NOT SIGN
    0x00AD: 0xCA,       #  SOFT HYPHEN
    0x00B0: 0x90,       #  DEGREE SIGN
    0x00B1: 0xDA,       #  PLUS-MINUS SIGN
    0x00B2: 0xEA,       #  SUPERSCRIPT TWO
    0x00B3: 0xFA,       #  SUPERSCRIPT THREE
    0x00B4: 0xA0,       #  ACUTE ACCENT
    0x00BB: 0xFE,       #  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00BD: 0xDB,       #  VULGAR FRACTION ONE HALF
    0x0385: 0x80,       #  GREEK DIALYTIKA TONOS
    0x0386: 0x71,       #  GREEK CAPITAL LETTER ALPHA WITH TONOS
    0x0387: 0xDD,       #  GREEK ANO TELEIA
    0x0388: 0x72,       #  GREEK CAPITAL LETTER EPSILON WITH TONOS
    0x0389: 0x73,       #  GREEK CAPITAL LETTER ETA WITH TONOS
    0x038A: 0x75,       #  GREEK CAPITAL LETTER IOTA WITH TONOS
    0x038C: 0x76,       #  GREEK CAPITAL LETTER OMICRON WITH TONOS
    0x038E: 0x77,       #  GREEK CAPITAL LETTER UPSILON WITH TONOS
    0x038F: 0x78,       #  GREEK CAPITAL LETTER OMEGA WITH TONOS
    0x0390: 0xCC,       #  GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
    0x0391: 0x41,       #  GREEK CAPITAL LETTER ALPHA
    0x0392: 0x42,       #  GREEK CAPITAL LETTER BETA
    0x0393: 0x43,       #  GREEK CAPITAL LETTER GAMMA
    0x0394: 0x44,       #  GREEK CAPITAL LETTER DELTA
    0x0395: 0x45,       #  GREEK CAPITAL LETTER EPSILON
    0x0396: 0x46,       #  GREEK CAPITAL LETTER ZETA
    0x0397: 0x47,       #  GREEK CAPITAL LETTER ETA
    0x0398: 0x48,       #  GREEK CAPITAL LETTER THETA
    0x0399: 0x49,       #  GREEK CAPITAL LETTER IOTA
    0x039A: 0x51,       #  GREEK CAPITAL LETTER KAPPA
    0x039B: 0x52,       #  GREEK CAPITAL LETTER LAMDA
    0x039C: 0x53,       #  GREEK CAPITAL LETTER MU
    0x039D: 0x54,       #  GREEK CAPITAL LETTER NU
    0x039E: 0x55,       #  GREEK CAPITAL LETTER XI
    0x039F: 0x56,       #  GREEK CAPITAL LETTER OMICRON
    0x03A0: 0x57,       #  GREEK CAPITAL LETTER PI
    0x03A1: 0x58,       #  GREEK CAPITAL LETTER RHO
    0x03A3: 0x59,       #  GREEK CAPITAL LETTER SIGMA
    0x03A4: 0x62,       #  GREEK CAPITAL LETTER TAU
    0x03A5: 0x63,       #  GREEK CAPITAL LETTER UPSILON
    0x03A6: 0x64,       #  GREEK CAPITAL LETTER PHI
    0x03A7: 0x65,       #  GREEK CAPITAL LETTER CHI
    0x03A8: 0x66,       #  GREEK CAPITAL LETTER PSI
    0x03A9: 0x67,       #  GREEK CAPITAL LETTER OMEGA
    0x03AA: 0x68,       #  GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
    0x03AB: 0x69,       #  GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
    0x03AC: 0xB1,       #  GREEK SMALL LETTER ALPHA WITH TONOS
    0x03AD: 0xB2,       #  GREEK SMALL LETTER EPSILON WITH TONOS
    0x03AE: 0xB3,       #  GREEK SMALL LETTER ETA WITH TONOS
    0x03AF: 0xB5,       #  GREEK SMALL LETTER IOTA WITH TONOS
    0x03B0: 0xCD,       #  GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
    0x03B1: 0x8A,       #  GREEK SMALL LETTER ALPHA
    0x03B2: 0x8B,       #  GREEK SMALL LETTER BETA
    0x03B3: 0x8C,       #  GREEK SMALL LETTER GAMMA
    0x03B4: 0x8D,       #  GREEK SMALL LETTER DELTA
    0x03B5: 0x8E,       #  GREEK SMALL LETTER EPSILON
    0x03B6: 0x8F,       #  GREEK SMALL LETTER ZETA
    0x03B7: 0x9A,       #  GREEK SMALL LETTER ETA
    0x03B8: 0x9B,       #  GREEK SMALL LETTER THETA
    0x03B9: 0x9C,       #  GREEK SMALL LETTER IOTA
    0x03BA: 0x9D,       #  GREEK SMALL LETTER KAPPA
    0x03BB: 0x9E,       #  GREEK SMALL LETTER LAMDA
    0x03BC: 0x9F,       #  GREEK SMALL LETTER MU
    0x03BD: 0xAA,       #  GREEK SMALL LETTER NU
    0x03BE: 0xAB,       #  GREEK SMALL LETTER XI
    0x03BF: 0xAC,       #  GREEK SMALL LETTER OMICRON
    0x03C0: 0xAD,       #  GREEK SMALL LETTER PI
    0x03C1: 0xAE,       #  GREEK SMALL LETTER RHO
    0x03C2: 0xBA,       #  GREEK SMALL LETTER FINAL SIGMA
    0x03C3: 0xAF,       #  GREEK SMALL LETTER SIGMA
    0x03C4: 0xBB,       #  GREEK SMALL LETTER TAU
    0x03C5: 0xBC,       #  GREEK SMALL LETTER UPSILON
    0x03C6: 0xBD,       #  GREEK SMALL LETTER PHI
    0x03C7: 0xBE,       #  GREEK SMALL LETTER CHI
    0x03C8: 0xBF,       #  GREEK SMALL LETTER PSI
    0x03C9: 0xCB,       #  GREEK SMALL LETTER OMEGA
    0x03CA: 0xB4,       #  GREEK SMALL LETTER IOTA WITH DIALYTIKA
    0x03CB: 0xB8,       #  GREEK SMALL LETTER UPSILON WITH DIALYTIKA
    0x03CC: 0xB6,       #  GREEK SMALL LETTER OMICRON WITH TONOS
    0x03CD: 0xB7,       #  GREEK SMALL LETTER UPSILON WITH TONOS
    0x03CE: 0xB9,       #  GREEK SMALL LETTER OMEGA WITH TONOS
    0x2015: 0xCF,       #  HORIZONTAL BAR
    0x2018: 0xCE,       #  LEFT SINGLE QUOTATION MARK
    0x2019: 0xDE,       #  RIGHT SINGLE QUOTATION MARK
}
