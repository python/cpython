""" Python Character Mapping Codec for KOI8U.

    This character scheme is compliant to RFC2319

Written by Marc-Andre Lemburg (mal@lemburg.com).
Modified by Maxim Dzumanenko <mvd@mylinux.com.ua>.

(c) Copyright 2002, Python Software Foundation.

"""#"

import codecs, koi8_r

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

decoding_map = koi8_r.decoding_map.copy()
decoding_map.update({
        0x00a4: 0x0454, #       CYRILLIC SMALL LETTER UKRAINIAN IE
        0x00a6: 0x0456, #       CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
        0x00a7: 0x0457, #       CYRILLIC SMALL LETTER YI (UKRAINIAN)
        0x00ad: 0x0491, #       CYRILLIC SMALL LETTER UKRAINIAN GHE WITH UPTURN
        0x00b4: 0x0404, #       CYRILLIC CAPITAL LETTER UKRAINIAN IE
        0x00b6: 0x0406, #       CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
        0x00b7: 0x0407, #       CYRILLIC CAPITAL LETTER YI (UKRAINIAN)
        0x00bd: 0x0490, #       CYRILLIC CAPITAL LETTER UKRAINIAN GHE WITH UPTURN
})

### Encoding Map

encoding_map = codecs.make_encoding_map(decoding_map)
