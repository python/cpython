""" Python Character Mapping Codec generated from '8859-15.TXT' with gencodec.py.

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
	0x00a4: 0x20ac,	# 	EURO SIGN
	0x00a6: 0x0160,	# 	LATIN CAPITAL LETTER S WITH CARON
	0x00a8: 0x0161,	# 	LATIN SMALL LETTER S WITH CARON
	0x00b4: 0x017d,	# 	LATIN CAPITAL LETTER Z WITH CARON
	0x00b8: 0x017e,	# 	LATIN SMALL LETTER Z WITH CARON
	0x00bc: 0x0152,	# 	LATIN CAPITAL LIGATURE OE
	0x00bd: 0x0153,	# 	LATIN SMALL LIGATURE OE
	0x00be: 0x0178,	# 	LATIN CAPITAL LETTER Y WITH DIAERESIS
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
