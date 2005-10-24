""" Python Character Mapping Codec generated from 'MAPPINGS/ISO8859/8859-5.TXT' with gencodec.py.

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
    u'\u0401'	#  0xa1 -> CYRILLIC CAPITAL LETTER IO
    u'\u0402'	#  0xa2 -> CYRILLIC CAPITAL LETTER DJE
    u'\u0403'	#  0xa3 -> CYRILLIC CAPITAL LETTER GJE
    u'\u0404'	#  0xa4 -> CYRILLIC CAPITAL LETTER UKRAINIAN IE
    u'\u0405'	#  0xa5 -> CYRILLIC CAPITAL LETTER DZE
    u'\u0406'	#  0xa6 -> CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
    u'\u0407'	#  0xa7 -> CYRILLIC CAPITAL LETTER YI
    u'\u0408'	#  0xa8 -> CYRILLIC CAPITAL LETTER JE
    u'\u0409'	#  0xa9 -> CYRILLIC CAPITAL LETTER LJE
    u'\u040a'	#  0xaa -> CYRILLIC CAPITAL LETTER NJE
    u'\u040b'	#  0xab -> CYRILLIC CAPITAL LETTER TSHE
    u'\u040c'	#  0xac -> CYRILLIC CAPITAL LETTER KJE
    u'\xad'	#  0xad -> SOFT HYPHEN
    u'\u040e'	#  0xae -> CYRILLIC CAPITAL LETTER SHORT U
    u'\u040f'	#  0xaf -> CYRILLIC CAPITAL LETTER DZHE
    u'\u0410'	#  0xb0 -> CYRILLIC CAPITAL LETTER A
    u'\u0411'	#  0xb1 -> CYRILLIC CAPITAL LETTER BE
    u'\u0412'	#  0xb2 -> CYRILLIC CAPITAL LETTER VE
    u'\u0413'	#  0xb3 -> CYRILLIC CAPITAL LETTER GHE
    u'\u0414'	#  0xb4 -> CYRILLIC CAPITAL LETTER DE
    u'\u0415'	#  0xb5 -> CYRILLIC CAPITAL LETTER IE
    u'\u0416'	#  0xb6 -> CYRILLIC CAPITAL LETTER ZHE
    u'\u0417'	#  0xb7 -> CYRILLIC CAPITAL LETTER ZE
    u'\u0418'	#  0xb8 -> CYRILLIC CAPITAL LETTER I
    u'\u0419'	#  0xb9 -> CYRILLIC CAPITAL LETTER SHORT I
    u'\u041a'	#  0xba -> CYRILLIC CAPITAL LETTER KA
    u'\u041b'	#  0xbb -> CYRILLIC CAPITAL LETTER EL
    u'\u041c'	#  0xbc -> CYRILLIC CAPITAL LETTER EM
    u'\u041d'	#  0xbd -> CYRILLIC CAPITAL LETTER EN
    u'\u041e'	#  0xbe -> CYRILLIC CAPITAL LETTER O
    u'\u041f'	#  0xbf -> CYRILLIC CAPITAL LETTER PE
    u'\u0420'	#  0xc0 -> CYRILLIC CAPITAL LETTER ER
    u'\u0421'	#  0xc1 -> CYRILLIC CAPITAL LETTER ES
    u'\u0422'	#  0xc2 -> CYRILLIC CAPITAL LETTER TE
    u'\u0423'	#  0xc3 -> CYRILLIC CAPITAL LETTER U
    u'\u0424'	#  0xc4 -> CYRILLIC CAPITAL LETTER EF
    u'\u0425'	#  0xc5 -> CYRILLIC CAPITAL LETTER HA
    u'\u0426'	#  0xc6 -> CYRILLIC CAPITAL LETTER TSE
    u'\u0427'	#  0xc7 -> CYRILLIC CAPITAL LETTER CHE
    u'\u0428'	#  0xc8 -> CYRILLIC CAPITAL LETTER SHA
    u'\u0429'	#  0xc9 -> CYRILLIC CAPITAL LETTER SHCHA
    u'\u042a'	#  0xca -> CYRILLIC CAPITAL LETTER HARD SIGN
    u'\u042b'	#  0xcb -> CYRILLIC CAPITAL LETTER YERU
    u'\u042c'	#  0xcc -> CYRILLIC CAPITAL LETTER SOFT SIGN
    u'\u042d'	#  0xcd -> CYRILLIC CAPITAL LETTER E
    u'\u042e'	#  0xce -> CYRILLIC CAPITAL LETTER YU
    u'\u042f'	#  0xcf -> CYRILLIC CAPITAL LETTER YA
    u'\u0430'	#  0xd0 -> CYRILLIC SMALL LETTER A
    u'\u0431'	#  0xd1 -> CYRILLIC SMALL LETTER BE
    u'\u0432'	#  0xd2 -> CYRILLIC SMALL LETTER VE
    u'\u0433'	#  0xd3 -> CYRILLIC SMALL LETTER GHE
    u'\u0434'	#  0xd4 -> CYRILLIC SMALL LETTER DE
    u'\u0435'	#  0xd5 -> CYRILLIC SMALL LETTER IE
    u'\u0436'	#  0xd6 -> CYRILLIC SMALL LETTER ZHE
    u'\u0437'	#  0xd7 -> CYRILLIC SMALL LETTER ZE
    u'\u0438'	#  0xd8 -> CYRILLIC SMALL LETTER I
    u'\u0439'	#  0xd9 -> CYRILLIC SMALL LETTER SHORT I
    u'\u043a'	#  0xda -> CYRILLIC SMALL LETTER KA
    u'\u043b'	#  0xdb -> CYRILLIC SMALL LETTER EL
    u'\u043c'	#  0xdc -> CYRILLIC SMALL LETTER EM
    u'\u043d'	#  0xdd -> CYRILLIC SMALL LETTER EN
    u'\u043e'	#  0xde -> CYRILLIC SMALL LETTER O
    u'\u043f'	#  0xdf -> CYRILLIC SMALL LETTER PE
    u'\u0440'	#  0xe0 -> CYRILLIC SMALL LETTER ER
    u'\u0441'	#  0xe1 -> CYRILLIC SMALL LETTER ES
    u'\u0442'	#  0xe2 -> CYRILLIC SMALL LETTER TE
    u'\u0443'	#  0xe3 -> CYRILLIC SMALL LETTER U
    u'\u0444'	#  0xe4 -> CYRILLIC SMALL LETTER EF
    u'\u0445'	#  0xe5 -> CYRILLIC SMALL LETTER HA
    u'\u0446'	#  0xe6 -> CYRILLIC SMALL LETTER TSE
    u'\u0447'	#  0xe7 -> CYRILLIC SMALL LETTER CHE
    u'\u0448'	#  0xe8 -> CYRILLIC SMALL LETTER SHA
    u'\u0449'	#  0xe9 -> CYRILLIC SMALL LETTER SHCHA
    u'\u044a'	#  0xea -> CYRILLIC SMALL LETTER HARD SIGN
    u'\u044b'	#  0xeb -> CYRILLIC SMALL LETTER YERU
    u'\u044c'	#  0xec -> CYRILLIC SMALL LETTER SOFT SIGN
    u'\u044d'	#  0xed -> CYRILLIC SMALL LETTER E
    u'\u044e'	#  0xee -> CYRILLIC SMALL LETTER YU
    u'\u044f'	#  0xef -> CYRILLIC SMALL LETTER YA
    u'\u2116'	#  0xf0 -> NUMERO SIGN
    u'\u0451'	#  0xf1 -> CYRILLIC SMALL LETTER IO
    u'\u0452'	#  0xf2 -> CYRILLIC SMALL LETTER DJE
    u'\u0453'	#  0xf3 -> CYRILLIC SMALL LETTER GJE
    u'\u0454'	#  0xf4 -> CYRILLIC SMALL LETTER UKRAINIAN IE
    u'\u0455'	#  0xf5 -> CYRILLIC SMALL LETTER DZE
    u'\u0456'	#  0xf6 -> CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
    u'\u0457'	#  0xf7 -> CYRILLIC SMALL LETTER YI
    u'\u0458'	#  0xf8 -> CYRILLIC SMALL LETTER JE
    u'\u0459'	#  0xf9 -> CYRILLIC SMALL LETTER LJE
    u'\u045a'	#  0xfa -> CYRILLIC SMALL LETTER NJE
    u'\u045b'	#  0xfb -> CYRILLIC SMALL LETTER TSHE
    u'\u045c'	#  0xfc -> CYRILLIC SMALL LETTER KJE
    u'\xa7'	#  0xfd -> SECTION SIGN
    u'\u045e'	#  0xfe -> CYRILLIC SMALL LETTER SHORT U
    u'\u045f'	#  0xff -> CYRILLIC SMALL LETTER DZHE
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
    0x00a7: 0xfd,	#  SECTION SIGN
    0x00ad: 0xad,	#  SOFT HYPHEN
    0x0401: 0xa1,	#  CYRILLIC CAPITAL LETTER IO
    0x0402: 0xa2,	#  CYRILLIC CAPITAL LETTER DJE
    0x0403: 0xa3,	#  CYRILLIC CAPITAL LETTER GJE
    0x0404: 0xa4,	#  CYRILLIC CAPITAL LETTER UKRAINIAN IE
    0x0405: 0xa5,	#  CYRILLIC CAPITAL LETTER DZE
    0x0406: 0xa6,	#  CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
    0x0407: 0xa7,	#  CYRILLIC CAPITAL LETTER YI
    0x0408: 0xa8,	#  CYRILLIC CAPITAL LETTER JE
    0x0409: 0xa9,	#  CYRILLIC CAPITAL LETTER LJE
    0x040a: 0xaa,	#  CYRILLIC CAPITAL LETTER NJE
    0x040b: 0xab,	#  CYRILLIC CAPITAL LETTER TSHE
    0x040c: 0xac,	#  CYRILLIC CAPITAL LETTER KJE
    0x040e: 0xae,	#  CYRILLIC CAPITAL LETTER SHORT U
    0x040f: 0xaf,	#  CYRILLIC CAPITAL LETTER DZHE
    0x0410: 0xb0,	#  CYRILLIC CAPITAL LETTER A
    0x0411: 0xb1,	#  CYRILLIC CAPITAL LETTER BE
    0x0412: 0xb2,	#  CYRILLIC CAPITAL LETTER VE
    0x0413: 0xb3,	#  CYRILLIC CAPITAL LETTER GHE
    0x0414: 0xb4,	#  CYRILLIC CAPITAL LETTER DE
    0x0415: 0xb5,	#  CYRILLIC CAPITAL LETTER IE
    0x0416: 0xb6,	#  CYRILLIC CAPITAL LETTER ZHE
    0x0417: 0xb7,	#  CYRILLIC CAPITAL LETTER ZE
    0x0418: 0xb8,	#  CYRILLIC CAPITAL LETTER I
    0x0419: 0xb9,	#  CYRILLIC CAPITAL LETTER SHORT I
    0x041a: 0xba,	#  CYRILLIC CAPITAL LETTER KA
    0x041b: 0xbb,	#  CYRILLIC CAPITAL LETTER EL
    0x041c: 0xbc,	#  CYRILLIC CAPITAL LETTER EM
    0x041d: 0xbd,	#  CYRILLIC CAPITAL LETTER EN
    0x041e: 0xbe,	#  CYRILLIC CAPITAL LETTER O
    0x041f: 0xbf,	#  CYRILLIC CAPITAL LETTER PE
    0x0420: 0xc0,	#  CYRILLIC CAPITAL LETTER ER
    0x0421: 0xc1,	#  CYRILLIC CAPITAL LETTER ES
    0x0422: 0xc2,	#  CYRILLIC CAPITAL LETTER TE
    0x0423: 0xc3,	#  CYRILLIC CAPITAL LETTER U
    0x0424: 0xc4,	#  CYRILLIC CAPITAL LETTER EF
    0x0425: 0xc5,	#  CYRILLIC CAPITAL LETTER HA
    0x0426: 0xc6,	#  CYRILLIC CAPITAL LETTER TSE
    0x0427: 0xc7,	#  CYRILLIC CAPITAL LETTER CHE
    0x0428: 0xc8,	#  CYRILLIC CAPITAL LETTER SHA
    0x0429: 0xc9,	#  CYRILLIC CAPITAL LETTER SHCHA
    0x042a: 0xca,	#  CYRILLIC CAPITAL LETTER HARD SIGN
    0x042b: 0xcb,	#  CYRILLIC CAPITAL LETTER YERU
    0x042c: 0xcc,	#  CYRILLIC CAPITAL LETTER SOFT SIGN
    0x042d: 0xcd,	#  CYRILLIC CAPITAL LETTER E
    0x042e: 0xce,	#  CYRILLIC CAPITAL LETTER YU
    0x042f: 0xcf,	#  CYRILLIC CAPITAL LETTER YA
    0x0430: 0xd0,	#  CYRILLIC SMALL LETTER A
    0x0431: 0xd1,	#  CYRILLIC SMALL LETTER BE
    0x0432: 0xd2,	#  CYRILLIC SMALL LETTER VE
    0x0433: 0xd3,	#  CYRILLIC SMALL LETTER GHE
    0x0434: 0xd4,	#  CYRILLIC SMALL LETTER DE
    0x0435: 0xd5,	#  CYRILLIC SMALL LETTER IE
    0x0436: 0xd6,	#  CYRILLIC SMALL LETTER ZHE
    0x0437: 0xd7,	#  CYRILLIC SMALL LETTER ZE
    0x0438: 0xd8,	#  CYRILLIC SMALL LETTER I
    0x0439: 0xd9,	#  CYRILLIC SMALL LETTER SHORT I
    0x043a: 0xda,	#  CYRILLIC SMALL LETTER KA
    0x043b: 0xdb,	#  CYRILLIC SMALL LETTER EL
    0x043c: 0xdc,	#  CYRILLIC SMALL LETTER EM
    0x043d: 0xdd,	#  CYRILLIC SMALL LETTER EN
    0x043e: 0xde,	#  CYRILLIC SMALL LETTER O
    0x043f: 0xdf,	#  CYRILLIC SMALL LETTER PE
    0x0440: 0xe0,	#  CYRILLIC SMALL LETTER ER
    0x0441: 0xe1,	#  CYRILLIC SMALL LETTER ES
    0x0442: 0xe2,	#  CYRILLIC SMALL LETTER TE
    0x0443: 0xe3,	#  CYRILLIC SMALL LETTER U
    0x0444: 0xe4,	#  CYRILLIC SMALL LETTER EF
    0x0445: 0xe5,	#  CYRILLIC SMALL LETTER HA
    0x0446: 0xe6,	#  CYRILLIC SMALL LETTER TSE
    0x0447: 0xe7,	#  CYRILLIC SMALL LETTER CHE
    0x0448: 0xe8,	#  CYRILLIC SMALL LETTER SHA
    0x0449: 0xe9,	#  CYRILLIC SMALL LETTER SHCHA
    0x044a: 0xea,	#  CYRILLIC SMALL LETTER HARD SIGN
    0x044b: 0xeb,	#  CYRILLIC SMALL LETTER YERU
    0x044c: 0xec,	#  CYRILLIC SMALL LETTER SOFT SIGN
    0x044d: 0xed,	#  CYRILLIC SMALL LETTER E
    0x044e: 0xee,	#  CYRILLIC SMALL LETTER YU
    0x044f: 0xef,	#  CYRILLIC SMALL LETTER YA
    0x0451: 0xf1,	#  CYRILLIC SMALL LETTER IO
    0x0452: 0xf2,	#  CYRILLIC SMALL LETTER DJE
    0x0453: 0xf3,	#  CYRILLIC SMALL LETTER GJE
    0x0454: 0xf4,	#  CYRILLIC SMALL LETTER UKRAINIAN IE
    0x0455: 0xf5,	#  CYRILLIC SMALL LETTER DZE
    0x0456: 0xf6,	#  CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
    0x0457: 0xf7,	#  CYRILLIC SMALL LETTER YI
    0x0458: 0xf8,	#  CYRILLIC SMALL LETTER JE
    0x0459: 0xf9,	#  CYRILLIC SMALL LETTER LJE
    0x045a: 0xfa,	#  CYRILLIC SMALL LETTER NJE
    0x045b: 0xfb,	#  CYRILLIC SMALL LETTER TSHE
    0x045c: 0xfc,	#  CYRILLIC SMALL LETTER KJE
    0x045e: 0xfe,	#  CYRILLIC SMALL LETTER SHORT U
    0x045f: 0xff,	#  CYRILLIC SMALL LETTER DZHE
    0x2116: 0xf0,	#  NUMERO SIGN
}