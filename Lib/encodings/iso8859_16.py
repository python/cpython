""" Python Character Mapping Codec generated from '8859-16.TXT' with gencodec.py.

    Generated from mapping found in
    ftp://ftp.unicode.org/Public/MAPPINGS/ISO8859/8859-16.TXT

"""#"

import codecs

### Codec APIs

class Codec(codecs.Codec):

    def encode(self,input,errors='strict'):

        return codecs.charmap_encode(input,errors,encoding_map)

    def decode(self,input,errors='strict'):

        return codecs.charmap_decode(input,errors,decoding_map)

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
        0x00a1: 0x0104, #       LATIN CAPITAL LETTER A WITH OGONEK
        0x00a2: 0x0105, #       LATIN SMALL LETTER A WITH OGONEK
        0x00a3: 0x0141, #       LATIN CAPITAL LETTER L WITH STROKE
        0x00a4: 0x20ac, #       EURO SIGN
        0x00a5: 0x201e, #       DOUBLE LOW-9 QUOTATION MARK
        0x00a6: 0x0160, #       LATIN CAPITAL LETTER S WITH CARON
        0x00a8: 0x0161, #       LATIN SMALL LETTER S WITH CARON
        0x00aa: 0x0218, #       LATIN CAPITAL LETTER S WITH COMMA BELOW
        0x00ac: 0x0179, #       LATIN CAPITAL LETTER Z WITH ACUTE
        0x00ae: 0x017a, #       LATIN SMALL LETTER Z WITH ACUTE
        0x00af: 0x017b, #       LATIN CAPITAL LETTER Z WITH DOT ABOVE
        0x00b2: 0x010c, #       LATIN CAPITAL LETTER C WITH CARON
        0x00b3: 0x0142, #       LATIN SMALL LETTER L WITH STROKE
        0x00b4: 0x017d, #       LATIN CAPITAL LETTER Z WITH CARON
        0x00b5: 0x201d, #       RIGHT DOUBLE QUOTATION MARK
        0x00b8: 0x017e, #       LATIN SMALL LETTER Z WITH CARON
        0x00b9: 0x010d, #       LATIN SMALL LETTER C WITH CARON
        0x00ba: 0x0219, #       LATIN SMALL LETTER S WITH COMMA BELOW
        0x00bc: 0x0152, #       LATIN CAPITAL LIGATURE OE
        0x00bd: 0x0153, #       LATIN SMALL LIGATURE OE
        0x00be: 0x0178, #       LATIN CAPITAL LETTER Y WITH DIAERESIS
        0x00bf: 0x017c, #       LATIN SMALL LETTER Z WITH DOT ABOVE
        0x00c3: 0x0102, #       LATIN CAPITAL LETTER A WITH BREVE
        0x00c5: 0x0106, #       LATIN CAPITAL LETTER C WITH ACUTE
        0x00d0: 0x0110, #       LATIN CAPITAL LETTER D WITH STROKE
        0x00d1: 0x0143, #       LATIN CAPITAL LETTER N WITH ACUTE
        0x00d5: 0x0150, #       LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
        0x00d7: 0x015a, #       LATIN CAPITAL LETTER S WITH ACUTE
        0x00d8: 0x0170, #       LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
        0x00dd: 0x0118, #       LATIN CAPITAL LETTER E WITH OGONEK
        0x00de: 0x021a, #       LATIN CAPITAL LETTER T WITH COMMA BELOW
        0x00e3: 0x0103, #       LATIN SMALL LETTER A WITH BREVE
        0x00e5: 0x0107, #       LATIN SMALL LETTER C WITH ACUTE
        0x00f0: 0x0111, #       LATIN SMALL LETTER D WITH STROKE
        0x00f1: 0x0144, #       LATIN SMALL LETTER N WITH ACUTE
        0x00f5: 0x0151, #       LATIN SMALL LETTER O WITH DOUBLE ACUTE
        0x00f7: 0x015b, #       LATIN SMALL LETTER S WITH ACUTE
        0x00f8: 0x0171, #       LATIN SMALL LETTER U WITH DOUBLE ACUTE
        0x00fd: 0x0119, #       LATIN SMALL LETTER E WITH OGONEK
        0x00fe: 0x021b, #       LATIN SMALL LETTER T WITH COMMA BELOW
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
