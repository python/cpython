""" Python Character Mapping Codec generated from '8859-4.TXT' with gencodec.py.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.
(c) Copyright 2000 Guido van Rossum.

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
        0x00a2: 0x0138, #       LATIN SMALL LETTER KRA
        0x00a3: 0x0156, #       LATIN CAPITAL LETTER R WITH CEDILLA
        0x00a5: 0x0128, #       LATIN CAPITAL LETTER I WITH TILDE
        0x00a6: 0x013b, #       LATIN CAPITAL LETTER L WITH CEDILLA
        0x00a9: 0x0160, #       LATIN CAPITAL LETTER S WITH CARON
        0x00aa: 0x0112, #       LATIN CAPITAL LETTER E WITH MACRON
        0x00ab: 0x0122, #       LATIN CAPITAL LETTER G WITH CEDILLA
        0x00ac: 0x0166, #       LATIN CAPITAL LETTER T WITH STROKE
        0x00ae: 0x017d, #       LATIN CAPITAL LETTER Z WITH CARON
        0x00b1: 0x0105, #       LATIN SMALL LETTER A WITH OGONEK
        0x00b2: 0x02db, #       OGONEK
        0x00b3: 0x0157, #       LATIN SMALL LETTER R WITH CEDILLA
        0x00b5: 0x0129, #       LATIN SMALL LETTER I WITH TILDE
        0x00b6: 0x013c, #       LATIN SMALL LETTER L WITH CEDILLA
        0x00b7: 0x02c7, #       CARON
        0x00b9: 0x0161, #       LATIN SMALL LETTER S WITH CARON
        0x00ba: 0x0113, #       LATIN SMALL LETTER E WITH MACRON
        0x00bb: 0x0123, #       LATIN SMALL LETTER G WITH CEDILLA
        0x00bc: 0x0167, #       LATIN SMALL LETTER T WITH STROKE
        0x00bd: 0x014a, #       LATIN CAPITAL LETTER ENG
        0x00be: 0x017e, #       LATIN SMALL LETTER Z WITH CARON
        0x00bf: 0x014b, #       LATIN SMALL LETTER ENG
        0x00c0: 0x0100, #       LATIN CAPITAL LETTER A WITH MACRON
        0x00c7: 0x012e, #       LATIN CAPITAL LETTER I WITH OGONEK
        0x00c8: 0x010c, #       LATIN CAPITAL LETTER C WITH CARON
        0x00ca: 0x0118, #       LATIN CAPITAL LETTER E WITH OGONEK
        0x00cc: 0x0116, #       LATIN CAPITAL LETTER E WITH DOT ABOVE
        0x00cf: 0x012a, #       LATIN CAPITAL LETTER I WITH MACRON
        0x00d0: 0x0110, #       LATIN CAPITAL LETTER D WITH STROKE
        0x00d1: 0x0145, #       LATIN CAPITAL LETTER N WITH CEDILLA
        0x00d2: 0x014c, #       LATIN CAPITAL LETTER O WITH MACRON
        0x00d3: 0x0136, #       LATIN CAPITAL LETTER K WITH CEDILLA
        0x00d9: 0x0172, #       LATIN CAPITAL LETTER U WITH OGONEK
        0x00dd: 0x0168, #       LATIN CAPITAL LETTER U WITH TILDE
        0x00de: 0x016a, #       LATIN CAPITAL LETTER U WITH MACRON
        0x00e0: 0x0101, #       LATIN SMALL LETTER A WITH MACRON
        0x00e7: 0x012f, #       LATIN SMALL LETTER I WITH OGONEK
        0x00e8: 0x010d, #       LATIN SMALL LETTER C WITH CARON
        0x00ea: 0x0119, #       LATIN SMALL LETTER E WITH OGONEK
        0x00ec: 0x0117, #       LATIN SMALL LETTER E WITH DOT ABOVE
        0x00ef: 0x012b, #       LATIN SMALL LETTER I WITH MACRON
        0x00f0: 0x0111, #       LATIN SMALL LETTER D WITH STROKE
        0x00f1: 0x0146, #       LATIN SMALL LETTER N WITH CEDILLA
        0x00f2: 0x014d, #       LATIN SMALL LETTER O WITH MACRON
        0x00f3: 0x0137, #       LATIN SMALL LETTER K WITH CEDILLA
        0x00f9: 0x0173, #       LATIN SMALL LETTER U WITH OGONEK
        0x00fd: 0x0169, #       LATIN SMALL LETTER U WITH TILDE
        0x00fe: 0x016b, #       LATIN SMALL LETTER U WITH MACRON
        0x00ff: 0x02d9, #       DOT ABOVE
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
