""" Python Character Mapping Codec generated from 'CP856.TXT' with gencodec.py.

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
        0x0080: 0x05d0, # HEBREW LETTER ALEF
        0x0081: 0x05d1, # HEBREW LETTER BET
        0x0082: 0x05d2, # HEBREW LETTER GIMEL
        0x0083: 0x05d3, # HEBREW LETTER DALET
        0x0084: 0x05d4, # HEBREW LETTER HE
        0x0085: 0x05d5, # HEBREW LETTER VAV
        0x0086: 0x05d6, # HEBREW LETTER ZAYIN
        0x0087: 0x05d7, # HEBREW LETTER HET
        0x0088: 0x05d8, # HEBREW LETTER TET
        0x0089: 0x05d9, # HEBREW LETTER YOD
        0x008a: 0x05da, # HEBREW LETTER FINAL KAF
        0x008b: 0x05db, # HEBREW LETTER KAF
        0x008c: 0x05dc, # HEBREW LETTER LAMED
        0x008d: 0x05dd, # HEBREW LETTER FINAL MEM
        0x008e: 0x05de, # HEBREW LETTER MEM
        0x008f: 0x05df, # HEBREW LETTER FINAL NUN
        0x0090: 0x05e0, # HEBREW LETTER NUN
        0x0091: 0x05e1, # HEBREW LETTER SAMEKH
        0x0092: 0x05e2, # HEBREW LETTER AYIN
        0x0093: 0x05e3, # HEBREW LETTER FINAL PE
        0x0094: 0x05e4, # HEBREW LETTER PE
        0x0095: 0x05e5, # HEBREW LETTER FINAL TSADI
        0x0096: 0x05e6, # HEBREW LETTER TSADI
        0x0097: 0x05e7, # HEBREW LETTER QOF
        0x0098: 0x05e8, # HEBREW LETTER RESH
        0x0099: 0x05e9, # HEBREW LETTER SHIN
        0x009a: 0x05ea, # HEBREW LETTER TAV
        0x009b: None,   # UNDEFINED
        0x009c: 0x00a3, # POUND SIGN
        0x009d: None,   # UNDEFINED
        0x009e: 0x00d7, # MULTIPLICATION SIGN
        0x009f: None,   # UNDEFINED
        0x00a0: None,   # UNDEFINED
        0x00a1: None,   # UNDEFINED
        0x00a2: None,   # UNDEFINED
        0x00a3: None,   # UNDEFINED
        0x00a4: None,   # UNDEFINED
        0x00a5: None,   # UNDEFINED
        0x00a6: None,   # UNDEFINED
        0x00a7: None,   # UNDEFINED
        0x00a8: None,   # UNDEFINED
        0x00a9: 0x00ae, # REGISTERED SIGN
        0x00aa: 0x00ac, # NOT SIGN
        0x00ab: 0x00bd, # VULGAR FRACTION ONE HALF
        0x00ac: 0x00bc, # VULGAR FRACTION ONE QUARTER
        0x00ad: None,   # UNDEFINED
        0x00ae: 0x00ab, # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x00af: 0x00bb, # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
        0x00b0: 0x2591, # LIGHT SHADE
        0x00b1: 0x2592, # MEDIUM SHADE
        0x00b2: 0x2593, # DARK SHADE
        0x00b3: 0x2502, # BOX DRAWINGS LIGHT VERTICAL
        0x00b4: 0x2524, # BOX DRAWINGS LIGHT VERTICAL AND LEFT
        0x00b5: None,   # UNDEFINED
        0x00b6: None,   # UNDEFINED
        0x00b7: None,   # UNDEFINED
        0x00b8: 0x00a9, # COPYRIGHT SIGN
        0x00b9: 0x2563, # BOX DRAWINGS DOUBLE VERTICAL AND LEFT
        0x00ba: 0x2551, # BOX DRAWINGS DOUBLE VERTICAL
        0x00bb: 0x2557, # BOX DRAWINGS DOUBLE DOWN AND LEFT
        0x00bc: 0x255d, # BOX DRAWINGS DOUBLE UP AND LEFT
        0x00bd: 0x00a2, # CENT SIGN
        0x00be: 0x00a5, # YEN SIGN
        0x00bf: 0x2510, # BOX DRAWINGS LIGHT DOWN AND LEFT
        0x00c0: 0x2514, # BOX DRAWINGS LIGHT UP AND RIGHT
        0x00c1: 0x2534, # BOX DRAWINGS LIGHT UP AND HORIZONTAL
        0x00c2: 0x252c, # BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
        0x00c3: 0x251c, # BOX DRAWINGS LIGHT VERTICAL AND RIGHT
        0x00c4: 0x2500, # BOX DRAWINGS LIGHT HORIZONTAL
        0x00c5: 0x253c, # BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
        0x00c6: None,   # UNDEFINED
        0x00c7: None,   # UNDEFINED
        0x00c8: 0x255a, # BOX DRAWINGS DOUBLE UP AND RIGHT
        0x00c9: 0x2554, # BOX DRAWINGS DOUBLE DOWN AND RIGHT
        0x00ca: 0x2569, # BOX DRAWINGS DOUBLE UP AND HORIZONTAL
        0x00cb: 0x2566, # BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
        0x00cc: 0x2560, # BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
        0x00cd: 0x2550, # BOX DRAWINGS DOUBLE HORIZONTAL
        0x00ce: 0x256c, # BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
        0x00cf: 0x00a4, # CURRENCY SIGN
        0x00d0: None,   # UNDEFINED
        0x00d1: None,   # UNDEFINED
        0x00d2: None,   # UNDEFINED
        0x00d3: None,   # UNDEFINEDS
        0x00d4: None,   # UNDEFINED
        0x00d5: None,   # UNDEFINED
        0x00d6: None,   # UNDEFINEDE
        0x00d7: None,   # UNDEFINED
        0x00d8: None,   # UNDEFINED
        0x00d9: 0x2518, # BOX DRAWINGS LIGHT UP AND LEFT
        0x00da: 0x250c, # BOX DRAWINGS LIGHT DOWN AND RIGHT
        0x00db: 0x2588, # FULL BLOCK
        0x00dc: 0x2584, # LOWER HALF BLOCK
        0x00dd: 0x00a6, # BROKEN BAR
        0x00de: None,   # UNDEFINED
        0x00df: 0x2580, # UPPER HALF BLOCK
        0x00e0: None,   # UNDEFINED
        0x00e1: None,   # UNDEFINED
        0x00e2: None,   # UNDEFINED
        0x00e3: None,   # UNDEFINED
        0x00e4: None,   # UNDEFINED
        0x00e5: None,   # UNDEFINED
        0x00e6: 0x00b5, # MICRO SIGN
        0x00e7: None,   # UNDEFINED
        0x00e8: None,   # UNDEFINED
        0x00e9: None,   # UNDEFINED
        0x00ea: None,   # UNDEFINED
        0x00eb: None,   # UNDEFINED
        0x00ec: None,   # UNDEFINED
        0x00ed: None,   # UNDEFINED
        0x00ee: 0x00af, # MACRON
        0x00ef: 0x00b4, # ACUTE ACCENT
        0x00f0: 0x00ad, # SOFT HYPHEN
        0x00f1: 0x00b1, # PLUS-MINUS SIGN
        0x00f2: 0x2017, # DOUBLE LOW LINE
        0x00f3: 0x00be, # VULGAR FRACTION THREE QUARTERS
        0x00f4: 0x00b6, # PILCROW SIGN
        0x00f5: 0x00a7, # SECTION SIGN
        0x00f6: 0x00f7, # DIVISION SIGN
        0x00f7: 0x00b8, # CEDILLA
        0x00f8: 0x00b0, # DEGREE SIGN
        0x00f9: 0x00a8, # DIAERESIS
        0x00fa: 0x00b7, # MIDDLE DOT
        0x00fb: 0x00b9, # SUPERSCRIPT ONE
        0x00fc: 0x00b3, # SUPERSCRIPT THREE
        0x00fd: 0x00b2, # SUPERSCRIPT TWO
        0x00fe: 0x25a0, # BLACK SQUARE
        0x00ff: 0x00a0, # NO-BREAK SPACE
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
