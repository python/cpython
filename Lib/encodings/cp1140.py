""" Python Character Mapping Codec for cp1140

Written by Brian Quinlan(brian@sweetapp.com). NO WARRANTY.
"""

import codecs
import copy
import cp037

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

decoding_map = copy.copy(cp037.decoding_map)

decoding_map.update({
        0x009f: 0x20ac # EURO SIGN
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
