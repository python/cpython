""" Python Character Mapping Codec generated from '8859-3.TXT'.


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

	0x00a1: 0x0126,	# 	LATIN CAPITAL LETTER H WITH STROKE
	0x00a2: 0x02d8,	# 	BREVE
	0x00a6: 0x0124,	# 	LATIN CAPITAL LETTER H WITH CIRCUMFLEX
	0x00a9: 0x0130,	# 	LATIN CAPITAL LETTER I WITH DOT ABOVE
	0x00aa: 0x015e,	# 	LATIN CAPITAL LETTER S WITH CEDILLA
	0x00ab: 0x011e,	# 	LATIN CAPITAL LETTER G WITH BREVE
	0x00ac: 0x0134,	# 	LATIN CAPITAL LETTER J WITH CIRCUMFLEX
	0x00af: 0x017b,	# 	LATIN CAPITAL LETTER Z WITH DOT ABOVE
	0x00b1: 0x0127,	# 	LATIN SMALL LETTER H WITH STROKE
	0x00b6: 0x0125,	# 	LATIN SMALL LETTER H WITH CIRCUMFLEX
	0x00b9: 0x0131,	# 	LATIN SMALL LETTER DOTLESS I
	0x00ba: 0x015f,	# 	LATIN SMALL LETTER S WITH CEDILLA
	0x00bb: 0x011f,	# 	LATIN SMALL LETTER G WITH BREVE
	0x00bc: 0x0135,	# 	LATIN SMALL LETTER J WITH CIRCUMFLEX
	0x00bf: 0x017c,	# 	LATIN SMALL LETTER Z WITH DOT ABOVE
	0x00c5: 0x010a,	# 	LATIN CAPITAL LETTER C WITH DOT ABOVE
	0x00c6: 0x0108,	# 	LATIN CAPITAL LETTER C WITH CIRCUMFLEX
	0x00d5: 0x0120,	# 	LATIN CAPITAL LETTER G WITH DOT ABOVE
	0x00d8: 0x011c,	# 	LATIN CAPITAL LETTER G WITH CIRCUMFLEX
	0x00dd: 0x016c,	# 	LATIN CAPITAL LETTER U WITH BREVE
	0x00de: 0x015c,	# 	LATIN CAPITAL LETTER S WITH CIRCUMFLEX
	0x00e5: 0x010b,	# 	LATIN SMALL LETTER C WITH DOT ABOVE
	0x00e6: 0x0109,	# 	LATIN SMALL LETTER C WITH CIRCUMFLEX
	0x00f5: 0x0121,	# 	LATIN SMALL LETTER G WITH DOT ABOVE
	0x00f8: 0x011d,	# 	LATIN SMALL LETTER G WITH CIRCUMFLEX
	0x00fd: 0x016d,	# 	LATIN SMALL LETTER U WITH BREVE
	0x00fe: 0x015d,	# 	LATIN SMALL LETTER S WITH CIRCUMFLEX
	0x00ff: 0x02d9,	# 	DOT ABOVE
}

### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k
