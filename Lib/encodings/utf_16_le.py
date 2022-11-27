""" Python 'utf-16-le' Codec - 16-bit Unicode Transformation Format


Written by Marc-Andre Lemburg (mal@lemburg.com). Modified by
Bernward Sanchez to support the Python Unicode API.

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""
import codecs, sys

### Codec APIs ###

def getregentry():
    return codecs.CodecInfo(
        name='utf-16-le',
        encode=codecs.utf_16_le_encode,
        decode=codecs.utf_16_le_decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )

### Incremental Encoder ###

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return codecs.utf_16_le_encode(input, self.errors)[0], len(input)

### Incremental Decoder ###

class IncrementalDecoder(codecs.IncrementalDecoder):
    def decode(self, input, final=False):
        return codecs.utf_16_le_decode(input, self.errors, final)[0]

### Stream Reader ###

class StreamReader(codecs.StreamReader):
    decode = codecs.utf_16_le_decode

### Stream Writer ###

class StreamWriter(codecs.StreamWriter):
    encode = codecs.utf_16_le_encode
