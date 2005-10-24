""" Python Character Mapping Codec generated from 'MAPPINGS/VENDORS/MISC/CP1006.TXT' with gencodec.py.

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
    u'\u06f0'	#  0xa1 -> EXTENDED ARABIC-INDIC DIGIT ZERO
    u'\u06f1'	#  0xa2 -> EXTENDED ARABIC-INDIC DIGIT ONE
    u'\u06f2'	#  0xa3 -> EXTENDED ARABIC-INDIC DIGIT TWO
    u'\u06f3'	#  0xa4 -> EXTENDED ARABIC-INDIC DIGIT THREE
    u'\u06f4'	#  0xa5 -> EXTENDED ARABIC-INDIC DIGIT FOUR
    u'\u06f5'	#  0xa6 -> EXTENDED ARABIC-INDIC DIGIT FIVE
    u'\u06f6'	#  0xa7 -> EXTENDED ARABIC-INDIC DIGIT SIX
    u'\u06f7'	#  0xa8 -> EXTENDED ARABIC-INDIC DIGIT SEVEN
    u'\u06f8'	#  0xa9 -> EXTENDED ARABIC-INDIC DIGIT EIGHT
    u'\u06f9'	#  0xaa -> EXTENDED ARABIC-INDIC DIGIT NINE
    u'\u060c'	#  0xab -> ARABIC COMMA
    u'\u061b'	#  0xac -> ARABIC SEMICOLON
    u'\xad'	#  0xad -> SOFT HYPHEN
    u'\u061f'	#  0xae -> ARABIC QUESTION MARK
    u'\ufe81'	#  0xaf -> ARABIC LETTER ALEF WITH MADDA ABOVE ISOLATED FORM
    u'\ufe8d'	#  0xb0 -> ARABIC LETTER ALEF ISOLATED FORM
    u'\ufe8e'	#  0xb1 -> ARABIC LETTER ALEF FINAL FORM
    u'\ufe8e'	#  0xb2 -> ARABIC LETTER ALEF FINAL FORM
    u'\ufe8f'	#  0xb3 -> ARABIC LETTER BEH ISOLATED FORM
    u'\ufe91'	#  0xb4 -> ARABIC LETTER BEH INITIAL FORM
    u'\ufb56'	#  0xb5 -> ARABIC LETTER PEH ISOLATED FORM
    u'\ufb58'	#  0xb6 -> ARABIC LETTER PEH INITIAL FORM
    u'\ufe93'	#  0xb7 -> ARABIC LETTER TEH MARBUTA ISOLATED FORM
    u'\ufe95'	#  0xb8 -> ARABIC LETTER TEH ISOLATED FORM
    u'\ufe97'	#  0xb9 -> ARABIC LETTER TEH INITIAL FORM
    u'\ufb66'	#  0xba -> ARABIC LETTER TTEH ISOLATED FORM
    u'\ufb68'	#  0xbb -> ARABIC LETTER TTEH INITIAL FORM
    u'\ufe99'	#  0xbc -> ARABIC LETTER THEH ISOLATED FORM
    u'\ufe9b'	#  0xbd -> ARABIC LETTER THEH INITIAL FORM
    u'\ufe9d'	#  0xbe -> ARABIC LETTER JEEM ISOLATED FORM
    u'\ufe9f'	#  0xbf -> ARABIC LETTER JEEM INITIAL FORM
    u'\ufb7a'	#  0xc0 -> ARABIC LETTER TCHEH ISOLATED FORM
    u'\ufb7c'	#  0xc1 -> ARABIC LETTER TCHEH INITIAL FORM
    u'\ufea1'	#  0xc2 -> ARABIC LETTER HAH ISOLATED FORM
    u'\ufea3'	#  0xc3 -> ARABIC LETTER HAH INITIAL FORM
    u'\ufea5'	#  0xc4 -> ARABIC LETTER KHAH ISOLATED FORM
    u'\ufea7'	#  0xc5 -> ARABIC LETTER KHAH INITIAL FORM
    u'\ufea9'	#  0xc6 -> ARABIC LETTER DAL ISOLATED FORM
    u'\ufb84'	#  0xc7 -> ARABIC LETTER DAHAL ISOLATED FORMN
    u'\ufeab'	#  0xc8 -> ARABIC LETTER THAL ISOLATED FORM
    u'\ufead'	#  0xc9 -> ARABIC LETTER REH ISOLATED FORM
    u'\ufb8c'	#  0xca -> ARABIC LETTER RREH ISOLATED FORM
    u'\ufeaf'	#  0xcb -> ARABIC LETTER ZAIN ISOLATED FORM
    u'\ufb8a'	#  0xcc -> ARABIC LETTER JEH ISOLATED FORM
    u'\ufeb1'	#  0xcd -> ARABIC LETTER SEEN ISOLATED FORM
    u'\ufeb3'	#  0xce -> ARABIC LETTER SEEN INITIAL FORM
    u'\ufeb5'	#  0xcf -> ARABIC LETTER SHEEN ISOLATED FORM
    u'\ufeb7'	#  0xd0 -> ARABIC LETTER SHEEN INITIAL FORM
    u'\ufeb9'	#  0xd1 -> ARABIC LETTER SAD ISOLATED FORM
    u'\ufebb'	#  0xd2 -> ARABIC LETTER SAD INITIAL FORM
    u'\ufebd'	#  0xd3 -> ARABIC LETTER DAD ISOLATED FORM
    u'\ufebf'	#  0xd4 -> ARABIC LETTER DAD INITIAL FORM
    u'\ufec1'	#  0xd5 -> ARABIC LETTER TAH ISOLATED FORM
    u'\ufec5'	#  0xd6 -> ARABIC LETTER ZAH ISOLATED FORM
    u'\ufec9'	#  0xd7 -> ARABIC LETTER AIN ISOLATED FORM
    u'\ufeca'	#  0xd8 -> ARABIC LETTER AIN FINAL FORM
    u'\ufecb'	#  0xd9 -> ARABIC LETTER AIN INITIAL FORM
    u'\ufecc'	#  0xda -> ARABIC LETTER AIN MEDIAL FORM
    u'\ufecd'	#  0xdb -> ARABIC LETTER GHAIN ISOLATED FORM
    u'\ufece'	#  0xdc -> ARABIC LETTER GHAIN FINAL FORM
    u'\ufecf'	#  0xdd -> ARABIC LETTER GHAIN INITIAL FORM
    u'\ufed0'	#  0xde -> ARABIC LETTER GHAIN MEDIAL FORM
    u'\ufed1'	#  0xdf -> ARABIC LETTER FEH ISOLATED FORM
    u'\ufed3'	#  0xe0 -> ARABIC LETTER FEH INITIAL FORM
    u'\ufed5'	#  0xe1 -> ARABIC LETTER QAF ISOLATED FORM
    u'\ufed7'	#  0xe2 -> ARABIC LETTER QAF INITIAL FORM
    u'\ufed9'	#  0xe3 -> ARABIC LETTER KAF ISOLATED FORM
    u'\ufedb'	#  0xe4 -> ARABIC LETTER KAF INITIAL FORM
    u'\ufb92'	#  0xe5 -> ARABIC LETTER GAF ISOLATED FORM
    u'\ufb94'	#  0xe6 -> ARABIC LETTER GAF INITIAL FORM
    u'\ufedd'	#  0xe7 -> ARABIC LETTER LAM ISOLATED FORM
    u'\ufedf'	#  0xe8 -> ARABIC LETTER LAM INITIAL FORM
    u'\ufee0'	#  0xe9 -> ARABIC LETTER LAM MEDIAL FORM
    u'\ufee1'	#  0xea -> ARABIC LETTER MEEM ISOLATED FORM
    u'\ufee3'	#  0xeb -> ARABIC LETTER MEEM INITIAL FORM
    u'\ufb9e'	#  0xec -> ARABIC LETTER NOON GHUNNA ISOLATED FORM
    u'\ufee5'	#  0xed -> ARABIC LETTER NOON ISOLATED FORM
    u'\ufee7'	#  0xee -> ARABIC LETTER NOON INITIAL FORM
    u'\ufe85'	#  0xef -> ARABIC LETTER WAW WITH HAMZA ABOVE ISOLATED FORM
    u'\ufeed'	#  0xf0 -> ARABIC LETTER WAW ISOLATED FORM
    u'\ufba6'	#  0xf1 -> ARABIC LETTER HEH GOAL ISOLATED FORM
    u'\ufba8'	#  0xf2 -> ARABIC LETTER HEH GOAL INITIAL FORM
    u'\ufba9'	#  0xf3 -> ARABIC LETTER HEH GOAL MEDIAL FORM
    u'\ufbaa'	#  0xf4 -> ARABIC LETTER HEH DOACHASHMEE ISOLATED FORM
    u'\ufe80'	#  0xf5 -> ARABIC LETTER HAMZA ISOLATED FORM
    u'\ufe89'	#  0xf6 -> ARABIC LETTER YEH WITH HAMZA ABOVE ISOLATED FORM
    u'\ufe8a'	#  0xf7 -> ARABIC LETTER YEH WITH HAMZA ABOVE FINAL FORM
    u'\ufe8b'	#  0xf8 -> ARABIC LETTER YEH WITH HAMZA ABOVE INITIAL FORM
    u'\ufef1'	#  0xf9 -> ARABIC LETTER YEH ISOLATED FORM
    u'\ufef2'	#  0xfa -> ARABIC LETTER YEH FINAL FORM
    u'\ufef3'	#  0xfb -> ARABIC LETTER YEH INITIAL FORM
    u'\ufbb0'	#  0xfc -> ARABIC LETTER YEH BARREE WITH HAMZA ABOVE ISOLATED FORM
    u'\ufbae'	#  0xfd -> ARABIC LETTER YEH BARREE ISOLATED FORM
    u'\ufe7c'	#  0xfe -> ARABIC SHADDA ISOLATED FORM
    u'\ufe7d'	#  0xff -> ARABIC SHADDA MEDIAL FORM
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
    0x00ad: 0xad,	#  SOFT HYPHEN
    0x060c: 0xab,	#  ARABIC COMMA
    0x061b: 0xac,	#  ARABIC SEMICOLON
    0x061f: 0xae,	#  ARABIC QUESTION MARK
    0x06f0: 0xa1,	#  EXTENDED ARABIC-INDIC DIGIT ZERO
    0x06f1: 0xa2,	#  EXTENDED ARABIC-INDIC DIGIT ONE
    0x06f2: 0xa3,	#  EXTENDED ARABIC-INDIC DIGIT TWO
    0x06f3: 0xa4,	#  EXTENDED ARABIC-INDIC DIGIT THREE
    0x06f4: 0xa5,	#  EXTENDED ARABIC-INDIC DIGIT FOUR
    0x06f5: 0xa6,	#  EXTENDED ARABIC-INDIC DIGIT FIVE
    0x06f6: 0xa7,	#  EXTENDED ARABIC-INDIC DIGIT SIX
    0x06f7: 0xa8,	#  EXTENDED ARABIC-INDIC DIGIT SEVEN
    0x06f8: 0xa9,	#  EXTENDED ARABIC-INDIC DIGIT EIGHT
    0x06f9: 0xaa,	#  EXTENDED ARABIC-INDIC DIGIT NINE
    0xfb56: 0xb5,	#  ARABIC LETTER PEH ISOLATED FORM
    0xfb58: 0xb6,	#  ARABIC LETTER PEH INITIAL FORM
    0xfb66: 0xba,	#  ARABIC LETTER TTEH ISOLATED FORM
    0xfb68: 0xbb,	#  ARABIC LETTER TTEH INITIAL FORM
    0xfb7a: 0xc0,	#  ARABIC LETTER TCHEH ISOLATED FORM
    0xfb7c: 0xc1,	#  ARABIC LETTER TCHEH INITIAL FORM
    0xfb84: 0xc7,	#  ARABIC LETTER DAHAL ISOLATED FORMN
    0xfb8a: 0xcc,	#  ARABIC LETTER JEH ISOLATED FORM
    0xfb8c: 0xca,	#  ARABIC LETTER RREH ISOLATED FORM
    0xfb92: 0xe5,	#  ARABIC LETTER GAF ISOLATED FORM
    0xfb94: 0xe6,	#  ARABIC LETTER GAF INITIAL FORM
    0xfb9e: 0xec,	#  ARABIC LETTER NOON GHUNNA ISOLATED FORM
    0xfba6: 0xf1,	#  ARABIC LETTER HEH GOAL ISOLATED FORM
    0xfba8: 0xf2,	#  ARABIC LETTER HEH GOAL INITIAL FORM
    0xfba9: 0xf3,	#  ARABIC LETTER HEH GOAL MEDIAL FORM
    0xfbaa: 0xf4,	#  ARABIC LETTER HEH DOACHASHMEE ISOLATED FORM
    0xfbae: 0xfd,	#  ARABIC LETTER YEH BARREE ISOLATED FORM
    0xfbb0: 0xfc,	#  ARABIC LETTER YEH BARREE WITH HAMZA ABOVE ISOLATED FORM
    0xfe7c: 0xfe,	#  ARABIC SHADDA ISOLATED FORM
    0xfe7d: 0xff,	#  ARABIC SHADDA MEDIAL FORM
    0xfe80: 0xf5,	#  ARABIC LETTER HAMZA ISOLATED FORM
    0xfe81: 0xaf,	#  ARABIC LETTER ALEF WITH MADDA ABOVE ISOLATED FORM
    0xfe85: 0xef,	#  ARABIC LETTER WAW WITH HAMZA ABOVE ISOLATED FORM
    0xfe89: 0xf6,	#  ARABIC LETTER YEH WITH HAMZA ABOVE ISOLATED FORM
    0xfe8a: 0xf7,	#  ARABIC LETTER YEH WITH HAMZA ABOVE FINAL FORM
    0xfe8b: 0xf8,	#  ARABIC LETTER YEH WITH HAMZA ABOVE INITIAL FORM
    0xfe8d: 0xb0,	#  ARABIC LETTER ALEF ISOLATED FORM
    0xfe8e: None,	#  ARABIC LETTER ALEF FINAL FORM
    0xfe8f: 0xb3,	#  ARABIC LETTER BEH ISOLATED FORM
    0xfe91: 0xb4,	#  ARABIC LETTER BEH INITIAL FORM
    0xfe93: 0xb7,	#  ARABIC LETTER TEH MARBUTA ISOLATED FORM
    0xfe95: 0xb8,	#  ARABIC LETTER TEH ISOLATED FORM
    0xfe97: 0xb9,	#  ARABIC LETTER TEH INITIAL FORM
    0xfe99: 0xbc,	#  ARABIC LETTER THEH ISOLATED FORM
    0xfe9b: 0xbd,	#  ARABIC LETTER THEH INITIAL FORM
    0xfe9d: 0xbe,	#  ARABIC LETTER JEEM ISOLATED FORM
    0xfe9f: 0xbf,	#  ARABIC LETTER JEEM INITIAL FORM
    0xfea1: 0xc2,	#  ARABIC LETTER HAH ISOLATED FORM
    0xfea3: 0xc3,	#  ARABIC LETTER HAH INITIAL FORM
    0xfea5: 0xc4,	#  ARABIC LETTER KHAH ISOLATED FORM
    0xfea7: 0xc5,	#  ARABIC LETTER KHAH INITIAL FORM
    0xfea9: 0xc6,	#  ARABIC LETTER DAL ISOLATED FORM
    0xfeab: 0xc8,	#  ARABIC LETTER THAL ISOLATED FORM
    0xfead: 0xc9,	#  ARABIC LETTER REH ISOLATED FORM
    0xfeaf: 0xcb,	#  ARABIC LETTER ZAIN ISOLATED FORM
    0xfeb1: 0xcd,	#  ARABIC LETTER SEEN ISOLATED FORM
    0xfeb3: 0xce,	#  ARABIC LETTER SEEN INITIAL FORM
    0xfeb5: 0xcf,	#  ARABIC LETTER SHEEN ISOLATED FORM
    0xfeb7: 0xd0,	#  ARABIC LETTER SHEEN INITIAL FORM
    0xfeb9: 0xd1,	#  ARABIC LETTER SAD ISOLATED FORM
    0xfebb: 0xd2,	#  ARABIC LETTER SAD INITIAL FORM
    0xfebd: 0xd3,	#  ARABIC LETTER DAD ISOLATED FORM
    0xfebf: 0xd4,	#  ARABIC LETTER DAD INITIAL FORM
    0xfec1: 0xd5,	#  ARABIC LETTER TAH ISOLATED FORM
    0xfec5: 0xd6,	#  ARABIC LETTER ZAH ISOLATED FORM
    0xfec9: 0xd7,	#  ARABIC LETTER AIN ISOLATED FORM
    0xfeca: 0xd8,	#  ARABIC LETTER AIN FINAL FORM
    0xfecb: 0xd9,	#  ARABIC LETTER AIN INITIAL FORM
    0xfecc: 0xda,	#  ARABIC LETTER AIN MEDIAL FORM
    0xfecd: 0xdb,	#  ARABIC LETTER GHAIN ISOLATED FORM
    0xfece: 0xdc,	#  ARABIC LETTER GHAIN FINAL FORM
    0xfecf: 0xdd,	#  ARABIC LETTER GHAIN INITIAL FORM
    0xfed0: 0xde,	#  ARABIC LETTER GHAIN MEDIAL FORM
    0xfed1: 0xdf,	#  ARABIC LETTER FEH ISOLATED FORM
    0xfed3: 0xe0,	#  ARABIC LETTER FEH INITIAL FORM
    0xfed5: 0xe1,	#  ARABIC LETTER QAF ISOLATED FORM
    0xfed7: 0xe2,	#  ARABIC LETTER QAF INITIAL FORM
    0xfed9: 0xe3,	#  ARABIC LETTER KAF ISOLATED FORM
    0xfedb: 0xe4,	#  ARABIC LETTER KAF INITIAL FORM
    0xfedd: 0xe7,	#  ARABIC LETTER LAM ISOLATED FORM
    0xfedf: 0xe8,	#  ARABIC LETTER LAM INITIAL FORM
    0xfee0: 0xe9,	#  ARABIC LETTER LAM MEDIAL FORM
    0xfee1: 0xea,	#  ARABIC LETTER MEEM ISOLATED FORM
    0xfee3: 0xeb,	#  ARABIC LETTER MEEM INITIAL FORM
    0xfee5: 0xed,	#  ARABIC LETTER NOON ISOLATED FORM
    0xfee7: 0xee,	#  ARABIC LETTER NOON INITIAL FORM
    0xfeed: 0xf0,	#  ARABIC LETTER WAW ISOLATED FORM
    0xfef1: 0xf9,	#  ARABIC LETTER YEH ISOLATED FORM
    0xfef2: 0xfa,	#  ARABIC LETTER YEH FINAL FORM
    0xfef3: 0xfb,	#  ARABIC LETTER YEH INITIAL FORM
}