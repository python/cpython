""" Python Character Mapping Codec generated from '8859-8.TXT'.


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

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

decoding_map = {

	0x00aa: 0x00d7,	# 	MULTIPLICATION SIGN
	0x00af: 0x203e,	# 	OVERLINE
	0x00ba: 0x00f7,	# 	DIVISION SIGN
	0x00df: 0x2017,	# 	DOUBLE LOW LINE
	0x00e0: 0x05d0,	# 	HEBREW LETTER ALEF
	0x00e1: 0x05d1,	# 	HEBREW LETTER BET
	0x00e2: 0x05d2,	# 	HEBREW LETTER GIMEL
	0x00e3: 0x05d3,	# 	HEBREW LETTER DALET
	0x00e4: 0x05d4,	# 	HEBREW LETTER HE
	0x00e5: 0x05d5,	# 	HEBREW LETTER VAV
	0x00e6: 0x05d6,	# 	HEBREW LETTER ZAYIN
	0x00e7: 0x05d7,	# 	HEBREW LETTER HET
	0x00e8: 0x05d8,	# 	HEBREW LETTER TET
	0x00e9: 0x05d9,	# 	HEBREW LETTER YOD
	0x00ea: 0x05da,	# 	HEBREW LETTER FINAL KAF
	0x00eb: 0x05db,	# 	HEBREW LETTER KAF
	0x00ec: 0x05dc,	# 	HEBREW LETTER LAMED
	0x00ed: 0x05dd,	# 	HEBREW LETTER FINAL MEM
	0x00ee: 0x05de,	# 	HEBREW LETTER MEM
	0x00ef: 0x05df,	# 	HEBREW LETTER FINAL NUN
	0x00f0: 0x05e0,	# 	HEBREW LETTER NUN
	0x00f1: 0x05e1,	# 	HEBREW LETTER SAMEKH
	0x00f2: 0x05e2,	# 	HEBREW LETTER AYIN
	0x00f3: 0x05e3,	# 	HEBREW LETTER FINAL PE
	0x00f4: 0x05e4,	# 	HEBREW LETTER PE
	0x00f5: 0x05e5,	# 	HEBREW LETTER FINAL TSADI
	0x00f6: 0x05e6,	# 	HEBREW LETTER TSADI
	0x00f7: 0x05e7,	# 	HEBREW LETTER QOF
	0x00f8: 0x05e8,	# 	HEBREW LETTER RESH
	0x00f9: 0x05e9,	# 	HEBREW LETTER SHIN
	0x00fa: 0x05ea,	# 	HEBREW LETTER TAV
}

### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k
