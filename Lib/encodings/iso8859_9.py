""" Python Character Mapping Codec generated from '8859-9.TXT'.


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

	0x00d0: 0x011e,	# 	LATIN CAPITAL LETTER G WITH BREVE
	0x00dd: 0x0130,	# 	LATIN CAPITAL LETTER I WITH DOT ABOVE
	0x00de: 0x015e,	# 	LATIN CAPITAL LETTER S WITH CEDILLA
	0x00f0: 0x011f,	# 	LATIN SMALL LETTER G WITH BREVE
	0x00fd: 0x0131,	# 	LATIN SMALL LETTER DOTLESS I
	0x00fe: 0x015f,	# 	LATIN SMALL LETTER S WITH CEDILLA
}

### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k
