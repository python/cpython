""" Python Character Mapping Codec generated from 'CP1258.TXT' with gencodec.py.

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
        0x0083: 0x0192, # LATIN SMALL LETTER F WITH HOOK
        0x0084: 0x201e, # DOUBLE LOW-9 QUOTATION MARK
        0x0085: 0x2026, # HORIZONTAL ELLIPSIS
        0x0086: 0x2020, # DAGGER
        0x0087: 0x2021, # DOUBLE DAGGER
        0x0088: 0x02c6, # MODIFIER LETTER CIRCUMFLEX ACCENT
        0x0089: 0x2030, # PER MILLE SIGN
        0x008a: None,   # UNDEFINED
        0x008b: 0x2039, # SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        0x008c: 0x0152, # LATIN CAPITAL LIGATURE OE
        0x008d: None,   # UNDEFINED
        0x008e: None,   # UNDEFINED
        0x008f: None,   # UNDEFINED
        0x0090: None,   # UNDEFINED
        0x0091: 0x2018, # LEFT SINGLE QUOTATION MARK
        0x0092: 0x2019, # RIGHT SINGLE QUOTATION MARK
        0x0093: 0x201c, # LEFT DOUBLE QUOTATION MARK
        0x0094: 0x201d, # RIGHT DOUBLE QUOTATION MARK
        0x0095: 0x2022, # BULLET
        0x0096: 0x2013, # EN DASH
        0x0097: 0x2014, # EM DASH
        0x0098: 0x02dc, # SMALL TILDE
        0x0099: 0x2122, # TRADE MARK SIGN
        0x009a: None,   # UNDEFINED
        0x009b: 0x203a, # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        0x009c: 0x0153, # LATIN SMALL LIGATURE OE
        0x009d: None,   # UNDEFINED
        0x009e: None,   # UNDEFINED
        0x009f: 0x0178, # LATIN CAPITAL LETTER Y WITH DIAERESIS
        0x00c3: 0x0102, # LATIN CAPITAL LETTER A WITH BREVE
        0x00cc: 0x0300, # COMBINING GRAVE ACCENT
        0x00d0: 0x0110, # LATIN CAPITAL LETTER D WITH STROKE
        0x00d2: 0x0309, # COMBINING HOOK ABOVE
        0x00d5: 0x01a0, # LATIN CAPITAL LETTER O WITH HORN
        0x00dd: 0x01af, # LATIN CAPITAL LETTER U WITH HORN
        0x00de: 0x0303, # COMBINING TILDE
        0x00e3: 0x0103, # LATIN SMALL LETTER A WITH BREVE
        0x00ec: 0x0301, # COMBINING ACUTE ACCENT
        0x00f0: 0x0111, # LATIN SMALL LETTER D WITH STROKE
        0x00f2: 0x0323, # COMBINING DOT BELOW
        0x00f5: 0x01a1, # LATIN SMALL LETTER O WITH HORN
        0x00fd: 0x01b0, # LATIN SMALL LETTER U WITH HORN
        0x00fe: 0x20ab, # DONG SIGN
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
