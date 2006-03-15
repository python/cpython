""" Python Character Mapping Codec iso8859_6 generated from 'MAPPINGS/ISO8859/8859-6.TXT' with gencodec.py.

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
        name='iso8859-6',
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
    u'\x80'     #  0x80 -> <control>
    u'\x81'     #  0x81 -> <control>
    u'\x82'     #  0x82 -> <control>
    u'\x83'     #  0x83 -> <control>
    u'\x84'     #  0x84 -> <control>
    u'\x85'     #  0x85 -> <control>
    u'\x86'     #  0x86 -> <control>
    u'\x87'     #  0x87 -> <control>
    u'\x88'     #  0x88 -> <control>
    u'\x89'     #  0x89 -> <control>
    u'\x8a'     #  0x8A -> <control>
    u'\x8b'     #  0x8B -> <control>
    u'\x8c'     #  0x8C -> <control>
    u'\x8d'     #  0x8D -> <control>
    u'\x8e'     #  0x8E -> <control>
    u'\x8f'     #  0x8F -> <control>
    u'\x90'     #  0x90 -> <control>
    u'\x91'     #  0x91 -> <control>
    u'\x92'     #  0x92 -> <control>
    u'\x93'     #  0x93 -> <control>
    u'\x94'     #  0x94 -> <control>
    u'\x95'     #  0x95 -> <control>
    u'\x96'     #  0x96 -> <control>
    u'\x97'     #  0x97 -> <control>
    u'\x98'     #  0x98 -> <control>
    u'\x99'     #  0x99 -> <control>
    u'\x9a'     #  0x9A -> <control>
    u'\x9b'     #  0x9B -> <control>
    u'\x9c'     #  0x9C -> <control>
    u'\x9d'     #  0x9D -> <control>
    u'\x9e'     #  0x9E -> <control>
    u'\x9f'     #  0x9F -> <control>
    u'\xa0'     #  0xA0 -> NO-BREAK SPACE
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\xa4'     #  0xA4 -> CURRENCY SIGN
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u060c'   #  0xAC -> ARABIC COMMA
    u'\xad'     #  0xAD -> SOFT HYPHEN
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
    u'\u061b'   #  0xBB -> ARABIC SEMICOLON
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u061f'   #  0xBF -> ARABIC QUESTION MARK
    u'\ufffe'
    u'\u0621'   #  0xC1 -> ARABIC LETTER HAMZA
    u'\u0622'   #  0xC2 -> ARABIC LETTER ALEF WITH MADDA ABOVE
    u'\u0623'   #  0xC3 -> ARABIC LETTER ALEF WITH HAMZA ABOVE
    u'\u0624'   #  0xC4 -> ARABIC LETTER WAW WITH HAMZA ABOVE
    u'\u0625'   #  0xC5 -> ARABIC LETTER ALEF WITH HAMZA BELOW
    u'\u0626'   #  0xC6 -> ARABIC LETTER YEH WITH HAMZA ABOVE
    u'\u0627'   #  0xC7 -> ARABIC LETTER ALEF
    u'\u0628'   #  0xC8 -> ARABIC LETTER BEH
    u'\u0629'   #  0xC9 -> ARABIC LETTER TEH MARBUTA
    u'\u062a'   #  0xCA -> ARABIC LETTER TEH
    u'\u062b'   #  0xCB -> ARABIC LETTER THEH
    u'\u062c'   #  0xCC -> ARABIC LETTER JEEM
    u'\u062d'   #  0xCD -> ARABIC LETTER HAH
    u'\u062e'   #  0xCE -> ARABIC LETTER KHAH
    u'\u062f'   #  0xCF -> ARABIC LETTER DAL
    u'\u0630'   #  0xD0 -> ARABIC LETTER THAL
    u'\u0631'   #  0xD1 -> ARABIC LETTER REH
    u'\u0632'   #  0xD2 -> ARABIC LETTER ZAIN
    u'\u0633'   #  0xD3 -> ARABIC LETTER SEEN
    u'\u0634'   #  0xD4 -> ARABIC LETTER SHEEN
    u'\u0635'   #  0xD5 -> ARABIC LETTER SAD
    u'\u0636'   #  0xD6 -> ARABIC LETTER DAD
    u'\u0637'   #  0xD7 -> ARABIC LETTER TAH
    u'\u0638'   #  0xD8 -> ARABIC LETTER ZAH
    u'\u0639'   #  0xD9 -> ARABIC LETTER AIN
    u'\u063a'   #  0xDA -> ARABIC LETTER GHAIN
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\ufffe'
    u'\u0640'   #  0xE0 -> ARABIC TATWEEL
    u'\u0641'   #  0xE1 -> ARABIC LETTER FEH
    u'\u0642'   #  0xE2 -> ARABIC LETTER QAF
    u'\u0643'   #  0xE3 -> ARABIC LETTER KAF
    u'\u0644'   #  0xE4 -> ARABIC LETTER LAM
    u'\u0645'   #  0xE5 -> ARABIC LETTER MEEM
    u'\u0646'   #  0xE6 -> ARABIC LETTER NOON
    u'\u0647'   #  0xE7 -> ARABIC LETTER HEH
    u'\u0648'   #  0xE8 -> ARABIC LETTER WAW
    u'\u0649'   #  0xE9 -> ARABIC LETTER ALEF MAKSURA
    u'\u064a'   #  0xEA -> ARABIC LETTER YEH
    u'\u064b'   #  0xEB -> ARABIC FATHATAN
    u'\u064c'   #  0xEC -> ARABIC DAMMATAN
    u'\u064d'   #  0xED -> ARABIC KASRATAN
    u'\u064e'   #  0xEE -> ARABIC FATHA
    u'\u064f'   #  0xEF -> ARABIC DAMMA
    u'\u0650'   #  0xF0 -> ARABIC KASRA
    u'\u0651'   #  0xF1 -> ARABIC SHADDA
    u'\u0652'   #  0xF2 -> ARABIC SUKUN
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
    0x0080: 0x80,       #  <control>
    0x0081: 0x81,       #  <control>
    0x0082: 0x82,       #  <control>
    0x0083: 0x83,       #  <control>
    0x0084: 0x84,       #  <control>
    0x0085: 0x85,       #  <control>
    0x0086: 0x86,       #  <control>
    0x0087: 0x87,       #  <control>
    0x0088: 0x88,       #  <control>
    0x0089: 0x89,       #  <control>
    0x008A: 0x8A,       #  <control>
    0x008B: 0x8B,       #  <control>
    0x008C: 0x8C,       #  <control>
    0x008D: 0x8D,       #  <control>
    0x008E: 0x8E,       #  <control>
    0x008F: 0x8F,       #  <control>
    0x0090: 0x90,       #  <control>
    0x0091: 0x91,       #  <control>
    0x0092: 0x92,       #  <control>
    0x0093: 0x93,       #  <control>
    0x0094: 0x94,       #  <control>
    0x0095: 0x95,       #  <control>
    0x0096: 0x96,       #  <control>
    0x0097: 0x97,       #  <control>
    0x0098: 0x98,       #  <control>
    0x0099: 0x99,       #  <control>
    0x009A: 0x9A,       #  <control>
    0x009B: 0x9B,       #  <control>
    0x009C: 0x9C,       #  <control>
    0x009D: 0x9D,       #  <control>
    0x009E: 0x9E,       #  <control>
    0x009F: 0x9F,       #  <control>
    0x00A0: 0xA0,       #  NO-BREAK SPACE
    0x00A4: 0xA4,       #  CURRENCY SIGN
    0x00AD: 0xAD,       #  SOFT HYPHEN
    0x060C: 0xAC,       #  ARABIC COMMA
    0x061B: 0xBB,       #  ARABIC SEMICOLON
    0x061F: 0xBF,       #  ARABIC QUESTION MARK
    0x0621: 0xC1,       #  ARABIC LETTER HAMZA
    0x0622: 0xC2,       #  ARABIC LETTER ALEF WITH MADDA ABOVE
    0x0623: 0xC3,       #  ARABIC LETTER ALEF WITH HAMZA ABOVE
    0x0624: 0xC4,       #  ARABIC LETTER WAW WITH HAMZA ABOVE
    0x0625: 0xC5,       #  ARABIC LETTER ALEF WITH HAMZA BELOW
    0x0626: 0xC6,       #  ARABIC LETTER YEH WITH HAMZA ABOVE
    0x0627: 0xC7,       #  ARABIC LETTER ALEF
    0x0628: 0xC8,       #  ARABIC LETTER BEH
    0x0629: 0xC9,       #  ARABIC LETTER TEH MARBUTA
    0x062A: 0xCA,       #  ARABIC LETTER TEH
    0x062B: 0xCB,       #  ARABIC LETTER THEH
    0x062C: 0xCC,       #  ARABIC LETTER JEEM
    0x062D: 0xCD,       #  ARABIC LETTER HAH
    0x062E: 0xCE,       #  ARABIC LETTER KHAH
    0x062F: 0xCF,       #  ARABIC LETTER DAL
    0x0630: 0xD0,       #  ARABIC LETTER THAL
    0x0631: 0xD1,       #  ARABIC LETTER REH
    0x0632: 0xD2,       #  ARABIC LETTER ZAIN
    0x0633: 0xD3,       #  ARABIC LETTER SEEN
    0x0634: 0xD4,       #  ARABIC LETTER SHEEN
    0x0635: 0xD5,       #  ARABIC LETTER SAD
    0x0636: 0xD6,       #  ARABIC LETTER DAD
    0x0637: 0xD7,       #  ARABIC LETTER TAH
    0x0638: 0xD8,       #  ARABIC LETTER ZAH
    0x0639: 0xD9,       #  ARABIC LETTER AIN
    0x063A: 0xDA,       #  ARABIC LETTER GHAIN
    0x0640: 0xE0,       #  ARABIC TATWEEL
    0x0641: 0xE1,       #  ARABIC LETTER FEH
    0x0642: 0xE2,       #  ARABIC LETTER QAF
    0x0643: 0xE3,       #  ARABIC LETTER KAF
    0x0644: 0xE4,       #  ARABIC LETTER LAM
    0x0645: 0xE5,       #  ARABIC LETTER MEEM
    0x0646: 0xE6,       #  ARABIC LETTER NOON
    0x0647: 0xE7,       #  ARABIC LETTER HEH
    0x0648: 0xE8,       #  ARABIC LETTER WAW
    0x0649: 0xE9,       #  ARABIC LETTER ALEF MAKSURA
    0x064A: 0xEA,       #  ARABIC LETTER YEH
    0x064B: 0xEB,       #  ARABIC FATHATAN
    0x064C: 0xEC,       #  ARABIC DAMMATAN
    0x064D: 0xED,       #  ARABIC KASRATAN
    0x064E: 0xEE,       #  ARABIC FATHA
    0x064F: 0xEF,       #  ARABIC DAMMA
    0x0650: 0xF0,       #  ARABIC KASRA
    0x0651: 0xF1,       #  ARABIC SHADDA
    0x0652: 0xF2,       #  ARABIC SUKUN
}
