""" Python Character Mapping Codec for TIS-620.

    According to
    ftp://ftp.unicode.org/Public/MAPPINGS/ISO8859/8859-11.TXT the
    TIS-620 is the identical to ISO_8859-11 with the 0xA0 (no-break
    space) mapping removed.

"""#"

import codecs
from encodings.iso8859_11 import decoding_map

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

decoding_map = decoding_map.copy()
decoding_map.update({
        0x00a0: None,
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
