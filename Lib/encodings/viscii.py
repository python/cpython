"""Python Character Mapping Codec viscii generated from 'python-mappings/VISCII.TXT' with gencodec.py."""  # "

import codecs

### Codec APIs


class Codec(codecs.Codec):
    def encode(self, input, errors="strict"):
        return codecs.charmap_encode(input, errors, encoding_table)

    def decode(self, input, errors="strict"):
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
        name="viscii",
        encode=Codec().encode,
        decode=Codec().decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )


### Decoding Table

decoding_table = (
    "\x00"  #  0x00 -> NUL
    "\x01"  #  0x01 -> SOH
    "\u1eb2"  #  0x02 -> Ẳ
    "\x03"  #  0x03 -> ETX
    "\x04"  #  0x04 -> EOT
    "\u1eb4"  #  0x05 -> Ẵ
    "\u1eaa"  #  0x06 -> Ẫ
    "\x07"  #  0x07 -> BEL
    "\x08"  #  0x08 -> BS
    "\t"  #  0x09 -> TAB
    "\n"  #  0x0A -> LF
    "\x0b"  #  0x0B -> VT
    "\x0c"  #  0x0C -> FF
    "\r"  #  0x0D -> CR
    "\x0e"  #  0x0E -> SO
    "\x0f"  #  0x0F -> SI
    "\x10"  #  0x10 -> DLE
    "\x11"  #  0x11 -> DC1
    "\x12"  #  0x12 -> DC2
    "\x13"  #  0x13 -> DC3
    "\u1ef6"  #  0x14 -> Ỷ
    "\x15"  #  0x15 -> NAK
    "\x16"  #  0x16 -> SYN
    "\x17"  #  0x17 -> ETB
    "\x18"  #  0x18 -> CAN
    "\u1ef8"  #  0x19 -> Ỹ
    "\x1a"  #  0x1A -> SUB
    "\x1b"  #  0x1B -> ESC
    "\x1c"  #  0x1C -> FS
    "\x1d"  #  0x1D -> GS
    "\u1ef4"  #  0x1E -> Ỵ
    "\x1f"  #  0x1F -> US
    " "  #  0x20 -> SPACE
    "!"  #  0x21 -> !
    '"'  #  0x22 -> "
    "#"  #  0x23 -> #
    "$"  #  0x24 -> $
    "%"  #  0x25 -> %
    "&"  #  0x26 -> &
    "'"  #  0x27 -> '
    "("  #  0x28 -> (
    ")"  #  0x29 -> )
    "*"  #  0x2A -> *
    "+"  #  0x2B -> +
    ","  #  0x2C -> ,
    "-"  #  0x2D -> -
    "."  #  0x2E -> .
    "/"  #  0x2F -> /
    "0"  #  0x30 -> 0
    "1"  #  0x31 -> 1
    "2"  #  0x32 -> 2
    "3"  #  0x33 -> 3
    "4"  #  0x34 -> 4
    "5"  #  0x35 -> 5
    "6"  #  0x36 -> 6
    "7"  #  0x37 -> 7
    "8"  #  0x38 -> 8
    "9"  #  0x39 -> 9
    ":"  #  0x3A -> :
    ";"  #  0x3B -> ;
    "<"  #  0x3C -> <
    "="  #  0x3D -> =
    ">"  #  0x3E -> >
    "?"  #  0x3F -> ?
    "@"  #  0x40 -> @
    "A"  #  0x41 -> A
    "B"  #  0x42 -> B
    "C"  #  0x43 -> C
    "D"  #  0x44 -> D
    "E"  #  0x45 -> E
    "F"  #  0x46 -> F
    "G"  #  0x47 -> G
    "H"  #  0x48 -> H
    "I"  #  0x49 -> I
    "J"  #  0x4A -> J
    "K"  #  0x4B -> K
    "L"  #  0x4C -> L
    "M"  #  0x4D -> M
    "N"  #  0x4E -> N
    "O"  #  0x4F -> O
    "P"  #  0x50 -> P
    "Q"  #  0x51 -> Q
    "R"  #  0x52 -> R
    "S"  #  0x53 -> S
    "T"  #  0x54 -> T
    "U"  #  0x55 -> U
    "V"  #  0x56 -> V
    "W"  #  0x57 -> W
    "X"  #  0x58 -> X
    "Y"  #  0x59 -> Y
    "Z"  #  0x5A -> Z
    "["  #  0x5B -> [
    "\\"  #  0x5C -> \
    "]"  #  0x5D -> ]
    "^"  #  0x5E -> ^
    "_"  #  0x5F -> _
    "`"  #  0x60 -> `
    "a"  #  0x61 -> a
    "b"  #  0x62 -> b
    "c"  #  0x63 -> c
    "d"  #  0x64 -> d
    "e"  #  0x65 -> e
    "f"  #  0x66 -> f
    "g"  #  0x67 -> g
    "h"  #  0x68 -> h
    "i"  #  0x69 -> i
    "j"  #  0x6A -> j
    "k"  #  0x6B -> k
    "l"  #  0x6C -> l
    "m"  #  0x6D -> m
    "n"  #  0x6E -> n
    "o"  #  0x6F -> o
    "p"  #  0x70 -> p
    "q"  #  0x71 -> q
    "r"  #  0x72 -> r
    "s"  #  0x73 -> s
    "t"  #  0x74 -> t
    "u"  #  0x75 -> u
    "v"  #  0x76 -> v
    "w"  #  0x77 -> w
    "x"  #  0x78 -> x
    "y"  #  0x79 -> y
    "z"  #  0x7A -> z
    "{"  #  0x7B -> {
    "|"  #  0x7C -> |
    "}"  #  0x7D -> }
    "~"  #  0x7E -> ~
    "\x7f"  #  0x7F -> DEL
    "\u1ea0"  #  0x80 -> Ạ
    "\u1eae"  #  0x81 -> Ắ
    "\u1eb0"  #  0x82 -> Ằ
    "\u1eb6"  #  0x83 -> Ặ
    "\u1ea4"  #  0x84 -> Ấ
    "\u1ea6"  #  0x85 -> Ầ
    "\u1ea8"  #  0x86 -> Ẩ
    "\u1eac"  #  0x87 -> Ậ
    "\u1ebc"  #  0x88 -> Ẽ
    "\u1eb8"  #  0x89 -> Ẹ
    "\u1ebe"  #  0x8A -> Ế
    "\u1ec0"  #  0x8B -> Ề
    "\u1ec2"  #  0x8C -> Ể
    "\u1ec4"  #  0x8D -> Ễ
    "\u1ec6"  #  0x8E -> Ệ
    "\u1ed0"  #  0x8F -> Ố
    "\u1ed2"  #  0x90 -> Ồ
    "\u1ed4"  #  0x91 -> Ổ
    "\u1ed6"  #  0x92 -> Ỗ
    "\u1ed8"  #  0x93 -> Ộ
    "\u1ee2"  #  0x94 -> Ợ
    "\u1eda"  #  0x95 -> Ớ
    "\u1edc"  #  0x96 -> Ờ
    "\u1ede"  #  0x97 -> Ở
    "\u1eca"  #  0x98 -> Ị
    "\u1ece"  #  0x99 -> Ỏ
    "\u1ecc"  #  0x9A -> Ọ
    "\u1ec8"  #  0x9B -> Ỉ
    "\u1ee6"  #  0x9C -> Ủ
    "\u0168"  #  0x9D -> Ũ
    "\u1ee4"  #  0x9E -> Ụ
    "\u1ef2"  #  0x9F -> Ỳ
    "\xd5"  #  0xA0 -> Õ
    "\u1eaf"  #  0xA1 -> ắ
    "\u1eb1"  #  0xA2 -> ằ
    "\u1eb7"  #  0xA3 -> ặ
    "\u1ea5"  #  0xA4 -> ấ
    "\u1ea7"  #  0xA5 -> ầ
    "\u1ea9"  #  0xA6 -> ẩ
    "\u1ead"  #  0xA7 -> ậ
    "\u1ebd"  #  0xA8 -> ẽ
    "\u1eb9"  #  0xA9 -> ẹ
    "\u1ebf"  #  0xAA -> ế
    "\u1ec1"  #  0xAB -> ề
    "\u1ec3"  #  0xAC -> ể
    "\u1ec5"  #  0xAD -> ễ
    "\u1ec7"  #  0xAE -> ệ
    "\u1ed1"  #  0xAF -> ố
    "\u1ed3"  #  0xB0 -> ồ
    "\u1ed5"  #  0xB1 -> ổ
    "\u1ed7"  #  0xB2 -> ỗ
    "\u1ee0"  #  0xB3 -> Ỡ
    "\u01a0"  #  0xB4 -> Ơ
    "\u1ed9"  #  0xB5 -> ộ
    "\u1edd"  #  0xB6 -> ờ
    "\u1edf"  #  0xB7 -> ở
    "\u1ecb"  #  0xB8 -> ị
    "\u1ef0"  #  0xB9 -> Ự
    "\u1ee8"  #  0xBA -> Ứ
    "\u1eea"  #  0xBB -> Ừ
    "\u1eec"  #  0xBC -> Ử
    "\u01a1"  #  0xBD -> ơ
    "\u1edb"  #  0xBE -> ớ
    "\u01af"  #  0xBF -> Ư
    "\xc0"  #  0xC0 -> À
    "\xc1"  #  0xC1 -> Á
    "\xc2"  #  0xC2 -> Â
    "\xc3"  #  0xC3 -> Ã
    "\u1ea2"  #  0xC4 -> Ả
    "\u0102"  #  0xC5 -> Ă
    "\u1eb3"  #  0xC6 -> ẳ
    "\u1eb5"  #  0xC7 -> ẵ
    "\xc8"  #  0xC8 -> È
    "\xc9"  #  0xC9 -> É
    "\xca"  #  0xCA -> Ê
    "\u1eba"  #  0xCB -> Ẻ
    "\xcc"  #  0xCC -> Ì
    "\xcd"  #  0xCD -> Í
    "\u0128"  #  0xCE -> Ĩ
    "\u1ef3"  #  0xCF -> ỳ
    "\u0110"  #  0xD0 -> Đ
    "\u1ee9"  #  0xD1 -> ứ
    "\xd2"  #  0xD2 -> Ò
    "\xd3"  #  0xD3 -> Ó
    "\xd4"  #  0xD4 -> Ô
    "\u1ea1"  #  0xD5 -> ạ
    "\u1ef7"  #  0xD6 -> ỷ
    "\u1eeb"  #  0xD7 -> ừ
    "\u1eed"  #  0xD8 -> ử
    "\xd9"  #  0xD9 -> Ù
    "\xda"  #  0xDA -> Ú
    "\u1ef9"  #  0xDB -> ỹ
    "\u1ef5"  #  0xDC -> ỵ
    "\xdd"  #  0xDD -> Ý
    "\u1ee1"  #  0xDE -> ỡ
    "\u01b0"  #  0xDF -> ư
    "\xe0"  #  0xE0 -> à
    "\xe1"  #  0xE1 -> á
    "\xe2"  #  0xE2 -> â
    "\xe3"  #  0xE3 -> ã
    "\u1ea3"  #  0xE4 -> ả
    "\u0103"  #  0xE5 -> ă
    "\u1eef"  #  0xE6 -> ữ
    "\u1eab"  #  0xE7 -> ẫ
    "\xe8"  #  0xE8 -> è
    "\xe9"  #  0xE9 -> é
    "\xea"  #  0xEA -> ê
    "\u1ebb"  #  0xEB -> ẻ
    "\xec"  #  0xEC -> ì
    "\xed"  #  0xED -> í
    "\u0129"  #  0xEE -> ĩ
    "\u1ec9"  #  0xEF -> ỉ
    "\u0111"  #  0xF0 -> đ
    "\u1ef1"  #  0xF1 -> ự
    "\xf2"  #  0xF2 -> ò
    "\xf3"  #  0xF3 -> ó
    "\xf4"  #  0xF4 -> ô
    "\xf5"  #  0xF5 -> õ
    "\u1ecf"  #  0xF6 -> ỏ
    "\u1ecd"  #  0xF7 -> ọ
    "\u1ee5"  #  0xF8 -> ụ
    "\xf9"  #  0xF9 -> ù
    "\xfa"  #  0xFA -> ú
    "\u0169"  #  0xFB -> ũ
    "\u1ee7"  #  0xFC -> ủ
    "\xfd"  #  0xFD -> ý
    "\u1ee3"  #  0xFE -> ợ
    "\u1eee"  #  0xFF -> Ữ
)

### Encoding table
encoding_table = codecs.charmap_build(decoding_table)
