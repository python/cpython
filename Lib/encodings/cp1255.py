""" Python Character Mapping Codec generated from 'CP1255.TXT' with gencodec.py.

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
        0x008c: None,   # UNDEFINED
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
        0x009c: None,   # UNDEFINED
        0x009d: None,   # UNDEFINED
        0x009e: None,   # UNDEFINED
        0x009f: None,   # UNDEFINED
        0x00a4: 0x20aa, # NEW SHEQEL SIGN
        0x00aa: 0x00d7, # MULTIPLICATION SIGN
        0x00ba: 0x00f7, # DIVISION SIGN
        0x00c0: 0x05b0, # HEBREW POINT SHEVA
        0x00c1: 0x05b1, # HEBREW POINT HATAF SEGOL
        0x00c2: 0x05b2, # HEBREW POINT HATAF PATAH
        0x00c3: 0x05b3, # HEBREW POINT HATAF QAMATS
        0x00c4: 0x05b4, # HEBREW POINT HIRIQ
        0x00c5: 0x05b5, # HEBREW POINT TSERE
        0x00c6: 0x05b6, # HEBREW POINT SEGOL
        0x00c7: 0x05b7, # HEBREW POINT PATAH
        0x00c8: 0x05b8, # HEBREW POINT QAMATS
        0x00c9: 0x05b9, # HEBREW POINT HOLAM
        0x00ca: None,   # UNDEFINED
        0x00cb: 0x05bb, # HEBREW POINT QUBUTS
        0x00cc: 0x05bc, # HEBREW POINT DAGESH OR MAPIQ
        0x00cd: 0x05bd, # HEBREW POINT METEG
        0x00ce: 0x05be, # HEBREW PUNCTUATION MAQAF
        0x00cf: 0x05bf, # HEBREW POINT RAFE
        0x00d0: 0x05c0, # HEBREW PUNCTUATION PASEQ
        0x00d1: 0x05c1, # HEBREW POINT SHIN DOT
        0x00d2: 0x05c2, # HEBREW POINT SIN DOT
        0x00d3: 0x05c3, # HEBREW PUNCTUATION SOF PASUQ
        0x00d4: 0x05f0, # HEBREW LIGATURE YIDDISH DOUBLE VAV
        0x00d5: 0x05f1, # HEBREW LIGATURE YIDDISH VAV YOD
        0x00d6: 0x05f2, # HEBREW LIGATURE YIDDISH DOUBLE YOD
        0x00d7: 0x05f3, # HEBREW PUNCTUATION GERESH
        0x00d8: 0x05f4, # HEBREW PUNCTUATION GERSHAYIM
        0x00d9: None,   # UNDEFINED
        0x00da: None,   # UNDEFINED
        0x00db: None,   # UNDEFINED
        0x00dc: None,   # UNDEFINED
        0x00dd: None,   # UNDEFINED
        0x00de: None,   # UNDEFINED
        0x00df: None,   # UNDEFINED
        0x00e0: 0x05d0, # HEBREW LETTER ALEF
        0x00e1: 0x05d1, # HEBREW LETTER BET
        0x00e2: 0x05d2, # HEBREW LETTER GIMEL
        0x00e3: 0x05d3, # HEBREW LETTER DALET
        0x00e4: 0x05d4, # HEBREW LETTER HE
        0x00e5: 0x05d5, # HEBREW LETTER VAV
        0x00e6: 0x05d6, # HEBREW LETTER ZAYIN
        0x00e7: 0x05d7, # HEBREW LETTER HET
        0x00e8: 0x05d8, # HEBREW LETTER TET
        0x00e9: 0x05d9, # HEBREW LETTER YOD
        0x00ea: 0x05da, # HEBREW LETTER FINAL KAF
        0x00eb: 0x05db, # HEBREW LETTER KAF
        0x00ec: 0x05dc, # HEBREW LETTER LAMED
        0x00ed: 0x05dd, # HEBREW LETTER FINAL MEM
        0x00ee: 0x05de, # HEBREW LETTER MEM
        0x00ef: 0x05df, # HEBREW LETTER FINAL NUN
        0x00f0: 0x05e0, # HEBREW LETTER NUN
        0x00f1: 0x05e1, # HEBREW LETTER SAMEKH
        0x00f2: 0x05e2, # HEBREW LETTER AYIN
        0x00f3: 0x05e3, # HEBREW LETTER FINAL PE
        0x00f4: 0x05e4, # HEBREW LETTER PE
        0x00f5: 0x05e5, # HEBREW LETTER FINAL TSADI
        0x00f6: 0x05e6, # HEBREW LETTER TSADI
        0x00f7: 0x05e7, # HEBREW LETTER QOF
        0x00f8: 0x05e8, # HEBREW LETTER RESH
        0x00f9: 0x05e9, # HEBREW LETTER SHIN
        0x00fa: 0x05ea, # HEBREW LETTER TAV
        0x00fb: None,   # UNDEFINED
        0x00fc: None,   # UNDEFINED
        0x00fd: 0x200e, # LEFT-TO-RIGHT MARK
        0x00fe: 0x200f, # RIGHT-TO-LEFT MARK
        0x00ff: None,   # UNDEFINED
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
