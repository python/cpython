""" Python Character Mapping Codec generated from 'CP1256.TXT' with gencodec.py.

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
        0x0081: 0x067e, # ARABIC LETTER PEH
        0x0082: 0x201a, # SINGLE LOW-9 QUOTATION MARK
        0x0083: 0x0192, # LATIN SMALL LETTER F WITH HOOK
        0x0084: 0x201e, # DOUBLE LOW-9 QUOTATION MARK
        0x0085: 0x2026, # HORIZONTAL ELLIPSIS
        0x0086: 0x2020, # DAGGER
        0x0087: 0x2021, # DOUBLE DAGGER
        0x0088: 0x02c6, # MODIFIER LETTER CIRCUMFLEX ACCENT
        0x0089: 0x2030, # PER MILLE SIGN
        0x008a: 0x0679, # ARABIC LETTER TTEH
        0x008b: 0x2039, # SINGLE LEFT-POINTING ANGLE QUOTATION MARK
        0x008c: 0x0152, # LATIN CAPITAL LIGATURE OE
        0x008d: 0x0686, # ARABIC LETTER TCHEH
        0x008e: 0x0698, # ARABIC LETTER JEH
        0x008f: 0x0688, # ARABIC LETTER DDAL
        0x0090: 0x06af, # ARABIC LETTER GAF
        0x0091: 0x2018, # LEFT SINGLE QUOTATION MARK
        0x0092: 0x2019, # RIGHT SINGLE QUOTATION MARK
        0x0093: 0x201c, # LEFT DOUBLE QUOTATION MARK
        0x0094: 0x201d, # RIGHT DOUBLE QUOTATION MARK
        0x0095: 0x2022, # BULLET
        0x0096: 0x2013, # EN DASH
        0x0097: 0x2014, # EM DASH
        0x0098: 0x06a9, # ARABIC LETTER KEHEH
        0x0099: 0x2122, # TRADE MARK SIGN
        0x009a: 0x0691, # ARABIC LETTER RREH
        0x009b: 0x203a, # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
        0x009c: 0x0153, # LATIN SMALL LIGATURE OE
        0x009d: 0x200c, # ZERO WIDTH NON-JOINER
        0x009e: 0x200d, # ZERO WIDTH JOINER
        0x009f: 0x06ba, # ARABIC LETTER NOON GHUNNA
        0x00a1: 0x060c, # ARABIC COMMA
        0x00aa: 0x06be, # ARABIC LETTER HEH DOACHASHMEE
        0x00ba: 0x061b, # ARABIC SEMICOLON
        0x00bf: 0x061f, # ARABIC QUESTION MARK
        0x00c0: 0x06c1, # ARABIC LETTER HEH GOAL
        0x00c1: 0x0621, # ARABIC LETTER HAMZA
        0x00c2: 0x0622, # ARABIC LETTER ALEF WITH MADDA ABOVE
        0x00c3: 0x0623, # ARABIC LETTER ALEF WITH HAMZA ABOVE
        0x00c4: 0x0624, # ARABIC LETTER WAW WITH HAMZA ABOVE
        0x00c5: 0x0625, # ARABIC LETTER ALEF WITH HAMZA BELOW
        0x00c6: 0x0626, # ARABIC LETTER YEH WITH HAMZA ABOVE
        0x00c7: 0x0627, # ARABIC LETTER ALEF
        0x00c8: 0x0628, # ARABIC LETTER BEH
        0x00c9: 0x0629, # ARABIC LETTER TEH MARBUTA
        0x00ca: 0x062a, # ARABIC LETTER TEH
        0x00cb: 0x062b, # ARABIC LETTER THEH
        0x00cc: 0x062c, # ARABIC LETTER JEEM
        0x00cd: 0x062d, # ARABIC LETTER HAH
        0x00ce: 0x062e, # ARABIC LETTER KHAH
        0x00cf: 0x062f, # ARABIC LETTER DAL
        0x00d0: 0x0630, # ARABIC LETTER THAL
        0x00d1: 0x0631, # ARABIC LETTER REH
        0x00d2: 0x0632, # ARABIC LETTER ZAIN
        0x00d3: 0x0633, # ARABIC LETTER SEEN
        0x00d4: 0x0634, # ARABIC LETTER SHEEN
        0x00d5: 0x0635, # ARABIC LETTER SAD
        0x00d6: 0x0636, # ARABIC LETTER DAD
        0x00d8: 0x0637, # ARABIC LETTER TAH
        0x00d9: 0x0638, # ARABIC LETTER ZAH
        0x00da: 0x0639, # ARABIC LETTER AIN
        0x00db: 0x063a, # ARABIC LETTER GHAIN
        0x00dc: 0x0640, # ARABIC TATWEEL
        0x00dd: 0x0641, # ARABIC LETTER FEH
        0x00de: 0x0642, # ARABIC LETTER QAF
        0x00df: 0x0643, # ARABIC LETTER KAF
        0x00e1: 0x0644, # ARABIC LETTER LAM
        0x00e3: 0x0645, # ARABIC LETTER MEEM
        0x00e4: 0x0646, # ARABIC LETTER NOON
        0x00e5: 0x0647, # ARABIC LETTER HEH
        0x00e6: 0x0648, # ARABIC LETTER WAW
        0x00ec: 0x0649, # ARABIC LETTER ALEF MAKSURA
        0x00ed: 0x064a, # ARABIC LETTER YEH
        0x00f0: 0x064b, # ARABIC FATHATAN
        0x00f1: 0x064c, # ARABIC DAMMATAN
        0x00f2: 0x064d, # ARABIC KASRATAN
        0x00f3: 0x064e, # ARABIC FATHA
        0x00f5: 0x064f, # ARABIC DAMMA
        0x00f6: 0x0650, # ARABIC KASRA
        0x00f8: 0x0651, # ARABIC SHADDA
        0x00fa: 0x0652, # ARABIC SUKUN
        0x00fd: 0x200e, # LEFT-TO-RIGHT MARK
        0x00fe: 0x200f, # RIGHT-TO-LEFT MARK
        0x00ff: 0x06d2, # ARABIC LETTER YEH BARREE
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
