""" Python Character Mapping Codec generated from '8859-14.TXT' with gencodec.py.

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
	0x00a1: 0x1e02,	# 	LATIN CAPITAL LETTER B WITH DOT ABOVE
	0x00a2: 0x1e03,	# 	LATIN SMALL LETTER B WITH DOT ABOVE
	0x00a4: 0x010a,	# 	LATIN CAPITAL LETTER C WITH DOT ABOVE
	0x00a5: 0x010b,	# 	LATIN SMALL LETTER C WITH DOT ABOVE
	0x00a6: 0x1e0a,	# 	LATIN CAPITAL LETTER D WITH DOT ABOVE
	0x00a8: 0x1e80,	# 	LATIN CAPITAL LETTER W WITH GRAVE
	0x00aa: 0x1e82,	# 	LATIN CAPITAL LETTER W WITH ACUTE
	0x00ab: 0x1e0b,	# 	LATIN SMALL LETTER D WITH DOT ABOVE
	0x00ac: 0x1ef2,	# 	LATIN CAPITAL LETTER Y WITH GRAVE
	0x00af: 0x0178,	# 	LATIN CAPITAL LETTER Y WITH DIAERESIS
	0x00b0: 0x1e1e,	# 	LATIN CAPITAL LETTER F WITH DOT ABOVE
	0x00b1: 0x1e1f,	# 	LATIN SMALL LETTER F WITH DOT ABOVE
	0x00b2: 0x0120,	# 	LATIN CAPITAL LETTER G WITH DOT ABOVE
	0x00b3: 0x0121,	# 	LATIN SMALL LETTER G WITH DOT ABOVE
	0x00b4: 0x1e40,	# 	LATIN CAPITAL LETTER M WITH DOT ABOVE
	0x00b5: 0x1e41,	# 	LATIN SMALL LETTER M WITH DOT ABOVE
	0x00b7: 0x1e56,	# 	LATIN CAPITAL LETTER P WITH DOT ABOVE
	0x00b8: 0x1e81,	# 	LATIN SMALL LETTER W WITH GRAVE
	0x00b9: 0x1e57,	# 	LATIN SMALL LETTER P WITH DOT ABOVE
	0x00ba: 0x1e83,	# 	LATIN SMALL LETTER W WITH ACUTE
	0x00bb: 0x1e60,	# 	LATIN CAPITAL LETTER S WITH DOT ABOVE
	0x00bc: 0x1ef3,	# 	LATIN SMALL LETTER Y WITH GRAVE
	0x00bd: 0x1e84,	# 	LATIN CAPITAL LETTER W WITH DIAERESIS
	0x00be: 0x1e85,	# 	LATIN SMALL LETTER W WITH DIAERESIS
	0x00bf: 0x1e61,	# 	LATIN SMALL LETTER S WITH DOT ABOVE
	0x00d0: 0x0174,	# 	LATIN CAPITAL LETTER W WITH CIRCUMFLEX
	0x00d7: 0x1e6a,	# 	LATIN CAPITAL LETTER T WITH DOT ABOVE
	0x00de: 0x0176,	# 	LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
	0x00f0: 0x0175,	# 	LATIN SMALL LETTER W WITH CIRCUMFLEX
	0x00f7: 0x1e6b,	# 	LATIN SMALL LETTER T WITH DOT ABOVE
	0x00fe: 0x0177,	# 	LATIN SMALL LETTER Y WITH CIRCUMFLEX
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
