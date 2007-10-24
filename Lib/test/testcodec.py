""" Test Codecs (used by test_charmapcodec)

Written by Marc-Andre Lemburg (mal@lemburg.com).

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
        0x78: "abc", # 1-n decoding mapping
        str8(b"abc"): 0x0078,# 1-n encoding mapping
        0x01: None,   # decoding mapping to <undefined>
        0x79: "",    # decoding mapping to <remove character>
})

### Encoding Map

encoding_map = {}
for k,v in decoding_map.items():
    encoding_map[v] = k
