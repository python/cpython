""" Python Character Mapping Codec generated from '8859-6.TXT' with gencodec.py.

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
        0x00a1: None,
        0x00a2: None,
        0x00a3: None,
        0x00a5: None,
        0x00a6: None,
        0x00a7: None,
        0x00a8: None,
        0x00a9: None,
        0x00aa: None,
        0x00ab: None,
        0x00ac: 0x060c, #       ARABIC COMMA
        0x00ae: None,
        0x00af: None,
        0x00b0: None,
        0x00b1: None,
        0x00b2: None,
        0x00b3: None,
        0x00b4: None,
        0x00b5: None,
        0x00b6: None,
        0x00b7: None,
        0x00b8: None,
        0x00b9: None,
        0x00ba: None,
        0x00bb: 0x061b, #       ARABIC SEMICOLON
        0x00bc: None,
        0x00bd: None,
        0x00be: None,
        0x00bf: 0x061f, #       ARABIC QUESTION MARK
        0x00c0: None,
        0x00c1: 0x0621, #       ARABIC LETTER HAMZA
        0x00c2: 0x0622, #       ARABIC LETTER ALEF WITH MADDA ABOVE
        0x00c3: 0x0623, #       ARABIC LETTER ALEF WITH HAMZA ABOVE
        0x00c4: 0x0624, #       ARABIC LETTER WAW WITH HAMZA ABOVE
        0x00c5: 0x0625, #       ARABIC LETTER ALEF WITH HAMZA BELOW
        0x00c6: 0x0626, #       ARABIC LETTER YEH WITH HAMZA ABOVE
        0x00c7: 0x0627, #       ARABIC LETTER ALEF
        0x00c8: 0x0628, #       ARABIC LETTER BEH
        0x00c9: 0x0629, #       ARABIC LETTER TEH MARBUTA
        0x00ca: 0x062a, #       ARABIC LETTER TEH
        0x00cb: 0x062b, #       ARABIC LETTER THEH
        0x00cc: 0x062c, #       ARABIC LETTER JEEM
        0x00cd: 0x062d, #       ARABIC LETTER HAH
        0x00ce: 0x062e, #       ARABIC LETTER KHAH
        0x00cf: 0x062f, #       ARABIC LETTER DAL
        0x00d0: 0x0630, #       ARABIC LETTER THAL
        0x00d1: 0x0631, #       ARABIC LETTER REH
        0x00d2: 0x0632, #       ARABIC LETTER ZAIN
        0x00d3: 0x0633, #       ARABIC LETTER SEEN
        0x00d4: 0x0634, #       ARABIC LETTER SHEEN
        0x00d5: 0x0635, #       ARABIC LETTER SAD
        0x00d6: 0x0636, #       ARABIC LETTER DAD
        0x00d7: 0x0637, #       ARABIC LETTER TAH
        0x00d8: 0x0638, #       ARABIC LETTER ZAH
        0x00d9: 0x0639, #       ARABIC LETTER AIN
        0x00da: 0x063a, #       ARABIC LETTER GHAIN
        0x00db: None,
        0x00dc: None,
        0x00dd: None,
        0x00de: None,
        0x00df: None,
        0x00e0: 0x0640, #       ARABIC TATWEEL
        0x00e1: 0x0641, #       ARABIC LETTER FEH
        0x00e2: 0x0642, #       ARABIC LETTER QAF
        0x00e3: 0x0643, #       ARABIC LETTER KAF
        0x00e4: 0x0644, #       ARABIC LETTER LAM
        0x00e5: 0x0645, #       ARABIC LETTER MEEM
        0x00e6: 0x0646, #       ARABIC LETTER NOON
        0x00e7: 0x0647, #       ARABIC LETTER HEH
        0x00e8: 0x0648, #       ARABIC LETTER WAW
        0x00e9: 0x0649, #       ARABIC LETTER ALEF MAKSURA
        0x00ea: 0x064a, #       ARABIC LETTER YEH
        0x00eb: 0x064b, #       ARABIC FATHATAN
        0x00ec: 0x064c, #       ARABIC DAMMATAN
        0x00ed: 0x064d, #       ARABIC KASRATAN
        0x00ee: 0x064e, #       ARABIC FATHA
        0x00ef: 0x064f, #       ARABIC DAMMA
        0x00f0: 0x0650, #       ARABIC KASRA
        0x00f1: 0x0651, #       ARABIC SHADDA
        0x00f2: 0x0652, #       ARABIC SUKUN
        0x00f3: None,
        0x00f4: None,
        0x00f5: None,
        0x00f6: None,
        0x00f7: None,
        0x00f8: None,
        0x00f9: None,
        0x00fa: None,
        0x00fb: None,
        0x00fc: None,
        0x00fd: None,
        0x00fe: None,
        0x00ff: None,
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
