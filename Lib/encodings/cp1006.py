""" Python Character Mapping Codec generated from 'CP1006.TXT' with gencodec.py.

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
        0x00a1: 0x06f0, #       EXTENDED ARABIC-INDIC DIGIT ZERO
        0x00a2: 0x06f1, #       EXTENDED ARABIC-INDIC DIGIT ONE
        0x00a3: 0x06f2, #       EXTENDED ARABIC-INDIC DIGIT TWO
        0x00a4: 0x06f3, #       EXTENDED ARABIC-INDIC DIGIT THREE
        0x00a5: 0x06f4, #       EXTENDED ARABIC-INDIC DIGIT FOUR
        0x00a6: 0x06f5, #       EXTENDED ARABIC-INDIC DIGIT FIVE
        0x00a7: 0x06f6, #       EXTENDED ARABIC-INDIC DIGIT SIX
        0x00a8: 0x06f7, #       EXTENDED ARABIC-INDIC DIGIT SEVEN
        0x00a9: 0x06f8, #       EXTENDED ARABIC-INDIC DIGIT EIGHT
        0x00aa: 0x06f9, #       EXTENDED ARABIC-INDIC DIGIT NINE
        0x00ab: 0x060c, #       ARABIC COMMA
        0x00ac: 0x061b, #       ARABIC SEMICOLON
        0x00ae: 0x061f, #       ARABIC QUESTION MARK
        0x00af: 0xfe81, #       ARABIC LETTER ALEF WITH MADDA ABOVE ISOLATED FORM
        0x00b0: 0xfe8d, #       ARABIC LETTER ALEF ISOLATED FORM
        0x00b1: 0xfe8e, #       ARABIC LETTER ALEF FINAL FORM
        0x00b2: 0xfe8e, #       ARABIC LETTER ALEF FINAL FORM
        0x00b3: 0xfe8f, #       ARABIC LETTER BEH ISOLATED FORM
        0x00b4: 0xfe91, #       ARABIC LETTER BEH INITIAL FORM
        0x00b5: 0xfb56, #       ARABIC LETTER PEH ISOLATED FORM
        0x00b6: 0xfb58, #       ARABIC LETTER PEH INITIAL FORM
        0x00b7: 0xfe93, #       ARABIC LETTER TEH MARBUTA ISOLATED FORM
        0x00b8: 0xfe95, #       ARABIC LETTER TEH ISOLATED FORM
        0x00b9: 0xfe97, #       ARABIC LETTER TEH INITIAL FORM
        0x00ba: 0xfb66, #       ARABIC LETTER TTEH ISOLATED FORM
        0x00bb: 0xfb68, #       ARABIC LETTER TTEH INITIAL FORM
        0x00bc: 0xfe99, #       ARABIC LETTER THEH ISOLATED FORM
        0x00bd: 0xfe9b, #       ARABIC LETTER THEH INITIAL FORM
        0x00be: 0xfe9d, #       ARABIC LETTER JEEM ISOLATED FORM
        0x00bf: 0xfe9f, #       ARABIC LETTER JEEM INITIAL FORM
        0x00c0: 0xfb7a, #       ARABIC LETTER TCHEH ISOLATED FORM
        0x00c1: 0xfb7c, #       ARABIC LETTER TCHEH INITIAL FORM
        0x00c2: 0xfea1, #       ARABIC LETTER HAH ISOLATED FORM
        0x00c3: 0xfea3, #       ARABIC LETTER HAH INITIAL FORM
        0x00c4: 0xfea5, #       ARABIC LETTER KHAH ISOLATED FORM
        0x00c5: 0xfea7, #       ARABIC LETTER KHAH INITIAL FORM
        0x00c6: 0xfea9, #       ARABIC LETTER DAL ISOLATED FORM
        0x00c7: 0xfb84, #       ARABIC LETTER DAHAL ISOLATED FORMN
        0x00c8: 0xfeab, #       ARABIC LETTER THAL ISOLATED FORM
        0x00c9: 0xfead, #       ARABIC LETTER REH ISOLATED FORM
        0x00ca: 0xfb8c, #       ARABIC LETTER RREH ISOLATED FORM
        0x00cb: 0xfeaf, #       ARABIC LETTER ZAIN ISOLATED FORM
        0x00cc: 0xfb8a, #       ARABIC LETTER JEH ISOLATED FORM
        0x00cd: 0xfeb1, #       ARABIC LETTER SEEN ISOLATED FORM
        0x00ce: 0xfeb3, #       ARABIC LETTER SEEN INITIAL FORM
        0x00cf: 0xfeb5, #       ARABIC LETTER SHEEN ISOLATED FORM
        0x00d0: 0xfeb7, #       ARABIC LETTER SHEEN INITIAL FORM
        0x00d1: 0xfeb9, #       ARABIC LETTER SAD ISOLATED FORM
        0x00d2: 0xfebb, #       ARABIC LETTER SAD INITIAL FORM
        0x00d3: 0xfebd, #       ARABIC LETTER DAD ISOLATED FORM
        0x00d4: 0xfebf, #       ARABIC LETTER DAD INITIAL FORM
        0x00d5: 0xfec1, #       ARABIC LETTER TAH ISOLATED FORM
        0x00d6: 0xfec5, #       ARABIC LETTER ZAH ISOLATED FORM
        0x00d7: 0xfec9, #       ARABIC LETTER AIN ISOLATED FORM
        0x00d8: 0xfeca, #       ARABIC LETTER AIN FINAL FORM
        0x00d9: 0xfecb, #       ARABIC LETTER AIN INITIAL FORM
        0x00da: 0xfecc, #       ARABIC LETTER AIN MEDIAL FORM
        0x00db: 0xfecd, #       ARABIC LETTER GHAIN ISOLATED FORM
        0x00dc: 0xfece, #       ARABIC LETTER GHAIN FINAL FORM
        0x00dd: 0xfecf, #       ARABIC LETTER GHAIN INITIAL FORM
        0x00de: 0xfed0, #       ARABIC LETTER GHAIN MEDIAL FORM
        0x00df: 0xfed1, #       ARABIC LETTER FEH ISOLATED FORM
        0x00e0: 0xfed3, #       ARABIC LETTER FEH INITIAL FORM
        0x00e1: 0xfed5, #       ARABIC LETTER QAF ISOLATED FORM
        0x00e2: 0xfed7, #       ARABIC LETTER QAF INITIAL FORM
        0x00e3: 0xfed9, #       ARABIC LETTER KAF ISOLATED FORM
        0x00e4: 0xfedb, #       ARABIC LETTER KAF INITIAL FORM
        0x00e5: 0xfb92, #       ARABIC LETTER GAF ISOLATED FORM
        0x00e6: 0xfb94, #       ARABIC LETTER GAF INITIAL FORM
        0x00e7: 0xfedd, #       ARABIC LETTER LAM ISOLATED FORM
        0x00e8: 0xfedf, #       ARABIC LETTER LAM INITIAL FORM
        0x00e9: 0xfee0, #       ARABIC LETTER LAM MEDIAL FORM
        0x00ea: 0xfee1, #       ARABIC LETTER MEEM ISOLATED FORM
        0x00eb: 0xfee3, #       ARABIC LETTER MEEM INITIAL FORM
        0x00ec: 0xfb9e, #       ARABIC LETTER NOON GHUNNA ISOLATED FORM
        0x00ed: 0xfee5, #       ARABIC LETTER NOON ISOLATED FORM
        0x00ee: 0xfee7, #       ARABIC LETTER NOON INITIAL FORM
        0x00ef: 0xfe85, #       ARABIC LETTER WAW WITH HAMZA ABOVE ISOLATED FORM
        0x00f0: 0xfeed, #       ARABIC LETTER WAW ISOLATED FORM
        0x00f1: 0xfba6, #       ARABIC LETTER HEH GOAL ISOLATED FORM
        0x00f2: 0xfba8, #       ARABIC LETTER HEH GOAL INITIAL FORM
        0x00f3: 0xfba9, #       ARABIC LETTER HEH GOAL MEDIAL FORM
        0x00f4: 0xfbaa, #       ARABIC LETTER HEH DOACHASHMEE ISOLATED FORM
        0x00f5: 0xfe80, #       ARABIC LETTER HAMZA ISOLATED FORM
        0x00f6: 0xfe89, #       ARABIC LETTER YEH WITH HAMZA ABOVE ISOLATED FORM
        0x00f7: 0xfe8a, #       ARABIC LETTER YEH WITH HAMZA ABOVE FINAL FORM
        0x00f8: 0xfe8b, #       ARABIC LETTER YEH WITH HAMZA ABOVE INITIAL FORM
        0x00f9: 0xfef1, #       ARABIC LETTER YEH ISOLATED FORM
        0x00fa: 0xfef2, #       ARABIC LETTER YEH FINAL FORM
        0x00fb: 0xfef3, #       ARABIC LETTER YEH INITIAL FORM
        0x00fc: 0xfbb0, #       ARABIC LETTER YEH BARREE WITH HAMZA ABOVE ISOLATED FORM
        0x00fd: 0xfbae, #       ARABIC LETTER YEH BARREE ISOLATED FORM
        0x00fe: 0xfe7c, #       ARABIC SHADDA ISOLATED FORM
        0x00ff: 0xfe7d, #       ARABIC SHADDA MEDIAL FORM
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
