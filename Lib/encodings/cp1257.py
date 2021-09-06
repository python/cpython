""" Python Character Mapping Codec cp1257 generated from 'WindowsBestFit/cp1257.txt' with gencodec.py.

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self, input, errors='strict'):
        return codecs.charmap_encode(input, errors, encoding_table)

    def decode(self, input, errors='strict'):
        return codecs.charmap_decode(input, errors, decoding_table)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return codecs.charmap_encode(input, self.errors, encoding_table)[0]

class IncrementalDecoder(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        return codecs.charmap_decode(input, self.errors, decoding_table)[0]

class StreamWriter(Codec, codecs.StreamWriter):
    pass

class StreamReader(Codec, codecs.StreamReader):
    pass

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='cp1257',
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )


### Decoding Table

decoding_table = (
    '\x00'      #  0x00 -> NULL
    '\x01'      #  0x01 -> START OF HEADING
    '\x02'      #  0x02 -> START OF TEXT
    '\x03'      #  0x03 -> END OF TEXT
    '\x04'      #  0x04 -> END OF TRANSMISSION
    '\x05'      #  0x05 -> ENQUIRY
    '\x06'      #  0x06 -> ACKNOWLEDGE
    '\x07'      #  0x07 -> BELL
    '\x08'      #  0x08 -> BACKSPACE
    '\t'        #  0x09 -> HORIZONTAL TABULATION
    '\n'        #  0x0A -> LINE FEED
    '\x0b'      #  0x0B -> VERTICAL TABULATION
    '\x0c'      #  0x0C -> FORM FEED
    '\r'        #  0x0D -> CARRIAGE RETURN
    '\x0e'      #  0x0E -> SHIFT OUT
    '\x0f'      #  0x0F -> SHIFT IN
    '\x10'      #  0x10 -> DATA LINK ESCAPE
    '\x11'      #  0x11 -> DEVICE CONTROL ONE
    '\x12'      #  0x12 -> DEVICE CONTROL TWO
    '\x13'      #  0x13 -> DEVICE CONTROL THREE
    '\x14'      #  0x14 -> DEVICE CONTROL FOUR
    '\x15'      #  0x15 -> NEGATIVE ACKNOWLEDGE
    '\x16'      #  0x16 -> SYNCHRONOUS IDLE
    '\x17'      #  0x17 -> END OF TRANSMISSION BLOCK
    '\x18'      #  0x18 -> CANCEL
    '\x19'      #  0x19 -> END OF MEDIUM
    '\x1a'      #  0x1A -> SUBSTITUTE
    '\x1b'      #  0x1B -> ESCAPE
    '\x1c'      #  0x1C -> FILE SEPARATOR
    '\x1d'      #  0x1D -> GROUP SEPARATOR
    '\x1e'      #  0x1E -> RECORD SEPARATOR
    '\x1f'      #  0x1F -> UNIT SEPARATOR
    ' '         #  0x20 -> SPACE
    '!'         #  0x21 -> EXCLAMATION MARK
    '"'         #  0x22 -> QUOTATION MARK
    '#'         #  0x23 -> NUMBER SIGN
    '$'         #  0x24 -> DOLLAR SIGN
    '%'         #  0x25 -> PERCENT SIGN
    '&'         #  0x26 -> AMPERSAND
    "'"         #  0x27 -> APOSTROPHE
    '('         #  0x28 -> LEFT PARENTHESIS
    ')'         #  0x29 -> RIGHT PARENTHESIS
    '*'         #  0x2A -> ASTERISK
    '+'         #  0x2B -> PLUS SIGN
    ','         #  0x2C -> COMMA
    '-'         #  0x2D -> HYPHEN-MINUS
    '.'         #  0x2E -> FULL STOP
    '/'         #  0x2F -> SOLIDUS
    '0'         #  0x30 -> DIGIT 0
    '1'         #  0x31 -> DIGIT 1
    '2'         #  0x32 -> DIGIT 2
    '3'         #  0x33 -> DIGIT 3
    '4'         #  0x34 -> DIGIT 4
    '5'         #  0x35 -> DIGIT 5
    '6'         #  0x36 -> DIGIT 6
    '7'         #  0x37 -> DIGIT 7
    '8'         #  0x38 -> DIGIT 8
    '9'         #  0x39 -> DIGIT 9
    ':'         #  0x3A -> COLON
    ';'         #  0x3B -> SEMICOLON
    '<'         #  0x3C -> LESS-THAN SIGN
    '='         #  0x3D -> EQUALS SIGN
    '>'         #  0x3E -> GREATER-THAN SIGN
    '?'         #  0x3F -> QUESTION MARK
    '@'         #  0x40 -> COMMERCIAL AT
    'A'         #  0x41 -> A
    'B'         #  0x42 -> B
    'C'         #  0x43 -> C
    'D'         #  0x44 -> D
    'E'         #  0x45 -> E
    'F'         #  0x46 -> F
    'G'         #  0x47 -> G
    'H'         #  0x48 -> H
    'I'         #  0x49 -> I
    'J'         #  0x4A -> J
    'K'         #  0x4B -> K
    'L'         #  0x4C -> L
    'M'         #  0x4D -> M
    'N'         #  0x4E -> N
    'O'         #  0x4F -> O
    'P'         #  0x50 -> P
    'Q'         #  0x51 -> Q
    'R'         #  0x52 -> R
    'S'         #  0x53 -> S
    'T'         #  0x54 -> T
    'U'         #  0x55 -> U
    'V'         #  0x56 -> V
    'W'         #  0x57 -> W
    'X'         #  0x58 -> X
    'Y'         #  0x59 -> Y
    'Z'         #  0x5A -> Z
    '['         #  0x5B -> LEFT SQUARE BRACKET
    '\\'        #  0x5C -> BACKSLASH
    ']'         #  0x5D -> RIGHT SQUARE BRACKET
    '^'         #  0x5E -> CIRCUMFLEX
    '_'         #  0x5F -> LOW LINE
    '`'         #  0x60 -> GRAVE
    'a'         #  0x61 -> #A
    'b'         #  0x62 -> #B
    'c'         #  0x63 -> #C
    'd'         #  0x64 -> #D
    'e'         #  0x65 -> #E
    'f'         #  0x66 -> #F
    'g'         #  0x67 -> #G
    'h'         #  0x68 -> #H
    'i'         #  0x69 -> #I
    'j'         #  0x6A -> #J
    'k'         #  0x6B -> #K
    'l'         #  0x6C -> #L
    'm'         #  0x6D -> #M
    'n'         #  0x6E -> #N
    'o'         #  0x6F -> #O
    'p'         #  0x70 -> #P
    'q'         #  0x71 -> #Q
    'r'         #  0x72 -> #R
    's'         #  0x73 -> #S
    't'         #  0x74 -> #T
    'u'         #  0x75 -> #U
    'v'         #  0x76 -> #V
    'w'         #  0x77 -> #W
    'x'         #  0x78 -> #X
    'y'         #  0x79 -> #Y
    'z'         #  0x7A -> #Z
    '{'         #  0x7B -> LEFT CURLY BRACKET
    '|'         #  0x7C -> VERTICAL LINE
    '}'         #  0x7D -> RIGHT CURLY BRACKET
    '~'         #  0x7E -> TILDE
    '\x7f'      #  0x7F -> DELETE
    '\u20ac'    #  0x80 -> EURO SIGN
    '\x81'
    '\u201a'    #  0x82 -> LOW SINGLE COMMA QUOTATION MARK
    '\x83'      #  0x83 -> NOT USED
    '\u201e'    #  0x84 -> LOW DOUBLE COMMA QUOTATION MARK
    '\u2026'    #  0x85 -> HORIZONTAL ELLIPSIS
    '\u2020'    #  0x86 -> DAGGER
    '\u2021'    #  0x87 -> DOUBLE DAGGER
    '\x88'
    '\u2030'    #  0x89 -> PER MILLE SIGN
    '\x8a'
    '\u2039'    #  0x8B -> LEFT POINTING SINGLE GUILLEMENT
    '\x8c'
    '\xa8'      #  0x8D -> DIAERESIS
    '\u02c7'    #  0x8E -> HACEK
    '\xb8'      #  0x8F -> CEDILLA
    '\x90'
    '\u2018'    #  0x91 -> LEFT SINGLE QUOTATION MARK
    '\u2019'    #  0x92 -> RIGHT SINGLE QUOTATION MARK
    '\u201c'    #  0x93 -> LEFT DOUBLE QUOTATION MARK
    '\u201d'    #  0x94 -> RIGHT DOUBLE QUOTATION MARK
    '\u2022'    #  0x95 -> BULLET
    '\u2013'    #  0x96 -> EN DASH
    '\u2014'    #  0x97 -> EM DASH
    '\x98'      #  0x98 -> NOT USED
    '\u2122'    #  0x99 -> TRADE MARK SIGN
    '\x9a'
    '\u203a'    #  0x9B -> RIGHT POINTING SINGLE GUILLEMENT
    '\x9c'
    '\xaf'      #  0x9D -> MACRON
    '\u02db'    #  0x9E -> OGONEK
    '\x9f'
    '\xa0'      #  0xA0 -> NO-BREAK SPACE
    '\uf8fc'    #  0xA1 -> UNDEFINED -> EUDC
    '\xa2'      #  0xA2 -> CENT SIGN
    '\xa3'      #  0xA3 -> POUND SIGN
    '\xa4'      #  0xA4 -> CURRENCY SIGN
    '\uf8fd'    #  0xA5 -> UNDEFINED -> EUDC
    '\xa6'      #  0xA6 -> BROKEN BAR
    '\xa7'      #  0xA7 -> SECTION SIGN
    '\xd8'      #  0xA8 -> O STROKE
    '\xa9'      #  0xA9 -> COPYRIGHT SIGN
    '\u0156'    #  0xAA -> R CEDILLA
    '\xab'      #  0xAB -> LEFT POINTING GUILLEMENT
    '\xac'      #  0xAC -> NOT SIGN
    '\xad'      #  0xAD -> SOFT HYPHEN
    '\xae'      #  0xAE -> REGISTERED SIGN
    '\xc6'      #  0xAF -> AE
    '\xb0'      #  0xB0 -> DEGREE SIGN
    '\xb1'      #  0xB1 -> PLUS-MINUS SIGN
    '\xb2'      #  0xB2 -> SUPERSCRIPT 2
    '\xb3'      #  0xB3 -> SUPERSCRIPT 3
    '\xb4'      #  0xB4 -> ACUTE
    '\xb5'      #  0xB5 -> MICRO SIGN
    '\xb6'      #  0xB6 -> PILCROW SIGN
    '\xb7'      #  0xB7 -> MIDDLE DOT
    '\xf8'      #  0xB8 -> #O STROKE
    '\xb9'      #  0xB9 -> SUPERSCRIPT 1
    '\u0157'    #  0xBA -> #R CEDILLA
    '\xbb'      #  0xBB -> RIGHT POINTING GUILLEMENT
    '\xbc'      #  0xBC -> FRACTION 1/4
    '\xbd'      #  0xBD -> FRACTION 1/2
    '\xbe'      #  0xBE -> FRACTION 3/4
    '\xe6'      #  0xBF -> #AE
    '\u0104'    #  0xC0 -> A OGONEK
    '\u012e'    #  0xC1 -> I OGONEK
    '\u0100'    #  0xC2 -> A MACRON
    '\u0106'    #  0xC3 -> C ACUTE
    '\xc4'      #  0xC4 -> A DIAERESIS
    '\xc5'      #  0xC5 -> A RING ABOVE
    '\u0118'    #  0xC6 -> E OGONEK
    '\u0112'    #  0xC7 -> E MACRON
    '\u010c'    #  0xC8 -> C HACEK
    '\xc9'      #  0xC9 -> E ACUTE
    '\u0179'    #  0xCA -> Z ACUTE
    '\u0116'    #  0xCB -> E DOT ABOVE
    '\u0122'    #  0xCC -> G CEDILLA
    '\u0136'    #  0xCD -> K CEDILLA
    '\u012a'    #  0xCE -> I MACRON
    '\u013b'    #  0xCF -> L CEDILLA
    '\u0160'    #  0xD0 -> S HACEK
    '\u0143'    #  0xD1 -> N ACUTE
    '\u0145'    #  0xD2 -> N CEDILLA
    '\xd3'      #  0xD3 -> O ACUTE
    '\u014c'    #  0xD4 -> O MACRON
    '\xd5'      #  0xD5 -> O TILDE
    '\xd6'      #  0xD6 -> O DIAERESIS
    '\xd7'      #  0xD7 -> MULTIPLICATION SIGN
    '\u0172'    #  0xD8 -> U OGONEK
    '\u0141'    #  0xD9 -> L STROKE
    '\u015a'    #  0xDA -> S ACUTE
    '\u016a'    #  0xDB -> U MACRON
    '\xdc'      #  0xDC -> U DIAERESIS
    '\u017b'    #  0xDD -> Z DOT ABOVE
    '\u017d'    #  0xDE -> Z HACEK
    '\xdf'      #  0xDF -> SHARP SS
    '\u0105'    #  0xE0 -> #A OGONEK
    '\u012f'    #  0xE1 -> #I OGONEK
    '\u0101'    #  0xE2 -> #A MACRON
    '\u0107'    #  0xE3 -> #C ACUTE
    '\xe4'      #  0xE4 -> #A DIAERESIS
    '\xe5'      #  0xE5 -> #A RING ABOVE
    '\u0119'    #  0xE6 -> #E OGONEK
    '\u0113'    #  0xE7 -> #E MACRON
    '\u010d'    #  0xE8 -> #C HACEK
    '\xe9'      #  0xE9 -> #E ACUTE
    '\u017a'    #  0xEA -> #Z ACUTE
    '\u0117'    #  0xEB -> #E DOT ABOVE
    '\u0123'    #  0xEC -> #G CEDILLA
    '\u0137'    #  0xED -> #K CEDILLA
    '\u012b'    #  0xEE -> #I MACRON
    '\u013c'    #  0xEF -> #L CEDILLA
    '\u0161'    #  0xF0 -> #S HACEK
    '\u0144'    #  0xF1 -> #N ACUTE
    '\u0146'    #  0xF2 -> #N CEDILLA
    '\xf3'      #  0xF3 -> #O ACUTE
    '\u014d'    #  0xF4 -> #O MACRON
    '\xf5'      #  0xF5 -> #O TILDE
    '\xf6'      #  0xF6 -> #O DIAERESIS
    '\xf7'      #  0xF7 -> DIVISION SIGN
    '\u0173'    #  0xF8 -> #U OGONEK
    '\u0142'    #  0xF9 -> #L STROKE
    '\u015b'    #  0xFA -> #S ACUTE
    '\u016b'    #  0xFB -> #U MACRON
    '\xfc'      #  0xFC -> #U DIAERESIS
    '\u017c'    #  0xFD -> #Z DOT ABOVE
    '\u017e'    #  0xFE -> #Z HACEK
    '\u02d9'    #  0xFF -> DOT ABOVE
)

### Encoding table
encoding_table = codecs.charmap_build(decoding_table)

