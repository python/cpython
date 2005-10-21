""" Python Character Mapping Codec generated from 'ISO8859/8859-8.TXT' with gencodec.py.

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

### Decoding Map

decoding_map = codecs.make_identity_dict(range(256))
decoding_map.update({
    0x00a1: None,
    0x00aa: 0x00d7,	#  MULTIPLICATION SIGN
    0x00ba: 0x00f7,	#  DIVISION SIGN
    0x00bf: None,
    0x00c0: None,
    0x00c1: None,
    0x00c2: None,
    0x00c3: None,
    0x00c4: None,
    0x00c5: None,
    0x00c6: None,
    0x00c7: None,
    0x00c8: None,
    0x00c9: None,
    0x00ca: None,
    0x00cb: None,
    0x00cc: None,
    0x00cd: None,
    0x00ce: None,
    0x00cf: None,
    0x00d0: None,
    0x00d1: None,
    0x00d2: None,
    0x00d3: None,
    0x00d4: None,
    0x00d5: None,
    0x00d6: None,
    0x00d7: None,
    0x00d8: None,
    0x00d9: None,
    0x00da: None,
    0x00db: None,
    0x00dc: None,
    0x00dd: None,
    0x00de: None,
    0x00df: 0x2017,	#  DOUBLE LOW LINE
    0x00e0: 0x05d0,	#  HEBREW LETTER ALEF
    0x00e1: 0x05d1,	#  HEBREW LETTER BET
    0x00e2: 0x05d2,	#  HEBREW LETTER GIMEL
    0x00e3: 0x05d3,	#  HEBREW LETTER DALET
    0x00e4: 0x05d4,	#  HEBREW LETTER HE
    0x00e5: 0x05d5,	#  HEBREW LETTER VAV
    0x00e6: 0x05d6,	#  HEBREW LETTER ZAYIN
    0x00e7: 0x05d7,	#  HEBREW LETTER HET
    0x00e8: 0x05d8,	#  HEBREW LETTER TET
    0x00e9: 0x05d9,	#  HEBREW LETTER YOD
    0x00ea: 0x05da,	#  HEBREW LETTER FINAL KAF
    0x00eb: 0x05db,	#  HEBREW LETTER KAF
    0x00ec: 0x05dc,	#  HEBREW LETTER LAMED
    0x00ed: 0x05dd,	#  HEBREW LETTER FINAL MEM
    0x00ee: 0x05de,	#  HEBREW LETTER MEM
    0x00ef: 0x05df,	#  HEBREW LETTER FINAL NUN
    0x00f0: 0x05e0,	#  HEBREW LETTER NUN
    0x00f1: 0x05e1,	#  HEBREW LETTER SAMEKH
    0x00f2: 0x05e2,	#  HEBREW LETTER AYIN
    0x00f3: 0x05e3,	#  HEBREW LETTER FINAL PE
    0x00f4: 0x05e4,	#  HEBREW LETTER PE
    0x00f5: 0x05e5,	#  HEBREW LETTER FINAL TSADI
    0x00f6: 0x05e6,	#  HEBREW LETTER TSADI
    0x00f7: 0x05e7,	#  HEBREW LETTER QOF
    0x00f8: 0x05e8,	#  HEBREW LETTER RESH
    0x00f9: 0x05e9,	#  HEBREW LETTER SHIN
    0x00fa: 0x05ea,	#  HEBREW LETTER TAV
    0x00fb: None,
    0x00fc: None,
    0x00fd: 0x200e,	#  LEFT-TO-RIGHT MARK
    0x00fe: 0x200f,	#  RIGHT-TO-LEFT MARK
    0x00ff: None,
})

### Decoding Table

decoding_table = (
    u'\x00'	#  0x0000 -> NULL
    u'\x01'	#  0x0001 -> START OF HEADING
    u'\x02'	#  0x0002 -> START OF TEXT
    u'\x03'	#  0x0003 -> END OF TEXT
    u'\x04'	#  0x0004 -> END OF TRANSMISSION
    u'\x05'	#  0x0005 -> ENQUIRY
    u'\x06'	#  0x0006 -> ACKNOWLEDGE
    u'\x07'	#  0x0007 -> BELL
    u'\x08'	#  0x0008 -> BACKSPACE
    u'\t'	#  0x0009 -> HORIZONTAL TABULATION
    u'\n'	#  0x000a -> LINE FEED
    u'\x0b'	#  0x000b -> VERTICAL TABULATION
    u'\x0c'	#  0x000c -> FORM FEED
    u'\r'	#  0x000d -> CARRIAGE RETURN
    u'\x0e'	#  0x000e -> SHIFT OUT
    u'\x0f'	#  0x000f -> SHIFT IN
    u'\x10'	#  0x0010 -> DATA LINK ESCAPE
    u'\x11'	#  0x0011 -> DEVICE CONTROL ONE
    u'\x12'	#  0x0012 -> DEVICE CONTROL TWO
    u'\x13'	#  0x0013 -> DEVICE CONTROL THREE
    u'\x14'	#  0x0014 -> DEVICE CONTROL FOUR
    u'\x15'	#  0x0015 -> NEGATIVE ACKNOWLEDGE
    u'\x16'	#  0x0016 -> SYNCHRONOUS IDLE
    u'\x17'	#  0x0017 -> END OF TRANSMISSION BLOCK
    u'\x18'	#  0x0018 -> CANCEL
    u'\x19'	#  0x0019 -> END OF MEDIUM
    u'\x1a'	#  0x001a -> SUBSTITUTE
    u'\x1b'	#  0x001b -> ESCAPE
    u'\x1c'	#  0x001c -> FILE SEPARATOR
    u'\x1d'	#  0x001d -> GROUP SEPARATOR
    u'\x1e'	#  0x001e -> RECORD SEPARATOR
    u'\x1f'	#  0x001f -> UNIT SEPARATOR
    u' '	#  0x0020 -> SPACE
    u'!'	#  0x0021 -> EXCLAMATION MARK
    u'"'	#  0x0022 -> QUOTATION MARK
    u'#'	#  0x0023 -> NUMBER SIGN
    u'$'	#  0x0024 -> DOLLAR SIGN
    u'%'	#  0x0025 -> PERCENT SIGN
    u'&'	#  0x0026 -> AMPERSAND
    u"'"	#  0x0027 -> APOSTROPHE
    u'('	#  0x0028 -> LEFT PARENTHESIS
    u')'	#  0x0029 -> RIGHT PARENTHESIS
    u'*'	#  0x002a -> ASTERISK
    u'+'	#  0x002b -> PLUS SIGN
    u','	#  0x002c -> COMMA
    u'-'	#  0x002d -> HYPHEN-MINUS
    u'.'	#  0x002e -> FULL STOP
    u'/'	#  0x002f -> SOLIDUS
    u'0'	#  0x0030 -> DIGIT ZERO
    u'1'	#  0x0031 -> DIGIT ONE
    u'2'	#  0x0032 -> DIGIT TWO
    u'3'	#  0x0033 -> DIGIT THREE
    u'4'	#  0x0034 -> DIGIT FOUR
    u'5'	#  0x0035 -> DIGIT FIVE
    u'6'	#  0x0036 -> DIGIT SIX
    u'7'	#  0x0037 -> DIGIT SEVEN
    u'8'	#  0x0038 -> DIGIT EIGHT
    u'9'	#  0x0039 -> DIGIT NINE
    u':'	#  0x003a -> COLON
    u';'	#  0x003b -> SEMICOLON
    u'<'	#  0x003c -> LESS-THAN SIGN
    u'='	#  0x003d -> EQUALS SIGN
    u'>'	#  0x003e -> GREATER-THAN SIGN
    u'?'	#  0x003f -> QUESTION MARK
    u'@'	#  0x0040 -> COMMERCIAL AT
    u'A'	#  0x0041 -> LATIN CAPITAL LETTER A
    u'B'	#  0x0042 -> LATIN CAPITAL LETTER B
    u'C'	#  0x0043 -> LATIN CAPITAL LETTER C
    u'D'	#  0x0044 -> LATIN CAPITAL LETTER D
    u'E'	#  0x0045 -> LATIN CAPITAL LETTER E
    u'F'	#  0x0046 -> LATIN CAPITAL LETTER F
    u'G'	#  0x0047 -> LATIN CAPITAL LETTER G
    u'H'	#  0x0048 -> LATIN CAPITAL LETTER H
    u'I'	#  0x0049 -> LATIN CAPITAL LETTER I
    u'J'	#  0x004a -> LATIN CAPITAL LETTER J
    u'K'	#  0x004b -> LATIN CAPITAL LETTER K
    u'L'	#  0x004c -> LATIN CAPITAL LETTER L
    u'M'	#  0x004d -> LATIN CAPITAL LETTER M
    u'N'	#  0x004e -> LATIN CAPITAL LETTER N
    u'O'	#  0x004f -> LATIN CAPITAL LETTER O
    u'P'	#  0x0050 -> LATIN CAPITAL LETTER P
    u'Q'	#  0x0051 -> LATIN CAPITAL LETTER Q
    u'R'	#  0x0052 -> LATIN CAPITAL LETTER R
    u'S'	#  0x0053 -> LATIN CAPITAL LETTER S
    u'T'	#  0x0054 -> LATIN CAPITAL LETTER T
    u'U'	#  0x0055 -> LATIN CAPITAL LETTER U
    u'V'	#  0x0056 -> LATIN CAPITAL LETTER V
    u'W'	#  0x0057 -> LATIN CAPITAL LETTER W
    u'X'	#  0x0058 -> LATIN CAPITAL LETTER X
    u'Y'	#  0x0059 -> LATIN CAPITAL LETTER Y
    u'Z'	#  0x005a -> LATIN CAPITAL LETTER Z
    u'['	#  0x005b -> LEFT SQUARE BRACKET
    u'\\'	#  0x005c -> REVERSE SOLIDUS
    u']'	#  0x005d -> RIGHT SQUARE BRACKET
    u'^'	#  0x005e -> CIRCUMFLEX ACCENT
    u'_'	#  0x005f -> LOW LINE
    u'`'	#  0x0060 -> GRAVE ACCENT
    u'a'	#  0x0061 -> LATIN SMALL LETTER A
    u'b'	#  0x0062 -> LATIN SMALL LETTER B
    u'c'	#  0x0063 -> LATIN SMALL LETTER C
    u'd'	#  0x0064 -> LATIN SMALL LETTER D
    u'e'	#  0x0065 -> LATIN SMALL LETTER E
    u'f'	#  0x0066 -> LATIN SMALL LETTER F
    u'g'	#  0x0067 -> LATIN SMALL LETTER G
    u'h'	#  0x0068 -> LATIN SMALL LETTER H
    u'i'	#  0x0069 -> LATIN SMALL LETTER I
    u'j'	#  0x006a -> LATIN SMALL LETTER J
    u'k'	#  0x006b -> LATIN SMALL LETTER K
    u'l'	#  0x006c -> LATIN SMALL LETTER L
    u'm'	#  0x006d -> LATIN SMALL LETTER M
    u'n'	#  0x006e -> LATIN SMALL LETTER N
    u'o'	#  0x006f -> LATIN SMALL LETTER O
    u'p'	#  0x0070 -> LATIN SMALL LETTER P
    u'q'	#  0x0071 -> LATIN SMALL LETTER Q
    u'r'	#  0x0072 -> LATIN SMALL LETTER R
    u's'	#  0x0073 -> LATIN SMALL LETTER S
    u't'	#  0x0074 -> LATIN SMALL LETTER T
    u'u'	#  0x0075 -> LATIN SMALL LETTER U
    u'v'	#  0x0076 -> LATIN SMALL LETTER V
    u'w'	#  0x0077 -> LATIN SMALL LETTER W
    u'x'	#  0x0078 -> LATIN SMALL LETTER X
    u'y'	#  0x0079 -> LATIN SMALL LETTER Y
    u'z'	#  0x007a -> LATIN SMALL LETTER Z
    u'{'	#  0x007b -> LEFT CURLY BRACKET
    u'|'	#  0x007c -> VERTICAL LINE
    u'}'	#  0x007d -> RIGHT CURLY BRACKET
    u'~'	#  0x007e -> TILDE
    u'\x7f'	#  0x007f -> DELETE
    u'\x80'	#  0x0080 -> <control>
    u'\x81'	#  0x0081 -> <control>
    u'\x82'	#  0x0082 -> <control>
    u'\x83'	#  0x0083 -> <control>
    u'\x84'	#  0x0084 -> <control>
    u'\x85'	#  0x0085 -> <control>
    u'\x86'	#  0x0086 -> <control>
    u'\x87'	#  0x0087 -> <control>
    u'\x88'	#  0x0088 -> <control>
    u'\x89'	#  0x0089 -> <control>
    u'\x8a'	#  0x008a -> <control>
    u'\x8b'	#  0x008b -> <control>
    u'\x8c'	#  0x008c -> <control>
    u'\x8d'	#  0x008d -> <control>
    u'\x8e'	#  0x008e -> <control>
    u'\x8f'	#  0x008f -> <control>
    u'\x90'	#  0x0090 -> <control>
    u'\x91'	#  0x0091 -> <control>
    u'\x92'	#  0x0092 -> <control>
    u'\x93'	#  0x0093 -> <control>
    u'\x94'	#  0x0094 -> <control>
    u'\x95'	#  0x0095 -> <control>
    u'\x96'	#  0x0096 -> <control>
    u'\x97'	#  0x0097 -> <control>
    u'\x98'	#  0x0098 -> <control>
    u'\x99'	#  0x0099 -> <control>
    u'\x9a'	#  0x009a -> <control>
    u'\x9b'	#  0x009b -> <control>
    u'\x9c'	#  0x009c -> <control>
    u'\x9d'	#  0x009d -> <control>
    u'\x9e'	#  0x009e -> <control>
    u'\x9f'	#  0x009f -> <control>
    u'\xa0'	#  0x00a0 -> NO-BREAK SPACE
    u'\ufffe'
    u'\xa2'	#  0x00a2 -> CENT SIGN
    u'\xa3'	#  0x00a3 -> POUND SIGN
    u'\xa4'	#  0x00a4 -> CURRENCY SIGN
    u'\xa5'	#  0x00a5 -> YEN SIGN
    u'\xa6'	#  0x00a6 -> BROKEN BAR
    u'\xa7'	#  0x00a7 -> SECTION SIGN
    u'\xa8'	#  0x00a8 -> DIAERESIS
    u'\xa9'	#  0x00a9 -> COPYRIGHT SIGN
    u'\xd7'	#  0x00aa -> MULTIPLICATION SIGN
    u'\xab'	#  0x00ab -> LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xac'	#  0x00ac -> NOT SIGN
    u'\xad'	#  0x00ad -> SOFT HYPHEN
    u'\xae'	#  0x00ae -> REGISTERED SIGN
    u'\xaf'	#  0x00af -> MACRON
    u'\xb0'	#  0x00b0 -> DEGREE SIGN
    u'\xb1'	#  0x00b1 -> PLUS-MINUS SIGN
    u'\xb2'	#  0x00b2 -> SUPERSCRIPT TWO
    u'\xb3'	#  0x00b3 -> SUPERSCRIPT THREE
    u'\xb4'	#  0x00b4 -> ACUTE ACCENT
    u'\xb5'	#  0x00b5 -> MICRO SIGN
    u'\xb6'	#  0x00b6 -> PILCROW SIGN
    u'\xb7'	#  0x00b7 -> MIDDLE DOT
    u'\xb8'	#  0x00b8 -> CEDILLA
    u'\xb9'	#  0x00b9 -> SUPERSCRIPT ONE
    u'\xf7'	#  0x00ba -> DIVISION SIGN
    u'\xbb'	#  0x00bb -> RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    u'\xbc'	#  0x00bc -> VULGAR FRACTION ONE QUARTER
    u'\xbd'	#  0x00bd -> VULGAR FRACTION ONE HALF
    u'\xbe'	#  0x00be -> VULGAR FRACTION THREE QUARTERS
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u2017'	#  0x00df -> DOUBLE LOW LINE
    u'\u05d0'	#  0x00e0 -> HEBREW LETTER ALEF
    u'\u05d1'	#  0x00e1 -> HEBREW LETTER BET
    u'\u05d2'	#  0x00e2 -> HEBREW LETTER GIMEL
    u'\u05d3'	#  0x00e3 -> HEBREW LETTER DALET
    u'\u05d4'	#  0x00e4 -> HEBREW LETTER HE
    u'\u05d5'	#  0x00e5 -> HEBREW LETTER VAV
    u'\u05d6'	#  0x00e6 -> HEBREW LETTER ZAYIN
    u'\u05d7'	#  0x00e7 -> HEBREW LETTER HET
    u'\u05d8'	#  0x00e8 -> HEBREW LETTER TET
    u'\u05d9'	#  0x00e9 -> HEBREW LETTER YOD
    u'\u05da'	#  0x00ea -> HEBREW LETTER FINAL KAF
    u'\u05db'	#  0x00eb -> HEBREW LETTER KAF
    u'\u05dc'	#  0x00ec -> HEBREW LETTER LAMED
    u'\u05dd'	#  0x00ed -> HEBREW LETTER FINAL MEM
    u'\u05de'	#  0x00ee -> HEBREW LETTER MEM
    u'\u05df'	#  0x00ef -> HEBREW LETTER FINAL NUN
    u'\u05e0'	#  0x00f0 -> HEBREW LETTER NUN
    u'\u05e1'	#  0x00f1 -> HEBREW LETTER SAMEKH
    u'\u05e2'	#  0x00f2 -> HEBREW LETTER AYIN
    u'\u05e3'	#  0x00f3 -> HEBREW LETTER FINAL PE
    u'\u05e4'	#  0x00f4 -> HEBREW LETTER PE
    u'\u05e5'	#  0x00f5 -> HEBREW LETTER FINAL TSADI
    u'\u05e6'	#  0x00f6 -> HEBREW LETTER TSADI
    u'\u05e7'	#  0x00f7 -> HEBREW LETTER QOF
    u'\u05e8'	#  0x00f8 -> HEBREW LETTER RESH
    u'\u05e9'	#  0x00f9 -> HEBREW LETTER SHIN
    u'\u05ea'	#  0x00fa -> HEBREW LETTER TAV
    u'\ufffe'
    u'\ufffe'
    u'\u200e'	#  0x00fd -> LEFT-TO-RIGHT MARK
    u'\u200f'	#  0x00fe -> RIGHT-TO-LEFT MARK
    u'\ufffe'
)

### Encoding Map

encoding_map = {
    0x0000: 0x0000,	#  NULL
    0x0001: 0x0001,	#  START OF HEADING
    0x0002: 0x0002,	#  START OF TEXT
    0x0003: 0x0003,	#  END OF TEXT
    0x0004: 0x0004,	#  END OF TRANSMISSION
    0x0005: 0x0005,	#  ENQUIRY
    0x0006: 0x0006,	#  ACKNOWLEDGE
    0x0007: 0x0007,	#  BELL
    0x0008: 0x0008,	#  BACKSPACE
    0x0009: 0x0009,	#  HORIZONTAL TABULATION
    0x000a: 0x000a,	#  LINE FEED
    0x000b: 0x000b,	#  VERTICAL TABULATION
    0x000c: 0x000c,	#  FORM FEED
    0x000d: 0x000d,	#  CARRIAGE RETURN
    0x000e: 0x000e,	#  SHIFT OUT
    0x000f: 0x000f,	#  SHIFT IN
    0x0010: 0x0010,	#  DATA LINK ESCAPE
    0x0011: 0x0011,	#  DEVICE CONTROL ONE
    0x0012: 0x0012,	#  DEVICE CONTROL TWO
    0x0013: 0x0013,	#  DEVICE CONTROL THREE
    0x0014: 0x0014,	#  DEVICE CONTROL FOUR
    0x0015: 0x0015,	#  NEGATIVE ACKNOWLEDGE
    0x0016: 0x0016,	#  SYNCHRONOUS IDLE
    0x0017: 0x0017,	#  END OF TRANSMISSION BLOCK
    0x0018: 0x0018,	#  CANCEL
    0x0019: 0x0019,	#  END OF MEDIUM
    0x001a: 0x001a,	#  SUBSTITUTE
    0x001b: 0x001b,	#  ESCAPE
    0x001c: 0x001c,	#  FILE SEPARATOR
    0x001d: 0x001d,	#  GROUP SEPARATOR
    0x001e: 0x001e,	#  RECORD SEPARATOR
    0x001f: 0x001f,	#  UNIT SEPARATOR
    0x0020: 0x0020,	#  SPACE
    0x0021: 0x0021,	#  EXCLAMATION MARK
    0x0022: 0x0022,	#  QUOTATION MARK
    0x0023: 0x0023,	#  NUMBER SIGN
    0x0024: 0x0024,	#  DOLLAR SIGN
    0x0025: 0x0025,	#  PERCENT SIGN
    0x0026: 0x0026,	#  AMPERSAND
    0x0027: 0x0027,	#  APOSTROPHE
    0x0028: 0x0028,	#  LEFT PARENTHESIS
    0x0029: 0x0029,	#  RIGHT PARENTHESIS
    0x002a: 0x002a,	#  ASTERISK
    0x002b: 0x002b,	#  PLUS SIGN
    0x002c: 0x002c,	#  COMMA
    0x002d: 0x002d,	#  HYPHEN-MINUS
    0x002e: 0x002e,	#  FULL STOP
    0x002f: 0x002f,	#  SOLIDUS
    0x0030: 0x0030,	#  DIGIT ZERO
    0x0031: 0x0031,	#  DIGIT ONE
    0x0032: 0x0032,	#  DIGIT TWO
    0x0033: 0x0033,	#  DIGIT THREE
    0x0034: 0x0034,	#  DIGIT FOUR
    0x0035: 0x0035,	#  DIGIT FIVE
    0x0036: 0x0036,	#  DIGIT SIX
    0x0037: 0x0037,	#  DIGIT SEVEN
    0x0038: 0x0038,	#  DIGIT EIGHT
    0x0039: 0x0039,	#  DIGIT NINE
    0x003a: 0x003a,	#  COLON
    0x003b: 0x003b,	#  SEMICOLON
    0x003c: 0x003c,	#  LESS-THAN SIGN
    0x003d: 0x003d,	#  EQUALS SIGN
    0x003e: 0x003e,	#  GREATER-THAN SIGN
    0x003f: 0x003f,	#  QUESTION MARK
    0x0040: 0x0040,	#  COMMERCIAL AT
    0x0041: 0x0041,	#  LATIN CAPITAL LETTER A
    0x0042: 0x0042,	#  LATIN CAPITAL LETTER B
    0x0043: 0x0043,	#  LATIN CAPITAL LETTER C
    0x0044: 0x0044,	#  LATIN CAPITAL LETTER D
    0x0045: 0x0045,	#  LATIN CAPITAL LETTER E
    0x0046: 0x0046,	#  LATIN CAPITAL LETTER F
    0x0047: 0x0047,	#  LATIN CAPITAL LETTER G
    0x0048: 0x0048,	#  LATIN CAPITAL LETTER H
    0x0049: 0x0049,	#  LATIN CAPITAL LETTER I
    0x004a: 0x004a,	#  LATIN CAPITAL LETTER J
    0x004b: 0x004b,	#  LATIN CAPITAL LETTER K
    0x004c: 0x004c,	#  LATIN CAPITAL LETTER L
    0x004d: 0x004d,	#  LATIN CAPITAL LETTER M
    0x004e: 0x004e,	#  LATIN CAPITAL LETTER N
    0x004f: 0x004f,	#  LATIN CAPITAL LETTER O
    0x0050: 0x0050,	#  LATIN CAPITAL LETTER P
    0x0051: 0x0051,	#  LATIN CAPITAL LETTER Q
    0x0052: 0x0052,	#  LATIN CAPITAL LETTER R
    0x0053: 0x0053,	#  LATIN CAPITAL LETTER S
    0x0054: 0x0054,	#  LATIN CAPITAL LETTER T
    0x0055: 0x0055,	#  LATIN CAPITAL LETTER U
    0x0056: 0x0056,	#  LATIN CAPITAL LETTER V
    0x0057: 0x0057,	#  LATIN CAPITAL LETTER W
    0x0058: 0x0058,	#  LATIN CAPITAL LETTER X
    0x0059: 0x0059,	#  LATIN CAPITAL LETTER Y
    0x005a: 0x005a,	#  LATIN CAPITAL LETTER Z
    0x005b: 0x005b,	#  LEFT SQUARE BRACKET
    0x005c: 0x005c,	#  REVERSE SOLIDUS
    0x005d: 0x005d,	#  RIGHT SQUARE BRACKET
    0x005e: 0x005e,	#  CIRCUMFLEX ACCENT
    0x005f: 0x005f,	#  LOW LINE
    0x0060: 0x0060,	#  GRAVE ACCENT
    0x0061: 0x0061,	#  LATIN SMALL LETTER A
    0x0062: 0x0062,	#  LATIN SMALL LETTER B
    0x0063: 0x0063,	#  LATIN SMALL LETTER C
    0x0064: 0x0064,	#  LATIN SMALL LETTER D
    0x0065: 0x0065,	#  LATIN SMALL LETTER E
    0x0066: 0x0066,	#  LATIN SMALL LETTER F
    0x0067: 0x0067,	#  LATIN SMALL LETTER G
    0x0068: 0x0068,	#  LATIN SMALL LETTER H
    0x0069: 0x0069,	#  LATIN SMALL LETTER I
    0x006a: 0x006a,	#  LATIN SMALL LETTER J
    0x006b: 0x006b,	#  LATIN SMALL LETTER K
    0x006c: 0x006c,	#  LATIN SMALL LETTER L
    0x006d: 0x006d,	#  LATIN SMALL LETTER M
    0x006e: 0x006e,	#  LATIN SMALL LETTER N
    0x006f: 0x006f,	#  LATIN SMALL LETTER O
    0x0070: 0x0070,	#  LATIN SMALL LETTER P
    0x0071: 0x0071,	#  LATIN SMALL LETTER Q
    0x0072: 0x0072,	#  LATIN SMALL LETTER R
    0x0073: 0x0073,	#  LATIN SMALL LETTER S
    0x0074: 0x0074,	#  LATIN SMALL LETTER T
    0x0075: 0x0075,	#  LATIN SMALL LETTER U
    0x0076: 0x0076,	#  LATIN SMALL LETTER V
    0x0077: 0x0077,	#  LATIN SMALL LETTER W
    0x0078: 0x0078,	#  LATIN SMALL LETTER X
    0x0079: 0x0079,	#  LATIN SMALL LETTER Y
    0x007a: 0x007a,	#  LATIN SMALL LETTER Z
    0x007b: 0x007b,	#  LEFT CURLY BRACKET
    0x007c: 0x007c,	#  VERTICAL LINE
    0x007d: 0x007d,	#  RIGHT CURLY BRACKET
    0x007e: 0x007e,	#  TILDE
    0x007f: 0x007f,	#  DELETE
    0x0080: 0x0080,	#  <control>
    0x0081: 0x0081,	#  <control>
    0x0082: 0x0082,	#  <control>
    0x0083: 0x0083,	#  <control>
    0x0084: 0x0084,	#  <control>
    0x0085: 0x0085,	#  <control>
    0x0086: 0x0086,	#  <control>
    0x0087: 0x0087,	#  <control>
    0x0088: 0x0088,	#  <control>
    0x0089: 0x0089,	#  <control>
    0x008a: 0x008a,	#  <control>
    0x008b: 0x008b,	#  <control>
    0x008c: 0x008c,	#  <control>
    0x008d: 0x008d,	#  <control>
    0x008e: 0x008e,	#  <control>
    0x008f: 0x008f,	#  <control>
    0x0090: 0x0090,	#  <control>
    0x0091: 0x0091,	#  <control>
    0x0092: 0x0092,	#  <control>
    0x0093: 0x0093,	#  <control>
    0x0094: 0x0094,	#  <control>
    0x0095: 0x0095,	#  <control>
    0x0096: 0x0096,	#  <control>
    0x0097: 0x0097,	#  <control>
    0x0098: 0x0098,	#  <control>
    0x0099: 0x0099,	#  <control>
    0x009a: 0x009a,	#  <control>
    0x009b: 0x009b,	#  <control>
    0x009c: 0x009c,	#  <control>
    0x009d: 0x009d,	#  <control>
    0x009e: 0x009e,	#  <control>
    0x009f: 0x009f,	#  <control>
    0x00a0: 0x00a0,	#  NO-BREAK SPACE
    0x00a2: 0x00a2,	#  CENT SIGN
    0x00a3: 0x00a3,	#  POUND SIGN
    0x00a4: 0x00a4,	#  CURRENCY SIGN
    0x00a5: 0x00a5,	#  YEN SIGN
    0x00a6: 0x00a6,	#  BROKEN BAR
    0x00a7: 0x00a7,	#  SECTION SIGN
    0x00a8: 0x00a8,	#  DIAERESIS
    0x00a9: 0x00a9,	#  COPYRIGHT SIGN
    0x00ab: 0x00ab,	#  LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00ac: 0x00ac,	#  NOT SIGN
    0x00ad: 0x00ad,	#  SOFT HYPHEN
    0x00ae: 0x00ae,	#  REGISTERED SIGN
    0x00af: 0x00af,	#  MACRON
    0x00b0: 0x00b0,	#  DEGREE SIGN
    0x00b1: 0x00b1,	#  PLUS-MINUS SIGN
    0x00b2: 0x00b2,	#  SUPERSCRIPT TWO
    0x00b3: 0x00b3,	#  SUPERSCRIPT THREE
    0x00b4: 0x00b4,	#  ACUTE ACCENT
    0x00b5: 0x00b5,	#  MICRO SIGN
    0x00b6: 0x00b6,	#  PILCROW SIGN
    0x00b7: 0x00b7,	#  MIDDLE DOT
    0x00b8: 0x00b8,	#  CEDILLA
    0x00b9: 0x00b9,	#  SUPERSCRIPT ONE
    0x00bb: 0x00bb,	#  RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    0x00bc: 0x00bc,	#  VULGAR FRACTION ONE QUARTER
    0x00bd: 0x00bd,	#  VULGAR FRACTION ONE HALF
    0x00be: 0x00be,	#  VULGAR FRACTION THREE QUARTERS
    0x00d7: 0x00aa,	#  MULTIPLICATION SIGN
    0x00f7: 0x00ba,	#  DIVISION SIGN
    0x05d0: 0x00e0,	#  HEBREW LETTER ALEF
    0x05d1: 0x00e1,	#  HEBREW LETTER BET
    0x05d2: 0x00e2,	#  HEBREW LETTER GIMEL
    0x05d3: 0x00e3,	#  HEBREW LETTER DALET
    0x05d4: 0x00e4,	#  HEBREW LETTER HE
    0x05d5: 0x00e5,	#  HEBREW LETTER VAV
    0x05d6: 0x00e6,	#  HEBREW LETTER ZAYIN
    0x05d7: 0x00e7,	#  HEBREW LETTER HET
    0x05d8: 0x00e8,	#  HEBREW LETTER TET
    0x05d9: 0x00e9,	#  HEBREW LETTER YOD
    0x05da: 0x00ea,	#  HEBREW LETTER FINAL KAF
    0x05db: 0x00eb,	#  HEBREW LETTER KAF
    0x05dc: 0x00ec,	#  HEBREW LETTER LAMED
    0x05dd: 0x00ed,	#  HEBREW LETTER FINAL MEM
    0x05de: 0x00ee,	#  HEBREW LETTER MEM
    0x05df: 0x00ef,	#  HEBREW LETTER FINAL NUN
    0x05e0: 0x00f0,	#  HEBREW LETTER NUN
    0x05e1: 0x00f1,	#  HEBREW LETTER SAMEKH
    0x05e2: 0x00f2,	#  HEBREW LETTER AYIN
    0x05e3: 0x00f3,	#  HEBREW LETTER FINAL PE
    0x05e4: 0x00f4,	#  HEBREW LETTER PE
    0x05e5: 0x00f5,	#  HEBREW LETTER FINAL TSADI
    0x05e6: 0x00f6,	#  HEBREW LETTER TSADI
    0x05e7: 0x00f7,	#  HEBREW LETTER QOF
    0x05e8: 0x00f8,	#  HEBREW LETTER RESH
    0x05e9: 0x00f9,	#  HEBREW LETTER SHIN
    0x05ea: 0x00fa,	#  HEBREW LETTER TAV
    0x200e: 0x00fd,	#  LEFT-TO-RIGHT MARK
    0x200f: 0x00fe,	#  RIGHT-TO-LEFT MARK
    0x2017: 0x00df,	#  DOUBLE LOW LINE
}