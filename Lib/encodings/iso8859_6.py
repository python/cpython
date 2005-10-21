""" Python Character Mapping Codec generated from 'ISO8859/8859-6.TXT' with gencodec.py.

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
    0x00a2: None,
    0x00a3: None,
    0x00a5: None,
    0x00a6: None,
    0x00a7: None,
    0x00a8: None,
    0x00a9: None,
    0x00aa: None,
    0x00ab: None,
    0x00ac: 0x060c,	#  ARABIC COMMA
    0x00ae: None,
    0x00af: None,
    0x00b0: None,
    0x00b1: None,
    0x00b2: None,
    0x00b3: None,
    0x00b4: None,
    0x00b5: None,
    0x00b6: None,
    0x00b7: None,
    0x00b8: None,
    0x00b9: None,
    0x00ba: None,
    0x00bb: 0x061b,	#  ARABIC SEMICOLON
    0x00bc: None,
    0x00bd: None,
    0x00be: None,
    0x00bf: 0x061f,	#  ARABIC QUESTION MARK
    0x00c0: None,
    0x00c1: 0x0621,	#  ARABIC LETTER HAMZA
    0x00c2: 0x0622,	#  ARABIC LETTER ALEF WITH MADDA ABOVE
    0x00c3: 0x0623,	#  ARABIC LETTER ALEF WITH HAMZA ABOVE
    0x00c4: 0x0624,	#  ARABIC LETTER WAW WITH HAMZA ABOVE
    0x00c5: 0x0625,	#  ARABIC LETTER ALEF WITH HAMZA BELOW
    0x00c6: 0x0626,	#  ARABIC LETTER YEH WITH HAMZA ABOVE
    0x00c7: 0x0627,	#  ARABIC LETTER ALEF
    0x00c8: 0x0628,	#  ARABIC LETTER BEH
    0x00c9: 0x0629,	#  ARABIC LETTER TEH MARBUTA
    0x00ca: 0x062a,	#  ARABIC LETTER TEH
    0x00cb: 0x062b,	#  ARABIC LETTER THEH
    0x00cc: 0x062c,	#  ARABIC LETTER JEEM
    0x00cd: 0x062d,	#  ARABIC LETTER HAH
    0x00ce: 0x062e,	#  ARABIC LETTER KHAH
    0x00cf: 0x062f,	#  ARABIC LETTER DAL
    0x00d0: 0x0630,	#  ARABIC LETTER THAL
    0x00d1: 0x0631,	#  ARABIC LETTER REH
    0x00d2: 0x0632,	#  ARABIC LETTER ZAIN
    0x00d3: 0x0633,	#  ARABIC LETTER SEEN
    0x00d4: 0x0634,	#  ARABIC LETTER SHEEN
    0x00d5: 0x0635,	#  ARABIC LETTER SAD
    0x00d6: 0x0636,	#  ARABIC LETTER DAD
    0x00d7: 0x0637,	#  ARABIC LETTER TAH
    0x00d8: 0x0638,	#  ARABIC LETTER ZAH
    0x00d9: 0x0639,	#  ARABIC LETTER AIN
    0x00da: 0x063a,	#  ARABIC LETTER GHAIN
    0x00db: None,
    0x00dc: None,
    0x00dd: None,
    0x00de: None,
    0x00df: None,
    0x00e0: 0x0640,	#  ARABIC TATWEEL
    0x00e1: 0x0641,	#  ARABIC LETTER FEH
    0x00e2: 0x0642,	#  ARABIC LETTER QAF
    0x00e3: 0x0643,	#  ARABIC LETTER KAF
    0x00e4: 0x0644,	#  ARABIC LETTER LAM
    0x00e5: 0x0645,	#  ARABIC LETTER MEEM
    0x00e6: 0x0646,	#  ARABIC LETTER NOON
    0x00e7: 0x0647,	#  ARABIC LETTER HEH
    0x00e8: 0x0648,	#  ARABIC LETTER WAW
    0x00e9: 0x0649,	#  ARABIC LETTER ALEF MAKSURA
    0x00ea: 0x064a,	#  ARABIC LETTER YEH
    0x00eb: 0x064b,	#  ARABIC FATHATAN
    0x00ec: 0x064c,	#  ARABIC DAMMATAN
    0x00ed: 0x064d,	#  ARABIC KASRATAN
    0x00ee: 0x064e,	#  ARABIC FATHA
    0x00ef: 0x064f,	#  ARABIC DAMMA
    0x00f0: 0x0650,	#  ARABIC KASRA
    0x00f1: 0x0651,	#  ARABIC SHADDA
    0x00f2: 0x0652,	#  ARABIC SUKUN
    0x00f3: None,
    0x00f4: None,
    0x00f5: None,
    0x00f6: None,
    0x00f7: None,
    0x00f8: None,
    0x00f9: None,
    0x00fa: None,
    0x00fb: None,
    0x00fc: None,
    0x00fd: None,
    0x00fe: None,
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
    u'\ufffe'
    u'\ufffe'
    u'\xa4'	#  0x00a4 -> CURRENCY SIGN
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u060c'	#  0x00ac -> ARABIC COMMA
    u'\xad'	#  0x00ad -> SOFT HYPHEN
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
    u'\u061b'	#  0x00bb -> ARABIC SEMICOLON
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u061f'	#  0x00bf -> ARABIC QUESTION MARK
    u'\ufffe'
    u'\u0621'	#  0x00c1 -> ARABIC LETTER HAMZA
    u'\u0622'	#  0x00c2 -> ARABIC LETTER ALEF WITH MADDA ABOVE
    u'\u0623'	#  0x00c3 -> ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0624'	#  0x00c4 -> ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0625'	#  0x00c5 -> ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0626'	#  0x00c6 -> ARABIC LETTER YEH WITH HAMZA ABOVE
    u'\u0627'	#  0x00c7 -> ARABIC LETTER ALEF
    u'\u0628'	#  0x00c8 -> ARABIC LETTER BEH
    u'\u0629'	#  0x00c9 -> ARABIC LETTER TEH MARBUTA
    u'\u062a'	#  0x00ca -> ARABIC LETTER TEH
    u'\u062b'	#  0x00cb -> ARABIC LETTER THEH
    u'\u062c'	#  0x00cc -> ARABIC LETTER JEEM
    u'\u062d'	#  0x00cd -> ARABIC LETTER HAH
    u'\u062e'	#  0x00ce -> ARABIC LETTER KHAH
    u'\u062f'	#  0x00cf -> ARABIC LETTER DAL
    u'\u0630'	#  0x00d0 -> ARABIC LETTER THAL
    u'\u0631'	#  0x00d1 -> ARABIC LETTER REH
    u'\u0632'	#  0x00d2 -> ARABIC LETTER ZAIN
    u'\u0633'	#  0x00d3 -> ARABIC LETTER SEEN
    u'\u0634'	#  0x00d4 -> ARABIC LETTER SHEEN
    u'\u0635'	#  0x00d5 -> ARABIC LETTER SAD
    u'\u0636'	#  0x00d6 -> ARABIC LETTER DAD
    u'\u0637'	#  0x00d7 -> ARABIC LETTER TAH
    u'\u0638'	#  0x00d8 -> ARABIC LETTER ZAH
    u'\u0639'	#  0x00d9 -> ARABIC LETTER AIN
    u'\u063a'	#  0x00da -> ARABIC LETTER GHAIN
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u0640'	#  0x00e0 -> ARABIC TATWEEL
    u'\u0641'	#  0x00e1 -> ARABIC LETTER FEH
    u'\u0642'	#  0x00e2 -> ARABIC LETTER QAF
    u'\u0643'	#  0x00e3 -> ARABIC LETTER KAF
    u'\u0644'	#  0x00e4 -> ARABIC LETTER LAM
    u'\u0645'	#  0x00e5 -> ARABIC LETTER MEEM
    u'\u0646'	#  0x00e6 -> ARABIC LETTER NOON
    u'\u0647'	#  0x00e7 -> ARABIC LETTER HEH
    u'\u0648'	#  0x00e8 -> ARABIC LETTER WAW
    u'\u0649'	#  0x00e9 -> ARABIC LETTER ALEF MAKSURA
    u'\u064a'	#  0x00ea -> ARABIC LETTER YEH
    u'\u064b'	#  0x00eb -> ARABIC FATHATAN
    u'\u064c'	#  0x00ec -> ARABIC DAMMATAN
    u'\u064d'	#  0x00ed -> ARABIC KASRATAN
    u'\u064e'	#  0x00ee -> ARABIC FATHA
    u'\u064f'	#  0x00ef -> ARABIC DAMMA
    u'\u0650'	#  0x00f0 -> ARABIC KASRA
    u'\u0651'	#  0x00f1 -> ARABIC SHADDA
    u'\u0652'	#  0x00f2 -> ARABIC SUKUN
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
    0x00a4: 0x00a4,	#  CURRENCY SIGN
    0x00ad: 0x00ad,	#  SOFT HYPHEN
    0x060c: 0x00ac,	#  ARABIC COMMA
    0x061b: 0x00bb,	#  ARABIC SEMICOLON
    0x061f: 0x00bf,	#  ARABIC QUESTION MARK
    0x0621: 0x00c1,	#  ARABIC LETTER HAMZA
    0x0622: 0x00c2,	#  ARABIC LETTER ALEF WITH MADDA ABOVE
    0x0623: 0x00c3,	#  ARABIC LETTER ALEF WITH HAMZA ABOVE
    0x0624: 0x00c4,	#  ARABIC LETTER WAW WITH HAMZA ABOVE
    0x0625: 0x00c5,	#  ARABIC LETTER ALEF WITH HAMZA BELOW
    0x0626: 0x00c6,	#  ARABIC LETTER YEH WITH HAMZA ABOVE
    0x0627: 0x00c7,	#  ARABIC LETTER ALEF
    0x0628: 0x00c8,	#  ARABIC LETTER BEH
    0x0629: 0x00c9,	#  ARABIC LETTER TEH MARBUTA
    0x062a: 0x00ca,	#  ARABIC LETTER TEH
    0x062b: 0x00cb,	#  ARABIC LETTER THEH
    0x062c: 0x00cc,	#  ARABIC LETTER JEEM
    0x062d: 0x00cd,	#  ARABIC LETTER HAH
    0x062e: 0x00ce,	#  ARABIC LETTER KHAH
    0x062f: 0x00cf,	#  ARABIC LETTER DAL
    0x0630: 0x00d0,	#  ARABIC LETTER THAL
    0x0631: 0x00d1,	#  ARABIC LETTER REH
    0x0632: 0x00d2,	#  ARABIC LETTER ZAIN
    0x0633: 0x00d3,	#  ARABIC LETTER SEEN
    0x0634: 0x00d4,	#  ARABIC LETTER SHEEN
    0x0635: 0x00d5,	#  ARABIC LETTER SAD
    0x0636: 0x00d6,	#  ARABIC LETTER DAD
    0x0637: 0x00d7,	#  ARABIC LETTER TAH
    0x0638: 0x00d8,	#  ARABIC LETTER ZAH
    0x0639: 0x00d9,	#  ARABIC LETTER AIN
    0x063a: 0x00da,	#  ARABIC LETTER GHAIN
    0x0640: 0x00e0,	#  ARABIC TATWEEL
    0x0641: 0x00e1,	#  ARABIC LETTER FEH
    0x0642: 0x00e2,	#  ARABIC LETTER QAF
    0x0643: 0x00e3,	#  ARABIC LETTER KAF
    0x0644: 0x00e4,	#  ARABIC LETTER LAM
    0x0645: 0x00e5,	#  ARABIC LETTER MEEM
    0x0646: 0x00e6,	#  ARABIC LETTER NOON
    0x0647: 0x00e7,	#  ARABIC LETTER HEH
    0x0648: 0x00e8,	#  ARABIC LETTER WAW
    0x0649: 0x00e9,	#  ARABIC LETTER ALEF MAKSURA
    0x064a: 0x00ea,	#  ARABIC LETTER YEH
    0x064b: 0x00eb,	#  ARABIC FATHATAN
    0x064c: 0x00ec,	#  ARABIC DAMMATAN
    0x064d: 0x00ed,	#  ARABIC KASRATAN
    0x064e: 0x00ee,	#  ARABIC FATHA
    0x064f: 0x00ef,	#  ARABIC DAMMA
    0x0650: 0x00f0,	#  ARABIC KASRA
    0x0651: 0x00f1,	#  ARABIC SHADDA
    0x0652: 0x00f2,	#  ARABIC SUKUN
}