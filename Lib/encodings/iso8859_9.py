""" Python Character Mapping Codec generated from '8859-9.TXT' with gencodec.py.

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
	0x00d0: 0x011e,	# 	LATIN CAPITAL LETTER G WITH BREVE
	0x00dd: 0x0130,	# 	LATIN CAPITAL LETTER I WITH DOT ABOVE
	0x00de: 0x015e,	# 	LATIN CAPITAL LETTER S WITH CEDILLA
	0x00f0: 0x011f,	# 	LATIN SMALL LETTER G WITH BREVE
	0x00fd: 0x0131,	# 	LATIN SMALL LETTER DOTLESS I
	0x00fe: 0x015f,	# 	LATIN SMALL LETTER S WITH CEDILLA
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
