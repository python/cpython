""" Python Character Mapping Codec generated from 'CP1257.TXT' with gencodec.py.

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
        0x0080: 0x20ac, # EURO SIGN
        0x0081: None,   # UNDEFINED
        0x0082: 0x201a, # SINGLE LOW-9 QUOTATION MARK
        0x0083: None,   # UNDEFINED
        0x0084: 0x201e, # DOUBLE LOW-9 QUOTATION MARK
        0x0085: 0x2026, # HORIZONTAL ELLIPSIS
        0x0086: 0x2020, # DAGGER
        0x0087: 0x2021, # DOUBLE DAGGER
        0x0088: None,   # UNDEFINED
        0x0089: 0x2030, # PER MILLE SIGN
        0x008a: None,   # UNDEFINED
        0x008b: 0x2039, # SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        0x008c: None,   # UNDEFINED
        0x008d: 0x00a8, # DIAERESIS
        0x008e: 0x02c7, # CARON
        0x008f: 0x00b8, # CEDILLA
        0x0090: None,   # UNDEFINED
        0x0091: 0x2018, # LEFT SINGLE QUOTATION MARK
        0x0092: 0x2019, # RIGHT SINGLE QUOTATION MARK
        0x0093: 0x201c, # LEFT DOUBLE QUOTATION MARK
        0x0094: 0x201d, # RIGHT DOUBLE QUOTATION MARK
        0x0095: 0x2022, # BULLET
        0x0096: 0x2013, # EN DASH
        0x0097: 0x2014, # EM DASH
        0x0098: None,   # UNDEFINED
        0x0099: 0x2122, # TRADE MARK SIGN
        0x009a: None,   # UNDEFINED
        0x009b: 0x203a, # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        0x009c: None,   # UNDEFINED
        0x009d: 0x00af, # MACRON
        0x009e: 0x02db, # OGONEK
        0x009f: None,   # UNDEFINED
        0x00a1: None,   # UNDEFINED
        0x00a5: None,   # UNDEFINED
        0x00a8: 0x00d8, # LATIN CAPITAL LETTER O WITH STROKE
        0x00aa: 0x0156, # LATIN CAPITAL LETTER R WITH CEDILLA
        0x00af: 0x00c6, # LATIN CAPITAL LETTER AE
        0x00b8: 0x00f8, # LATIN SMALL LETTER O WITH STROKE
        0x00ba: 0x0157, # LATIN SMALL LETTER R WITH CEDILLA
        0x00bf: 0x00e6, # LATIN SMALL LETTER AE
        0x00c0: 0x0104, # LATIN CAPITAL LETTER A WITH OGONEK
        0x00c1: 0x012e, # LATIN CAPITAL LETTER I WITH OGONEK
        0x00c2: 0x0100, # LATIN CAPITAL LETTER A WITH MACRON
        0x00c3: 0x0106, # LATIN CAPITAL LETTER C WITH ACUTE
        0x00c6: 0x0118, # LATIN CAPITAL LETTER E WITH OGONEK
        0x00c7: 0x0112, # LATIN CAPITAL LETTER E WITH MACRON
        0x00c8: 0x010c, # LATIN CAPITAL LETTER C WITH CARON
        0x00ca: 0x0179, # LATIN CAPITAL LETTER Z WITH ACUTE
        0x00cb: 0x0116, # LATIN CAPITAL LETTER E WITH DOT ABOVE
        0x00cc: 0x0122, # LATIN CAPITAL LETTER G WITH CEDILLA
        0x00cd: 0x0136, # LATIN CAPITAL LETTER K WITH CEDILLA
        0x00ce: 0x012a, # LATIN CAPITAL LETTER I WITH MACRON
        0x00cf: 0x013b, # LATIN CAPITAL LETTER L WITH CEDILLA
        0x00d0: 0x0160, # LATIN CAPITAL LETTER S WITH CARON
        0x00d1: 0x0143, # LATIN CAPITAL LETTER N WITH ACUTE
        0x00d2: 0x0145, # LATIN CAPITAL LETTER N WITH CEDILLA
        0x00d4: 0x014c, # LATIN CAPITAL LETTER O WITH MACRON
        0x00d8: 0x0172, # LATIN CAPITAL LETTER U WITH OGONEK
        0x00d9: 0x0141, # LATIN CAPITAL LETTER L WITH STROKE
        0x00da: 0x015a, # LATIN CAPITAL LETTER S WITH ACUTE
        0x00db: 0x016a, # LATIN CAPITAL LETTER U WITH MACRON
        0x00dd: 0x017b, # LATIN CAPITAL LETTER Z WITH DOT ABOVE
        0x00de: 0x017d, # LATIN CAPITAL LETTER Z WITH CARON
        0x00e0: 0x0105, # LATIN SMALL LETTER A WITH OGONEK
        0x00e1: 0x012f, # LATIN SMALL LETTER I WITH OGONEK
        0x00e2: 0x0101, # LATIN SMALL LETTER A WITH MACRON
        0x00e3: 0x0107, # LATIN SMALL LETTER C WITH ACUTE
        0x00e6: 0x0119, # LATIN SMALL LETTER E WITH OGONEK
        0x00e7: 0x0113, # LATIN SMALL LETTER E WITH MACRON
        0x00e8: 0x010d, # LATIN SMALL LETTER C WITH CARON
        0x00ea: 0x017a, # LATIN SMALL LETTER Z WITH ACUTE
        0x00eb: 0x0117, # LATIN SMALL LETTER E WITH DOT ABOVE
        0x00ec: 0x0123, # LATIN SMALL LETTER G WITH CEDILLA
        0x00ed: 0x0137, # LATIN SMALL LETTER K WITH CEDILLA
        0x00ee: 0x012b, # LATIN SMALL LETTER I WITH MACRON
        0x00ef: 0x013c, # LATIN SMALL LETTER L WITH CEDILLA
        0x00f0: 0x0161, # LATIN SMALL LETTER S WITH CARON
        0x00f1: 0x0144, # LATIN SMALL LETTER N WITH ACUTE
        0x00f2: 0x0146, # LATIN SMALL LETTER N WITH CEDILLA
        0x00f4: 0x014d, # LATIN SMALL LETTER O WITH MACRON
        0x00f8: 0x0173, # LATIN SMALL LETTER U WITH OGONEK
        0x00f9: 0x0142, # LATIN SMALL LETTER L WITH STROKE
        0x00fa: 0x015b, # LATIN SMALL LETTER S WITH ACUTE
        0x00fb: 0x016b, # LATIN SMALL LETTER U WITH MACRON
        0x00fd: 0x017c, # LATIN SMALL LETTER Z WITH DOT ABOVE
        0x00fe: 0x017e, # LATIN SMALL LETTER Z WITH CARON
        0x00ff: 0x02d9, # DOT ABOVE
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
